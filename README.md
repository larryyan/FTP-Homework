# FTP-Homework

- This is an assignment for a computer network internship course. 

- This project consists of the following parts:
  - FTP server Implemented in C Language
  - FTP client implemented in C language
  - A web-based FTP file browser implemented in Python language
  - A socket based web crawler in C language
  
- Both FTP server & client should be run on a Linux operating system.

## Preparation

### Compile C server, client and the crawler

```bash
make
```

### Install iconv


#### Ubuntu/Debian

  ```bash
  sudo apt-get install libc-bin
  ```


#### CentOS/RHEL

  ```bash
  sudo yum install glibc-common
  ```

#### MacOS

  ```bash
  brew install libiconv
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
./ftp_server
```

### FTP Client

```bash
./ftp_client
```

### Web based FTP browser

```bash
python3 ftp_web/ftp_client_web.py
```

#### Socket based web crawler

```bash
cd crawler
./crawler
```

