#pragma once


#include <winsdkver.h>
#define _WIN32_WINNT 0x0A00
#include <sdkddkver.h>

// Use the C++ standard templated min/max
#define NOMINMAX

// DirectX apps don't need GDI
#define NODRAWTEXT
#define NOGDI
#define NOBITMAP

// Include <mcx.h> if you need this
#define NOMCX

// Include <winsvc.h> if you need this
#define NOSERVICE

// WinHelp is deprecated
#define NOHELP

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <wrl/client.h>
#include <wrl/event.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <DirectXMath.h>
#include <DirectXColors.h>

#include "d3dx12.h"

#include <algorithm>
#include <cstdint>
#include <exception>
#include <memory>
#include <stdexcept>

#ifdef _DEBUG
#include <dxgidebug.h>
#endif

#include <stdio.h>

// To use graphics and CPU markup events with the latest version of PIX, change this to include <pix3.h>
// then add the NuGet package WinPixEventRuntime to the project.
#include <pix3.h>

#pragma comment(lib,"d3d12.lib")

#include <crtdbg.h>
#include <filesystem>


namespace DX
{
    // Helper class for COM exceptions
    class com_exception : public std::exception
    {
    public:
        com_exception(HRESULT hr) noexcept : result(hr) {}

        const char* what() const override
        {
            static char s_str[64] = {};
            sprintf_s(s_str, "Failure with HRESULT of %08X", static_cast<unsigned int>(result));
            return s_str;
        }

    private:
        HRESULT result;
    };

    // Helper utility converts D3D API failures into exceptions.
    inline void ThrowIfFailed(HRESULT hr)
    {
        if (FAILED(hr))
        {
            throw com_exception(hr);
        }
    }

    inline void ShowAssertWindow(const char* message, const char* file, int line)
    {
        std::stringstream ss;
        ss << "Assertion Failed!\n\n"
            << "Message: " << message << "\n"
            << "File: " << file << "\n"
            << "Line: " << line;

        MessageBoxA(nullptr, ss.str().c_str(), "Assertion Failed", MB_OK | MB_ICONERROR);
    }

    //wchar variant
    inline void ShowAssertWindow(const wchar_t* message, const char* file, int line)
    {
        std::wstringstream wss;
        wss << "Assertion Failed!\n\n"
            << "Message: " << message << "\n"
            << "File: " << file << "\n"
            << "Line: " << line;

        MessageBoxW(nullptr, wss.str().c_str(), L"Assertion Failed", MB_OK | MB_ICONERROR);
    }

    inline void astro_assert(bool condition, const char* message) 
    {
        if (!(condition))
        {
            ShowAssertWindow(message, __FILE__, __LINE__);
            DebugBreak();// Break into the debugger  
        }
    }

    inline void astro_assert(bool condition, const wchar_t* message)
    {
        if (!(condition))
        {
            ShowAssertWindow(message, __FILE__, __LINE__);
            DebugBreak();// Break into the debugger  
        }
    }

    static std::string GetWorkingDirectory()
    {
        wchar_t buffer[MAX_PATH];
        GetModuleFileName(NULL, buffer, MAX_PATH);
        const auto pathStr = std::wstring(buffer);
        std::filesystem::path path = pathStr;
        auto rootFolder = path.parent_path().parent_path().parent_path();

        const auto workingDirWStr = rootFolder.string()+"\\AstroDX12";
        return workingDirWStr;
    }
}

static std::wstring s2ws(const std::string& s)
{
    int len;
    int slength = (int)s.length() + 1;
    len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
    wchar_t* buf = new wchar_t[len];
    MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
    std::wstring r(buf);
    delete[] buf;
    return r;
}
