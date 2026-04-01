#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>

#pragma comment(lib, "dxgi.lib")

extern HWND         g_hWnd;
extern HINSTANCE    g_hInstance;

inline bool Get_MonitorResolution(_Out_ unsigned int* pWidth, _Out_ unsigned int* pHeight)
{
    IDXGIFactory* pFactory = nullptr;
    if (FAILED(CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&pFactory)))
        return false;

    IDXGIAdapter* pAdapter = nullptr;
    if (FAILED(pFactory->EnumAdapters(0, &pAdapter)))
    {
        pFactory->Release();
        return false;
    }

    IDXGIOutput* pOutput = nullptr;
    if (FAILED(pAdapter->EnumOutputs(0, &pOutput)))
    {
        pAdapter->Release();
        pFactory->Release();
        return false;
    }

    DXGI_OUTPUT_DESC outputDesc{};
    pOutput->GetDesc(&outputDesc);

    *pWidth = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
    *pHeight = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;

    pOutput->Release();
    pAdapter->Release();
    pFactory->Release();

    return true;
}

namespace Editor 
{
	static unsigned int     g_iWinSizeX = { 1600 };  
	static unsigned int     g_iWinSizeY = { 900 };

}


using namespace Editor;