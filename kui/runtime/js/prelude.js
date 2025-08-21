(() => {
    console.log("Initializing page.");

    // ---------- native bridge ----------
    async function native(name, payload) {
        const fn = globalThis[name];
        if (typeof fn !== "function") {
            throw new Error(name + " not bound");
        }

        // just pass the object; webview hands it as JSON text to C
        const out = await fn(payload ?? {});

        // out is either a stringified JSON or an object already
        if (typeof out === "string") {
            try {
                return JSON.parse(out);
            } catch (e) {
                console.error("native parse failed:", out, e);
                throw e;
            }
        }
        return out ?? {};
    }
    
    // API
    const kuiApi = Object.freeze({
        version: () => native("__kui_version", {}).then(x => x.version),
        native: native
    });

    // Install global
    globalThis.kui = globalThis.kui || kuiApi;
    // (optional classic alias; declare it so you don’t shadow/throw in strict mode)
    var kui = globalThis.kui;

    
    const finalize = () => {
        console.log("Finalizing page.");

        // ---------- resource registry ----------
        async function resolveToBlobURL(url) {
            const res = await kui.native("__kui_resolve", { url });
            const bytes = Uint8Array.from(atob(res.b64), c => c.charCodeAt(0));
            const blob = new Blob([bytes], { type: res.mime });
            return URL.createObjectURL(blob);
        }

        function upgradeAttr(el, attr, realAttr) {
            const val = el.getAttribute(attr);
            if (val) {
                el.removeAttribute(attr);
                resolveToBlobURL(val)
                    .then(url => el.setAttribute(realAttr, url))
                    .catch(err => {
                        console.error("Failed to resolve", val, err);
                    });
            }
        }

        function processElement(el) {
            if (el.hasAttribute("kui_src")) {
                upgradeAttr(el, "kui_src", "src");
            }
            if (el.hasAttribute("kui_href")) {
                upgradeAttr(el, "kui_href", "href");
            }
        }

        // Initial DOM walk
        document.querySelectorAll("[kui_src],[kui_href]").forEach(processElement);

        // Also watch for dynamically added elements
        const obs = new MutationObserver(muts => {
            for (const m of muts) {
                for (const n of m.addedNodes) {
                    if (n.nodeType === 1) { // element
                        processElement(n);
                        n.querySelectorAll?.("[kui_src],[kui_href]").forEach(processElement);
                    }
                }
            }
        });
        obs.observe(document.documentElement, { childList: true, subtree: true });
    };
    if (document.readyState === "complete") finalize();
    else window.addEventListener("load", finalize, { once: true });
})();