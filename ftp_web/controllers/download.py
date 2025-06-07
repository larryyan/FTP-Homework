from flask import Blueprint, request, send_file
from services.ftp_service import download

download_bp = Blueprint("download", __name__, url_prefix="/ftp")

@download_bp.route('/download', methods=['GET'])
def download_route():
    user = request.args.get("user")
    passwd = request.args.get("passwd")
    path = request.args.get("path", ".")
    filename = request.args.get("file")
    try:
        bio = download(user, passwd, path, filename)
        return send_file(bio, as_attachment=True, download_name=filename or 'file', mimetype="application/octet-stream")
    except Exception as e:
        return f"FTP 下载失败: {e}", 500
