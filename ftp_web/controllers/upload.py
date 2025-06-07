from flask import Blueprint, request, jsonify
from services.ftp_service import upload_file

upload_bp = Blueprint("upload", __name__, url_prefix="/ftp")

@upload_bp.route('/upload', methods=['POST'])
def upload_route():
    user = request.form.get("user")
    passwd = request.form.get("passwd")
    path = request.form.get("path") or "."
    file = request.files.get("file")
    if not file:
        return jsonify({"success": False, "error": "No file uploaded"})
    try:
        upload_file(user, passwd, path, file.filename, file.read())
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"success": False, "error": str(e)})
