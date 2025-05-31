#include "ftp_common.h"

int COMMAND_PORT = 21;  // 默认命令端口
int DATA_PORT = 20;     // 默认数据端口

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
        strcmp(cmd->cmd, CMD_MKD) == 0 || strcmp(cmd->cmd, CMD_RMD) == 0 ||
        strcmp(cmd->cmd, CMD_CWD) == 0 || strcmp(cmd->cmd, CMD_CDUP) == 0 ||
        strcmp(cmd->cmd, CMD_PORT) == 0 || strcmp(cmd->cmd, CMD_PASV) == 0 ||
        strcmp(cmd->cmd, CMD_DELE) == 0 || strcmp(cmd->cmd, CMD_TYPE) == 0 || 
        strcmp(cmd->cmd, CMD_QUIT) == 0) {
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


// 获取随机端口
int get_random_port() {
    int sockfd;
    struct sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);

    // 先尝试默认端口
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    int opt = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)); // 端口可复用

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(DATA_PORT);

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        // 默认端口可用
        close(sockfd);
        return DATA_PORT;
    }
    close(sockfd);

    // 默认端口被占用，随机分配
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0; // 让系统分配端口

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sockfd);
        return -1;
    }

    if (getsockname(sockfd, (struct sockaddr*)&addr, &addrlen) < 0) {
        close(sockfd);
        return -1;
    }

    int port = ntohs(addr.sin_port);
    close(sockfd);
    return port;
}

int load_ports_from_yaml(const char *filename) {
    FILE *fh = fopen(filename, "r");
    if (!fh) return -1;

    yaml_parser_t parser;
    yaml_token_t token;
    int state = 0; // 0: key, 1: value
    char key[64] = {0};

    yaml_parser_initialize(&parser);
    yaml_parser_set_input_file(&parser, fh);

    while (1) {
        yaml_parser_scan(&parser, &token);
        if (token.type == YAML_STREAM_END_TOKEN) break;
        if (token.type == YAML_KEY_TOKEN) state = 0;
        if (token.type == YAML_VALUE_TOKEN) state = 1;
        if (token.type == YAML_SCALAR_TOKEN) {
            if (state == 0) {
                strncpy(key, (char *)token.data.scalar.value, sizeof(key)-1);
            } else {
                if (strcmp(key, "command_port") == 0)
                    COMMAND_PORT = atoi((char *)token.data.scalar.value);
                else if (strcmp(key, "data_port") == 0)
                    DATA_PORT = atoi((char *)token.data.scalar.value);
            }
        }
        yaml_token_delete(&token);
    }
    yaml_parser_delete(&parser);
    fclose(fh);
    return 0;
}