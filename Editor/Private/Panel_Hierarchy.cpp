#include "Panel_Hierarchy.h"
#include "Panel_Manager.h"
#include "GameInstance.h"
#include "Layer.h"
#include "GameObject.h"

CPanel_Hierarchy::CPanel_Hierarchy(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Hierarchy::Initialize()
{
	strcpy_s(m_szName, "Hierarchy");

	return S_OK;
}

void CPanel_Hierarchy::Update(_float fTimeDelta)
{
	_int iCurLevel = m_pGameInstance->Get_CurrentLevelIndex();
	if (iCurLevel != m_iPrevLevelIndex)
	{
		m_pPanel_Manager->Clear_Selection();
		m_iPrevLevelIndex = iCurLevel;
	}
}

void CPanel_Hierarchy::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	_int iLevelIndex = m_pGameInstance->Get_CurrentLevelIndex();
	if (iLevelIndex < 0)
	{
		ImGui::TextDisabled("No Level Loaded");
		ImGui::End();
		return;
	}

	const auto* pLayers = m_pGameInstance->Get_Layers(static_cast<_uint>(iLevelIndex));
	if (nullptr == pLayers)
	{
		ImGui::TextDisabled("No Layers");
		ImGui::End();
		return;
	}

	CGameObject* pSelected = m_pPanel_Manager->Get_SelectedObject();

	for (const auto& Pair : *pLayers)
	{
		const string	strLayerName = WTOA(Pair.first);
		_uint			iObjectCount = static_cast<_uint>(Pair.second->Get_GameObjects().size());

		_char			szLabel[MAX_PATH] = {};
		sprintf_s(szLabel, "%s (%u)", strLayerName.c_str(), iObjectCount);

		if (ImGui::TreeNode(szLabel))
		{
			for (auto& pObject : Pair.second->Get_GameObjects())
			{
				string strDisplay;
				if (!pObject->Get_Name().empty())
					strDisplay = WTOA(pObject->Get_Name());
				else if (!pObject->Get_Tag().empty())
					strDisplay = WTOA(pObject->Get_Tag());
				else
					strDisplay = "(Unnamed)";

				_bool bIsSelected = (pObject == pSelected);

				if (ImGui::Selectable(strDisplay.c_str(), bIsSelected))
				{
					m_pPanel_Manager->Set_SelectedObject(pObject);
				}
			}
			ImGui::TreePop();
		}
	}

	ImGui::End();
}

CPanel_Hierarchy* CPanel_Hierarchy::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_Hierarchy* pInstance = new CPanel_Hierarchy(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_Hierarchy");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_Hierarchy::Free()
{
	__super::Free();
}
