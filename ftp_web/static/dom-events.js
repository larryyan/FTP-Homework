// dom-events.js
// 处理DOM相关的事件和渲染

function renderFileList(files) {
    const tbody = document.getElementById("filelist");
    tbody.innerHTML = "";
    files = files.filter(item => item.name !== "." && item.name !== "..");
    if (files.length) {
        files.forEach(item => {
            const tr = document.createElement("tr");

            const nameTd = document.createElement("td");
            const hrefPath = (currentPath === "." ? item.name : `${currentPath}/${item.name}`);
            nameTd.innerHTML = item.type === "dir"
                ? `<a href="#" onclick="navigateTo('${hrefPath}')">📂 ${item.name}</a>`
                : `<a href="/ftp/download?user=${encodeURIComponent(username)}&passwd=${encodeURIComponent(password)}&path=${encodeURIComponent(currentPath)}&file=${encodeURIComponent(item.name)}" target="_blank">📄 ${item.name}</a>`;
            tr.appendChild(nameTd);

            const sizeTd = document.createElement("td");
            sizeTd.textContent = item.size >= 0 ? formatSize(item.size) : "—";
            tr.appendChild(sizeTd);

            const timeTd = document.createElement("td");
            timeTd.textContent = item.modify || "—";
            tr.appendChild(timeTd);

            const opTd = document.createElement("td");
            const delBtn = document.createElement("button");
            delBtn.textContent = "删除";
            delBtn.onclick = async e => {
                e.stopPropagation();
                if (!confirm(`确定删除 ${item.type === "dir" ? "文件夹" : "文件"} \"${item.name}\" 吗？`)) return;
                const resp = await fetch(`/ftp/delete`, {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    body: JSON.stringify({
                        user: username,
                        passwd: password,
                        path: currentPath,
                        file: item.name
                    })
                });
                const result = await resp.json();
                if (result.success) {
                    alert("删除成功");
                    loadDirectory(currentPath);
                } else {
                    alert("删除失败: " + result.error);
                }
            };
            opTd.appendChild(delBtn);
            tr.appendChild(opTd);

            tbody.appendChild(tr);
        });
    } else {
        tbody.innerHTML = `<tr><td colspan="4" style="text-align:center; color:#999;">空目录或无权限查看</td></tr>`;
    }
}

function formatSize(bytes) {
    if (bytes === 0) return '0 B';
    const k = 1024;
    const sizes = ['B', 'KB', 'MB', 'GB', 'TB'];
    const i = Math.floor(Math.log(bytes) / Math.log(k));
    return (bytes / Math.pow(k, i)).toFixed(2) + ' ' + sizes[i];
}

function handleFileUpload() {
    const fileInput = this;
    if (!fileInput.files.length) return;
    const file = fileInput.files[0];
    const formData = new FormData();
    formData.append("file", file);
    formData.append("user", username);
    formData.append("passwd", password);
    formData.append("path", currentPath);
    formData.append("filename", file.name);

    fetch("/ftp/upload", { method: "POST", body: formData })
        .then(res => res.json())
        .then(data => {
            if (!data.success) alert(`上传 ${file.name} 失败: ${data.error}`);
            else alert("上传完成");
            fileInput.value = "";
            loadDirectory(currentPath);
        });
}

function handleNewFolder(e) {
    e.preventDefault();
    const folderName = document.getElementById("newFolderName").value.trim();
    if (!folderName) return alert("请输入文件夹名称");
    fetch("/ftp/mkdir", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ user: username, passwd: password, path: currentPath, name: folderName })
    })
    .then(res => res.json())
    .then(result => {
        if (result.success) {
            alert("文件夹创建成功");
            loadDirectory(currentPath);
        } else {
            alert("创建失败: " + result.error);
        }
    });
}

function logout() {
    localStorage.removeItem("ftp_user");
    localStorage.removeItem("ftp_passwd");
    alert("已退出登录");
    window.location.reload();
}
