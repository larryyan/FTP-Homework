// ftp-actions.js
// 负责与后端 API 通信

function reloadDirectory() {
    loadDirectory(currentPath);
}

function navigateTo(path) {
    currentPath = normalizePath(path);
    loadDirectory(currentPath);
    history.pushState({}, "", window.location.pathname);
}

function normalizePath(path) {
    if (!path || path === "") return ".";
    path = path.replace(/^\/+|\/+$/g, "");
    return path === "" ? "." : path;
}

async function loadDirectory(path = ".") {
    currentPath = normalizePath(path);
    document.getElementById("backBtn").style.display = (currentPath !== ".") ? "inline-block" : "none";
    document.getElementById("logoutBtn").style.display = "inline-block";

    if (!username) {
        username = prompt("请输入FTP用户名");
        if (!username) return alert("必须输入用户名！");
        password = username === "anonymous" ? "" : prompt("请输入FTP密码");
        if (username !== "anonymous" && !password) return alert("必须输入密码！");
        localStorage.setItem("ftp_user", username);
        localStorage.setItem("ftp_passwd", password);
    }

    const res = await fetch(`/ftp/list`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ user: username, passwd: password, path: currentPath })
    });
    const data = await res.json();

    // 权限处理
    if (data.error && (data.error.includes("无权限") || data.error.includes("认证") || data.error.includes("密码") || data.error.includes("未登录"))) {
        localStorage.removeItem("ftp_user");
        localStorage.removeItem("ftp_passwd");
        username = prompt("无权限或未登录，请输入FTP用户名");
        if (!username) return alert("必须输入用户名！");
        password = username === "anonymous" ? "" : prompt("请输入FTP密码");
        if (username !== "anonymous" && !password) return alert("必须输入密码！");
        localStorage.setItem("ftp_user", username);
        localStorage.setItem("ftp_passwd", password);
        return loadDirectory(currentPath);
    }

    renderFileList(data.files || []);
}
