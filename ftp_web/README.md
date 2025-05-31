# Web-based FTP File Browser

This module provides a simple web-based FTP file browser implemented in Python (using the built-in `http.server` and `ftplib` modules) and a modern HTML+JavaScript frontend.

## Features

- Browse FTP server files and directories in your web browser.
- Download, upload, delete, rename files, and create new folders.
- Supports login with any FTP user, including anonymous login (no password required).
- User-friendly interface with file size and modification time display.
- No third-party Python dependencies required (uses standard library).

## How to Run

1. **Start the Web FTP Server**

   ```bash
   python3 ftp_web/ftp_client_web.py
   ```

   By default, the server listens on `http://localhost:8000/index.html`.

2. **Open the Web Interface**

   Open your browser and visit: [http://localhost:8000/index.html](http://localhost:8000/index.html)

3. **Login**

   - Enter your FTP username and password.
   - For anonymous login, enter `anonymous` as the username and leave the password blank.

4. **Browse and Manage Files**

   - Navigate directories, download and upload files, delete or rename items, and create new folders using the web interface.

## Notes

- The backend connects to an FTP server running on `127.0.0.1:2121` by default. You can modify `FTP_HOST` and `FTP_PORT` in `ftp_client_web.py` if needed.
- Make sure your FTP server is running and accessible before using the web interface.
- All operations are performed via the FTP protocol; file permissions and access depend on your FTP server configuration.
- Designed for use on Linux systems.

## File List

- `ftp_web/ftp_client_web.py` — Python backend server for handling FTP operations and HTTP requests.
- `ftp_web/index.html` — Frontend web interface for browsing and managing FTP files.
