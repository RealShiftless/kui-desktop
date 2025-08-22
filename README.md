# Kui Desktop alpha 1.0

**Kui** is a tiny cross-platform desktop framework that lets you build apps with HTML/CSS/JS — powered by [webview](https://github.com/webview/webview) and C.  
It aims to provide a lightweight alternative to Electron with a minimal footprint and a simple C API.

> ⚠️ **Status:** Work in Progress  
> Kui is still under heavy development. Expect breaking changes and missing features.  

---

![Kui on Windows and Linux](docs\screenshots\multi-platform.png)

## ✨ Features (current)

- 🚀 **Cross-platform:** Windows (WebView2), Linux (WebKitGTK), macOS (WebKit)  
- 📦 **Resource embedding:** HTML, CSS, JS, images, etc. are compiled directly into the executable
- 🔗 **Native bridge:** Call into C from JavaScript with a simple `kui.native(name, payload)` helper  
- 🧩 **Prelude script:** Injected automatically on every page load, sets up the bridge and resource resolver  
- 🖼️ **Demo app:** included under `/src` and `\runtime`, shows basic usage  

---

## 🔮 Roadmap

- [x] **User resource loading & embedding**
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
```