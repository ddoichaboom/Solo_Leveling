#include "Boss_Monster.h"
#include "GameInstance.h"
#include "Monster_StateMachine.h"
#include "NavigationAgent.h"
#include "NavMesh.h"
#include "Transform_3D.h"

CBoss_Monster::CBoss_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CMonster{ pDevice, pContext }
{
}

CBoss_Monster::CBoss_Monster(const CBoss_Monster& Prototype)
    : CMonster{ Prototype }
{
}

HRESULT CBoss_Monster::Initialize_Prototype()
{
    return __super::Initialize_Prototype();
}

HRESULT CBoss_Monster::Initialize(void* pArg)
{
    MONSTER_DESC Desc{};

    if (nullptr != pArg)
        Desc = *static_cast<MONSTER_DESC*>(pArg);

    Desc.eSpawnType = SPAWN_TYPE::MONSTER_BOSS;

    if (0.f == Desc.fSpeedPerSec)
        Desc.fSpeedPerSec = 4.f;

    if (0.f == Desc.fRotationPerSec)
        Desc.fRotationPerSec = XMConvertToRadians(360.f);

    if (0.f == Desc.fMaxHP)
        Desc.fMaxHP = 1000.f;
    
    if (0.f == Desc.fMaxBreak)
        Desc.fMaxBreak = 500.f;

    Desc.bHasBreak = true;

    if (FAILED(__super::Initialize(&Desc)))
        return E_FAIL;

    m_bAIEnabled = false;
    m_fAIDecisionInterval = 0.4f;

    m_fMeleeRange = 15.0f;
    m_fMidRange = 30.0f;
    m_fLongRange = 50.0f;

    return S_OK;
}

void CBoss_Monster::Begin_Encounter()
{
    if (true == m_bEncounterStarted)
        return;

    m_bEncounterStarted = true;
    m_bAIEnabled = true;
    m_fAIDecisionTimer = 0.f;

    CGameObject* pTarget = Resolve_Target();
    Face_TargetImmediately(pTarget);

    if (nullptr != m_pStateMachine)
        m_pStateMachine->Try_Action(MONSTER_ACTION::INTRO, MONSTER_ACTION_STEP::NONE);
}

void CBoss_Monster::Handle_ActionTransition(MONSTER_ACTION eFromAction, MONSTER_ACTION_STEP eFromStep,
    MONSTER_ACTION eToAction, MONSTER_ACTION_STEP eToStep, _bool bInitial)
{
    __super::Handle_ActionTransition(eFromAction, eFromStep, eToAction, eToStep, bInitial);

    if (MONSTER_ACTION::SKILL_01 == eToAction && MONSTER_ACTION_STEP::NONE == eToStep)
    {
        Begin_Skill01Dash(Resolve_Target());
        return;
    }

    if (MONSTER_ACTION::SKILL_01 == eFromAction && MONSTER_ACTION::SKILL_01 != eToAction)
        End_Skill01Dash();
}

void CBoss_Monster::Update(_float fTimeDelta)
{
    if (false == m_bEncounterStarted)
    {
        CGameObject* pTriggerTarget = Resolve_Target();
        if (nullptr != pTriggerTarget &&
            nullptr != pTriggerTarget->Get_Transform() &&
            nullptr != m_pNavigationAgent &&
            true == m_pNavigationAgent->Has_NavMesh())
        {
            _float3 vTargetPosition{};
            XMStoreFloat3(&vTargetPosition, pTriggerTarget->Get_Transform()->Get_State(STATE::POSITION));

            if (97 == m_pNavigationAgent->Get_NavMesh()->Find_Cell(vTargetPosition))
                Begin_Encounter();
        }
    }

    Tick_PatternCooldowns(fTimeDelta);

    const _bool bCrashBefore =
        nullptr != m_pStateMachine &&
        MONSTER_ACTION::CRASH == m_pStateMachine->Get_CurrentMonsterAction();

    __super::Update(fTimeDelta);

    if (nullptr == m_pStateMachine)
        return;

    const MONSTER_ACTION eAction = m_pStateMachine->Get_CurrentMonsterAction();
    const MONSTER_ACTION_STEP eStep = m_pStateMachine->Get_CurrentMonsterStep();

    const _bool bCrashNow = (MONSTER_ACTION::CRASH == eAction);
    if (true == bCrashBefore && false == bCrashNow)
        m_bPostCrashPatternPending = true;

    if (MONSTER_ACTION::SKILL_10 == eAction && MONSTER_ACTION_STEP::START == eStep)
        m_fSkill10LoopElapsed = 0.f;

    if (MONSTER_ACTION::SKILL_10 == eAction && MONSTER_ACTION_STEP::LOOP == eStep)
    {
        m_fSkill10LoopElapsed += fTimeDelta;

        if (m_fSkill10LoopElapsed >= m_fSkill10LoopDuration)
        {
            m_fSkill10LoopElapsed = 0.f;
            m_pStateMachine->Try_Action(MONSTER_ACTION::SKILL_10, MONSTER_ACTION_STEP::END);
        }
    }
}

MONSTER_ACTION CBoss_Monster::Select_AIAction(CGameObject* pTarget, _float fDistance)
{
    if (false == m_bEncounterStarted || nullptr == pTarget)
        return MONSTER_ACTION::IDLE;

    if (true == m_bPostCrashPatternPending)
    {
        m_bPostCrashPatternPending = false;

        const MONSTER_ACTION ePostCrashAction = Select_PostCrashPattern(pTarget, fDistance);
        if (MONSTER_ACTION::END != ePostCrashAction)
            return ePostCrashAction;
    }

    if (false == m_bOpeningSkillUsed &&
        fDistance <= m_fMidRange &&
        Is_PatternReady(MONSTER_ACTION::SKILL_01))
    {
        Face_TargetImmediately(pTarget);
        m_bOpeningSkillUsed = true;
        Start_PatternCooldown(MONSTER_ACTION::SKILL_01);
        return MONSTER_ACTION::SKILL_01;
    }

    if (fDistance <= m_fMeleeRange)
    {
        Face_TargetImmediately(pTarget);

        if (Is_PatternReady(MONSTER_ACTION::SKILL_04))
        {
            Start_PatternCooldown(MONSTER_ACTION::SKILL_04);
            return MONSTER_ACTION::SKILL_04;
        }

        if (Is_PatternReady(MONSTER_ACTION::BASIC_ATTACK_02))
        {
            Start_PatternCooldown(MONSTER_ACTION::BASIC_ATTACK_02);
            return MONSTER_ACTION::BASIC_ATTACK_02;
        }

        Start_PatternCooldown(MONSTER_ACTION::BASIC_ATTACK_01);
        return MONSTER_ACTION::BASIC_ATTACK_01;
    }

    if (fDistance <= m_fMidRange)
    {
        Face_TargetImmediately(pTarget);

        if (Is_PatternReady(MONSTER_ACTION::SKILL_13))
        {
            Start_PatternCooldown(MONSTER_ACTION::SKILL_13);
            return MONSTER_ACTION::SKILL_13;
        }

        if (Is_PatternReady(MONSTER_ACTION::SKILL_05))
        {
            Start_PatternCooldown(MONSTER_ACTION::SKILL_05);
            return MONSTER_ACTION::SKILL_05;
        }
    }

    if (fDistance <= m_fLongRange)
    {
        Face_TargetImmediately(pTarget);

        if (Is_PatternReady(MONSTER_ACTION::SKILL_01))
        {
            Start_PatternCooldown(MONSTER_ACTION::SKILL_01);
            return MONSTER_ACTION::SKILL_01;
        }

        if (fDistance > m_fMidRange && Is_PatternReady(MONSTER_ACTION::SKILL_06))
        {
            Start_PatternCooldown(MONSTER_ACTION::SKILL_06);
            return MONSTER_ACTION::SKILL_06;
        }


        if (Is_PatternReady(MONSTER_ACTION::SKILL_05))
        {
            Start_PatternCooldown(MONSTER_ACTION::SKILL_05);
            return MONSTER_ACTION::SKILL_05;
        }
    }

    return MONSTER_ACTION::IDLE;
}

MONSTER_ACTION_STEP CBoss_Monster::Select_AIActionStep(MONSTER_ACTION eAction) const
{
    if (MONSTER_ACTION::SKILL_10 == eAction)
        return MONSTER_ACTION_STEP::START;

    return __super::Select_AIActionStep(eAction);
}

void CBoss_Monster::Apply_RootMotion(const _float3& vLocalDelta)
{
    if (false == m_bSkill01DashActive ||
        nullptr == m_pStateMachine ||
        MONSTER_ACTION::SKILL_01 != m_pStateMachine->Get_CurrentMonsterAction() ||
        nullptr == m_pTransformCom)
    {
        __super::Apply_RootMotion(vLocalDelta);
        return;
    }

    _float3 vCurrentPosition{};
    XMStoreFloat3(&vCurrentPosition, m_pTransformCom->Get_State(STATE::POSITION));

    const _float fRemainX = m_vSkill01DashTargetPosition.x - vCurrentPosition.x;
    const _float fRemainZ = m_vSkill01DashTargetPosition.z - vCurrentPosition.z;
    const _float fRemainSq = fRemainX * fRemainX + fRemainZ * fRemainZ;

    _float3 vAdjustedDelta = vLocalDelta;

    if (fRemainSq <= 0.01f)
    {
        vAdjustedDelta.x = 0.f;
        vAdjustedDelta.z = 0.f;
        End_Skill01Dash();

        __super::Apply_RootMotion(vAdjustedDelta);
        return;
    }

    vAdjustedDelta.x *= m_fSkill01RootMotionScale;
    vAdjustedDelta.z *= m_fSkill01RootMotionScale;

    const _float fFrameMove = sqrtf(vAdjustedDelta.x * vAdjustedDelta.x + vAdjustedDelta.z * vAdjustedDelta.z);
    const _float fRemain = sqrtf(fRemainSq);

    if (fFrameMove > fRemain && fFrameMove > 0.f)
    {
        const _float fClampScale = fRemain / fFrameMove;
        vAdjustedDelta.x *= fClampScale;
        vAdjustedDelta.z *= fClampScale;
        End_Skill01Dash();
    }

    __super::Apply_RootMotion(vAdjustedDelta);
}

MONSTER_ACTION CBoss_Monster::Select_PostCrashPattern(CGameObject* pTarget, _float fDistance)
{
    struct POST_CRASH_PATTERN
    {
        MONSTER_ACTION eAction = { MONSTER_ACTION::END };
        _float         fWeight = { 0.f };
        _bool          bAllowed = { false };
    };

    POST_CRASH_PATTERN Candidates[] =
    {
        { MONSTER_ACTION::SKILL_11, 35.f, fDistance <= m_fMidRange },
        { MONSTER_ACTION::SKILL_10, 25.f, fDistance <= m_fMidRange },
        { MONSTER_ACTION::SKILL_06, 25.f, fDistance > m_fMidRange && fDistance <= m_fLongRange },
        { MONSTER_ACTION::SKILL_01, 20.f, fDistance > m_fMeleeRange && fDistance <= m_fLongRange },
        { MONSTER_ACTION::SKILL_13, 20.f, fDistance <= m_fMidRange },
    };

    _float fTotalWeight = 0.f;
    for (_uint i = 0; i < _countof(Candidates); ++i)
    {
        if (true == Candidates[i].bAllowed && true == Is_PatternReady(Candidates[i].eAction))
            fTotalWeight += Candidates[i].fWeight;
    }

    if (fTotalWeight <= 0.f)
        return MONSTER_ACTION::END;

    const _float fPick = (nullptr != m_pGameInstance)
        ? m_pGameInstance->Random(0.f, fTotalWeight)
        : 0.f;

    _float fAccumulatedWeight = 0.f;
    for (_uint i = 0; i < _countof(Candidates); ++i)
    {
        if (false == Candidates[i].bAllowed || false == Is_PatternReady(Candidates[i].eAction))
            continue;

        fAccumulatedWeight += Candidates[i].fWeight;
        if (fPick <= fAccumulatedWeight)
        {
            Face_TargetImmediately(pTarget);
            Start_PatternCooldown(Candidates[i].eAction);
            return Candidates[i].eAction;
        }
    }

    return MONSTER_ACTION::END;
}

void CBoss_Monster::Begin_Skill01Dash(CGameObject* pTarget)
{
    End_Skill01Dash();

    if (nullptr == pTarget || nullptr == pTarget->Get_Transform() || nullptr == m_pTransformCom)
        return;

    _float3 vCurrentPosition{};
    _float3 vTargetPosition{};

    XMStoreFloat3(&vCurrentPosition, m_pTransformCom->Get_State(STATE::POSITION));
    XMStoreFloat3(&vTargetPosition, pTarget->Get_Transform()->Get_State(STATE::POSITION));

    _float fDirX = vTargetPosition.x - vCurrentPosition.x;
    _float fDirZ = vTargetPosition.z - vCurrentPosition.z;
    const _float fDistanceSq = fDirX * fDirX + fDirZ * fDirZ;

    if (fDistanceSq <= m_fSkill01StopDistance * m_fSkill01StopDistance)
        return;

    const _float fDistance = sqrtf(fDistanceSq);
    fDirX /= fDistance;
    fDirZ /= fDistance;

    Face_TargetImmediately(pTarget);

    m_vSkill01DashTargetPosition = vTargetPosition;
    m_vSkill01DashTargetPosition.x -= fDirX * m_fSkill01StopDistance;
    m_vSkill01DashTargetPosition.z -= fDirZ * m_fSkill01StopDistance;

    if (nullptr != m_pNavigationAgent && true == m_pNavigationAgent->Has_NavMesh())
    {
        CNavMesh* pNavMesh = m_pNavigationAgent->Get_NavMesh();
        const _int iTargetCellIndex = pNavMesh->Find_Cell(m_vSkill01DashTargetPosition);

        if (NAVMESH_INVALID_INDEX != iTargetCellIndex)
            m_vSkill01DashTargetPosition.y = pNavMesh->Compute_Height(iTargetCellIndex, m_vSkill01DashTargetPosition);
        else
            m_vSkill01DashTargetPosition.y = vCurrentPosition.y;
    }
    else
    {
        m_vSkill01DashTargetPosition.y = vCurrentPosition.y;
    }

    const _float fDashDistance = max(0.f, fDistance - m_fSkill01StopDistance);
    m_fSkill01RootMotionScale = fDashDistance / m_fSkill01BaseTravelDistance;
    m_fSkill01RootMotionScale = max(0.35f, min(m_fSkill01RootMotionScale, 5.0f));
    m_bSkill01DashActive = true;
}

void CBoss_Monster::End_Skill01Dash()
{
    m_bSkill01DashActive = false;
    m_fSkill01RootMotionScale = 1.f;
    m_vSkill01DashTargetPosition = {};
}

void CBoss_Monster::Tick_PatternCooldowns(_float fTimeDelta)
{
    for (_uint i = 0; i < static_cast<_uint>(MONSTER_ACTION::END); ++i)
    {
        if (m_fPatternCooldowns[i] > 0.f)
        {
            m_fPatternCooldowns[i] -= fTimeDelta;

            if (m_fPatternCooldowns[i] < 0.f)
                m_fPatternCooldowns[i] = 0.f;
        }
    }
}

void CBoss_Monster::Start_PatternCooldown(MONSTER_ACTION eAction)
{
    const _uint iIndex = static_cast<_uint>(eAction);
    if (iIndex >= static_cast<_uint>(MONSTER_ACTION::END))
        return;

    m_fPatternCooldowns[iIndex] = Get_PatternCooldown(eAction);
}

_bool CBoss_Monster::Is_PatternReady(MONSTER_ACTION eAction) const
{
    const _uint iIndex = static_cast<_uint>(eAction);
    if (iIndex >= static_cast<_uint>(MONSTER_ACTION::END))
        return false;

    return m_fPatternCooldowns[iIndex] <= 0.f;
}

_float CBoss_Monster::Get_PatternCooldown(MONSTER_ACTION eAction) const
{
    switch (eAction)
    {
    case MONSTER_ACTION::BASIC_ATTACK_01:
        return 1.1f;

    case MONSTER_ACTION::BASIC_ATTACK_02:
        return 4.0f;

    case MONSTER_ACTION::SKILL_01:
        return 13.0f;

    case MONSTER_ACTION::SKILL_04:
        return 8.0f;

    case MONSTER_ACTION::SKILL_05:
        return 9.0f;

    case MONSTER_ACTION::SKILL_06:
        return 10.0f;

    case MONSTER_ACTION::SKILL_10:
        return 16.0f;

    case MONSTER_ACTION::SKILL_11:
        return 16.0f;

    case MONSTER_ACTION::SKILL_13:
        return 14.0f;

    default:
        return 0.f;
    }
}

_float3 CBoss_Monster::Get_DirectionToTargetXZ(CGameObject* pTarget) const
{
    _float3 vDirection{};

    if (nullptr == pTarget || nullptr == m_pTransformCom || nullptr == pTarget->Get_Transform())
        return vDirection;

    _vector vSelf = m_pTransformCom->Get_State(STATE::POSITION);
    _vector vTarget = pTarget->Get_Transform()->Get_State(STATE::POSITION);
    _vector vDir = vTarget - vSelf;
    vDir = XMVectorSetY(vDir, 0.f);

    if (XMVectorGetX(XMVector3LengthSq(vDir)) <= 0.0001f)
        return vDirection;

    XMStoreFloat3(&vDirection, XMVector3Normalize(vDir));
    return vDirection;
}

void CBoss_Monster::Face_TargetImmediately(CGameObject* pTarget)
{
    if (nullptr == m_pTransformCom)
        return;

    const _float3 vDirection = Get_DirectionToTargetXZ(pTarget);
    if (0.f == vDirection.x && 0.f == vDirection.z)
        return;

    CTransform_3D* pTransform = static_cast<CTransform_3D*>(m_pTransformCom);
    pTransform->Rotate_Toward_XZ(XMLoadFloat3(&vDirection), XM_PI);
}

void CBoss_Monster::Face_TargetTracking(CGameObject* pTarget, _float fTimeDelta)
{
    if (nullptr == m_pTransformCom)
        return;

    const _float3 vDirection = Get_DirectionToTargetXZ(pTarget);
    if (0.f == vDirection.x && 0.f == vDirection.z)
        return;

    CTransform_3D* pTransform = static_cast<CTransform_3D*>(m_pTransformCom);
    pTransform->Rotate_Toward_XZ(
        XMLoadFloat3(&vDirection),
        m_pTransformCom->Get_RotationPerSec() * fTimeDelta);
}

CBoss_Monster* CBoss_Monster::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CBoss_Monster* pInstance = new CBoss_Monster(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CBoss_Monster");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CGameObject* CBoss_Monster::Clone(void* pArg)
{
    CBoss_Monster* pInstance = new CBoss_Monster(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CBoss_Monster");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBoss_Monster::Free()
{
    __super::Free();
}
