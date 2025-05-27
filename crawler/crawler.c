// 基于Socket+OpenSSL的简易HTTPS网络爬虫（抓取文本和图片，保存到download文件夹）
// 编译：gcc -o crawler_https crawler_https.c -lssl -lcrypto
// 运行：./crawler_https https://example.com

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

#define BUFFER_SIZE 8192

void parse_url(const char *url, char *host, char *path, int *port, int *is_https) {
    *port = 80;
    *is_https = 0;
    if (strncmp(url, "https://", 8) == 0) {
        *is_https = 1;
        *port = 443;
        url += 8;
    } else if (strncmp(url, "http://", 7) == 0) {
        url += 7;
    }
    const char *slash = strchr(url, '/');
    if (slash) {
        strncpy(host, url, slash - url);
        host[slash - url] = '\0';
        strcpy(path, slash);
    } else {
        strcpy(host, url);
        strcpy(path, "/");
    }
    char *colon = strchr(host, ':');
    if (colon) {
        *port = atoi(colon + 1);
        *colon = '\0';
    }
}

int connect_host(const char *host, int port) {
    struct hostent *he = gethostbyname(host);
    if (!he) return -1;
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(sock);
        return -1;
    }
    return sock;
}

void save_file(const char *fname, const char *data, size_t len) {
    char fullpath[256];
    sprintf(fullpath, "download/%s", fname);
    FILE *f = fopen(fullpath, "wb");
    if (f) {
        fwrite(data, 1, len, f);
        fclose(f);
        printf("保存：%s\n", fullpath);
    }
}

int ssl_send_recv(SSL *ssl, const char *req, char **out_buf, size_t *out_size) {
    char buf[BUFFER_SIZE];
    int header = 1, n;
    size_t file_size = 0, cap = BUFFER_SIZE * 4;
    char *file_data = malloc(cap);
    SSL_write(ssl, req, strlen(req));
    while ((n = SSL_read(ssl, buf, sizeof(buf))) > 0) {
        if (header) {
            char *p = strstr(buf, "\r\n\r\n");
            if (p) {
                size_t hlen = p + 4 - buf;
                memcpy(file_data, buf + hlen, n - hlen);
                file_size = n - hlen;
                header = 0;
            }
        } else {
            if (file_size + n > cap) { cap *= 2; file_data = realloc(file_data, cap); }
            memcpy(file_data + file_size, buf, n);
            file_size += n;
        }
    }
    *out_buf = file_data;
    *out_size = file_size;
    return 0;
}

void fetch_url(const char *url, int is_img) {
    char host[128], path[256];
    int port, is_https;
    parse_url(url, host, path, &port, &is_https);
    int sock = connect_host(host, port);
    if (sock < 0) { puts("连接失败"); return; }

    char req[512];
    snprintf(req, sizeof(req),
        "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
        path, host);

    char *file_data = NULL;
    size_t file_size = 0;
    if (!is_https) {
        // HTTP 方式
        char buf[BUFFER_SIZE];
        int header = 1, n;
        size_t cap = BUFFER_SIZE * 4;
        file_data = malloc(cap);
        send(sock, req, strlen(req), 0);
        while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
            if (header) {
                char *p = strstr(buf, "\r\n\r\n");
                if (p) {
                    size_t hlen = p + 4 - buf;
                    memcpy(file_data, buf + hlen, n - hlen);
                    file_size = n - hlen;
                    header = 0;
                }
            } else {
                if (file_size + n > cap) { cap *= 2; file_data = realloc(file_data, cap); }
                memcpy(file_data + file_size, buf, n);
                file_size += n;
            }
        }
    } else {
        // HTTPS 方式
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        const SSL_METHOD *method = TLS_client_method();
        SSL_CTX *ctx = SSL_CTX_new(method);
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, sock);
        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl); SSL_CTX_free(ctx); close(sock); puts("SSL连接失败"); return;
        }
        ssl_send_recv(ssl, req, &file_data, &file_size);
        SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(ctx);
    }
    close(sock);
    // 生成文件名
    const char *fname = strrchr(path, '/');
    if (!fname || strlen(fname) <= 1) fname = is_img ? "index.jpg" : "index.html"; else fname++;
    save_file(fname, file_data, file_size);
    free(file_data);
}

void mkdir_download() { mkdir("download", 0755); }

void find_and_fetch_imgs(const char *html, size_t len, const char *base_url) {
    const char *p = html;
    while ((p = strstr(p, "<img"))) {
        const char *src = strstr(p, "src=");
        if (!src) break;
        src += 4;
        while (*src == ' ' || *src == '\'' || *src == '"') src++;
        char img_url[512] = {0};
        int j = 0;
        while (*src && *src != '"' && *src != '\'' && *src != ' ' && *src != '>') {
            img_url[j++] = *src++;
        }
        img_url[j] = '\0';
        char full_url[512];
        if (strncmp(img_url, "http://", 7) == 0 || strncmp(img_url, "https://", 8) == 0) {
            strcpy(full_url, img_url);
        } else if (img_url[0] == '/') {
            char host[128], path[256]; int port, is_https;
            parse_url(base_url, host, path, &port, &is_https);
            sprintf(full_url, is_https ? "https://%s%s" : "http://%s%s", host, img_url);
        } else {
            sprintf(full_url, "%s/%s", base_url, img_url);
        }
        if (strstr(img_url, ".jpg") || strstr(img_url, ".png") || strstr(img_url, ".jpeg") || strstr(img_url, ".gif")) {
            printf("图片URL: %s\n", full_url);
            fetch_url(full_url, 1);
        }
        p = src;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("用法: %s <网址>\n", argv[0]);
        return 1;
    }
    mkdir_download();
    char host[128], path[256]; int port, is_https;
    parse_url(argv[1], host, path, &port, &is_https);
    int sock = connect_host(host, port);
    if (sock < 0) { puts("连接失败"); return 1; }
    char req[512];
    snprintf(req, sizeof(req),
        "GET %s HTTP/1.0\r\nHost: %s\r\nConnection: close\r\n\r\n",
        path, host);
    char *html = NULL;
    size_t html_size = 0;
    if (!is_https) {
        // HTTP
        char buf[BUFFER_SIZE];
        int n, header = 1;
        size_t cap = BUFFER_SIZE * 10;
        html = malloc(cap);
        send(sock, req, strlen(req), 0);
        while ((n = recv(sock, buf, sizeof(buf), 0)) > 0) {
            if (header) {
                char *p = strstr(buf, "\r\n\r\n");
                if (p) {
                    size_t hlen = p + 4 - buf;
                    memcpy(html, buf + hlen, n - hlen);
                    html_size = n - hlen;
                    header = 0;
                }
            } else {
                if (html_size + n > cap) { cap *= 2; html = realloc(html, cap); }
                memcpy(html + html_size, buf, n);
                html_size += n;
            }
        }
    } else {
        // HTTPS
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        SSL_load_error_strings();
        const SSL_METHOD *method = TLS_client_method();
        SSL_CTX *ctx = SSL_CTX_new(method);
        SSL *ssl = SSL_new(ctx);
        SSL_set_fd(ssl, sock);
        if (SSL_connect(ssl) <= 0) {
            SSL_free(ssl); SSL_CTX_free(ctx); close(sock); puts("SSL连接失败"); return 1;
        }
        char buf[BUFFER_SIZE];
        int n, header = 1;
        size_t cap = BUFFER_SIZE * 10;
        html = malloc(cap);
        SSL_write(ssl, req, strlen(req));
        while ((n = SSL_read(ssl, buf, sizeof(buf))) > 0) {
            if (header) {
                char *p = strstr(buf, "\r\n\r\n");
                if (p) {
                    size_t hlen = p + 4 - buf;
                    memcpy(html, buf + hlen, n - hlen);
                    html_size = n - hlen;
                    header = 0;
                }
            } else {
                if (html_size + n > cap) { cap *= 2; html = realloc(html, cap); }
                memcpy(html + html_size, buf, n);
                html_size += n;
            }
        }
        SSL_shutdown(ssl); SSL_free(ssl); SSL_CTX_free(ctx);
    }
    close(sock);
    save_file("index.html", html, html_size);
    find_and_fetch_imgs(html, html_size, argv[1]);
    free(html);
    return 0;
}
