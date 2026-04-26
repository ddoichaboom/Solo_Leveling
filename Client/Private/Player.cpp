#include "Player.h"
#include "GameInstance.h"
#include "Body_Player.h"
#include "Weapon.h"
#include "Transform_3D.h"
#include "IntentResolver.h"
#include "Player_StateMachine.h"

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

	if (nullptr != pArg)
		Desc = *static_cast<PLAYER_DESC*>(pArg);

	if (0.f == Desc.fSpeedPerSec)
		Desc.fSpeedPerSec = 10.f;

	if (0.f == Desc.fRotationPerSec)
		Desc.fRotationPerSec = XMConvertToRadians(180.f);

	if (FAILED(__super::Initialize(&Desc)))
		return E_FAIL;

	if (FAILED(Ready_PartObjects()))
		return E_FAIL;

	m_pIntentResolver = CIntentResolver::Create();
	if (nullptr == m_pIntentResolver)
		return E_FAIL;

	if (FAILED(Ready_StateMachine()))
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
	Tick_DashRegen(fTimeDelta);

	PLAYER_RAW_INPUT_FRAME Raw{};
	PLAYER_INTENT_FRAME Intent{};

	Gather_RawInput(&Raw);

	const _float fCameraYaw = Query_CameraYaw();

	if (nullptr != m_pIntentResolver)
		m_pIntentResolver->Resolve(Raw, fCameraYaw, &Intent);

	if (nullptr != m_pStateMachine)
	{
		m_pStateMachine->Update_LocoMotion(Intent);
		m_pStateMachine->Update(fTimeDelta);
	}

	for (auto& Pair : m_PartObjects)
	{
		if (nullptr != Pair.second)
			Pair.second->Update(fTimeDelta);
	}

	if (nullptr != m_pBody)
		Apply_RootMotion(m_pBody->Get_LastRootMotionDelta());

	Apply_MoveIntent(Intent, fTimeDelta);
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

void CPlayer::Handle_ActionTransition(CHARACTER_ACTION eFrom, CHARACTER_ACTION eTo, _bool bInitial)
{
	if (nullptr == m_pBody)
		return;

	m_pBody->Play_Action(eTo);
}

void CPlayer::Face_DirectionImmediately(const _float3& vDirWorld)
{
	if (nullptr == m_pTransformCom)
		return;

	CTransform_3D* pTransform = static_cast<CTransform_3D*>(m_pTransformCom);
	pTransform->Rotate_Toward_XZ(XMLoadFloat3(&vDirWorld), XM_PI);
}

_bool CPlayer::Consume_DashCharge()
{
	if (m_iDashChargeCurent <= 0)
		return false;

	--m_iDashChargeCurent;

	return true;
}

void CPlayer::Tick_DashRegen(_float fTimeDelta)
{
	// 항상 일정 주기 - 풀 차지 여도 타이머는 리셋
	m_fDashRegenTimer += fTimeDelta;

	while (m_fDashRegenTimer >= m_fDashRegenInterval)
	{
		m_fDashRegenTimer -= m_fDashRegenInterval;

		if (m_iDashChargeCurent < m_iDashChargeMax)
			++m_iDashChargeCurent;
	}
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

HRESULT CPlayer::Ready_StateMachine()
{
	const CHARACTER_ANIM_TABLE_DESC* pAnimTable = Find_CharacterAnimTable(CHARACTER_ANIM_SET::SUNGJINWOO_ERANK);
	if (nullptr == pAnimTable)
		return E_FAIL;

	m_pStateMachine = CPlayer_StateMachine::Create(pAnimTable);
	if (nullptr == m_pStateMachine)
		return E_FAIL;

	m_pStateMachine->Bind_Owner(this);

	if (nullptr != m_pBody)
		m_pBody->Set_Listener(m_pStateMachine);

	if (false == m_pStateMachine->Enter_InitialState(CHARACTER_ACTION::IDLE))
		return E_FAIL;

	return S_OK;
}

void CPlayer::Gather_RawInput(PLAYER_RAW_INPUT_FRAME* pOutRaw)
{
	if (nullptr == pOutRaw)
		return;

	pOutRaw->bRButtonHeld = (m_pGameInstance->Get_MouseBtnState(MOUSEBTN::RBUTTON) & 0x80) != 0;
	if (false == pOutRaw->bRButtonHeld)
	{
		pOutRaw->bMoveForwardHeld = (m_pGameInstance->Get_KeyState('W') & 0x80) != 0;
		pOutRaw->bMoveBackwardHeld = (m_pGameInstance->Get_KeyState('S') & 0x80) != 0;
		pOutRaw->bMoveLeftHeld = (m_pGameInstance->Get_KeyState('A') & 0x80) != 0;
		pOutRaw->bMoveRightHeld = (m_pGameInstance->Get_KeyState('D') & 0x80) != 0;
	}
	pOutRaw->bDashPressed = m_pGameInstance->Get_KeyDown(VK_SPACE);

	pOutRaw->lMouseDeltaX = m_pGameInstance->Get_MouseDelta(MOUSEAXIS::X);
	pOutRaw->lMouseDeltaY = m_pGameInstance->Get_MouseDelta(MOUSEAXIS::Y);
}

void CPlayer::Apply_MoveIntent(const PLAYER_INTENT_FRAME& Intent, _float fTimeDelta)
{
	if (nullptr != m_pStateMachine)
	{
		const CHARACTER_ACTION eCurrent = m_pStateMachine->Get_CurrentCharacterAction();

		const _bool bIsLocomotion =
			(CHARACTER_ACTION::IDLE == eCurrent) ||
			(CHARACTER_ACTION::WALK == eCurrent) ||
			(CHARACTER_ACTION::RUN == eCurrent);

		if (false == bIsLocomotion)
			return;
	}

	if (false == Intent.bHasMoveIntent)
		return;


	CTransform_3D* pTransform = static_cast<CTransform_3D*>(m_pTransformCom);

	// 카메라 기준 이동 방향으로 부드럽게 회전
	_vector vDirWorld = XMLoadFloat3(&Intent.vMoveDirWorld);
	pTransform->Rotate_Toward_XZ(vDirWorld, pTransform->Get_RotationPerSec() * fTimeDelta);

	// 회전 후 갱신된 Look 방향으로 이동
	_vector vLookXZ = pTransform->Get_State(STATE::LOOK);
	vLookXZ = XMVectorSetY(vLookXZ, 0.f);
	vLookXZ = XMVector3Normalize(vLookXZ);

	const _float fSpeed = pTransform->Get_SpeedPerSec() * m_fSpeedCoeff;
	_vector vPos = pTransform->Get_State(STATE::POSITION);
	vPos = XMVectorAdd(vPos, XMVectorScale(vLookXZ, fSpeed * fTimeDelta));
	pTransform->Set_State(STATE::POSITION, vPos);
}

_float CPlayer::Query_CameraYaw() const
{
	// VIEW 회전부 = 카메라 World 회전부의 Transpose
	// 카메라 Look (월드) = (View._13, View._23, View._33)
	const _float4x4* pView = m_pGameInstance->Get_Transform(D3DTS::VIEW);
	if (nullptr == pView)
		return 0.f;

	return atan2f(pView->_13, pView->_33);
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

	Safe_Release(m_pStateMachine);
	Safe_Release(m_pIntentResolver);
	Safe_Release(m_pBody);
}
