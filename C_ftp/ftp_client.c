#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <termios.h>

#include "ftp_common.h"

char server_ip[16];
int cmd_port;
int data_port; // 数据端口
int data_sock = -1; // 数据连接套接字

int passive_mode = 0; // 被动模式

void print_help() {
    printf("支持的命令:\n");
    printf("open <ip> <port>  连接到FTP服务器\n");
    printf("user <username>   登录用户名\n");
    printf("get <filename>    下载文件\n");
    printf("put <filename>    上传文件\n");
    printf("pwd               显示远程当前目录\n");
    printf("dir               列出远程目录\n");
    printf("cd <dirname>      切换远程目录\n");
    printf("?                 显示帮助\n");
    printf("quit              退出\n");
}

void recv_response(int sockfd) {
    char buffer[BUFFER_SIZE] = {0};
    int n = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (n > 0) {
        printf("%s", buffer);
    }
}

void get_local_ip(char *ip_buf, size_t buf_size) {
    struct ifaddrs *ifaddr, *ifa;

    if (ip_buf == NULL || buf_size < INET_ADDRSTRLEN) return;

    if (getifaddrs(&ifaddr) == -1) return;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        // 跳过无效接口和非 IPv4 地址
        if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) {
            continue;
        }
        // 跳过回环接口 (lo)
        if (strcmp(ifa->ifa_name, "lo") == 0) {
            continue;
        }
        // 转换为字符串并复制到缓冲区
        struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
        inet_ntop(AF_INET, &addr->sin_addr, ip_buf, buf_size);
        break;
    }
    freeifaddrs(ifaddr);
}

int data_connect(int sockfd) {
    if (passive_mode) {
        // 被动模式：发送PASV命令，解析服务器返回的IP和端口，连接
        send_all(sockfd, "PASV\r\n", 6);
        char resp[BUFFER_SIZE] = {0};
        recv(sockfd, resp, sizeof(resp) - 1, 0);
        printf("%s", resp);

        // 解析 227 Entering Passive Mode (h1,h2,h3,h4,p1,p2)
        int h1, h2, h3, h4, p1, p2;
        char *p = strchr(resp, '(');
        if (!p) return -1;
        sscanf(p+1, "%d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
        char ip[32];
        snprintf(ip, sizeof(ip), "%d.%d.%d.%d", h1, h2, h3, h4);
        int port = p1 * 256 + p2;

        // 连接数据通道
        return connect_data_channel(ip, port);
    } else {
        // 主动模式：本地监听端口，发送PORT命令给服务器
        // 获取本地IP
        char local_ip[16];
        get_local_ip(local_ip, sizeof(local_ip));
        // 计算端口号
        data_port = get_random_port();
        int p1 = (data_port >> 8) & 0xFF;
        int p2 = data_port & 0xFF;
        int ip1, ip2, ip3, ip4;
        sscanf(local_ip, "%d.%d.%d.%d", &ip1, &ip2, &ip3, &ip4);

        char port_cmd[64];
        snprintf(port_cmd, sizeof(port_cmd), "PORT %d,%d,%d,%d,%d,%d\r\n", ip1, ip2, ip3, ip4, p1, p2);
        send_all(sockfd, port_cmd, strlen(port_cmd));

        int data_sock = create_data_connection(data_port);
        if (data_sock < 0) return -1;

        char resp[BUFFER_SIZE] = {0};
        recv(sockfd, resp, sizeof(resp) - 1, 0);
        printf("%s", resp);

        // 主动模式下，客户端作为服务器监听数据端口，等待服务器连接
        return data_sock;
    }
}

void download_file(int sockfd, const char *filename) {
    data_sock = data_connect(sockfd);
    if (data_sock < 0) {
        printf("数据连接失败\n");
        return;
    }

    char cmd[BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "RETR %s\r\n", filename);
    send_all(sockfd, cmd, strlen(cmd));
    recv_response(sockfd); // 读取服务器150响应

    recv_file(data_sock, filename); // 接收文件

    printf("下载完成: %s\n", filename);
    close(data_sock); // 关闭数据连接
    recv_response(sockfd); // 读取服务器226响应
}

void upload_file(int sockfd, const char *filename) {
    data_sock = data_connect(sockfd);
    if (data_sock < 0) {
        printf("数据连接失败\n");
        return;
    }

    char cmd[BUFFER_SIZE];
    snprintf(cmd, sizeof(cmd), "STOR %s\r\n", filename);
    send_all(sockfd, cmd, strlen(cmd));
    recv_response(sockfd); // 读取服务器150响应

    send_file(data_sock, filename); // 发送文件

    printf("上传完成: %s\n", filename);
    close(data_sock); // 关闭数据连接
    recv_response(sockfd); // 读取服务器226响应
}

void get_file_list(int sockfd) {
    data_sock = data_connect(sockfd);
    if (data_sock < 0) {
        printf("数据连接失败\n");
        return;
    }

    send_all(sockfd, "LIST\r\n", 6);
    recv_response(sockfd); // 读取服务器150响应

    char buffer[BUFFER_SIZE];
    int n;
    while ((n = recv(data_sock, buffer, sizeof(buffer), 0)) > 0) {
        fwrite(buffer, 1, n, stdout);
    }

    close(data_sock); // 关闭数据连接
    recv_response(sockfd); // 读取服务器226响应
}

void login(int sockfd, const char *username) {
    char cmd[BUFFER_SIZE];
    char resp[BUFFER_SIZE] = {0};

    // 发送USER命令
    snprintf(cmd, sizeof(cmd), "USER %s\r\n", username);
    send_all(sockfd, cmd, strlen(cmd));

    // 接收服务器响应
    int n = recv(sockfd, resp, sizeof(resp) - 1, 0);
    if (n > 0) {
        printf("%s", resp);
        // 331 需要输入密码
        if (strncmp(resp, "331", 3) == 0) {
            char password[BUFFER_SIZE/2];
            printf("请输入密码: ");
            fflush(stdout);
            // 关闭回显
            struct termios oldt, newt;
            tcgetattr(STDIN_FILENO, &oldt);
            newt = oldt;
            newt.c_lflag &= ~ECHO;
            tcsetattr(STDIN_FILENO, TCSANOW, &newt);

            if (!fgets(password, sizeof(password), stdin)) {
                // 恢复回显
                tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
                return;
            }

            // 恢复回显
            tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
            printf("\n");

            trim_newline(password);

            // 发送PASS命令
            snprintf(cmd, sizeof(cmd), "PASS %s\r\n", password);
            send_all(sockfd, cmd, strlen(cmd));

            memset(resp, 0, sizeof(resp));
            n = recv(sockfd, resp, sizeof(resp) - 1, 0);
            if (n > 0) {
                printf("%s", resp);
                if (strncmp(resp, "230", 3) == 0) {
                    printf("登录成功！\n");
                } else if (strncmp(resp, "530", 3) == 0) {
                    printf("登录失败，请检查用户名或密码。\n");
                }
            }
        } else if (strncmp(resp, "230", 3) == 0) {
            printf("登录成功！\n");
            printf("Remote system type is UNIX.\nUsing binary mode to transfer files.\n");
        } else if (strncmp(resp, "530", 3) == 0) {
            printf("登录失败，请检查用户名。\n");
        }
    }
}

int main() {
    load_ports_from_yaml("ftp_config.yaml"); // 从 YAML 文件加载端口配置
    data_port = DATA_PORT;

    int sockfd;
    struct sockaddr_in server_addr;
    char input[BUFFER_SIZE];

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket error");
        exit(1);
    }

    printf("请选择FTP模式(主动 0/被动 1): ");
    scanf("%d", &passive_mode);
    getchar(); // 清除换行符

    while (1) {
        printf("ftp> ");
        fflush(stdout);
        if (!fgets(input, sizeof(input), stdin)) break;
        trim_newline(input);

        if(strncmp(input, "open ", 5) == 0) {
            // 支持 open <ip> 或 open <ip> <port>
            char ip[16] = {0}, port[6] = {0};
            int cnt = sscanf(input + 5, "%15s %5s", ip, port);
            if (cnt == 1) {
                strcpy(server_ip, ip);
                cmd_port = COMMAND_PORT;      // 默认端口
            } else if (cnt == 2) {
                strcpy(server_ip, ip);
                cmd_port = atoi(port);
            } else {
                printf("用法: open <ip> [port]\n");
                continue;
            }
            printf("连接到 %s:%d\n", server_ip, cmd_port);
            close(sockfd);
            sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (sockfd < 0) {
                perror("socket error");
                exit(1);
            }
            memset(&server_addr, 0, sizeof(server_addr));
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(cmd_port);
            server_addr.sin_addr.s_addr = inet_addr(server_ip);
            if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                perror("connect error");
                exit(1);
            }
            recv_response(sockfd);
        } else if (strncmp(input, "user ", 5) == 0) {
            char username[BUFFER_SIZE];
            sscanf(input + 5, "%s", username);
            login(sockfd, username);
        } else if (strncmp(input, "quit", 4) == 0) {
            send_all(sockfd, "QUIT\r\n", 6);
            recv_response(sockfd);
            break;
        } else if (strncmp(input, "?", 1) == 0) {
            print_help();
        } else if (strncmp(input, "pwd", 3) == 0) {
            send_all(sockfd, "PWD\r\n", 5);
            recv_response(sockfd);
        } else if (strncmp(input, "dir", 3) == 0) {
            get_file_list(sockfd);
        } else if (strncmp(input, "cd ", 3) == 0) {
            char cmd[BUFFER_SIZE];
            snprintf(cmd, sizeof(cmd), "CWD %s\r\n", input + 3);
            send_all(sockfd, cmd, strlen(cmd));
            recv_response(sockfd);
        } else if (strncmp(input, "get ", 4) == 0) {
            download_file(sockfd, input + 4);
        } else if (strncmp(input, "put ", 4) == 0) {
            upload_file(sockfd, input + 4);
        } else {
            printf("未知命令，输入 ? 查看帮助。\n");
        }
    }

    close(sockfd);
    return 0;
}