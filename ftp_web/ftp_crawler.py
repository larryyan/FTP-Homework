#!/usr/bin/env python3
"""
FTP Web Browser Crawler

This script crawls the provided web-based FTP browser to locate and download files
matching a specified pattern.

Usage:
    python ftp_crawler.py --base-url http://localhost:8000 --user <username> --passwd <password> \
        --pattern "*.txt" --dest ./downloads

"""
import os
import argparse
import fnmatch
import requests
from urllib.parse import urljoin, urlencode

def list_directory(base_url, user, passwd, path):
    """
    Send a POST to /list to retrieve directory contents as JSON.
    """
    url = urljoin(base_url, '/list')
    payload = {'user': user, 'passwd': passwd, 'path': path}
    resp = requests.post(url, json=payload)
    resp.raise_for_status()
    data = resp.json()
    if 'error' in data:
        raise RuntimeError(f"Error listing {path}: {data['error']}")
    return data.get('files', [])


def download_file(base_url, user, passwd, path, filename, dest_dir):
    """
    Download a file via the /download endpoint, saving to dest_dir.
    """
    params = {'user': user, 'passwd': passwd, 'path': path, 'file': filename}
    download_url = urljoin(base_url, '/download') + '?' + urlencode(params)
    resp = requests.get(download_url, stream=True)
    resp.raise_for_status()
    os.makedirs(dest_dir, exist_ok=True)
    out_path = os.path.join(dest_dir, filename)
    with open(out_path, 'wb') as f:
        for chunk in resp.iter_content(chunk_size=8192):
            if chunk:
                f.write(chunk)
    print(f"Downloaded: {out_path}")


def crawl_and_download(base_url, user, passwd, start_path, pattern, dest_dir, recursive=False):
    """
    Crawl directories starting from start_path, skip '.' and '..', and download matching files.
    Uses a visited set to avoid infinite loops.
    """
    stack = [start_path]
    visited = set()

    while stack:
        current = stack.pop()
        if current in visited:
            continue
        visited.add(current)

        print(f"Listing: {current}")
        items = list_directory(base_url, user, passwd, current)

        for item in items:
            name = item['name']
            ftype = item['type']

            # 跳过当前和父目录
            if name in ('.', '..'):
                continue

            # 构造子目录全路径
            if ftype == 'dir':
                subpath = name if current == '.' else f"{current}/{name}"
                if recursive:
                    stack.append(subpath)

            # 下载符合模式的文件
            elif ftype == 'file' and fnmatch.fnmatch(name, pattern):
                download_file(base_url, user, passwd, current, name, dest_dir)


def main():
    parser = argparse.ArgumentParser(description="Crawl FTP web UI and download matching files.")
    parser.add_argument('--base-url', required=True, help='Base URL of the FTP web interface, e.g. http://localhost:8000')
    parser.add_argument('--user', required=True, help='FTP username')
    parser.add_argument('--passwd', required=True, help='FTP password')
    parser.add_argument('--pattern', required=True, help='Filename pattern to match (e.g. "*.log")')
    parser.add_argument('--dest', default='./downloads', help='Local directory to save files')
    parser.add_argument('--path', default='.', help='Remote start path (default: root ".")')
    parser.add_argument('--recursive', action='store_true', help='Recursively crawl subdirectories')
    args = parser.parse_args()

    crawl_and_download(
        base_url=args.base_url,
        user=args.user,
        passwd=args.passwd,
        start_path=args.path,
        pattern=args.pattern,
        dest_dir=args.dest,
        recursive=args.recursive
    )

if __name__ == '__main__':
    main()
