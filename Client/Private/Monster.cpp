#include "Monster.h"
#include "GameInstance.h"
#include "NavigationAgent.h"
#include "NavMesh.h"
#include "PartObject.h"
#include "Body_Monster.h"
#include "Weapon.h"
#include "Collider.h"
#include "Monster_StateMachine.h"
#include "MonsterAnimTable.h"

CMonster::CMonster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CContainerObject{ pDevice, pContext }
{
}

CMonster::CMonster(const CMonster& Prototype)
    : CContainerObject{ Prototype }
{
}

void CMonster::Take_Damage(_float fAmount)
{
    if (m_fCurrentHP <= 0.f)
        return;

    m_fCurrentHP = max(0.f, m_fCurrentHP - fAmount);

    char szLog[128] = {};
    sprintf_s(szLog, "[Monster] HP -%.1f  =>  %.1f / %.1f\n", fAmount, m_fCurrentHP, m_fMaxHP);
    OutputDebugStringA(szLog);
}

void CMonster::Handle_ActionTransition(MONSTER_ACTION eFromAction, MONSTER_ACTION_STEP eFromStep, MONSTER_ACTION eToAction, MONSTER_ACTION_STEP eToStep, _bool bInitial)
{
    if (nullptr == m_pBody)
        return;

    m_pBody->Play_Action(eToAction, eToStep, MONSTER_PHASE::COMMON);
}

void CMonster::Set_WeaponHitboxActive(_bool bActive)
{
    if (true == bActive)
        m_AttackHitTargets.clear();

    if (nullptr != m_pWeapon)
        m_pWeapon->Set_AttackHitboxActive(bActive);

    if (false == bActive)
        m_AttackHitTargets.clear();
}

#ifdef _DEBUG
void CMonster::Debug_TryAction(MONSTER_ACTION eAction, MONSTER_ACTION_STEP eStep)
{
    if (nullptr == m_pStateMachine)
        return;

    m_pStateMachine->Try_Action(eAction, eStep);
}
#endif

HRESULT CMonster::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CMonster::Initialize(void* pArg)
{
    MONSTER_DESC Desc{};

    if (nullptr != pArg)
        Desc = *static_cast<MONSTER_DESC*>(pArg);

    if (SPAWN_TYPE::END == Desc.eSpawnType)
        Desc.eSpawnType = Get_DefaultSpawnType();

    if (MONSTER_ANIM_SET::NONE == Desc.eAnimSet)
        Desc.eAnimSet = Get_DefaultAnimSet();

    if (nullptr == Desc.pBodyModelPrototypeTag)
        Desc.pBodyModelPrototypeTag = Get_DefaultBodyModelPrototypeTag();

    if (nullptr == Desc.pWeaponModelPrototypeTag)
        Desc.pWeaponModelPrototypeTag = Get_DefaultWeaponModelPrototypeTag();

    if (nullptr == Desc.pWeaponSocketBoneName)
        Desc.pWeaponSocketBoneName = Get_DefaultWeaponSocketBoneName();

    if (0.f == Desc.fMaxHP)
        Desc.fMaxHP = 1.f;

    m_strName = Get_DefaultName();
    m_strTag = Get_DefaultName();

    m_eSpawnType = Desc.eSpawnType;
    m_eAnimSet = Desc.eAnimSet;

    m_fMaxHP = Desc.fMaxHP;
    m_fCurrentHP = Desc.fMaxHP;

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    if (FAILED(Ready_Components(Desc)))
        return E_FAIL;

    if (FAILED(Ready_PartObjects(Desc)))
        return E_FAIL;

    if (FAILED(Ready_StateMachine()))
        return E_FAIL;

    return S_OK;
}

void CMonster::Priority_Update(_float fTimeDelta)
{
    for (auto& Pair : m_PartObjects)
    {
        if (nullptr != Pair.second)
            Pair.second->Priority_Update(fTimeDelta);
    }
}

void CMonster::Update(_float fTimeDelta)
{
    if (nullptr != m_pStateMachine)
        m_pStateMachine->Update(fTimeDelta);

    for (auto& Pair : m_PartObjects)
    {
        if (nullptr != Pair.second)
            Pair.second->Update(fTimeDelta);
    }

    if (nullptr != m_pBody)
        Apply_RootMotion(m_pBody->Get_LastRootMotionDelta());
}

void CMonster::Late_Update(_float fTimeDelta)
{
    for (auto& Pair : m_PartObjects)
    {
        if (nullptr != Pair.second)
            Pair.second->Late_Update(fTimeDelta);
    }

    if (nullptr != m_pCollider && nullptr != m_pTransformCom)
    {
        m_pCollider->Update(XMLoadFloat4x4(m_pTransformCom->Get_WorldMatrixPtr()));
        m_pCollider->Register();
    }
}

HRESULT CMonster::Render()
{
    return S_OK;
}

HRESULT CMonster::Ready_Components(const MONSTER_DESC& Desc)
{
    CNavigationAgent::NAVIGATION_AGENT_DESC NavigationDesc{};
    NavigationDesc.pNavMesh = Desc.pNavMesh;
    NavigationDesc.iStartCellIndex = Desc.iStartCellIndex;

    if (FAILED(__super::Add_Component(
        ETOUI(LEVEL::GAMEPLAY),
        TEXT("Prototype_Component_NavigationAgent"),
        TEXT("Com_NavigationAgent"),
        reinterpret_cast<CComponent**>(&m_pNavigationAgent),
        &NavigationDesc)))
        return E_FAIL;

    if (nullptr != m_pNavigationAgent &&
        m_pNavigationAgent->Has_NavMesh() &&
        NAVMESH_INVALID_INDEX == m_pNavigationAgent->Get_CurrentCellIndex())
    {
        _float3 vPosition{};
        XMStoreFloat3(&vPosition, m_pTransformCom->Get_State(STATE::POSITION));
        m_pNavigationAgent->Find_CurrentCell(vPosition);
    }

    // Body OBB Collider
    m_pCollider = CCollider::Create(m_pDevice, m_pContext);
    if (nullptr == m_pCollider)
        return E_FAIL;

    CCollider::COLLIDER_DESC ColliderDesc{};
    ColliderDesc.eBoundingType = COLLIDER::OBB;
    ColliderDesc.eGroup = COLLISION_GROUP::MONSTER_BODY;
    ColliderDesc.vCenter = _float3(0.f, 1.2f, 0.f);   // Boss 가 더 큼 → 약간 위
    ColliderDesc.vSize = _float3(1.2f, 2.4f, 1.2f); // Boss Igris 기준 임시값
    ColliderDesc.vRadians = _float3(0.f, 0.f, 0.f);
    ColliderDesc.pOwner = this;

    if (FAILED(m_pCollider->Initialize(&ColliderDesc)))
        return E_FAIL;

    //m_pCollider->Set_OnHitEnter([](CCollider* pOther) {
    //    OutputDebugStringA("[Collision] Monster Body ENTER\n");
    //    });
    //m_pCollider->Set_OnHitExit([](CCollider* pOther) {
    //    OutputDebugStringA("[Collision] Monster Body EXIT\n");
    //    });

    return S_OK;
}

HRESULT CMonster::Ready_PartObjects(const MONSTER_DESC& Desc)
{
    if (nullptr != Desc.pBodyModelPrototypeTag)
    {
        CBody_Monster::BODY_MONSTER_DESC BodyDesc{};
        BodyDesc.pParentMatrix = m_pTransformCom->Get_WorldMatrixPtr();
        BodyDesc.pModelPrototypeTag = Desc.pBodyModelPrototypeTag;
        BodyDesc.eAnimSet = Desc.eAnimSet;

        if (FAILED(__super::Add_PartObject(
            ETOUI(LEVEL::GAMEPLAY),
            TEXT("Prototype_GameObject_Body_Monster"),
            TEXT("Body"),
            &BodyDesc)))
            return E_FAIL;

        m_pBody = dynamic_cast<CBody_Monster*>(m_PartObjects[TEXT("Body")]);
        Safe_AddRef(m_pBody);
    }

    if (nullptr != Desc.pWeaponModelPrototypeTag)
    {
        if (nullptr == Desc.pWeaponSocketBoneName)
            return E_FAIL;

        const _float4x4* pSocketBoneMatrix =
            m_pBody->Get_BoneMatrixPtr(Desc.pWeaponSocketBoneName);

        if (nullptr == pSocketBoneMatrix)
            return E_FAIL;

        CWeapon::WEAPON_DESC WeaponDesc{};
        WeaponDesc.pParentMatrix        = m_pTransformCom->Get_WorldMatrixPtr();
        WeaponDesc.pSocketBoneMatrix    = pSocketBoneMatrix;
        WeaponDesc.pModelPrototypeTag   = Desc.pWeaponModelPrototypeTag;
        WeaponDesc.bInitiallyVisible    = Desc.bWeaponInitiallyVisible;
        WeaponDesc.eAttackGroup         = COLLISION_GROUP::MONSTER_ATTACK;

        if (FAILED(__super::Add_PartObject(
            ETOUI(LEVEL::GAMEPLAY),
            TEXT("Prototype_GameObject_Weapon"),
            TEXT("Igris_Weapon"),
            &WeaponDesc)))
            return E_FAIL;

        auto iter = m_PartObjects.find(TEXT("Igris_Weapon"));
        if (m_PartObjects.end() == iter)
            return E_FAIL;

        m_pWeapon = dynamic_cast<CWeapon*>(iter->second);
        if (nullptr == m_pWeapon)
            return E_FAIL;

        Safe_AddRef(m_pWeapon);

        if (nullptr != m_pWeapon->Get_BladeCollider())
        {
            m_pWeapon->Get_BladeCollider()->Set_OnHitEnter(
                [this](CCollider* pOther)
                {
                    On_WeaponHitEnter(pOther);
                });
        }
    }


    return S_OK;
}

HRESULT CMonster::Ready_StateMachine()
{
    if (nullptr == m_pBody)
        return S_OK;

    if (MONSTER_ANIM_SET::NONE == m_eAnimSet)
        return S_OK;

    const MONSTER_ANIM_TABLE_DESC* pAnimTable = Find_MonsterAnimTable(m_eAnimSet);
    if (nullptr == pAnimTable)
        return E_FAIL;

    m_pStateMachine = CMonster_StateMachine::Create(pAnimTable);
    if (nullptr == m_pStateMachine)
        return E_FAIL;

    m_pStateMachine->Bind_Owner(this);

    m_pBody->Set_Listener(m_pStateMachine);

    if (false == m_pStateMachine->Enter_InitialState(MONSTER_ACTION::IDLE, MONSTER_ACTION_STEP::NONE))
        return E_FAIL;

    return S_OK;
}

_bool CMonster::Try_ApplyNavigationPosition(const _float3& vCandidatePosition)
{
    if (nullptr == m_pTransformCom)
        return false;

    if (nullptr == m_pNavigationAgent || false == m_pNavigationAgent->Has_NavMesh())
    {
        m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat3(&vCandidatePosition), 1.f));
        return true;
    }

    _float3 vAdjustedPosition{};
    if (false == m_pNavigationAgent->Try_Move(vCandidatePosition, &vAdjustedPosition))
        return false;

    m_pTransformCom->Set_State(STATE::POSITION, XMVectorSetW(XMLoadFloat3(&vAdjustedPosition), 1.f));
    return true;
}

void CMonster::Apply_RootMotion(const _float3& vLocalDelta)
{
    if (0.f == vLocalDelta.x && 0.f == vLocalDelta.y && 0.f == vLocalDelta.z)
        return;

    if (nullptr == m_pTransformCom)
        return;

    _vector vRight = XMVector3Normalize(m_pTransformCom->Get_State(STATE::RIGHT));
    _vector vUp = XMVector3Normalize(m_pTransformCom->Get_State(STATE::UP));
    _vector vLook = XMVector3Normalize(m_pTransformCom->Get_State(STATE::LOOK));

    _vector vWorldDelta = XMVectorScale(vRight, vLocalDelta.x);
    vWorldDelta = XMVectorAdd(vWorldDelta, XMVectorScale(vUp, vLocalDelta.y));
    vWorldDelta = XMVectorAdd(vWorldDelta, XMVectorScale(vLook, vLocalDelta.z));

    _vector vPosition = m_pTransformCom->Get_State(STATE::POSITION);
    vPosition = XMVectorAdd(vPosition, vWorldDelta);

    _float3 vCandidatePosition{};
    XMStoreFloat3(&vCandidatePosition, vPosition);

    Try_ApplyNavigationPosition(vCandidatePosition);
}

void CMonster::On_WeaponHitEnter(CCollider* pOther)
{
    if (nullptr == pOther)
        return;

    if (COLLISION_GROUP::PLAYER_BODY != pOther->Get_Group())
        return;

    CGameObject* pTarget = pOther->Get_Owner();
    if (nullptr == pTarget)
        return;

    if (m_AttackHitTargets.end() != m_AttackHitTargets.find(pTarget))
        return;

    m_AttackHitTargets.insert(pTarget);

#ifdef _DEBUG
    OutputDebugStringA("[Monster] Weapon Hit Player Body\n");
#endif

    // Player HP/피격 상태 연결 후:
    // CPlayer* pPlayer = dynamic_cast<CPlayer*>(pTarget);
    // if (nullptr != pPlayer)
    //     pPlayer->Take_Damage(...);
}

void CMonster::Free()
{
    __super::Free();

    if (nullptr != m_pWeapon && nullptr != m_pWeapon->Get_BladeCollider())
        m_pWeapon->Get_BladeCollider()->Clear_Callbacks();

    m_AttackHitTargets.clear();

    if (nullptr != m_pCollider)
        m_pCollider->Clear_Callbacks();

    Safe_Release(m_pCollider);
    Safe_Release(m_pStateMachine);
    Safe_Release(m_pWeapon);
    Safe_Release(m_pBody);
    Safe_Release(m_pNavigationAgent);
}
