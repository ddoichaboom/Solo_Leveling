#include "Player.h"
#include "GameInstance.h"
#include "Body_Player.h"
#include "Weapon.h"
#include "Transform_3D.h"
#include "IntentResolver.h"
#include "Player_StateMachine.h"

namespace
{
	typedef struct tagWeaponInfo
	{
		EQUIPPED_WEAPON_ID	eId;
		WEAPON_TYPE			eCategory;
		const _tchar*		pModelTag;
	}WEAPON_INFO;

	static const WEAPON_INFO s_WeaponInfo[] = {
		{EQUIPPED_WEAPON_ID::NONE, WEAPON_TYPE::DEFAULT, nullptr },
		{EQUIPPED_WEAPON_ID::KNIGHT_KILLER, WEAPON_TYPE::DAGGER, TEXT("Prototype_Component_Model_Weapon_KnightKiller") },
		{EQUIPPED_WEAPON_ID::KASAKA_VENOM_FANG, WEAPON_TYPE::DAGGER, TEXT("Prototype_Component_Model_Weapon_KasakaVenomFang") },
	};

	// 양손 단검 디폴트 풀 - DEFAULT/DAGGER 장착 시 보조 손에 사용
	static const _tchar* DAGGER_POOL[] = {
		TEXT("Prototype_Component_Model_Weapon_KnightKiller"),
		TEXT("Prototype_Component_Model_Weapon_KasakaVenomFang"),
	};

	const WEAPON_INFO* Find_WeaponInfo(EQUIPPED_WEAPON_ID eId)
	{
		for (const auto& Info : s_WeaponInfo)
		{
			if (Info.eId == eId)
				return &Info;
		}
		return nullptr;
	}
}

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

	//Set_EquippedWeapon(EQUIPPED_WEAPON_ID::KASAKA_VENOM_FANG);

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
	Tick_WeaponHideTimer(fTimeDelta);

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

CHARACTER_ACTION CPlayer::Pick_RunEndByFoot() const
{
	if (nullptr == m_pBody)
		return CHARACTER_ACTION::RUN_END_LEFT;

	return m_pBody->Pick_RunEndAction();
}

CHARACTER_ACTION CPlayer::Pick_RunFastVariant(const _float3& vMoveDirWolrd) const
{
	if (nullptr == m_pTransformCom)
		return CHARACTER_ACTION::RUN_FAST;

	_vector vLook = XMVector3Normalize(
		XMVectorSetY(m_pTransformCom->Get_State(STATE::LOOK), 0.f));
	_vector vDir = XMVector3Normalize(
		XMVectorSetY(XMLoadFloat3(&vMoveDirWolrd), 0.f));
	
	// dot 으로 각도, cross.y로 좌/우 판단
	_float fDot = XMVectorGetX(XMVector3Dot(vLook, vDir));
	_float fCrossY = XMVectorGetY(XMVector3Cross(vLook, vDir));

	constexpr _float fThresholdCos = 0.9f;

	if (fDot >= fThresholdCos)
		return CHARACTER_ACTION::RUN_FAST;

	return (fCrossY > 0.f)
		? CHARACTER_ACTION::RUN_FAST_RIGHT
		: CHARACTER_ACTION::RUN_FAST_LEFT;
}

void CPlayer::Set_EquippedWeapon(EQUIPPED_WEAPON_ID eId)
{
	if (m_eEquippedWeapon == eId)
		return;

	m_eEquippedWeapon = eId;
	Apply_Loadout();
}

_bool CPlayer::Consume_DashCharge()
{
	if (m_iDashChargeCurent <= 0)
		return false;

	--m_iDashChargeCurent;

	return true;
}

void CPlayer::Set_WeaponsVisible(_bool bVisible)
{
	if (m_bWeaponsVisible == bVisible)
		return;

	m_bWeaponsVisible = bVisible;

	if (true == bVisible)
		m_fIdleTimer = 0.f;

	Refresh_WeaponVisibility();
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

void CPlayer::Tick_WeaponHideTimer(_float fTimeDelta)
{
	if (false == m_bWeaponsVisible || nullptr == m_pStateMachine)
	{
		m_fIdleTimer = 0.f;
		return;
	}

	if (CHARACTER_ACTION::IDLE != m_pStateMachine->Get_CurrentCharacterAction())
	{
		m_fIdleTimer = 0.f;
		return;
	}

	m_fIdleTimer += fTimeDelta;

	if (m_fIdleTimer >= m_fIdleThreshold)
	{
		m_fIdleTimer = 0.f;
		m_pStateMachine->Try_Transition(ETOUI(CHARACTER_ACTION::UNDRAW));
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

	// Weapon Right
	CWeapon::WEAPON_DESC WeaponRDesc{};
	WeaponRDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	WeaponRDesc.pSocketBoneMatrix =	m_pBody->Get_BoneMatrixPtr("Prop_Weapon_Dualwield_01_R");
	WeaponRDesc.pModelPrototypeTag = TEXT("Prototype_Component_Model_Weapon_KnightKiller");
	WeaponRDesc.bInitiallyVisible = true;
	
	if (FAILED(__super::Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_GameObject_Weapon"),
		TEXT("Weapon_R"), &WeaponRDesc)))
		return E_FAIL;

	m_pWeaponR = dynamic_cast<CWeapon*>(m_PartObjects[TEXT("Weapon_R")]);
	Safe_AddRef(m_pWeaponR);

	// Weapon Left
	CWeapon::WEAPON_DESC WeaponLDesc{};
	WeaponLDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
	WeaponLDesc.pSocketBoneMatrix = m_pBody->Get_BoneMatrixPtr("Prop_Weapon_Dualwield_01_L");
	WeaponLDesc.pModelPrototypeTag = TEXT("Prototype_Component_Model_Weapon_KasakaVenomFang");
	WeaponLDesc.bInitiallyVisible = true;

	if (FAILED(__super::Add_PartObject(ETOUI(LEVEL::GAMEPLAY),
		TEXT("Prototype_GameObject_Weapon"),
		TEXT("Weapon_L"), &WeaponLDesc)))
		return E_FAIL;

	m_pWeaponL = dynamic_cast<CWeapon*>(m_PartObjects[TEXT("Weapon_L")]);
	Safe_AddRef(m_pWeaponL);

	return S_OK;
}

HRESULT CPlayer::Ready_StateMachine()
{
	const CHARACTER_ANIM_TABLE_DESC* pAnimTable = Find_CharacterAnimTable(CHARACTER_TYPE::SUNGJINWOO_OVERDRIVE);
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
			(CHARACTER_ACTION::RUN == eCurrent) ||
			(CHARACTER_ACTION::RUN_FAST == eCurrent) ||
			(CHARACTER_ACTION::RUN_FAST_LEFT == eCurrent) ||
			(CHARACTER_ACTION::RUN_FAST_RIGHT == eCurrent);


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

void CPlayer::Apply_Loadout()
{
	if (nullptr == m_pWeaponR || nullptr == m_pWeaponL)
		return;

	const WEAPON_INFO* pInfo = Find_WeaponInfo(m_eEquippedWeapon);
	if (nullptr == pInfo)
		return;

	const _tchar* pRightModel = nullptr;
	const _tchar* pLeftModel = nullptr;
	_bool bLeftVisible = true;

	if (EQUIPPED_WEAPON_ID::NONE == m_eEquippedWeapon)
	{
		// DEFAULT - 양손 디폴트 단검 풀
		pRightModel = DAGGER_POOL[0];		// KnightKiller
		pLeftModel = DAGGER_POOL[1];		// KasakaVenomFang
	}
	else if (WEAPON_TYPE::DAGGER == pInfo->eCategory)
	{
		// 단검 장착
		pRightModel = pInfo->pModelTag;
		for (const _tchar* pCandidate : DAGGER_POOL)
		{
			if (0 != lstrcmpW(pCandidate, pRightModel))
			{
				pLeftModel = pCandidate;
				break;
			}
		}
	}
	else
	{
		// DAGGER 무기 아닌 단일 무기 경우
		pRightModel = pInfo->pModelTag;
		bLeftVisible = false;
	}

	if (nullptr != pRightModel)
		m_pWeaponR->Set_Model(pRightModel);

	if (nullptr != pLeftModel)
		m_pWeaponL->Set_Model(pLeftModel);

	m_bLeftVisibleFromLoadOut = bLeftVisible;
	Refresh_WeaponVisibility();
}

void CPlayer::Refresh_WeaponVisibility()
{
	if (nullptr != m_pWeaponR)
		m_pWeaponR->Set_Visible(m_bWeaponsVisible);
	if (nullptr != m_pWeaponL)
		m_pWeaponL->Set_Visible(m_bWeaponsVisible && m_bLeftVisibleFromLoadOut);
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
	Safe_Release(m_pWeaponL);
	Safe_Release(m_pWeaponR);
	Safe_Release(m_pBody);
}
