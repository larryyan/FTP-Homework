#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <pwd.h>
#include <grp.h>
#include <shadow.h>
#include <crypt.h>

#include "ftp_common.h"

// 存储当前客户端的用户名和认证状态
typedef struct {
    int client_sock;
    int data_sock;
    int logged_in;
    int data_port;
    int passive_mode;
    char username[MAX_PATH_LEN];
    char current_dir[MAX_PATH_LEN];
} FTPClient;


// 向客户端发送响应
void send_response(int client_sock, const char *message, int code) {
    char response[BUFFER_SIZE];
    snprintf(response, sizeof(response), "%d %s\r\n", code, message);
    send(client_sock, response, strlen(response), 0);
}

// 登录验证（使用 Linux 系统用户/支持 anonymous 匿名用户）
void authenticate(FTPClient *client, const char *username, const char *password) {
    if (strcmp(username, "anonymous") == 0) {
        client->logged_in = 1;
        send_response(client->client_sock, "Anonymous login ok", 230);
        return;
    }

    struct passwd *pw = getpwnam(username);
    if (!pw) {
        send_response(client->client_sock, "Invalid username or password", 530);
        return;
    }

    struct spwd *sp = getspnam(username);
    if (!sp) {
        send_response(client->client_sock, "Unable to access shadow file", 550);
        return;
    }

    // 用 shadow 文件中的加密盐加密输入密码
    char *encrypted = crypt(password, sp->sp_pwdp);
    if (encrypted && strcmp(encrypted, sp->sp_pwdp) == 0) {
        client->logged_in = 1;
        send_response(client->client_sock, "Login successful", 230);
    } else {
        send_response(client->client_sock, "Invalid username or password", 530);
    }
}

// 处理客户端命令
void handle_client(FTPClient *client) {
    char buffer[BUFFER_SIZE];
    FTPCommand cmd;

    // 发送欢迎消息
    send_response(client->client_sock, "Welcome to the FTP server", 220);

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int len = recv(client->client_sock, buffer, sizeof(buffer) - 1, 0);
        if (len <= 0) {
            break;
        }

        trim_newline(buffer);

        printf("Received command: %s\n", buffer);

        if (parse_command(buffer, &cmd) < 0) {
            send_response(client->client_sock, "Invalid command", 500);
            continue;
        }

        // 处理命令
        if (strcmp(cmd.cmd, "USER") == 0) {
            strcpy(client->username, cmd.arg);
            send_response(client->client_sock, "Password required", 331);
            continue;

        } else if (strcmp(cmd.cmd, "PASS") == 0) {
            authenticate(client, client->username, cmd.arg);    // 验证用户名和密码
            continue;

        } else if (strcmp(cmd.cmd, "PWD") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
            } else {
                char cwd[MAX_PATH_LEN];
                getcwd(cwd, sizeof(cwd));
                strcat(cwd, "\n");
                send_response(client->client_sock, cwd, 257);
            }

        } else if (strcmp(cmd.cmd, "TYPE") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            if (strcmp(cmd.arg, "I") == 0) {
                send_response(client->client_sock, "Switching to binary mode", 200);
            } else if (strcmp(cmd.arg, "A") == 0) {
                send_response(client->client_sock, "Switching to ASCII mode", 200);
            } else {
                send_response(client->client_sock, "Invalid type", 501);
            }

        } else if (strcmp(cmd.cmd, "LIST") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            send_response(client->client_sock, "Opening data connection", 150);

            DIR *dir;
            struct dirent *entry;
            char path[MAX_PATH_LEN];
            getcwd(path, sizeof(path));
            dir = opendir(path);
            if (!dir) {
                send_response(client->client_sock, "Unable to open directory", 550);
                continue;
            }

            char line[BUFFER_SIZE];
            struct stat st;
            char fullpath[MAX_PATH_LEN];
            while ((entry = readdir(dir)) != NULL) {
                snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
                if (stat(fullpath, &st) == 0) {
                    // 权限字符串
                    char perms[11] = "----------";
                    if (S_ISDIR(st.st_mode)) perms[0] = 'd';
                    if (st.st_mode & S_IRUSR) perms[1] = 'r';
                    if (st.st_mode & S_IWUSR) perms[2] = 'w';
                    if (st.st_mode & S_IXUSR) perms[3] = 'x';
                    if (st.st_mode & S_IRGRP) perms[4] = 'r';
                    if (st.st_mode & S_IWGRP) perms[5] = 'w';
                    if (st.st_mode & S_IXGRP) perms[6] = 'x';
                    if (st.st_mode & S_IROTH) perms[7] = 'r';
                    if (st.st_mode & S_IWOTH) perms[8] = 'w';
                    if (st.st_mode & S_IXOTH) perms[9] = 'x';

                    // 获取用户名和组名
                    struct passwd *pw = getpwuid(st.st_uid);
                    struct group  *gr = getgrgid(st.st_gid);
                    char *owner = pw ? pw->pw_name : "owner";
                    char *group = gr ? gr->gr_name : "group";

                    // 获取文件的修改时间
                    char timebuf[32];
                    struct tm *tm_info = localtime(&st.st_mtime);
                    strftime(timebuf, sizeof(timebuf), "%b %d %H:%M", tm_info);

                    // 构造 ls -l 风格输出
                    snprintf(line, sizeof(line), "%s %3ld %s %s %8ld %s %s\r\n",
                        perms,
                        (long)st.st_nlink,
                        owner,
                        group,
                        (long)st.st_size,
                        timebuf,
                        entry->d_name
                    );
                    send_all(client->data_sock, line, strlen(line));
                }
            }
            closedir(dir);

            close(client->data_sock);
            client->data_sock = -1;
            send_response(client->client_sock, "Directory send OK", 226);

        } else if (strcmp(cmd.cmd, "CWD") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            if (chdir(cmd.arg) == 0) {
                send_response(client->client_sock, "Directory changed", 250);
            } else {
                send_response(client->client_sock, "Directory change failed", 550);
            }

        }else if (strcmp(cmd.cmd, "MKD") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            if (mkdir(cmd.arg, 0755) == 0) {
                char response_msg[BUFFER_SIZE];
                char abs_path[MAX_PATH_LEN];
                realpath(cmd.arg, abs_path);
                snprintf(response_msg, sizeof(response_msg), "\"%s\" directory created", abs_path);
                send_response(client->client_sock, response_msg, 257);
            } else {
                send_response(client->client_sock, "Create directory failed", 550);
            }

        } else if (strcmp(cmd.cmd, "RMD") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            if (rmdir(cmd.arg) == 0) {
                send_response(client->client_sock, "Directory removed", 250);
            } else {
                send_response(client->client_sock, "Remove directory failed", 550);
            }
        } else if (strcmp(cmd.cmd, "CDUP") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            if (chdir("..") == 0) {
                send_response(client->client_sock, "Changed to parent directory", 250);
            } else {
                send_response(client->client_sock, "Failed to change to parent directory", 550);
            }

        } else if (strcmp(cmd.cmd, "PORT") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            // 解析 PORT 命令参数，获取客户端的 IP 地址和端口号
            char *token = strtok(cmd.arg, ",");
            char ip[16] = "";
            int port1 = 0, port2 = 0;

            // 提取 IP 地址
            for (int i = 0; token != NULL && i < 4; i++) {
                strcat(ip, token);
                if (i < 3) {
                    strcat(ip, ".");
                }
                token = strtok(NULL, ",");
            }

            // 提取端口号
            if (token != NULL) {
                port1 = atoi(token);   // 端口的高位
                token = strtok(NULL, ",");
            }
            if (token != NULL) {
                port2 = atoi(token);   // 端口的低位
            }

            // 计算最终端口号
            client->data_port = (port1 * 256) + port2;

            // 打印 IP 和端口号
            printf("PORT command received: IP = %s, PORT = %d\n", ip, client->data_port);

            client->data_sock = connect_data_channel(ip, client->data_port);
            client->passive_mode = 0; // 设置为主动模式

            send_response(client->client_sock, "Data connection established", 200);
        } else if (strcmp(cmd.cmd, "PASV") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            // 随机分配一个临时端口
            client->data_port = get_random_port();

            // 获取本机IP
            struct sockaddr_in addr;
            socklen_t addrlen = sizeof(addr);
            getsockname(client->client_sock, (struct sockaddr*)&addr, &addrlen);
            unsigned int ip = ntohl(addr.sin_addr.s_addr);

            // 组装 PASV 响应
            int h1 = (ip >> 24) & 0xFF;
            int h2 = (ip >> 16) & 0xFF;
            int h3 = (ip >> 8) & 0xFF;
            int h4 = ip & 0xFF;
            int p1 = (client->data_port >> 8) & 0xFF;
            int p2 = client->data_port & 0xFF;

            char pasv_msg[128];
            snprintf(pasv_msg, sizeof(pasv_msg),
                "Entering Passive Mode (%d,%d,%d,%d,%d,%d)", h1, h2, h3, h4, p1, p2);

            send_response(client->client_sock, pasv_msg, 227);

            // 创建数据通道监听
            client->data_sock = create_data_connection(client->data_port);
            if (client->data_sock < 0) {
                send_response(client->client_sock, "Failed to create data connection", 425);
                continue;
            }
            client->passive_mode = 1; // 设置为被动模式

        } else if (strcmp(cmd.cmd, "RETR") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            send_response(client->client_sock, "Opening data connection", 150);

            if (send_file(client->data_sock, cmd.arg) < 0) {
                send_response(client->client_sock, "File transfer failed", 550);
            }

            close(client->data_sock); // 关闭数据连接
            client->data_sock = -1;   // 重置数据连接
            // 关闭数据连接
            send_response(client->client_sock, "File transfer complete", 226);
                } else if (strcmp(cmd.cmd, "STOR") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            send_response(client->client_sock, "Opening data connection", 150);

            if (recv_file(client->data_sock, cmd.arg) < 0) {
                send_response(client->client_sock, "File upload failed", 550);
                close(client->data_sock);
                client->data_sock = -1;
                continue; // 直接跳过后续
            }

            close(client->data_sock);
            client->data_sock = -1;
            send_response(client->client_sock, "File upload complete", 226);
        } else if (strcmp(cmd.cmd, "DELE") == 0) {
            if (!client->logged_in) {
                send_response(client->client_sock, "Please login first", 530);
                continue;
            }

            if (remove(cmd.arg) == 0) {
                send_response(client->client_sock, "File deleted successfully", 250);
            } else {
                send_response(client->client_sock, "Failed to delete file", 550);
            }
        } else if (strcmp(cmd.cmd, "QUIT") == 0) {
            send_response(client->client_sock, "Goodbye", 221);
            break;
        }
    }
}

void handle_client_process(int client_sock) {
    FTPClient client = { client_sock, -1, 0, 0, 0, "", "/" };
    printf("客户端连接已建立。\n");
    handle_client(&client);  // 处理客户端请求
    close(client_sock);
    printf("客户端断开连接。\n");
}

int main() {
    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);

    // 创建命令通道 socket
    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket error");
        exit(1);
    }

    // 配置服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(COMMAND_PORT);

    // 绑定
    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind error");
        exit(1);
    }

    // 监听
    if (listen(server_sock, 5) < 0) {
        perror("listen error");
        exit(1);
    }

    printf("FTP 服务器已启动，监听端口 %d...\n", COMMAND_PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);
        if (client_sock < 0) {
            perror("accept error");
            continue;
        }
        
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork error");
            close(client_sock);
            continue;
        } else if (pid == 0) {
            // 子进程
            close(server_sock); // 子进程不需要监听socket
            handle_client_process(client_sock);
            exit(0);
        } else {
            // 父进程
            close(client_sock); // 父进程不处理该连接
        }
    }

    close(server_sock);
    return 0;
}