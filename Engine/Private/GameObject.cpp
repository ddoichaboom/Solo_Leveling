#include "GameObject.h"
#include "GameInstance.h"

#include "Transform_2D.h"
#include "Transform_3D.h"

CGameObject::CGameObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{pDevice}
	, m_pContext{pContext}
	, m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

CGameObject::CGameObject(const CGameObject& Prototype)
	: m_pDevice{Prototype.m_pDevice}
	, m_pContext{Prototype.m_pContext}
	, m_pGameInstance{ CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CGameObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CGameObject::Initialize(void* pArg)
{
	// 기본 설정 : 3D
	TRANSFORMTYPE eType = TRANSFORMTYPE::TRANSFORM_3D;

	if (nullptr != pArg)
	{
		auto pDesc = static_cast<GAMEOBJECT_DESC*>(pArg);
		m_iFlag = pDesc->iFlag;
		eType = pDesc->eTransformType;
	}

	// 객체당 부여되어야 할 Transform-Component를 생성한다.
	switch (eType)
	{
	case TRANSFORMTYPE::TRANSFORM_3D:
		m_pTransformCom = CTransform_3D::Create(m_pDevice, m_pContext);
		break;
	case TRANSFORMTYPE::TRANSFORM_2D:
		m_pTransformCom = CTransform_2D::Create(m_pDevice, m_pContext);
		break;
	}
	if (nullptr == m_pTransformCom)
		return E_FAIL;

	// 객체에게 부여된 초기 월드 상태를 Transform에게 동기화시킨다.
	if (FAILED(m_pTransformCom->Initialize(pArg)))								
		return E_FAIL;

	m_Components.emplace(g_strTransformTag, m_pTransformCom);

	Safe_AddRef(m_pTransformCom);

	return S_OK;
}

void CGameObject::Priority_Update(_float fTimeDelta)
{
}

void CGameObject::Update(_float fTimeDelta)
{

}

void CGameObject::Late_Update(_float fTimeDelta)
{
}

HRESULT CGameObject::Render()
{
	return S_OK;
}

HRESULT CGameObject::Add_Component(_uint iPrototypeLevelIndex, const _wstring& strPrototypeTag, const _wstring& strComponentTag, CComponent** ppOut, void* pArg)
{
	if (nullptr != Find_Component(strComponentTag))
		return E_FAIL;

	CComponent* pComponent = dynamic_cast<CComponent*>(m_pGameInstance->Clone_Prototype(PROTOTYPE::COMPONENT, iPrototypeLevelIndex, strPrototypeTag, pArg));
	if (nullptr == pComponent)
		return E_FAIL;

	m_Components.emplace(strComponentTag, pComponent);

	*ppOut = pComponent;

	Safe_AddRef(pComponent);

	return S_OK;
}

CComponent* CGameObject::Find_Component(const _wstring& strComponentTag)
{
	auto iter = m_Components.find(strComponentTag);

	if (iter == m_Components.end())
		return nullptr;

	return iter->second;
}

void CGameObject::Free()
{
	__super::Free();

	for (auto& Pair : m_Components)
		Safe_Release(Pair.second);
	m_Components.clear();

	Safe_Release(m_pVIBufferCom);
	Safe_Release(m_pTransformCom);

	Safe_Release(m_pGameInstance);
	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
}