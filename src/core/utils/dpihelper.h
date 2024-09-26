//
// Created by Vishnu Rajendran on 2024-09-25.
//
#pragma once
#ifndef METEOR_ENGINE_DPIHELPER_H
#define METEOR_ENGINE_DPIHELPER_H

#if defined(_WIN32) || defined(_WIN64)
#include <windows.h>
    #include <shellscalingapi.h>  // For GetDpiForMonitor on Windows 8.1+
    #pragma comment(lib, "Shcore.lib")
#elif defined(__APPLE__)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(__linux__)
#include <X11/Xlib.h>
    #include <X11/extensions/Xrandr.h>
#endif

class DPIHelper
{
public:
    // Returns the DPI scale factor (1.0 = 96 DPI on Windows, 72 DPI on macOS, Linux typically 96 DPI)
    static float GetDPIScaleFactor()
    {
#if defined(_WIN32) || defined(_WIN64)
        return GetWindowsDPIScaleFactor();
#elif defined(__APPLE__)
        return GetMacOSDPIScaleFactor();
#elif defined(__linux__)
        return GetLinuxDPIScaleFactor();
#endif
    }

private:
#if defined(_WIN32) || defined(_WIN64)
    static float GetWindowsDPIScaleFactor()
    {
        float dpiScale = 1.0f;

        // For Windows 8.1 and later, use GetDpiForMonitor
        HMODULE hShcore = LoadLibraryA("Shcore.dll");
        if (hShcore)
        {
            typedef HRESULT(WINAPI* GetDpiForMonitorFunc)(HMONITOR, MONITOR_DPI_TYPE, UINT*, UINT*);
            GetDpiForMonitorFunc pGetDpiForMonitor = (GetDpiForMonitorFunc)GetProcAddress(hShcore, "GetDpiForMonitor");
            if (pGetDpiForMonitor)
            {
                HMONITOR hMonitor = MonitorFromWindow(GetForegroundWindow(), MONITOR_DEFAULTTOPRIMARY);
                UINT dpiX, dpiY;
                if (SUCCEEDED(pGetDpiForMonitor(hMonitor, MDT_EFFECTIVE_DPI, &dpiX, &dpiY)))
                {
                    dpiScale = dpiX / 96.0f;
                }
            }
            FreeLibrary(hShcore);
        }
        else
        {
            // Fallback for older Windows versions
            HDC screen = GetDC(0);
            int dpi = GetDeviceCaps(screen, LOGPIXELSX);
            dpiScale = dpi / 96.0f;
            ReleaseDC(0, screen);
        }
        return dpiScale;
    }
#elif defined(__APPLE__)
    static float GetMacOSDPIScaleFactor()
    {
        float dpiScale = 1.0f;

        CGDirectDisplayID displayID = CGMainDisplayID();
        size_t pixelWidth = CGDisplayPixelsWide(displayID);
        size_t pixelHeight = CGDisplayPixelsHigh(displayID);
        CGSize displaySize = CGDisplayScreenSize(displayID);

        float dpi = (pixelWidth / (displaySize.width / 25.4));  // DPI = pixels per inch (25.4 mm = 1 inch)
        dpiScale = dpi / 72.0f;  // macOS typically uses 72 DPI as the base

        return dpiScale;
    }
#elif defined(__linux__)
    static float GetLinuxDPIScaleFactor()
    {
        float dpiScale = 1.0f;

        Display* display = XOpenDisplay(NULL);
        if (display)
        {
            // Get screen size in millimeters
            Screen* screen = DefaultScreenOfDisplay(display);
            int widthMM = screen->mwidth;
            int heightMM = screen->mheight;

            // Get screen resolution in pixels
            int widthPx = screen->width;
            int heightPx = screen->height;

            // Calculate DPI using width as reference
            float dpi = (widthPx / (widthMM / 25.4f));  // DPI = pixels per inch
            dpiScale = dpi / 96.0f;  // 96 DPI is the default on most Linux systems

            XCloseDisplay(display);
        }
        return dpiScale;
    }
#endif
};




#endif //METEOR_ENGINE_DPIHELPER_H
