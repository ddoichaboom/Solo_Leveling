#include "Player.h"
#include "GameInstance.h"
#include "Body_Player.h"
#include "Weapon.h"
#include "Transform.h"

CPlayer::CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CContainerObject{ pDevice, pContext }
{
}

CPlayer::CPlayer(const CPlayer& Prototype)
	: CContainerObject{ Prototype }
{
}

HRESULT CPlayer::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CPlayer::Initialize(void* pArg)
{
	GAMEOBJECT_DESC Desc{};
	Desc.fSpeedPerSec = 10.f;
	Desc.fRotationPerSec = XMConvertToRadians(180.f);

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	if (FAILED(Ready_PartObjects()))
		return E_FAIL;

	return S_OK;
}

void CPlayer::Priority_Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Priority_Update(fTimeDelta);
	}
}

void CPlayer::Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Update(fTimeDelta);
	}

	if (nullptr != m_pBody)
		Apply_RootMotion(m_pBody->Get_LastRootMotionDelta());
}

void CPlayer::Late_Update(_float fTimeDelta)
{
	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Late_Update(fTimeDelta);
	}
}

HRESULT CPlayer::Render()
{
	return S_OK;
}

void CPlayer::Apply_RootMotion(const _float3& vLocalDelta)
{
	// delta 0이면 Skip
	if (0.f == vLocalDelta.x && 0.f == vLocalDelta.y && 0.f == vLocalDelta.z)
		return;

	// CTransform의 축을 정규화해 회전 기저만 추출
	_vector vRight = XMVector3Normalize(m_pTransformCom->Get_State(STATE::RIGHT));
	_vector vUp = XMVector3Normalize(m_pTransformCom->Get_State(STATE::UP));
	_vector vLook = XMVector3Normalize(m_pTransformCom->Get_State(STATE::LOOK));

	// 로컬 델타 -> 월드 델타 (회전만 적용)
	_vector vWorldDelta = XMVectorScale(vRight, vLocalDelta.x);
	vWorldDelta = XMVectorAdd(vWorldDelta, XMVectorScale(vUp, vLocalDelta.y));
	vWorldDelta = XMVectorAdd(vWorldDelta, XMVectorScale(vLook, vLocalDelta.z));

	// 현재 Position에 누적
	_vector vPos = m_pTransformCom->Get_State(STATE::POSITION);
	vPos = XMVectorAdd(vPos, vWorldDelta);
	m_pTransformCom->Set_State(STATE::POSITION, vPos);
}

HRESULT CPlayer::Ready_PartObjects()
{
	// Body 
	CBody_Player::BODY_PLAYER_DESC BodyDesc{};
	BodyDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	BodyDesc.pParentState = &m_iState;

	if (FAILED(__super::Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_GameObject_Body_Player"),
		TEXT("Body"), &BodyDesc)))
		return E_FAIL;

	m_pBody = dynamic_cast<CBody_Player*>(m_PartObjects[TEXT("Body")]);
	Safe_AddRef(m_pBody);

	// Weapon 
	CWeapon::WEAPON_DESC WeaponDesc{};
	WeaponDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
/*	WeaponDesc.pSocketBoneMatrix =
		dynamic_cast<CBody_Player*>(m_PartObjects[TEXT("Body")])->Get_BoneMatrixPtr("Point_Weapon_B_Root");*/		// Socket 안되면 다른거로 교체
	WeaponDesc.pSocketBoneMatrix =
		dynamic_cast<CBody_Player*>(m_PartObjects[TEXT("Body")])->Get_BoneMatrixPtr("Prop_Weapon_Dualwield_01_R");
	
	if (FAILED(__super::Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_GameObject_Weapon"),
		TEXT("Weapon"), &WeaponDesc)))
		return E_FAIL;

	return S_OK;
}

CPlayer* CPlayer::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPlayer* pInstance = new CPlayer(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CPlayer");
		Safe_Release(pInstance);
	}

	return pInstance;
}


CGameObject* CPlayer::Clone(void* pArg)
{
	CPlayer* pInstance = new CPlayer(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CPlayer");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPlayer::Free()
{
	__super::Free();

	Safe_Release(m_pBody);
}
