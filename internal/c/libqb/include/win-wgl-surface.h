// Private Windows drawable for the GLUT emulation layer.
// GLFW continues to own the top-level window and HGLRC. This helper owns only
// a client-local child HWND/DC and a removable parent-window subclass. Use it
// on the GLFW/window owner thread, and destroy it before glfwDestroyWindow.
#pragma once

#include "libqb-common.h"

#if defined(QB64_WINDOWS)
#include "logging.h"
#include <windows.h>
#include <commctrl.h>
#include <versionhelpers.h>
#include <bit>
#include <cstdint>
#include <string>

class WinWglSurface {
  public:
    using RefreshFunction = void (*)(GLFWwindow *);

    WinWglSurface() = default;
    WinWglSurface(const WinWglSurface &) = delete;
    WinWglSurface &operator=(const WinWglSurface &) = delete;
    ~WinWglSurface() { Destroy(); }

    bool Create(GLFWwindow *owner, RefreshFunction refresh = nullptr) {
        // Transparent/EGL windows retain GLFW's original drawable path. This
        // helper never accesses GLFW private structures or changes its sources.
        if (glfwGetWindowAttrib(owner, GLFW_TRANSPARENT_FRAMEBUFFER) == GLFW_TRUE ||
            glfwGetWindowAttrib(owner, GLFW_CONTEXT_CREATION_API) != GLFW_NATIVE_CONTEXT_API)
            return true;

        window = owner;
        refreshFunction = refresh;
        parent = glfwGetWin32Window(owner);
        context = glfwGetWGLContext(owner);
        if (!parent || !context)
            return Fail("cannot obtain the native window/context");

        RECT client = {};
        if (!GetClientRect(parent, &client))
            return Fail("cannot query the client area");

        // A unique class per helper makes teardown independent of other windows
        // and of GLFW's registration/class naming policy. The child has its own
        // DC, no non-client frame and no input/focus responsibility.
        module = GetModuleHandleW(nullptr);
        className = L"QB64PE_WGL_CLIENT_" + std::to_wstring(reinterpret_cast<uintptr_t>(this));
        WNDCLASSEXW wc = {};
        wc.cbSize = sizeof(wc);
        wc.style = CS_OWNDC;
        wc.lpfnWndProc = ChildProc;
        wc.hInstance = module;
        wc.lpszClassName = className.c_str();
        classAtom = RegisterClassExW(&wc);
        if (!classAtom)
            return Fail("cannot register the drawable class");

        width = client.right - client.left;
        height = client.bottom - client.top;
        child = CreateWindowExW(0, className.c_str(), L"", WS_CHILD | WS_VISIBLE | WS_DISABLED | WS_CLIPSIBLINGS,
                                0, 0, width, height, parent, nullptr, module, this);
        if (!child)
            return Fail("cannot create the drawable");
        dc = GetDC(child);
        if (!dc)
            return Fail("cannot obtain the drawable DC");

        // Reuse the exact pixel-format index selected by GLFW (including its
        // multisample/depth/stencil choice), not a separately chosen format.
        // WGL permits the same HGLRC on another DC on the same device when its
        // pixel format matches. GLFW still owns and destroys that HGLRC.
        const HDC parentDC = GetDC(parent); // GLFW's CS_OWNDC; no release needed.
        PIXELFORMATDESCRIPTOR pfd = {};
        const int format = parentDC ? GetPixelFormat(parentDC) : 0;
        if (!format || !DescribePixelFormat(parentDC, format, sizeof(pfd), &pfd) || !SetPixelFormat(dc, format, &pfd))
            return Fail("cannot copy the GLFW pixel format");

        // Resolve the supported subclass API dynamically: no new linker flags
        // or modification of third-party build files is required. Unlike raw
        // WNDPROC replacement, this API composes with other subclass users.
        subclassModule = LoadLibraryW(L"comctl32.dll");
        if (!subclassModule)
            return Fail("cannot load the subclass API");
        setSubclass = LoadAPI<SetSubclassFn>(subclassModule, "SetWindowSubclass");
        removeSubclass = LoadAPI<RemoveSubclassFn>(subclassModule, "RemoveWindowSubclass");
        defSubclass = LoadAPI<DefSubclassFn>(subclassModule, "DefSubclassProc");
        if (!setSubclass || !removeSubclass || !defSubclass)
            return Fail("cannot resolve the subclass API");
        if (!setSubclass(parent, ParentProc, reinterpret_cast<UINT_PTR>(this), reinterpret_cast<DWORD_PTR>(this)))
            return Fail("cannot install the resize subclass");
        subclassInstalled = true;

        // Match GLFW's Windows 7 DWM swap pacing. Modern Windows uses the WGL
        // swap interval directly. These APIs are optional and dynamically loaded.
        if (!IsWindows8OrGreater()) {
            dwmModule = LoadLibraryW(L"dwmapi.dll");
            if (dwmModule) {
                dwmComposition = LoadAPI<DwmCompositionFn>(dwmModule, "DwmIsCompositionEnabled");
                dwmFlush = LoadAPI<DwmFlushFn>(dwmModule, "DwmFlush");
            }
        }

        if (!Bind()) {
            Destroy();
            return false;
        }
        return true;
    }

    bool Active() const { return dc != nullptr; }

    bool Bind() const {
        if (!Active())
            return true;
        // Keep GLFW's public current-context bookkeeping in agreement with the
        // HGLRC, then select our DC. Native rebinds do not change the HGLRC.
        if (glfwGetCurrentContext() != window)
            glfwMakeContextCurrent(window);
        if (wglGetCurrentContext() == context && wglGetCurrentDC() == dc)
            return true;
        if (wglMakeCurrent(dc, context))
            return true;
        const DWORD error = GetLastError();
        glfwMakeContextCurrent(nullptr);
        libqb_log_error("Windows WGL surface: cannot bind the drawable (%lu)", static_cast<unsigned long>(error));
        return false;
    }

    bool Resize() {
        if (!Active() || !parent || IsIconic(parent))
            return true;
        RECT client = {};
        if (!GetClientRect(parent, &client))
            return false;
        const int newWidth = client.right - client.left;
        const int newHeight = client.bottom - client.top;
        // Keep a usable drawable while minimized. Query the real client RECT,
        // rather than the truncated 16-bit fields in WM_SIZE's lParam.
        if (newWidth <= 0 || newHeight <= 0 || (newWidth == width && newHeight == height))
            return true;
        if (!SetWindowPos(child, nullptr, 0, 0, newWidth, newHeight, SWP_NOACTIVATE | SWP_NOZORDER | SWP_NOCOPYBITS))
            return false;
        width = newWidth;
        height = newHeight;
        return true;
    }

    void RequestRefresh() {
        // Windows' native move/size loop can suspend the ordinary GLFW/idle
        // loop. Queue one render after the current DPI/size notification has
        // finished, and coalesce later notifications into that same request.
        // Mere movement, unchanged sizes and time spent holding the caption
        // do not schedule work. No timer or additional event pump is needed.
        if (!Active() || !inSizeMove || !refreshFunction || refreshPending || IsIconic(parent))
            return;
        if (!refreshMessage) {
            // The parent window class belongs to GLFW. A registered message
            // avoids assuming that its WM_USER/WM_APP range is available.
            refreshMessage = RegisterWindowMessageW(L"QB64PE_WGL_CLIENT_REFRESH_7F59A824");
            if (!refreshMessage) {
                libqb_log_error("Windows WGL surface: cannot register the refresh message (%lu)",
                                static_cast<unsigned long>(GetLastError()));
                return; // The normal idle/refresh path remains available.
            }
        }
        refreshPending = true;
        if (!PostMessageW(parent, refreshMessage, reinterpret_cast<WPARAM>(this), 0)) {
            refreshPending = false;
            libqb_log_error("Windows WGL surface: cannot post the refresh message (%lu)",
                            static_cast<unsigned long>(GetLastError()));
        }
    }

    void Swap() const {
        if (!Bind())
            return;
        // GLUTEmu uses interval 1. GLFW disables that WGL interval on a
        // composited Windows 7 desktop, so preserve its DwmFlush path here.
        BOOL composition = FALSE;
        if (!glfwGetWindowMonitor(window) && dwmComposition && dwmFlush &&
            SUCCEEDED(dwmComposition(&composition)) && composition)
            dwmFlush();
        if (!::SwapBuffers(dc))
            libqb_log_error("Windows WGL surface: cannot swap the drawable (%lu)", static_cast<unsigned long>(GetLastError()));
    }

    void Destroy() {
        // Queued requests cannot render during teardown. They carry only an
        // identity token; removing the subclass removes their consumer.
        inSizeMove = refreshPending = false;
        refreshFunction = nullptr;
        // Unbind before releasing the DC or destroying its HWND. Do not delete
        // the HGLRC: its owner is the unmodified GLFW context implementation.
        if (dc && wglGetCurrentDC() == dc)
            glfwMakeContextCurrent(nullptr);
        if (subclassInstalled && parent) {
            removeSubclass(parent, ParentProc, reinterpret_cast<UINT_PTR>(this));
            subclassInstalled = false;
        }
        if (dc) {
            ReleaseDC(child, dc);
            dc = nullptr;
        }
        if (child) {
            DestroyWindow(child);
            child = nullptr;
        }
        if (classAtom) {
            UnregisterClassW(className.c_str(), module);
            classAtom = 0;
        }
        if (dwmModule) {
            FreeLibrary(dwmModule);
            dwmModule = nullptr;
        }
        if (subclassModule) {
            FreeLibrary(subclassModule);
            subclassModule = nullptr;
        }
        window = nullptr;
        parent = nullptr;
        context = nullptr;
        setSubclass = nullptr;
        removeSubclass = nullptr;
        defSubclass = nullptr;
        dwmComposition = nullptr;
        dwmFlush = nullptr;
        width = height = 0;
        refreshMessage = 0;
    }

  private:
    using SetSubclassFn = BOOL(WINAPI *)(HWND, SUBCLASSPROC, UINT_PTR, DWORD_PTR);
    using RemoveSubclassFn = BOOL(WINAPI *)(HWND, SUBCLASSPROC, UINT_PTR);
    using DefSubclassFn = LRESULT(WINAPI *)(HWND, UINT, WPARAM, LPARAM);
    using DwmCompositionFn = HRESULT(WINAPI *)(BOOL *);
    using DwmFlushFn = HRESULT(WINAPI *)();

    template <typename Function> static Function LoadAPI(HMODULE module, const char *name) {
        // Windows loader function pointers have the target pointer width. Copy
        // that representation into the exact WINAPI prototype without a
        // mismatched-function-type cast or a 32-bit integer intermediate.
        return std::bit_cast<Function>(GetProcAddress(module, name));
    }

    bool Fail(const char *reason) {
        const DWORD error = GetLastError();
        libqb_log_error("Windows WGL surface: %s (%lu)", reason, static_cast<unsigned long>(error));
        Destroy();
        return false;
    }

    static LRESULT CALLBACK ChildProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
        if (message == WM_NCCREATE) {
            const auto *create = reinterpret_cast<const CREATESTRUCTW *>(lParam);
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(create->lpCreateParams));
        }
        auto *self = reinterpret_cast<WinWglSurface *>(GetWindowLongPtrW(hwnd, GWLP_USERDATA));
        // Validate the damaged child area before presenting through its WGL DC.
        // Parent GLFW damage callbacks alone cannot cover child invalidation.
        // Reuse the renderer's last frame, including when no new BASIC frame is
        // ready; the owning GLUT layer marks this as a damage refresh and guards
        // against recursive rendering. No timer or private event pump is used.
        const LRESULT result = DefWindowProcW(hwnd, message, wParam, lParam);
        if (message == WM_PAINT && self && self->Active() && self->refreshFunction && !IsIconic(self->parent))
            self->refreshFunction(self->window);
        if (message == WM_NCDESTROY)
            SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);
        return result;
    }

    static LRESULT CALLBACK ParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR reference) {
        auto *self = reinterpret_cast<WinWglSurface *>(reference);
        if (self->refreshMessage && message == self->refreshMessage && wParam == id && lParam == 0) {
            const bool requested = self->refreshPending;
            self->refreshPending = false;
            // The request is consumed by the native move/size loop, after
            // GLFW and libqb have published coherent size/scale state. If
            // dragging already ended, ordinary idle rendering takes over.
            if (requested && self->inSizeMove && !IsIconic(hwnd) && self->refreshFunction)
                self->refreshFunction(self->window);
            return 0;
        }
        if (message == WM_ENTERSIZEMOVE)
            self->inSizeMove = true;
        else if (message == WM_EXITSIZEMOVE)
            self->inSizeMove = self->refreshPending = false;
        // Run before DefSubclassProc forwards to GLFW: its maximize, restore
        // and framebuffer/window-size callbacks must see the resized drawable.
        if (message == WM_SIZE && wParam != SIZE_MINIMIZED && !self->Resize())
            libqb_log_error("Windows WGL surface: cannot resize the drawable (%lu)", static_cast<unsigned long>(GetLastError()));
        if (message == WM_NCDESTROY) {
            self->inSizeMove = self->refreshPending = false;
            self->removeSubclass(hwnd, ParentProc, id);
            self->subclassInstalled = false;
        }
        return self->defSubclass(hwnd, message, wParam, lParam);
    }

    GLFWwindow *window = nullptr;
    HWND parent = nullptr, child = nullptr;
    HDC dc = nullptr;
    HGLRC context = nullptr;
    HINSTANCE module = nullptr;
    ATOM classAtom = 0;
    std::wstring className;
    int width = 0, height = 0;
    bool subclassInstalled = false;
    bool inSizeMove = false, refreshPending = false;
    UINT refreshMessage = 0;
    RefreshFunction refreshFunction = nullptr;
    HMODULE subclassModule = nullptr, dwmModule = nullptr;
    SetSubclassFn setSubclass = nullptr;
    RemoveSubclassFn removeSubclass = nullptr;
    DefSubclassFn defSubclass = nullptr;
    DwmCompositionFn dwmComposition = nullptr;
    DwmFlushFn dwmFlush = nullptr;
};
#endif
