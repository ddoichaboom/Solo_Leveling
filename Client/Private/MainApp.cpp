#include "MainApp.h"

#include "GameInstance.h"
#include "Level_Loading.h"
#include "Level_Logo.h"
#include "UI_Image.h"
#include "UI_Video.h"
#include "UI_Text.h"
#include "UI_SpriteAnim.h"

CMainApp::CMainApp()
	: m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT CMainApp::Initialize(HWND hWnd, HINSTANCE hInstance, _uint iWinSizeX, _uint iWinSizeY)
{
	ENGINE_DESC		EngineDesc{};
	EngineDesc.hInstance = hInstance;
	EngineDesc.hWnd = hWnd;
	EngineDesc.eWinMode = WINMODE::WIN;
	EngineDesc.iViewportWidth = iWinSizeX;
	EngineDesc.iViewportHeight = iWinSizeY;
	EngineDesc.iNumLevels = ETOUI(LEVEL::END);

	if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext)))
	{
		MSG_BOX("Failed to Initialize : Engine");
		return E_FAIL;
	}

	if (FAILED(Ready_Prototype_For_Static()))
		return E_FAIL;

	if (FAILED(Ready_Prototype_For_Loading()))
		return E_FAIL;

	if (FAILED(Start_Level(LEVEL::LOGO)))
		return E_FAIL;

	return S_OK;
}

void CMainApp::Update(_float fTimeDelta)
{
	m_pGameInstance->Update_Engine(fTimeDelta);
}

HRESULT CMainApp::Render()
{
	if (FAILED(m_pGameInstance->Begin_Draw()))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Draw()))
		return E_FAIL;

	if (FAILED(m_pGameInstance->End_Draw()))
		return E_FAIL;

	return S_OK;
}

HRESULT CMainApp::Ready_Prototype_For_Static()
{
	const LEVEL eLevel = LEVEL::STATIC;

#pragma region ADD_FONT
	// 영문/숫자 강조 - HUD 수치, 데미지, 카운터
	if (FAILED(m_pGameInstance->Add_Font(TEXT("Font_NumericEN"),
		TEXT("../../Resources/Fonts/agencyb_48.spritefont"))))
		return E_FAIL;

	// 굵은 한글 제목/버튼/메뉴 헤더
	if (FAILED(m_pGameInstance->Add_Font(TEXT("Font_HeaderKR"),
		TEXT("../../Resources/Fonts/nanumsquareb_48.spritefont"))))
		return E_FAIL;

	// 굵은 한글 일반 UI - 리스트, 라벨
	if (FAILED(m_pGameInstance->Add_Font(TEXT("Font_LabelKR"),
		TEXT("../../Resources/Fonts/nanumgothic_32.spritefont"))))
		return E_FAIL;

	// 기본 한글 본문 - 시스템 메시지, 범용 UI (Default)
	if (FAILED(m_pGameInstance->Add_Font(TEXT("Font_Default"),
		TEXT("../../Resources/Fonts/notosanskr_32.spritefont"))))
		return E_FAIL;

	// 한글 강조/ 포인트 텍스트 (둥근 느낌)
	if (FAILED(m_pGameInstance->Add_Font(TEXT("Font_AccentKR"),
		TEXT("../../Resources/Fonts/binggrae2_32.spritefont"))))
		return E_FAIL;
#pragma endregion

	// (1) VIBuffer_Rect 프로토타입 등록 (STATIC 레벨)
	if (FAILED(m_pGameInstance->Add_Prototype(
				ETOUI(eLevel),
				TEXT("Prototype_Component_VIBuffer_Rect"),
				CVIBuffer_Rect::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// (2) Shader 프로토타입 등록 (STATIC 레벨)
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel), TEXT("Prototype_Component_Shader_VtxTex"),
					CShader::Create(m_pDevice, m_pContext, TEXT("../../Resources/ShaderFiles/Shader_VtxTex.hlsl"),
					VTXTEX::Elements, VTXTEX::iNumElements))))
		return E_FAIL;

	// Prototype_GameObject_UI_Image
	if (FAILED(m_pGameInstance->Add_Prototype(
		ETOUI(eLevel),
		TEXT("Prototype_GameObject_UI_Image"),
		CUI_Image::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_UI_SpriteAnim
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_GameObject_UI_SpriteAnim"),
		CUI_SpriteAnim::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_UI_Video 
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_GameObject_UI_Video"),
		CUI_Video::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Prototype_GameObject_UI_Text 
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(eLevel),
		TEXT("Prototype_GameObject_UI_Text"),
		CUI_Text::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	return S_OK;
}

HRESULT CMainApp::Ready_Prototype_For_Loading()
{
	const LEVEL eLevel = LEVEL::LOADING;

	struct LoadingTextureEntry
	{
		const _tchar* pProtoTag;
		const _tchar* pFilePath;
	};

	static const LoadingTextureEntry aEntries[] =
	{
		// === 배경 ===
		{ TEXT("Prototype_Component_Texture_BG_BlackScreen"),
		  TEXT("../../Resources/Textures/Loading/BG_BlackScreen.png") },
		{ TEXT("Prototype_Component_Texture_BG_Loading"),
		  TEXT("../../Resources/Textures/Loading/BG_Loading.png") },

		  // === 애니메이션 ===
		{ TEXT("Prototype_Component_Texture_Img_Tank_Atlas"),
			TEXT("../../Resources/Textures/Loading/Img_Tank_Atlas.png") },

		// === 게이지 본체 ===
		{ TEXT("Prototype_Component_Texture_LoadingGauge"),
		  TEXT("../../Resources/Textures/Loading/LoadingGauge.png") },

		  // === 게이지 배경 (원본 + 3-slice 분할) ===
		 { TEXT("Prototype_Component_Texture_LoadingGauge_Bg"),
		TEXT("../../Resources/Textures/Loading/LoadingGauge_Bg.png") },
		 { TEXT("Prototype_Component_Texture_LoadingGauge_Bg_L"),
		TEXT("../../Resources/Textures/Loading/LoadingGauge_Bg_L.png") },
		 { TEXT("Prototype_Component_Texture_LoadingGauge_Bg_M"),
		TEXT("../../Resources/Textures/Loading/LoadingGauge_Bg_M.png") },
		 { TEXT("Prototype_Component_Texture_LoadingGauge_Bg_R"),
		TEXT("../../Resources/Textures/Loading/LoadingGauge_Bg_R.png") },

		// === 게이지 양 끝 장식 ===
		{ TEXT("Prototype_Component_Texture_LoadingGauge_Deco_L"),
		  TEXT("../../Resources/Textures/Loading/LoadingGauge_Deco_L.png") },
		{ TEXT("Prototype_Component_Texture_LoadingGauge_Deco_R"),
		  TEXT("../../Resources/Textures/Loading/LoadingGauge_Deco_R.png") },
	};

	for (const LoadingTextureEntry& Entry : aEntries)
	{
		if (FAILED(m_pGameInstance->Add_Prototype(
			ETOUI(eLevel),
			Entry.pProtoTag,
			CTexture::Create(m_pDevice, m_pContext, Entry.pFilePath, 1))))
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CMainApp::Start_Level(LEVEL eStartLevelID)
{
	CLevel* pPreLevel = CLevel_Loading::Create(m_pDevice, m_pContext, eStartLevelID);
	if (nullptr == pPreLevel)
		return E_FAIL;

	if (FAILED(m_pGameInstance->Change_Level(ETOI(LEVEL::LOADING), pPreLevel)))
		return E_FAIL;

	return S_OK;
}

CMainApp* CMainApp::Create(HWND hWnd, HINSTANCE hInstance, _uint iWinSizeX, _uint iWinSizeY)
{
	CMainApp* pInstance = new CMainApp();

	if (FAILED(pInstance->Initialize(hWnd, hInstance, iWinSizeX, iWinSizeY)))
	{
		MSG_BOX("Failed to Created : CMainApp");
		Safe_Release(pInstance);
	}

	return pInstance;

}

void CMainApp::Free()
{
	__super::Free();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	m_pGameInstance->Release_Engine();
	Safe_Release(m_pGameInstance);
}
