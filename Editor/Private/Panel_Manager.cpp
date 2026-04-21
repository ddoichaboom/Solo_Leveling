#include "Panel_Manager.h"
#include "Panel.h"
#include "GameObject.h"
#include "Body_Player.h"

IMPLEMENT_SINGLETON(CPanel_Manager)

CPanel_Manager::CPanel_Manager()
{

}

HRESULT CPanel_Manager::Add_Panel(const _wstring& strPanelTag, CPanel* pPanel)
{
	if (nullptr == pPanel)
		return E_FAIL;

	m_Panels.emplace(strPanelTag, pPanel);

	return S_OK;
}

CPanel* CPanel_Manager::Get_Panel(const _wstring& strPanelTag)
{
	auto iter = m_Panels.find(strPanelTag);

	if (iter == m_Panels.end())
		return nullptr;

	return iter->second;

}

void CPanel_Manager::Update_Panels(_float fTimeDelta)
{
	for (auto& Pair : m_Panels)
	{
		if (Pair.second->Is_Open())
			Pair.second->Update(fTimeDelta);
	}
}

void CPanel_Manager::Render_Panels()
{
	for (auto& Pair : m_Panels)
	{
		if (Pair.second->Is_Open())
			Pair.second->Render();
	}
}

void CPanel_Manager::End_Preview_If_Needed(CGameObject* pObject)
{
	CBody_Player* pBodyPlayer = dynamic_cast<CBody_Player*>(pObject);
	if (nullptr != pBodyPlayer && true == pBodyPlayer->Is_Previewing())
		pBodyPlayer->End_Preview();
}

void CPanel_Manager::Clear_Selection()
{
	End_Preview_If_Needed(m_pSelectedObject);

	Safe_Release(m_pSelectedObject);
	m_pSelectedObject = nullptr;
}

void CPanel_Manager::Set_SelectedObject(CGameObject* pObject)
{
	if (m_pSelectedObject != pObject)
		End_Preview_If_Needed(m_pSelectedObject);

	Safe_Release(m_pSelectedObject);
	m_pSelectedObject = pObject;
	Safe_AddRef(m_pSelectedObject);
}

void CPanel_Manager::Release_Panels()
{
	Safe_Release(m_pSelectedObject);

	for (auto& Pair : m_Panels)
		Safe_Release(Pair.second);

	m_Panels.clear();

	DestroyInstance();
}

void CPanel_Manager::Free()
{
	__super::Free();
}
