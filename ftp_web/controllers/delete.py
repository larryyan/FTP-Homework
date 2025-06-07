from flask import Blueprint, request, jsonify
from services.ftp_service import delete

delete_bp = Blueprint("delete", __name__, url_prefix="/ftp")

@delete_bp.route('/delete', methods=['POST'])
def delete_route():
    data = request.get_json(force=True)
    try:
        delete(
            data.get("user"),
            data.get("passwd"),
            data.get("path", "."),
            filename=data.get("file"),
            dirname=data.get("dir")
        )
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"success": False, "error": str(e)})
