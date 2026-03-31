#ifndef Engine_Struct_h__
#define Engine_Struct_h__

namespace Engine
{
	typedef struct tagEngineDesc
	{
		HWND			hWnd;
		WINMODE			eWinMode;
		unsigned int	iViewportWidth, iViewportHeight;
		unsigned int	iNumLevels;
	}ENGINE_DESC;

	typedef struct tagVertexPositionTexcoord
	{
		XMFLOAT3		vPosition;		// 3D 공간 위치 (x, y, z)
		XMFLOAT2		vTexcoord;		// 텍스처 좌표 (u, v)
	}VTXTEX;
}

#endif // Engine_Struct_h__
