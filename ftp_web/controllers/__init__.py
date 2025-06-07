from .list import list_bp
from .upload import upload_bp
from .delete import delete_bp
from .rename import rename_bp
from .mkdir import mkdir_bp
from .download import download_bp

blueprints = [
    list_bp,
    upload_bp,
    delete_bp,
    rename_bp,
    mkdir_bp,
    download_bp
]
