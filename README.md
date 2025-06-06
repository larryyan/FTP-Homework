# FTP-Homework

![Platform](https://img.shields.io/badge/platform-Linux-blue)
![Language](https://img.shields.io/badge/language-C%20%7C%20Python-blueviolet)
![License](https://img.shields.io/badge/license-MIT-green)
![Status](https://img.shields.io/badge/status-educational-important)

- This is an assignment for a computer network internship course.

- This project consists of the following parts:
  - FTP server implemented in C language ([details here](C_ftp/README.md))
  - FTP client implemented in C language ([details here](C_ftp/README.md))
  - A web-based FTP file browser implemented in Python ([details here](ftp_web/README.md))
  - A socket-based web crawler in C language ([details here](crawler/README.md))
  
- All FTP client/server and crawler should be run on a Linux operating system.

## Preparation

### Compile C server, client and the crawler

```bash
make
```

### Install required Python packages

#### Ubuntu / Debian

```bash
sudo apt install python-requests
sudo apt install python-cgi
```

#### Arch Linux

```bash
sudo pacman -S python-requests
sudo pacman -S python-cgi
```

## Run

### FTP Server

```bash
cd C_ftp
sudo ./ftp_server
```

### FTP Client

```bash
cd C_ftp
./ftp_client
```

![image-20250606112255811](assets/image-20250606112255811.png)

![image-20250528190021652](assets/image-20250528190021652.png)

### Web based FTP browser

```bash
python3 ftp_web/ftp_client_web.py
```

![image-20250528190106619](assets/image-20250528190106619.png)

- login by your account and password

![image-20250528190129700](assets/image-20250528190129700.png)

### Socket based web crawler

```bash
cd crawler
./crawler_https https://example.com
```

![image-20250528190143889](assets/image-20250528190143889.png)

![image-20250528190150785](assets/image-20250528190150785.png)

- The website and images have been downloaded to the download/"domain" folder.

---

## More Details

- [C FTP server/client details](C_ftp/README.md)
- [Web-based FTP browser details](ftp_web/README.md)
- [Socket-based web crawler details](crawler/README.md)

---

[License (MIT)](LICENSE)