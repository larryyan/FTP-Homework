# Web-based FTP File Browser

This module provides a simple web-based FTP file browser implemented in Python (using Flask and the standard `ftplib` module) with a modern HTML+JavaScript frontend.

## Features

- Browse FTP server files and directories in your web browser.
- Download, upload, delete, rename files, and create new folders.
- Supports login with any FTP user, including anonymous login (no password required).
- User-friendly interface with file size and modification time display.
- No third-party Python dependencies required except Flask.
- **Requires Python 3.6 or newer.**

## How to Run

   1. **Install Flask**

      ```bash
      pip install flask
      ```

   2. **Start the Web FTP Server**

      ```bash
      cd ftp_web
      python3 app.py
      ```

      By default, the server listens on [http://localhost:8021/](http://localhost:8021/).

   3. **Open the Web Interface**

      Open your browser and visit: [http://localhost:8021/](http://localhost:8021/)

   4. **Login**

      - Enter your FTP username and password.
      - For anonymous login, enter `anonymous` as the username and leave the password blank.

   5. **Browse and Manage Files**

      - Navigate directories, download and upload files, delete or rename items, and create new folders using the web interface.

## Notes

- The backend connects to an FTP server running on `127.0.0.1:2121` by default. You can modify `FTP_HOST` and `FTP_PORT` in [`services/ftp_service.py`](services/ftp_service.py) if needed.
- Make sure your FTP server is running and accessible before using the web interface.
- All operations are performed via the FTP protocol; file permissions and access depend on your FTP server configuration.
- Designed for use on Linux systems.

## File List

- `app.py` — Flask backend server for handling FTP operations and HTTP requests.
- `controllers/` — Flask blueprints for each API endpoint.
- `services/` — FTP logic abstraction.
- `static/` — Frontend web interface (HTML, JS, CSS).
