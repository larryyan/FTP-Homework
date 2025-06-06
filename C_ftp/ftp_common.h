#ifndef FTP_COMMON_H
#define FTP_COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <yaml.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ---------------------
// 通用常量定义
// ---------------------
extern int COMMAND_PORT;  // 命令通道端口
extern int DATA_PORT;     // 数据通道端口
extern int IS_PASSIVE;    // 是否处于被动模式

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
#define CMD_CDUP "CDUP"
#define CMD_MKD "MKD"
#define CMD_RMD "RMD"
#define CMD_CWD "CWD"
#define CMD_PWD "PWD"
#define CMD_QUIT "QUIT"
#define CMD_PORT "PORT"
#define CMD_PASV "PASV"
#define CMD_TYPE "TYPE"
#define CMD_DELE "DELE"

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

// 获取随机端口和本地 IP
int get_random_port();
void get_local_ip(char *ip, size_t len);

// 读取配置文件
int load_yaml(const char *filename);

#endif // FTP_COMMON_H