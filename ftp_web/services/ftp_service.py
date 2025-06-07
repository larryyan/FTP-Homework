# services/ftp_service.py
from ftplib import FTP
import io, re
from datetime import datetime

FTP_HOST = "127.0.0.1"
FTP_PORT = 21

def parse_ftp_list_line(line):
    regex = r'^([\-ld])([rwx\-]{9})\s+\d+\s+\S+\s+\S+\s+(\d+)\s+(\w{3})\s+(\d{1,2})\s+([\d:]{4,5}|\d{4})\s+(.+)$'
    m = re.match(regex, line)
    if not m: return None
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

def list_files(user, passwd, path="."):
    ftp = FTP()
    ftp.connect(FTP_HOST, FTP_PORT)
    ftp.login(user, passwd)
    ftp.cwd(path)
    lines = []
    ftp.dir(lines.append)
    items = [parse_ftp_list_line(line) for line in lines if parse_ftp_list_line(line)]
    ftp.quit()
    return items

def upload_file(user, passwd, path, filename, filecontent):
    ftp = FTP()
    ftp.connect(FTP_HOST, FTP_PORT)
    ftp.login(user, passwd)
    ftp.cwd(path)
    ftp.storbinary(f"STOR {filename}", io.BytesIO(filecontent))
    ftp.quit()

def delete(user, passwd, path, filename=None, dirname=None):
    ftp = FTP()
    ftp.connect(FTP_HOST, FTP_PORT)
    ftp.login(user, passwd)
    ftp.cwd(path)
    if dirname:
        ftp.rmd(dirname)
    elif filename:
        ftp.delete(filename)
    ftp.quit()

def rename(user, passwd, path, oldname, newname):
    ftp = FTP()
    ftp.connect(FTP_HOST, FTP_PORT)
    ftp.login(user, passwd)
    ftp.cwd(path)
    ftp.rename(oldname, newname)
    ftp.quit()

def mkdir(user, passwd, path, dirname):
    ftp = FTP()
    ftp.connect(FTP_HOST, FTP_PORT)
    ftp.login(user, passwd)
    ftp.cwd(path)
    ftp.mkd(dirname)
    ftp.quit()

def download(user, passwd, path, filename):
    ftp = FTP()
    ftp.connect(FTP_HOST, FTP_PORT)
    ftp.login(user, passwd)
    ftp.cwd(path)
    bio = io.BytesIO()
    ftp.retrbinary(f"RETR {filename}", bio.write)
    ftp.quit()
    bio.seek(0)
    return bio
