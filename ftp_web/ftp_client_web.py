from http.server import SimpleHTTPRequestHandler, HTTPServer
from urllib.parse import urlparse, parse_qs
from ftplib import FTP
import json
import cgi
import io
import re
from datetime import datetime

FTP_HOST = "127.0.0.1"
FTP_PORT = 2121

def parse_ftp_list_line(line):
    """
    解析FTP LIST命令返回的单行, 类Unix格式: 
    drwxr-xr-x  2 owner group 4096 Jan 01 12:34 dirname
    -rw-r--r--  1 owner group 1234 Jan 01 2022 filename.txt
    """
    regex = r'^([\-ld])([rwx\-]{9})\s+\d+\s+\S+\s+\S+\s+(\d+)\s+(\w{3})\s+(\d{1,2})\s+([\d:]{4,5}|\d{4})\s+(.+)$'
    m = re.match(regex, line)
    if not m:
        return None
    type_flag = m.group(1)
    size = int(m.group(3))
    month = m.group(4)
    day = int(m.group(5))
    time_or_year = m.group(6)
    name = m.group(7)

    ftype = "dir" if type_flag == "d" else "file"

    current_year = datetime.now().year
    try:
        if ':' in time_or_year:
            dt_str = f"{month} {day} {current_year} {time_or_year}"
            dt = datetime.strptime(dt_str, "%b %d %Y %H:%M")
        else:
            dt_str = f"{month} {day} {time_or_year} 00:00"
            dt = datetime.strptime(dt_str, "%b %d %Y %H:%M")
        mtime = dt.strftime("%Y-%m-%d %H:%M:%S")
    except Exception:
        mtime = None

    return {"name": name, "type": ftype, "size": size, "modify": mtime}


class FTPHandler(SimpleHTTPRequestHandler):

    def do_POST(self):
        if self.path == "/list":
            length = int(self.headers.get('Content-Length'))
            data = json.loads(self.rfile.read(length).decode())
            user = data.get("user")
            passwd = data.get("passwd")
            path = data.get("path", ".")

            try:
                ftp = FTP()
                ftp.connect(FTP_HOST, FTP_PORT)
                ftp.login(user, passwd)
                ftp.cwd(path)

                lines = []
                ftp.dir(lines.append)

                items = []
                for line in lines:
                    item = parse_ftp_list_line(line)
                    if item:
                        items.append(item)
                ftp.quit()

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"files": items}).encode())
            except Exception as e:
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"error": str(e)}).encode())

        elif self.path == "/uploadFile":
            content_type = self.headers.get('Content-Type')
            if not content_type:
                self.send_error(400, "Missing Content-Type")
                return
            ctype, pdict = cgi.parse_header(content_type)
            if ctype != 'multipart/form-data':
                self.send_error(400, "Content-Type must be multipart/form-data")
                return

            pdict['boundary'] = bytes(pdict['boundary'], "utf-8")
            length = int(self.headers.get('Content-Length'))

            # 使用FieldStorage读取上传的文件和表单字段
            fs = cgi.FieldStorage(
                fp=self.rfile,
                headers=self.headers,
                environ={'REQUEST_METHOD': 'POST'},
                keep_blank_values=True
            )
            user = fs.getvalue("user")
            passwd = fs.getvalue("passwd")
            path = fs.getvalue("path") or "."

            if "file" not in fs:
                self.send_error(400, "No file uploaded")
                return

            file_field = fs["file"]
            filename = file_field.filename
            filecontent = file_field.file.read()

            try:
                ftp = FTP()
                ftp.connect(FTP_HOST, FTP_PORT)
                ftp.login(user, passwd)
                ftp.cwd(path)
                ftp.storbinary(f"STOR {filename}", io.BytesIO(filecontent))
                ftp.quit()

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": True}).encode())
            except Exception as e:
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": False, "error": str(e)}).encode())
        
        elif self.path == "/uploadFolder":
            length = int(self.headers.get('Content-Length'))
            data = json.loads(self.rfile.read(length).decode())
            user, passwd = data.get("user"), data.get("passwd")
            path = data.get("path", ".")
            dirname = data.get("name")

            try:
                ftp = FTP()
                ftp.connect(FTP_HOST, FTP_PORT)
                ftp.login(user, passwd)
                ftp.cwd(path)
                ftp.mkd(dirname)
                ftp.quit()

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": True}).encode())
            except Exception as e:
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": False, "error": str(e)}).encode())
        
        elif self.path == "/delete":
            length = int(self.headers.get('Content-Length'))
            data = json.loads(self.rfile.read(length).decode())
            user, passwd = data.get("user"), data.get("passwd")

            try:
                ftp = FTP()
                ftp.connect(FTP_HOST, FTP_PORT)
                ftp.login(user, passwd)

                # 文件和目录分别删除
                if "dir" in data:
                    path = data.get("path", ".")
                    dirname = data.get("dir")
                    ftp.cwd(path)
                    ftp.rmd(dirname)

                else:
                    path = data.get("path", ".")
                    filename = data.get("file")
                    ftp.cwd(path)
                    ftp.delete(filename)

                # 关闭FTP连接
                ftp.quit()
                
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": True}).encode())
            except Exception as e:
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": False, "error": str(e)}).encode())

        elif self.path == "/rename":
            length = int(self.headers.get('Content-Length'))
            data = json.loads(self.rfile.read(length).decode())
            user, passwd = data.get("user"), data.get("passwd")
            path = data.get("path", ".")
            oldname = data.get("oldname")
            newname = data.get("newname")

            try:
                ftp = FTP()
                ftp.connect(FTP_HOST, FTP_PORT)
                ftp.login(user, passwd)
                ftp.cwd(path)
                ftp.rename(oldname, newname)
                ftp.quit()

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": True}).encode())
            except Exception as e:
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": False, "error": str(e)}).encode())

        elif self.path == "/mkdir":
            length = int(self.headers.get('Content-Length'))
            data = json.loads(self.rfile.read(length).decode())
            user, passwd = data.get("user"), data.get("passwd")
            path = data.get("path", ".")
            dirname = data.get("name")

            try:
                ftp = FTP()
                ftp.connect(FTP_HOST, FTP_PORT)
                ftp.login(user, passwd)
                ftp.cwd(path)
                ftp.mkd(dirname)
                ftp.quit()

                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": True}).encode())
            except Exception as e:
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"success": False, "error": str(e)}).encode())

        else:
            self.send_error(404, "Not Found")

    def do_GET(self):
        if self.path.startswith("/download"):
            query = parse_qs(urlparse(self.path).query)
            user = query.get("user", [""])[0]
            passwd = query.get("passwd", [""])[0]
            path = query.get("path", ["."])[0]
            filename = query.get("file", [""])[0]

            try:
                ftp = FTP()
                ftp.connect(FTP_HOST, FTP_PORT)
                ftp.login(user, passwd)
                ftp.cwd(path)

                self.send_response(200)
                self.send_header("Content-Type", "application/octet-stream")
                self.send_header("Content-Disposition", f'attachment; filename="{filename}"')
                self.end_headers()

                ftp.retrbinary(f"RETR {filename}", self.wfile.write)
                ftp.quit()

            except Exception as e:
                self.send_error(500, f"FTP 下载失败: {str(e)}")
        else:
            super().do_GET()


if __name__ == "__main__":
    print("🌐 Web FTP 浏览器启动: http://localhost:8000/index.html")
    server = HTTPServer(("0.0.0.0", 8000), FTPHandler)
    server.serve_forever()
