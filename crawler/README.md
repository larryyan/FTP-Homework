# C Socket-based Web Crawler

This module provides a simple web crawler implemented in C, supporting both HTTP and HTTPS (via OpenSSL). It can fetch HTML pages and images from a given URL and save them to the `download` directory.

## Features

- Supports both HTTP and HTTPS protocols.
- Downloads the main HTML page and all images (`.jpg`, `.jpeg`, `.png`, `.gif`) found in `<img src=...>` tags.
- Saves all downloaded files into a `download/<domain>` directory.

## Build

Make sure you have OpenSSL development libraries installed.

**On Arch Linux:**

```bash
sudo pacman -S openssl
```

**On Ubuntu/Debian:**

```bash
sudo apt install libssl-dev
```

Compile the crawler:

```bash
make
```

or compile manually:

```bash
gcc -o crawler_https crawler.c -lssl -lcrypto
```

## Usage

```bash
./crawler_https https://example.com
```

- The program will create a `download` directory (if not exists) and a subdirectory named after the domain.
- The main HTML and all images will be saved in this subdirectory.

## Example

```bash
./crawler_https https://www.example.com
```

- Downloads `index.html` and all images to `download/www.example.com/`.

## Notes

- This program is for educational purposes and demonstrates basic socket and OpenSSL usage in C.
- Only supports basic HTML parsing for `<img src=...>` tags.
- Should be run on a Linux system.
