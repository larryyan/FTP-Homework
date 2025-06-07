from flask import Blueprint, request, jsonify
from services.ftp_service import rename

rename_bp = Blueprint("rename", __name__, url_prefix="/ftp")

@rename_bp.route('/rename', methods=['POST'])
def rename_route():
    data = request.get_json(force=True)
    try:
        rename(
            data.get("user"),
            data.get("passwd"),
            data.get("path", "."),
            data.get("oldname"),
            data.get("newname")
        )
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"success": False, "error": str(e)})
