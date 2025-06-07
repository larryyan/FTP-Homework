from flask import Blueprint, request, jsonify
from services.ftp_service import mkdir

mkdir_bp = Blueprint("mkdir", __name__, url_prefix="/ftp")

@mkdir_bp.route('/mkdir', methods=['POST'])
def mkdir_route():
    data = request.get_json(force=True)
    try:
        mkdir(
            data.get("user"),
            data.get("passwd"),
            data.get("path", "."),
            data.get("name")
        )
        return jsonify({"success": True})
    except Exception as e:
        return jsonify({"success": False, "error": str(e)})
