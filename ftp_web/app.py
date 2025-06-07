from flask import Flask
from controllers import blueprints

app = Flask(__name__, static_folder="static", static_url_path="/")

# 注册所有控制器
for bp in blueprints:
    app.register_blueprint(bp)

@app.route('/')
def home():
    return app.send_static_file("index.html")

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=8021, debug=True)
