# Kui Desktop

**Kui** is a tiny cross-platform desktop framework that lets you build apps with HTML/CSS/JS — powered by [webview](https://github.com/webview/webview) and C.  
It aims to provide a lightweight alternative to Electron with a minimal footprint and a simple C API.

> ⚠️ **Status:** Work in Progress  
> Kui is still under heavy development. Expect breaking changes and missing features.  

---

## ✨ Features (current)

- 🚀 **Cross-platform:** Windows (WebView2), Linux (WebKitGTK), macOS (WebKit)  
- 📦 **Resource embedding:** HTML, CSS, JS, images, etc. are compiled directly into the shared library (`libkui.so` / `kui.dll`)  
- 🔗 **Native bridge:** Call into C from JavaScript with a simple `native(name, payload)` helper  
- 🧩 **Prelude script:** Injected automatically on every page load, sets up the bridge and resource resolver  
- 🖼️ **Demo app:** included under `/demo`, shows basic usage  

---

## 🔮 Roadmap

- [ ] **User resource loading** (currently only built-in resources are supported)  
- [ ] Expand API surface for easier native ↔ web communication  
- [ ] Improve packaging for Linux (Flatpak, DEB, RPM)  
- [ ] More examples & documentation  

---

## 🛠️ Building

### Requirements
- CMake ≥ 3.16
- A C compiler (GCC, Clang, or MSVC/MinGW)
- [webview](https://github.com/webview/webview) (fetched automatically via CMake)

### Linux
```bash
sudo apt install build-essential cmake ninja-build \
    libwebkit2gtk-4.0-dev libgtk-3-dev pkg-config
cmake -S . -B build -G Ninja
cmake --build build