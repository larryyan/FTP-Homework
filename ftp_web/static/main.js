let currentPath = ".";
let username = localStorage.getItem("ftp_user") || "";
let password = localStorage.getItem("ftp_passwd") || "";

window.addEventListener('DOMContentLoaded', () => {
    document.getElementById("backBtn").onclick = handleBack;
    document.getElementById("uploadBtn").onclick = handleUploadBtn;
    document.getElementById("upload").onchange = handleFileUpload;
    document.getElementById("newFolderForm").onsubmit = handleNewFolder;
    document.getElementById("reloadBtn").onclick = reloadDirectory;
    loadDirectory(currentPath);
});

function handleBack() {
    if (currentPath === "." || currentPath === "") return;
    let idx = currentPath.lastIndexOf("/");
    let newPath = idx > 0 ? currentPath.slice(0, idx) : ".";
    navigateTo(newPath);
}

function handleUploadBtn() {
    document.getElementById("upload").click();
}
