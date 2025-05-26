#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <ctype.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFFER_SIZE 4096

// 初始化 OpenSSL 库
void init_openssl() {
    SSL_load_error_strings();
    OpenSSL_add_ssl_algorithms();
}

// 清理 OpenSSL
void cleanup_openssl() {
    EVP_cleanup();
}

// 从 URL 中提取主机名和路径
void parse_url(const char *url, char *host, char *path) {
    if (strncmp(url, "https://", 8) == 0) url += 8; // 处理 HTTPS
    const char *slash = strchr(url, '/');
    if (slash) {
        strncpy(host, url, slash - url);
        host[slash - url] = '\0';
        strcpy(path, slash);
    } else {
        strcpy(host, url);
        strcpy(path, "/");
    }
}

// 建立 SSL 连接
SSL* connect_to_server(const char *host) {
    struct hostent *server = gethostbyname(host);
    if (!server) {
        perror("gethostbyname failed");
        return NULL;
    }

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket creation failed");
        return NULL;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(443); // HTTPS 使用端口 443
    if (server->h_addr_list[0] == NULL) {
        fprintf(stderr, "No IP address found for host.\n");
        close(sockfd);
        return NULL;
    }
    memcpy(&serv_addr.sin_addr.s_addr, server->h_addr_list[0], server->h_length);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("connect failed");
        close(sockfd);
        return NULL;
    }

    SSL_CTX *ctx = SSL_CTX_new(TLS_client_method());
    if (!ctx) {
        perror("SSL_CTX_new failed");
        close(sockfd);
        return NULL;
    }

    SSL *ssl = SSL_new(ctx);
    if (!ssl) {
        perror("SSL_new failed");
        SSL_CTX_free(ctx);
        close(sockfd);
        return NULL;
    }

    SSL_set_fd(ssl, sockfd);
    if (SSL_connect(ssl) != 1) {
        perror("SSL_connect failed");
        SSL_free(ssl);
        SSL_CTX_free(ctx);
        close(sockfd);
        return NULL;
    }

    return ssl; // 返回 SSL 对象
}

// 发送 HTTPS 请求
void send_get_request(SSL *ssl, const char *host, const char *path) {
    char request[BUFFER_SIZE];
    snprintf(request, sizeof(request),
             "GET %s HTTP/1.1\r\n"
             "Host: %s\r\n"
             "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) "
             "AppleWebKit/537.36 (KHTML, like Gecko) "
             "Chrome/123.0.0.0 Safari/537.36\r\n"
             "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,image/apng,*/*;q=0.8\r\n"
             "Connection: close\r\n\r\n",
             path, host);
    SSL_write(ssl, request, strlen(request));
}

// 去除 HTML 标签，只保留文本内容（简单方式：去除 <> 中的内容）
void strip_html_tags(const char *input, FILE *out_fp) {
    int in_tag = 0;
    for (; *input; input++) {
        if (*input == '<') {
            in_tag = 1;
        } else if (*input == '>') {
            in_tag = 0;
        } else if (!in_tag) {
            fputc(*input, out_fp);
        }
    }
}

// 读取 SSL 数据直到结束并处理内容
void save_text_only(SSL *ssl, const char *text_file) {
    FILE *fp = fopen(text_file, "w");
    if (!fp) {
        perror("fopen failed");
        return;
    }
    
    char buffer[BUFFER_SIZE];
    int total_bytes = 0;
    int bytes_read;
    int header_end = 0;
    char *body_ptr = NULL;
    
    while (1) {
        bytes_read = SSL_read(ssl, buffer + total_bytes, sizeof(buffer) - total_bytes - 1); // 继续读数据
        if (bytes_read <= 0) break;  // 如果没有更多数据，退出循环
        total_bytes += bytes_read;
        if (total_bytes >= sizeof(buffer) - 1) break;  // 防止溢出

        // 处理 HTML 内容的转换
        buffer[total_bytes] = '\0';  // 保证 buffer 结束符
    }

    // 完整数据读取完成后进行 HTML 标签去除
    buffer[total_bytes] = '\0';  // 确保数据是一个完整的字符串
    
    // 跳过 HTTP 头部部分
    if (!header_end) {
        body_ptr = strstr(buffer, "\r\n\r\n");
        if (body_ptr) {
            header_end = 1;
            body_ptr += 4;  // 跳过头部的 \r\n\r\n
            strip_html_tags(body_ptr, fp);  // 去除 HTML 标签
        }
    } else {
        strip_html_tags(buffer, fp);  // 如果头部已处理，直接处理 body 内容
    }

    fclose(fp);
}

int main() {
    init_openssl();

    const char *url = "https://baike.baidu.com/item/FBADS/1991071";  // 替换为你要爬取的 HTTPS 网站
    char host[256], path[1024];

    parse_url(url, host, path);

    SSL *ssl = connect_to_server(host);
    if (!ssl) {
        cleanup_openssl();
        return 1;
    }

    send_get_request(ssl, host, path);
    save_text_only(ssl, "output.txt");

    SSL_free(ssl);
    cleanup_openssl();
    printf("网页文本已保存为 output.txt\n");
    return 0;
}
