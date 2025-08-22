async function getVersion() {
    const v = await kui.version();
    document.getElementById("versionBox").textContent = v;
}