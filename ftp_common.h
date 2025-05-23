#ifndef FTP_COMMON_H
#define FTP_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ---------------------
// 通用常量定义
// ---------------------
#define COMMAND_PORT 2121       // 命令端口
#define DATA_PORT 2020          // 默认数据端口
#define BUFFER_SIZE 1024        // 缓冲区大小
#define MAX_PATH_LEN 256        // 文件路径长度

// ---------------------
// FTP 命令定义
// ---------------------
#define CMD_USER "USER"
#define CMD_PASS "PASS"
#define CMD_RETR "RETR"
#define CMD_STOR "STOR"
#define CMD_LIST "LIST"
#define CMD_CWD "CWD"
#define CMD_CDUP "CDUP"
#define CMD_PWD "PWD"
#define CMD_QUIT "QUIT"
#define CMD_PORT "PORT"
#define CMD_PASV "PASV"
#define CMD_TYPE "TYPE"

// ---------------------
// FTP 用户名和密码
// ---------------------
#define FTP_USERNAME "user"
#define FTP_PASSWORD "password"

// ---------------------
// 命令结构体（可选）
// ---------------------
typedef struct {
    char cmd[255];
    char arg[MAX_PATH_LEN];
} FTPCommand;

// ---------------------
// 函数声明
// ---------------------

// 发送和接收
ssize_t send_all(int sockfd, const void *buf, size_t len);
ssize_t recv_all(int sockfd, void *buf, size_t len);

// 文件传输
int send_file(int data_sock, const char *filename);
int recv_file(int data_sock, const char *filename);

// 命令处理工具
void trim_newline(char *str);
int parse_command(const char *input, FTPCommand *cmd);

// 数据连接建立
int create_data_connection(int listen_port);   // 服务端使用
int connect_data_channel(const char *ip, int port); // 客户端使用

// 去掉字符串末尾的换行符
void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len - 1] == '\n' || str[len - 1] == '\r')) {
        str[-- len] = '\0';  // 去掉换行符
    }
}

// 解析命令字符串，提取命令和参数
int parse_command(const char *input, FTPCommand *cmd) {
    char *space_pos = strchr(input, ' ');
    if (space_pos) {
        // 提取命令和参数
        size_t cmd_len = space_pos - input;
        strncpy(cmd->cmd, input, cmd_len);
        cmd->cmd[cmd_len] = '\0';

        // 提取参数
        strncpy(cmd->arg, space_pos + 1, MAX_PATH_LEN - 1);
        cmd->arg[MAX_PATH_LEN - 1] = '\0';  // 防止溢出
    } else {
        // 只包含命令，没有参数
        strncpy(cmd->cmd, input, MAX_PATH_LEN - 1);
        cmd->cmd[MAX_PATH_LEN - 1] = '\0';
        cmd->arg[0] = '\0';  // 参数为空
    }

    // 检查命令是否有效
    if (strcmp(cmd->cmd, CMD_USER) == 0 || strcmp(cmd->cmd, CMD_PASS) == 0 ||
        strcmp(cmd->cmd, CMD_RETR) == 0 || strcmp(cmd->cmd, CMD_STOR) == 0 ||
        strcmp(cmd->cmd, CMD_LIST) == 0 || strcmp(cmd->cmd, CMD_PWD) == 0 ||
        strcmp(cmd->cmd, CMD_CWD) == 0 || strcmp(cmd->cmd, CMD_CDUP) == 0 ||
        strcmp(cmd->cmd, CMD_PORT) == 0 || strcmp(cmd->cmd, CMD_PASV) == 0 ||
        strcmp(cmd->cmd, CMD_TYPE) == 0 || strcmp(cmd->cmd, CMD_QUIT) == 0) {
        return 0;  // 成功解析
    }

    return -1;  // 无效命令
}

// 发送指定长度的数据
ssize_t send_all(int sockfd, const void *buf, size_t len) {
    size_t total_sent = 0;
    ssize_t bytes_sent;

    while (total_sent < len) {
        bytes_sent = send(sockfd, (const char *)buf + total_sent, len - total_sent, 0);
        if (bytes_sent == -1) {
            return -1;  // 错误发生，返回 -1
        }
        total_sent += bytes_sent;
    }
    return total_sent;
}

// 接收指定长度的数据
ssize_t recv_all(int sockfd, void *buf, size_t len) {
    size_t total_received = 0;
    ssize_t bytes_received;

    while (total_received < len) {
        bytes_received = recv(sockfd, (char *)buf + total_received, len - total_received, 0);
        if (bytes_received == -1) {
            return -1;  // 错误发生，返回 -1
        }
        total_received += bytes_received;

        // 客户端关闭连接
        if (bytes_received == 0) {
            break;
        }
    }
    return total_received;
}

// 创建数据连接（服务器端）
int create_data_connection(int listen_port) {
    int data_sock;
    struct sockaddr_in data_addr;

    data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0) {
        perror("data socket error");
        return -1;
    }

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_addr.s_addr = INADDR_ANY;
    data_addr.sin_port = htons(listen_port);

    if (bind(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
        perror("bind data port error");
        return -1;
    }

    if (listen(data_sock, 1) < 0) {
        perror("listen data port error");
        return -1;
    }

    int client_data_sock = accept(data_sock, NULL, NULL);
    if (client_data_sock < 0) {
        perror("accept data socket error");
        return -1;
    }

    close(data_sock);
    return client_data_sock;
}

// 发送文件
int send_file(int data_sock, const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) {
        perror("fopen error");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        send_all(data_sock, buffer, bytes_read);
    }

    fclose(file);
    return 0;
}

// 接收文件
int recv_file(int data_sock, const char *filename) {
    FILE *file = fopen(filename, "wb");
    if (!file) {
        perror("fopen error");
        return -1;
    }

    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    while ((bytes_received = recv_all(data_sock, buffer, sizeof(buffer))) > 0) {
        fwrite(buffer, 1, bytes_received, file);
    }

    fclose(file);
    return 0;
}

// 连接到数据通道
int connect_data_channel(const char *ip, int port) {
    int data_sock;
    struct sockaddr_in data_addr;

    data_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (data_sock < 0) {
        perror("socket error");
        return -1;
    }

    memset(&data_addr, 0, sizeof(data_addr));
    data_addr.sin_family = AF_INET;
    data_addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &data_addr.sin_addr) <= 0) {
        perror("invalid address or address not supported");
        return -1;
    }

    if (connect(data_sock, (struct sockaddr *)&data_addr, sizeof(data_addr)) < 0) {
        perror("connection failed");
        return -1;
    }

    return data_sock;
}

#endif // FTP_COMMON_H