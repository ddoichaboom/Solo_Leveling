#ifndef Engine_Struct_h__
#define Engine_Struct_h__

namespace Engine
{
	typedef struct tagEngineDesc
	{
		HINSTANCE		hInstance;
		HWND			hWnd;
		WINMODE			eWinMode;
		unsigned int	iViewportWidth, iViewportHeight;
		unsigned int	iNumLevels;
	}ENGINE_DESC;

	typedef struct tagLightDesc
	{
		LIGHT		eType;									// 광원 타입 (예: 방향성 광원, 점 광원 등)
		XMFLOAT4	vDiffuse, vAmbient, vSpecular;			// 빛의 3가지 색상 성분
		XMFLOAT4	vDirection;								// 방향광용 : 빛이 향하는 방향
		XMFLOAT4	vPosition;								// 점광원용 : 광원 위치
		float		fRange;									// 점광원용 : 광원의 유효 범위
	}LIGHT_DESC;

	typedef struct tagVertexPositionTexcoord
	{
		XMFLOAT3		vPosition;		
		XMFLOAT2		vTexcoord;		

		static const unsigned int iNumElements = { 2 };

		static constexpr D3D11_INPUT_ELEMENT_DESC Elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

	}VTXTEX;

	typedef struct tagVertexPositionNormalTexcoord
	{
		XMFLOAT3		vPosition;
		XMFLOAT3		vNormal;
		XMFLOAT2		vTexcoord;

		static const unsigned int		iNumElements = { 3 };

		static constexpr D3D11_INPUT_ELEMENT_DESC   Elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};

	}VTXNORTEX;

	typedef struct tagVertexMesh
	{
		XMFLOAT3		vPosition;
		XMFLOAT3		vNormal;
		XMFLOAT2		vTexcoord;
		XMFLOAT3		vTangent;
		XMFLOAT3		vBinormal;

		static const unsigned int		iNumElements = { 5 };

		static constexpr D3D11_INPUT_ELEMENT_DESC   Elements[] = {
			{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
			{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0 },
			{"BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D11_INPUT_PER_VERTEX_DATA, 0}
		};
	}VTXMESH;

	typedef struct tagPickingData
	{
		XMFLOAT3*			pVerticesPos	= { nullptr };
		unsigned int*		pIndices		= { nullptr };
		unsigned int		iNumVertices	= {};
		unsigned int 		iNumIndices		= {};
	}PICK_DATA;
}

#endif // Engine_Struct_h__
