#include "Level_Loading.h"
#include "Loader.h"

#include "GameInstance.h"
#include "Level_Logo.h"
#include "Level_GamePlay.h"
#include "UISceneLoader.h"
#include "UI_Image.h"
#include "UI_Text.h"
#include "Layer.h"

CLevel_Loading::CLevel_Loading(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CLevel{ pDevice, pContext }
{

}

HRESULT CLevel_Loading::Initialize(LEVEL eNextLevelID)
{
	m_eNextLevelID = eNextLevelID;

	m_pLoader = CLoader::Create(m_pDevice, m_pContext, m_eNextLevelID);
	if (nullptr == m_pLoader)
		return E_FAIL;

	if (const _tchar* pScenePath = Get_Loading_UIScene_Path(eNextLevelID))
	{
		// 실패해도 Loading 자체는 계속 진행 (자원 미준비 단계 대응)
		CUISceneLoader::Load_Into_Layer(pScenePath, ETOUI(LEVEL::LOADING), TEXT("Layer_UI"));
	}

	if (const auto* pLayers = m_pGameInstance->Get_Layers(ETOUI(LEVEL::LOADING)))
	{
		auto it = pLayers->find(TEXT("Layer_UI"));
		if (it != pLayers->end())
		{
			for (CGameObject* pObj : it->second->Get_GameObjects())
			{
				CUIObject* pUI = dynamic_cast<CUIObject*>(pObj);
				if (!pUI) continue;

				const _wstring& strName = pUI->Get_ObjectName();
				if (strName == TEXT("LoadingGauge_Fill"))
				{
					m_pGaugeFill = dynamic_cast<CUI_Image*>(pUI);
					Safe_AddRef(m_pGaugeFill);
				}
				else if (strName == TEXT("LoadingPercent_Text"))
				{
					m_pPercentText = dynamic_cast<CUI_Text*>(pUI);
					Safe_AddRef(m_pPercentText);
				}
				else if (strName == TEXT("LoadingStatus_Text"))
				{
					m_pStatusText = dynamic_cast<CUI_Text*>(pUI);
					Safe_AddRef(m_pStatusText);
				}
			}
		}
	}

	return S_OK;
}

void CLevel_Loading::Update(_float fTimeDelta)
{
	const _float fProgress = m_pLoader->Get_Progress();

	if (m_pGaugeFill)
		m_pGaugeFill->Set_Progress(fProgress);

	if (m_pPercentText)
	{
		_tchar szPercent[16];
		swprintf_s(szPercent, TEXT("%d%%"), static_cast<_int>(fProgress * 100.f));
		m_pPercentText->Set_Text(szPercent);
	}

	if (m_pStatusText)
		m_pStatusText->Set_Text(m_pLoader->Get_LoadingText());

	if (m_pGameInstance->Get_KeyState(VK_SPACE) & 0x80 &&
		true == m_pLoader->isFinished())
	{

		CLevel* pNextLevel = { nullptr };

		switch (m_eNextLevelID)
		{
		case LEVEL::LOGO:
			pNextLevel = CLevel_Logo::Create(m_pDevice, m_pContext);
			break;
		case LEVEL::GAMEPLAY:
			pNextLevel = CLevel_GamePlay::Create(m_pDevice, m_pContext);
			break;
		}

		if (nullptr == pNextLevel)
		{
			MSG_BOX("Failed to Changed");
			return;
		}

		if (SUCCEEDED(m_pGameInstance->Change_Level(ETOI(m_eNextLevelID), pNextLevel)))
			return;
	}
}

HRESULT CLevel_Loading::Render()
{
#ifdef _DEBUG
	m_pLoader->Show();
#endif
	return S_OK;
}

const _tchar* CLevel_Loading::Get_Loading_UIScene_Path(LEVEL eTarget)
{
	switch (eTarget)
	{
	case LEVEL::LOGO:     
		return TEXT("../../Resources/Scenes/UI/Loading_Logo.uiscene");
	case LEVEL::GAMEPLAY: 
		return TEXT("../../Resources/Scenes/UI/Loading_Gameplay.uiscene");
	default:              
		return nullptr;
	}
}

CLevel_Loading* CLevel_Loading::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, LEVEL eNextLevelID)
{
	CLevel_Loading* pInstance = new CLevel_Loading(pDevice, pContext);

	if (FAILED(pInstance->Initialize(eNextLevelID)))
	{
		MSG_BOX("Failed to Created : CLevel_Loading");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CLevel_Loading::Free()
{
	__super::Free();

	Safe_Release(m_pStatusText);
	Safe_Release(m_pPercentText);
	Safe_Release(m_pGaugeFill);
	Safe_Release(m_pLoader);
}