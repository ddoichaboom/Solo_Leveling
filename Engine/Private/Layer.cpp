#include "Layer.h"
#include "GameObject.h"

CLayer::CLayer()
{
}

HRESULT CLayer::Add_GameObject(CGameObject* pGameObject)
{
	if (nullptr == pGameObject)
		return E_FAIL;

	m_GameObjects.push_back(pGameObject);

	return S_OK;
}

HRESULT CLayer::Detach_GameObject(CGameObject* pObject)
{
	if (nullptr == pObject)
		return E_FAIL;

	auto iter = find(m_GameObjects.begin(), m_GameObjects.end(), pObject);
	if (iter == m_GameObjects.end())
		return E_FAIL;

	m_GameObjects.erase(iter);

	return S_OK;
}

HRESULT CLayer::Attach_GameObject(CGameObject* pObject)
{
	if (nullptr == pObject)
		return E_FAIL;

	m_GameObjects.push_back(pObject);

	return S_OK;
}

HRESULT CLayer::Reorder_GameObject(CGameObject* pObject, _uint iNewIndex)
{
	if (nullptr == pObject)
		return E_FAIL;

	auto itSrc = find(m_GameObjects.begin(), m_GameObjects.end(), pObject);
	if (itSrc == m_GameObjects.end())
		return E_FAIL;

	// 인덱스가 size 이상이면 끝으로
	auto itDst = m_GameObjects.begin();
	_uint i = 0;
	while (itDst != m_GameObjects.end() && i < iNewIndex)
	{
		++itDst;
		++i;
	}

	// splice는 같은 list 내에서 노드만 옮김 
	m_GameObjects.splice(itDst, m_GameObjects, itSrc);

	return S_OK;
}

void CLayer::Priority_Update(_float fTimeDelta)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if (nullptr != pGameObject)
			pGameObject->Priority_Update(fTimeDelta);
	}
}

void CLayer::Update(_float fTimeDelta)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if (nullptr != pGameObject)
			pGameObject->Update(fTimeDelta);
	}
}

void CLayer::Late_Update(_float fTimeDelta)
{
	for (auto& pGameObject : m_GameObjects)
	{
		if (nullptr != pGameObject)
			pGameObject->Late_Update(fTimeDelta);
	}
}

CLayer* CLayer::Create()
{
	return new CLayer();
}

void CLayer::Free()
{
	__super::Free();

	for (auto& pGameObject : m_GameObjects)
		Safe_Release(pGameObject);

	m_GameObjects.clear();
}