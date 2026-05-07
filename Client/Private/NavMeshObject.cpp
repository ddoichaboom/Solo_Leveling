#include "NavMeshObject.h"
#include "NavMesh.h"

CNavMeshObject::CNavMeshObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
{
}

CNavMeshObject::CNavMeshObject(const CNavMeshObject& Prototype)
	: CGameObject { Prototype }
{
}

HRESULT CNavMeshObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CNavMeshObject::Initialize(void* pArg)
{
	NAVMESHOBJECT_DESC DefaultDesc{};
	NAVMESHOBJECT_DESC* pDesc = (nullptr != pArg)
		? static_cast<NAVMESHOBJECT_DESC*>(pArg)
		: &DefaultDesc;

	m_strName = TEXT("NavMeshObject");

	if (FAILED(__super::Initialize(pDesc)))
		return E_FAIL;

	if (FAILED(Ready_Components(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CNavMeshObject::Priority_Update(_float fTimeDelta)
{
}

void CNavMeshObject::Update(_float fTimeDelta)
{
}

void CNavMeshObject::Late_Update(_float fTimeDelta)
{
}

HRESULT CNavMeshObject::Render()
{
	return S_OK;
}

HRESULT CNavMeshObject::Ready_Components(const NAVMESHOBJECT_DESC* pDesc)
{
	CNavMesh::NAVMESH_DESC NavMeshDesc{};
	NavMeshDesc.pInitialSnapshot = (nullptr != pDesc) ? pDesc->pInitialSnapshot : nullptr;

	if (FAILED(__super::Add_Component(
		ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_Component_NavMesh"),
		TEXT("Com_NavMesh"),
		reinterpret_cast<CComponent**>(&m_pNavMeshCom),
		&NavMeshDesc)))
		return E_FAIL;

	return S_OK;
}

CNavMeshObject* CNavMeshObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CNavMeshObject* pInstance = new CNavMeshObject(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CNavMeshObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CGameObject* CNavMeshObject::Clone(void* pArg)
{
	CNavMeshObject* pInstance = new CNavMeshObject(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CNavMeshObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNavMeshObject::Free()
{
	__super::Free();

	Safe_Release(m_pNavMeshCom);
}
