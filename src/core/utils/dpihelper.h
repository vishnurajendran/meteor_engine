//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #include <shellscalingapi.h>
#elif defined(__APPLE__)
    #include <ApplicationServices/ApplicationServices.h>
    #ifdef __OBJC__
        #import <AppKit/AppKit.h>
    #endif
#elif defined(__linux__)
    #include <X11/Xlib.h>
    #include <X11/extensions/Xrandr.h>
    #include <cstdlib>   // for setenv
#endif

#include <cmath>

class DPIHelper
{
public:
    // -------------------------------------------------------------------------
    // Call ONCE before any window is created.
    // Tells the OS this process handles DPI itself — prevents bitmap stretching.
    // -------------------------------------------------------------------------
    static void InitDPIAwareness()
    {
#if defined(_WIN32) || defined(_WIN64)
        InitWindowsDPIAwareness();
#elif defined(__APPLE__)
        // macOS: nothing to call at runtime — set NSHighResolutionCapable in
        // the app's Info.plist instead (see comment below).
        // If you are NOT using a bundle (e.g. CLI / raw SFML), the key has no
        // effect, so Retina is disabled by default. Nothing to do here.
#elif defined(__linux__)
        InitLinuxDPIAwareness();
#endif
    }

    // -------------------------------------------------------------------------
    // Returns the DPI scale factor for the primary / foreground monitor.
    //   1.0  = 96 DPI  (Windows default, most Linux)
    //   1.25 = 120 DPI (Windows 125 %)
    //   2.0  = 192 DPI (Retina / 4 K)
    // -------------------------------------------------------------------------
    static float GetDPIScaleFactor()
    {
#if defined(_WIN32) || defined(_WIN64)
        return GetWindowsDPIScaleFactor();
#elif defined(__APPLE__)
        return GetMacOSDPIScaleFactor();
#elif defined(__linux__)
        return GetLinuxDPIScaleFactor();
#else
        return 1.0f;
#endif
    }

private:

    // =========================================================================
    // WINDOWS
    // =========================================================================
#if defined(_WIN32) || defined(_WIN64)

    static void InitWindowsDPIAwareness()
    {
        // ---- Tier 1: Windows 10 1703+ — Per-Monitor V2 (best) --------------
        // Gives you WM_DPICHANGED with suggested rects, non-client area
        // scaling, and correct dialog scaling for free.
        HMODULE hUser32 = LoadLibraryA("user32.dll");
        if (hUser32)
        {
            typedef BOOL (WINAPI* Fn_SetProcessDpiAwarenessContext)(DPI_AWARENESS_CONTEXT);
            auto pFn = reinterpret_cast<Fn_SetProcessDpiAwarenessContext>(
                GetProcAddress(hUser32, "SetProcessDpiAwarenessContext"));

            if (pFn)
            {
                // Try V2 first, fall back to V1 if the constant isn't recognised
                if (!pFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2))
                    pFn(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE);

                FreeLibrary(hUser32);
                return;
            }
            FreeLibrary(hUser32);
        }

        // ---- Tier 2: Windows 8.1 — Per-Monitor V1 ---------------------------
        HMODULE hShcore = LoadLibraryA("Shcore.dll");
        if (hShcore)
        {
            typedef HRESULT (WINAPI* Fn_SetProcessDpiAwareness)(PROCESS_DPI_AWARENESS);
            auto pFn = reinterpret_cast<Fn_SetProcessDpiAwareness>(
                GetProcAddress(hShcore, "SetProcessDpiAwareness"));

            if (pFn)
            {
                pFn(PROCESS_PER_MONITOR_DPI_AWARE);
                FreeLibrary(hShcore);
                return;
            }
            FreeLibrary(hShcore);
        }

        // ---- Tier 3: Windows Vista — System DPI aware (last resort) ---------
        // Prevents bitmap stretching but uses a single system-wide DPI value.
        SetProcessDPIAware();
    }

    static float GetWindowsDPIScaleFactor()
    {
        // Dynamically load so the binary still runs on older Windows
        HMODULE hUser32 = LoadLibraryA("user32.dll");
        if (hUser32)
        {
            // Windows 10 1607+: GetDpiForWindow — most accurate
            typedef UINT (WINAPI* Fn_GetDpiForWindow)(HWND);
            auto pGetDpiForWindow = reinterpret_cast<Fn_GetDpiForWindow>(
                GetProcAddress(hUser32, "GetDpiForWindow"));

            if (pGetDpiForWindow)
            {
                HWND hwnd = GetForegroundWindow();
                if (!hwnd) hwnd = GetDesktopWindow();
                UINT dpi = pGetDpiForWindow(hwnd);
                FreeLibrary(hUser32);
                return dpi / 96.0f;
            }
            FreeLibrary(hUser32);
        }

        // Windows 8.1+: GetDpiForMonitor
        HMODULE hShcore = LoadLibraryA("Shcore.dll");
        if (hShcore)
        {
            typedef HRESULT (WINAPI* Fn_GetDpiForMonitor)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
            auto pGetDpiForMonitor = reinterpret_cast<Fn_GetDpiForMonitor>(
                GetProcAddress(hShcore, "GetDpiForMonitor"));

            if (pGetDpiForMonitor)
            {
                HWND hwnd = GetForegroundWindow();
                HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTOPRIMARY);
                UINT dpiX = 96, dpiY = 96;
                pGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY);
                FreeLibrary(hShcore);
                return dpiX / 96.0f;
            }
            FreeLibrary(hShcore);
        }

        // Fallback: system DPI via GDI
        HDC hdc = GetDC(nullptr);
        int dpi = GetDeviceCaps(hdc, LOGPIXELSX);
        ReleaseDC(nullptr, hdc);
        return dpi / 96.0f;
    }

    // =========================================================================
    // macOS
    // =========================================================================
#elif defined(__APPLE__)

    static float GetMacOSDPIScaleFactor()
    {
#ifdef __OBJC__
        // Preferred: use NSScreen when compiled as Objective-C++
        NSScreen* screen = [NSScreen mainScreen];
        if (screen)
            return static_cast<float>(screen.backingScaleFactor);
#endif
        // Pure C++ fallback: compare physical vs logical pixel widths
        CGDirectDisplayID displayID = CGMainDisplayID();

        CGDisplayModeRef mode = CGDisplayCopyDisplayMode(displayID);
        if (!mode) return 1.0f;

        size_t physicalWidth = CGDisplayModeGetPixelWidth(mode);   // Physical pixels
        size_t logicalWidth  = CGDisplayModeGetWidth(mode);        // Logical pixels
        CGDisplayModeRelease(mode);

        if (logicalWidth == 0) return 1.0f;
        return static_cast<float>(physicalWidth) / static_cast<float>(logicalWidth);
    }

    // =========================================================================
    // Linux
    // =========================================================================
#elif defined(__linux__)

    static void InitLinuxDPIAwareness()
    {
        // Wayland: tell the compositor we will handle fractional scaling.
        // GDK_SCALE=1 prevents GTK/GDK from adding its own integer scaling on
        // top of ours; GDK_DPI_SCALE=1 prevents further DPI scaling in GDK.
        // These must be set before the display connection is opened.
        setenv("GDK_SCALE",     "1", 0);   // 0 = don't overwrite if already set
        setenv("GDK_DPI_SCALE", "1", 0);
        setenv("QT_AUTO_SCREEN_SCALE_FACTOR", "0", 0);
        setenv("QT_SCALE_FACTOR", "1", 0);

        // For X11 / XWayland apps that respect Xft.dpi, you would normally
        // set it via xrdb. We can't do that at runtime portably here, so we
        // leave it to the system. GetLinuxDPIScaleFactor() reads it correctly.
    }

    static float GetLinuxDPIScaleFactor()
    {
        float dpiScale = 1.0f;

        Display* display = XOpenDisplay(nullptr);
        if (!display) return dpiScale;

        // ---- Method 1: Xft.dpi resource (most desktop environments set this)
        // GDK, Qt, and most apps honour this value, so it's the most reliable.
        char* xftDpi = XGetDefault(display, "Xft", "dpi");
        if (xftDpi)
        {
            float dpi = std::strtof(xftDpi, nullptr);
            if (dpi > 0.0f)
            {
                XCloseDisplay(display);
                return dpi / 96.0f;
            }
        }

        // ---- Method 2: XRandR — physical output dimensions (accurate) -------
        int screen = DefaultScreen(display);
        Window root = RootWindow(display, screen);

        XRRScreenResources* res = XRRGetScreenResourcesCurrent(display, root);
        if (res)
        {
            for (int i = 0; i < res->noutput; ++i)
            {
                XRROutputInfo* out = XRRGetOutputInfo(display, res, res->outputs[i]);
                if (!out) continue;

                bool connected = (out->connection == RR_Connected);
                bool hasCrtc   = (out->crtc != None);
                bool hasMM     = (out->mm_width > 0);

                if (connected && hasCrtc && hasMM)
                {
                    XRRCrtcInfo* crtc = XRRGetCrtcInfo(display, res, out->crtc);
                    if (crtc)
                    {
                        float dpi = crtc->width / (out->mm_width / 25.4f);
                        dpiScale  = dpi / 96.0f;
                        XRRFreeCrtcInfo(crtc);
                        XRRFreeOutputInfo(out);
                        break;    // Use the first connected output
                    }
                }
                XRRFreeOutputInfo(out);
            }
            XRRFreeScreenResources(res);
        }

        XCloseDisplay(display);
        return dpiScale;
    }

#endif // platform
};