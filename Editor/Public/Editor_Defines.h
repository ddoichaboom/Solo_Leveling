#pragma once

#include <Windows.h>
#include <d3d11.h>
#include <dxgi.h>
#include <filesystem>

#include <rttr/type.h>

// Assimp
#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>

#pragma comment(lib, "dxgi.lib")

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"
#include "imgui_internal.h"


extern HWND         g_hWnd;
extern HINSTANCE    g_hInstance;

#include "Engine_Defines.h"
#include "Editor_Enum.h"
#include "Editor_Struct.h"
#include "Editor_Function.h"

namespace Editor 
{
	static unsigned int     g_iWinSizeX = { 1600 };  
	static unsigned int     g_iWinSizeY = { 900 };
}


using namespace Editor;

using namespace std;

