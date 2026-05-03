#include "Object_Manager.h"
#include "GameInstance.h"
#include "GameObject.h"
#include "Layer.h"

CObject_Manager::CObject_Manager()
	: m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
}

const map<const _wstring, CLayer*>* CObject_Manager::Get_Layers(_uint iLevelIndex) const
{
	if (iLevelIndex >= m_iNumLevels)
		return nullptr;

	return &m_pLayers[iLevelIndex];
}

HRESULT CObject_Manager::Initialize(_uint iNumLevels)
{
	if (nullptr != m_pLayers)
		return E_FAIL;

	m_iNumLevels = iNumLevels;

	m_pLayers = new LAYERS[iNumLevels];

	return S_OK;
}

HRESULT CObject_Manager::Add_GameObject(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, _uint iLayerLevelIndex, const _wstring& strLayerTag, void* pArg)
{
	if (iLayerLevelIndex >= m_iNumLevels ||
		nullptr == m_pLayers)
		return E_FAIL;

	// 레이어에 추가할 사본을 준비한다.
	CGameObject* pGameObject = dynamic_cast<CGameObject*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::GAMEOBJECT, iPrototypeLevelIndex, strPrototypeTag, pArg));
	if (nullptr == pGameObject)
		return E_FAIL;

	// 레이어를 찾는다. 없으면 새로 만든다.
	CLayer* pLayer = Find_Layer(iLayerLevelIndex, strLayerTag);
	if (nullptr == pLayer)
	{
		pLayer = CLayer::Create();
		pLayer->Add_GameObject(pGameObject);
		m_pLayers[iLayerLevelIndex].emplace(strLayerTag, pLayer);
	}
	else
		pLayer->Add_GameObject(pGameObject);

	return S_OK;
}

HRESULT CObject_Manager::Move_GameObject(_uint iSrcLevel, const _wstring& strSrcLayer, _uint iDstLevel, const _wstring& strDstLayer, CGameObject* pObject)
{
	if (iSrcLevel >= m_iNumLevels || iDstLevel >= m_iNumLevels || nullptr == pObject)
		return E_FAIL;

	CLayer* pSrcLayer = Find_Layer(iSrcLevel, strSrcLayer);
	if (nullptr == pSrcLayer)
		return E_FAIL;

	// 같은 레이어 내 이동이면 no-op
	if (iSrcLevel == iDstLevel && strSrcLayer == strDstLayer)
		return S_OK;

	if (FAILED(pSrcLayer->Detach_GameObject(pObject)))
		return E_FAIL;

	CLayer* pDstLayer = Find_Layer(iDstLevel, strDstLayer);
	if (nullptr == pDstLayer)
	{
		pDstLayer = CLayer::Create();
		m_pLayers[iDstLevel].emplace(strDstLayer, pDstLayer);
	}
	
	return pDstLayer->Attach_GameObject(pObject);
}

HRESULT CObject_Manager::Reorder_GameObject(_uint iLevel, const _wstring& strLayer, CGameObject* pObject, _uint iNewIndex)
{
	if (iLevel >= m_iNumLevels || nullptr == pObject)
		return E_FAIL;

	CLayer* pLayer = Find_Layer(iLevel, strLayer);
	if (nullptr == pLayer)
		return E_FAIL;

	return pLayer->Reorder_GameObject(pObject, iNewIndex);
}

HRESULT CObject_Manager::Remove_GameObject(_uint iLevel, const _wstring& strLayer, CGameObject* pObject)
{
	if (iLevel >= m_iNumLevels || nullptr == pObject)
		return E_FAIL;

	CLayer* pLayer = Find_Layer(iLevel, strLayer);
	if (pLayer == nullptr)
		return E_FAIL;

	if (FAILED(pLayer->Detach_GameObject(pObject)))
		return E_FAIL;

	Safe_Release(pObject);

	return S_OK;
}

void CObject_Manager::Priority_Update(_float fTimeDelta)
{
	for (size_t i = 0; i < m_iNumLevels; i++)
	{
		for ( auto& Pair : m_pLayers[i])
			Pair.second->Priority_Update(fTimeDelta);
	}
}

void CObject_Manager::Update(_float fTimeDelta)
{
	for (size_t i = 0; i < m_iNumLevels; i++)
	{
		for (auto& Pair : m_pLayers[i])
			Pair.second->Update(fTimeDelta);
	}
}

void CObject_Manager::Late_Update(_float fTimeDelta)
{
	for (size_t i = 0; i < m_iNumLevels; i++)
	{
		for (auto& Pair : m_pLayers[i])
			Pair.second->Late_Update(fTimeDelta);
	}
}

void CObject_Manager::Clear(_uint iLevelIndex)
{
	for (auto& Pair : m_pLayers[iLevelIndex])
		Safe_Release(Pair.second);

	m_pLayers[iLevelIndex].clear();
}

CLayer* CObject_Manager::Find_Layer(_uint iLayerLevelIndex, const _wstring& strLayerTag)
{
	auto iter = m_pLayers[iLayerLevelIndex].find(strLayerTag);
	if (iter == m_pLayers[iLayerLevelIndex].end())
		return nullptr;

	return iter->second;
}

CObject_Manager* CObject_Manager::Create(_uint iNumLevels)
{
	CObject_Manager* pInstance = new CObject_Manager();

	if (FAILED(pInstance->Initialize(iNumLevels)))
	{
		MSG_BOX("Failed to Created : CObject_Manager");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CObject_Manager::Free()
{
	__super::Free();

	for (size_t i = 0; i < m_iNumLevels; i++)
	{
		for (auto& Pair : m_pLayers[i])
			Safe_Release(Pair.second);

		m_pLayers[i].clear();
	}

	Safe_Release(m_pGameInstance);
	Safe_Delete_Array(m_pLayers);
}