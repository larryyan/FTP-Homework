from flask import Blueprint, request, jsonify
from services.ftp_service import list_files

list_bp = Blueprint("list", __name__, url_prefix="/ftp")

@list_bp.route('/list', methods=['POST'])
def list_route():
    data = request.get_json(force=True)
    try:
        files = list_files(data.get("user"), data.get("passwd"), data.get("path", "."))
        return jsonify({"files": files})
    except Exception as e:
        return jsonify({"error": str(e)})
