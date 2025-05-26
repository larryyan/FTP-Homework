# FTP-Homework

- This is an assignment for a computer network internship course. 

- This project consists of the following parts:
  - FTP server Implemented in C Language
  - FTP client implemented in C language
  - A web-based FTP file browser implemented in Python language
  
- Both FTP server & client should be run on a Linux operating system.

## Preparation

### Compile C server and client

```bash
make
```

- Afterwards you'll see two programs, ftp_server and ftp_client. 

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

