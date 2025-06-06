# C_ftp

This directory contains a simple FTP server and client implemented in C, supporting basic FTP file transfer functions.

## Build

Enter the `C_ftp` directory and run:

```bash
make
```

or compile manually:

```bash
gcc -o ftp_server ftp_server.c ftp_common.c -lyaml -lpthread
gcc -o ftp_client ftp_client.c ftp_common.c -lyaml
```

This will generate two executables: `ftp_server` (server) and `ftp_client` (client).

## Configuration

Edit `ftp_config.yaml` to set the command port, data port, and passive mode:

```yaml
command_port: 2121
data_port: 2020
passive: true   # true/false or 1/0, controls whether passive mode is enabled by default
```

## Usage

### Start the FTP Server

```bash
sudo ./ftp_server
```

- The server reads the command port, data port, and passive mode from `ftp_config.yaml`.
- Supports login with Linux system users, as well as anonymous login (`anonymous` user does not require a password).

### Start the FTP Client

```bash
./ftp_client
```

- Supports active/passive mode selection. The default mode can be set via the `passive` field in the configuration file.
- Supported commands: `open`, `user`, `get`, `put`, `pwd`, `dir`, `cd`, `quit`, `?`, etc.
- After connecting to the server, use `user <username>` to log in. For the `anonymous` user, you can just press Enter to skip the password.

### Example Commands

```shell
open 127.0.0.1 2121
user anonymous
dir
get <filename>
put <filename>
cd <directory>
pwd
quit
```

## Notes

- Must be run on a Linux system.
- The server must have appropriate file read/write permissions.
- If logging in with a system user, the server needs permission to read the shadow file (it is recommended to run the server with sudo).
- Requires [libyaml](https://pyyaml.org/wiki/LibYAML). Install with `sudo pacman -S libyaml` (Arch) or `sudo apt install libyaml-dev` (Debian/Ubuntu).

## File Description

- `ftp_server.c`: FTP server main program
- `ftp_client.c`: FTP client main program
- `ftp_common.c` / `ftp_common.h`: Common utility functions and constants for both client and server

For more details, please refer to the source code comments.
