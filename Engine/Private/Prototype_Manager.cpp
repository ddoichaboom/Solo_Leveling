#include "Prototype_Manager.h"
#include "GameObject.h"
#include "Component.h"

CPrototype_Manager::CPrototype_Manager()
{
}

HRESULT CPrototype_Manager::Initialize(_uint iNumLevels)
{
	if (nullptr != m_pPrototypes)
		return E_FAIL;
	
	m_iNumLevels = iNumLevels;

	// 몇개의 레벨이 있는지 Level의 개수를 받아와서 동적으로 할당
	m_pPrototypes = new PROTOTYPES[iNumLevels];

	return S_OK;
}

HRESULT CPrototype_Manager::Add_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag, CBase* pPrototype)
{
	if (nullptr == m_pPrototypes ||
		iLevelIndex >= m_iNumLevels ||
		nullptr != Find_Prototype(iLevelIndex, strPrototypeTag))
		return E_FAIL;

	m_pPrototypes[iLevelIndex].emplace(strPrototypeTag, pPrototype);

	return S_OK;
}

CBase* CPrototype_Manager::Clone_Prototype(PROTOTYPE eType, _uint iLevelIndex, const _wstring& strPrototypeTag, void* pArg)
{
	CBase* pPrototype = Find_Prototype(iLevelIndex, strPrototypeTag);
	if (nullptr == pPrototype)
		return nullptr;

	CBase* pInstance = { nullptr };

	if (PROTOTYPE::GAMEOBJECT == eType)
	{
		CGameObject* pGameObject = dynamic_cast<CGameObject*>(pPrototype)->Clone(pArg);
		if (nullptr != pGameObject)
			pGameObject->Set_PrototypeTag(strPrototypeTag);
		pInstance = pGameObject;
	}
	else
		pInstance = dynamic_cast<CComponent*>(pPrototype)->Clone(pArg);

	if (nullptr == pInstance)
		return nullptr;

	return pInstance;
}

void CPrototype_Manager::Clear(_uint iLevelIndex)
{
	for (auto& Pair : m_pPrototypes[iLevelIndex])
		Safe_Release(Pair.second);

	m_pPrototypes[iLevelIndex].clear();
}

HRESULT CPrototype_Manager::Enum_Prototypes(_uint iLevelIndex, vector<PROTOTYPE_INFO>& out) const
{
	if (iLevelIndex >= m_iNumLevels)
		return E_FAIL;

	out.clear();
	out.reserve(m_pPrototypes[iLevelIndex].size());

	for (auto& Pair : m_pPrototypes[iLevelIndex])
	{
		PROTOTYPE eType = (nullptr != dynamic_cast<class CGameObject*>(Pair.second))
			? PROTOTYPE::GAMEOBJECT
			: PROTOTYPE::COMPONENT;

		out.push_back({ Pair.first, eType, Pair.second });
	}

	return S_OK;
}

CBase* CPrototype_Manager::Find_Prototype(_uint iLevelIndex, const _wstring& strPrototypeTag)
{
	auto iter = m_pPrototypes[iLevelIndex].find(strPrototypeTag);
	if (iter == m_pPrototypes[iLevelIndex].end())
		return nullptr;

	return iter->second;
}

CPrototype_Manager* CPrototype_Manager::Create(_uint iNumLevels)
{
	CPrototype_Manager* pInstance = new CPrototype_Manager();

	if (FAILED(pInstance->Initialize(iNumLevels)))
	{
		MSG_BOX("Failed to Created : CPrototype_Manager");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPrototype_Manager::Free()
{
	__super::Free();

	for (size_t i = 0; i < m_iNumLevels; i++)
	{
		for (auto& Pair : m_pPrototypes[i])
			Safe_Release(Pair.second);
		m_pPrototypes[i].clear();
	}

	Safe_Delete_Array(m_pPrototypes);
}
