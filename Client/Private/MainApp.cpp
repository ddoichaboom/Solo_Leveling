#include "MainApp.h"

#include "GameInstance.h"
#include "Level_Loading.h"
#include "AnimController.h"
#include "SpringArm.h"

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

	// 엔진 초기화 후 STATIC 레벨 프로토타입 등록
	if (FAILED(Ready_Prototype_For_Static()))
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
	// (1) VIBuffer_Rect 프로토타입 등록 (STATIC 레벨)
	if (FAILED(m_pGameInstance->Add_Prototype(
				ETOUI(LEVEL::STATIC),
				TEXT("Prototype_Component_VIBuffer_Rect"),
				CVIBuffer_Rect::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// (2) Shader 프로토타입 등록 (STATIC 레벨)
	if (FAILED(m_pGameInstance->Add_Prototype(ETOUI(LEVEL::STATIC), TEXT("Prototype_Component_Shader_VtxTex"),
					CShader::Create(m_pDevice, m_pContext, TEXT("../../Resources/ShaderFiles/Shader_VtxTex.hlsl"),
					VTXTEX::Elements, VTXTEX::iNumElements))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(
				ETOUI(LEVEL::STATIC),
				TEXT("Prototype_Component_AnimController"),
				CAnimController::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(
				ETOUI(LEVEL::STATIC),
				TEXT("Prototype_Component_SpringArm"),
				CSpringArm::Create(m_pDevice, m_pContext))))
		return E_FAIL;

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
