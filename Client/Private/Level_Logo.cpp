#include "Level_Logo.h"
#include "Level_Loading.h"
#include "GameInstance.h"
#include "UISceneLoader.h"

CLevel_Logo::CLevel_Logo(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CLevel{ pDevice , pContext }
{
}

HRESULT CLevel_Logo::Initialize()
{
	//if (FAILED(Ready_Layer_BackGround(TEXT("Layer_BackGround"))))
	//	return E_FAIL;

	if (FAILED(Ready_Layer_UI(TEXT("Layer_UI"))))
		return E_FAIL;

	return S_OK;
}

void CLevel_Logo::Update(_float fTimeDelta)
{
	if (m_pGameInstance->Get_KeyDown(VK_RETURN) ||
		m_pGameInstance->Get_KeyDown(VK_SPACE) ||
		m_pGameInstance->Get_MouseBtnDown(MOUSEBTN::LBUTTON))
	{
		if (SUCCEEDED(m_pGameInstance->Change_Level(
			ETOI(LEVEL::LOADING),
			CLevel_Loading::Create(m_pDevice, m_pContext, LEVEL::GAMEPLAY))))
			return;
	}
}

HRESULT CLevel_Logo::Render()
{
#ifdef _DEBUG
	SetWindowText(m_pGameInstance->Get_hWnd(), TEXT("로고 레벨 입니다."));
#endif

	return S_OK;
}

HRESULT CLevel_Logo::Ready_Layer_BackGround(const _wstring& strLayerTag)
{
	if (FAILED(m_pGameInstance->Add_GameObject(ETOUI(LEVEL::LOGO), TEXT("Prototype_GameObject_BackGround"),
		ETOUI(LEVEL::LOGO), strLayerTag)))
		return E_FAIL;

	return S_OK;
}

HRESULT CLevel_Logo::Ready_Layer_UI(const _wstring& strLayerTag)
{
	return CUISceneLoader::Load_Into_Layer(
		TEXT("../../Resources/Scenes/UI/Logo.uiscene"),
		ETOUI(LEVEL::LOGO),
		strLayerTag);
}

CLevel_Logo* CLevel_Logo::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CLevel_Logo* pInstance = new CLevel_Logo(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Created : CLevel_Logo");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLevel_Logo::Free()
{
	__super::Free();
}