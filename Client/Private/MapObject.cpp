#include "MapObject.h"
#include "GameInstance.h"
#include "MapStaticObject.h"

CMapObject::CMapObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CContainerObject{ pDevice, pContext }
{
}

CMapObject::CMapObject(const CMapObject& Prototype)
	: CContainerObject { Prototype }
{
}

HRESULT CMapObject::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CMapObject::Initialize(void* pArg)
{
	GAMEOBJECT_DESC		Desc{};
	Desc.fSpeedPerSec = 0.f;
	Desc.fRotationPerSec = 0.f;

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	if (FAILED(Ready_PartObjects()))
		return E_FAIL;

	return S_OK;
}

void CMapObject::Priority_Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Priority_Update(fTimeDelta);
	}
}

void CMapObject::Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Update(fTimeDelta);
	}
}

void CMapObject::Late_Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Late_Update(fTimeDelta);
	}
}

HRESULT CMapObject::Render()
{
	return S_OK;
}

HRESULT CMapObject::Ready_PartObjects()
{
	// Static PartObject
	CMapStaticObject::MAPSTATICOBJECT_DESC StaticDesc{};
	StaticDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	StaticDesc.pShaderProtoTag = TEXT("Prototype_Component_Shader_VtxMesh");
	StaticDesc.pModelProtoTag = TEXT("Prototype_Component_Model_Map_ThroneRoom");

	if (FAILED(__super::Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
				TEXT("Prototype_GameObject_MapStaticObject"),
				TEXT("Statics"), &StaticDesc)))
		return E_FAIL;

	return S_OK;
}

CMapObject* CMapObject::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CMapObject* pInstance = new CMapObject(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CMapObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CMapObject::Clone(void* pArg)
{
	CMapObject* pInstance = new CMapObject(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CMapObject");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CMapObject::Free()
{
	__super::Free();
}

