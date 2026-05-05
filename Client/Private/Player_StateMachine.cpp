#include "Player_StateMachine.h"
#include "Player.h"

CPlayer_StateMachine::CPlayer_StateMachine()
{
}

HRESULT	CPlayer_StateMachine::Initialize(const CHARACTER_ANIM_TABLE_DESC* pAnimTable)
{
    if (nullptr == pAnimTable)
        return E_FAIL;

    for (_uint i = 0; i < pAnimTable->iNumPolicies; ++i)
    {
        const CHARACTER_ACTION_POLICY& SrcPolicy = pAnimTable->pPolicies[i];

        ACTION_POLICY_BASE DstPolicy{};
        DstPolicy.iAction = ETOUI(SrcPolicy.eAction);
        DstPolicy.iPriority = SrcPolicy.iPriority;
        DstPolicy.bAutoReturn = SrcPolicy.bAutoReturn;
        DstPolicy.iReturnAction = ETOUI(SrcPolicy.eReturnAction);
        DstPolicy.fCooldown = 0.f;
        DstPolicy.fEnterBlendTime = SrcPolicy.fEnterBlendTime;;

        if (FAILED(Register_Policy(DstPolicy)))
            return E_FAIL;
    }

    if (FAILED(Register_Reject(ETOUI(CHARACTER_ACTION::DASH), ETOUI(CHARACTER_ACTION::GUARD_START))))
        return E_FAIL;
    if (FAILED(Register_Reject(ETOUI(CHARACTER_ACTION::BACK_DASH), ETOUI(CHARACTER_ACTION::GUARD_START))))
        return E_FAIL;

    return S_OK;
}

void CPlayer_StateMachine::Update_LocoMotion(const PLAYER_INTENT_FRAME& Intent)
{
    const _bool bHasMoveIntent = Has_MoveIntent(Intent);
    m_bLastHasMoveIntent = bHasMoveIntent;

    if (true == Intent.bDashRequested && nullptr != m_pOwner)
    {
        if (true == m_pOwner->Can_ConsumeDashCharge())
        {
            const CHARACTER_ACTION eDashAction = (false == bHasMoveIntent)
                ? CHARACTER_ACTION::BACK_DASH
                : CHARACTER_ACTION::DASH;

            if (CHARACTER_ACTION::DASH == eDashAction)
                m_pOwner->Face_DirectionImmediately(Intent.vMoveDirWorld);

            if (true == Try_Transition(ETOUI(eDashAction)))
                m_pOwner->Consume_DashCharge();
            return;
        }
    }

    const CHARACTER_ACTION eCurrent = Get_CurrentCharacterAction();

    if (CHARACTER_ACTION::UNDRAW == eCurrent)
    {
        if (true == bHasMoveIntent && nullptr != m_pOwner)
        {
            m_pOwner->Set_WeaponsVisible(false);
            __super::On_ActionFinished();
        }
        return;
    }

    const _bool bIsRunAction =
        (CHARACTER_ACTION::RUN == eCurrent) ||
        (CHARACTER_ACTION::RUN_FAST == eCurrent) ||
        (CHARACTER_ACTION::RUN_FAST_LEFT == eCurrent) ||
        (CHARACTER_ACTION::RUN_FAST_RIGHT == eCurrent);

    if (bIsRunAction)
    {
        if (false == bHasMoveIntent && nullptr != m_pOwner)
        {
            const CHARACTER_ACTION eEndAction = m_pOwner->Pick_RunEndByFoot();
            Try_Transition(ETOUI(eEndAction));
            return;
        }

        if (CHARACTER_ACTION::RUN != eCurrent && nullptr != m_pOwner)
        {
            const CHARACTER_ACTION eVariant = m_pOwner->Pick_RunFastVariant(Intent.vMoveDirWorld, eCurrent);
            if (eVariant != eCurrent)
                Try_Transition(ETOUI(eVariant));
        }
        return;
    }

    if (CHARACTER_ACTION::RUN_END == eCurrent ||
        CHARACTER_ACTION::RUN_END_LEFT == eCurrent ||
        CHARACTER_ACTION::RUN_END_RIGHT == eCurrent)
    {
        if (true == bHasMoveIntent)
            Try_Transition(ETOUI(CHARACTER_ACTION::RUN));
        return;
    }

    if (true == bHasMoveIntent)
        Try_Transition(ETOUI(CHARACTER_ACTION::RUN));
    else
        Try_Transition(ETOUI(CHARACTER_ACTION::IDLE));
}

void CPlayer_StateMachine::Update_Combat(const PLAYER_INTENT_FRAME& Intent)
{
    const CHARACTER_ACTION eCur = Get_CurrentCharacterAction();

    const _bool bIsAttacking =
        (CHARACTER_ACTION::BASIC_ATTACK_01 == eCur) ||
        (CHARACTER_ACTION::BASIC_ATTACK_02 == eCur) ||
        (CHARACTER_ACTION::BASIC_ATTACK_03 == eCur);
    
    const _bool bHasMoveIntent = Has_MoveIntent(Intent);

    // 1) 공격 진행 중
    if (true == bIsAttacking
        && true == m_bComboWindowOpen
        && true == bHasMoveIntent
        && false == Intent.bAttackRequested)
    {
        __super::On_ActionFinished();
        Try_Transition(ETOUI(CHARACTER_ACTION::IDLE));
        m_iComboStep = 0;
        m_bComboWindowOpen = false;
        return;
    }

    // 2) LMB 미 입력 시 종료
    if (false == Intent.bAttackRequested)
        return;

    // 3) 콤보 진행 중
    if (true == bIsAttacking)
    {
        if (false == m_bComboWindowOpen) return;

        const CHARACTER_ACTION eNext =
            (CHARACTER_ACTION::BASIC_ATTACK_01 == eCur) ? CHARACTER_ACTION::BASIC_ATTACK_02 :
            (CHARACTER_ACTION::BASIC_ATTACK_02 == eCur) ? CHARACTER_ACTION::BASIC_ATTACK_03 :
            CHARACTER_ACTION::BASIC_ATTACK_01; 

        __super::On_ActionFinished();
        if (true == Try_Transition(ETOUI(eNext)))
        {
            m_iComboStep =
                (CHARACTER_ACTION::BASIC_ATTACK_02 == eNext) ? 2 :
                (CHARACTER_ACTION::BASIC_ATTACK_03 == eNext) ? 3 : 1;
            m_bComboWindowOpen = false;
        }
        return;
    }

    // 4) 비공격 상태
    if (true == Try_Transition(ETOUI(CHARACTER_ACTION::BASIC_ATTACK_01)))
    {
        m_iComboStep = 1;
        m_bComboWindowOpen = false;
    }
}

void CPlayer_StateMachine::Update_Guard(const PLAYER_INTENT_FRAME& Intent)
{
    m_bLastGuardHeld = Intent.bGuardHeld;

    const CHARACTER_ACTION eCur = Get_CurrentCharacterAction();

    const _bool bIsGuarding =
        (CHARACTER_ACTION::GUARD_START == eCur) ||
        (CHARACTER_ACTION::GUARD_LOOP == eCur);

    // 가드 진입
    if (false == bIsGuarding && true == Intent.bGuardHeld)
    {
        if (CHARACTER_ACTION::GUARD_END == eCur)
            return;

        Try_Transition(ETOUI(CHARACTER_ACTION::GUARD_START));
        return;
    }

    // 가드 해제
    if (true == bIsGuarding && false == Intent.bGuardHeld)
    {
        __super::On_ActionFinished();
        Try_Transition(ETOUI(CHARACTER_ACTION::GUARD_END));
        return;
    }
}

_bool CPlayer_StateMachine::Is_GuardLocked() const
{
    const CHARACTER_ACTION eCur = Get_CurrentCharacterAction();
    return (CHARACTER_ACTION::GUARD_START == eCur) ||
        (CHARACTER_ACTION::GUARD_LOOP == eCur) ||
        (CHARACTER_ACTION::GUARD_END == eCur);
}

_bool CPlayer_StateMachine::Is_AttackLocked() const
{
    const CHARACTER_ACTION eCur = Get_CurrentCharacterAction();

    const _bool bIsAttacking = 
        (CHARACTER_ACTION::BASIC_ATTACK_01 == eCur) ||
        (CHARACTER_ACTION::BASIC_ATTACK_02 == eCur) ||
        (CHARACTER_ACTION::BASIC_ATTACK_03 == eCur);

    if (false == bIsAttacking)
        return false;

    return (false == m_bComboWindowOpen);
}

void CPlayer_StateMachine::Bind_Owner(CPlayer* pOwner)
{
    m_pOwner = pOwner;
}

_bool CPlayer_StateMachine::Enter_InitialState(CHARACTER_ACTION eInitialAction)
{
    return Try_Transition(ETOUI(eInitialAction));
}

void CPlayer_StateMachine::OnNotify(const NOTIFY_EVENT& Event)
{
    switch (Event.eType)
    {
    case NOTIFY_TYPE::ACTION_FINISHED:
    {
        const CHARACTER_ACTION eFinished = static_cast<CHARACTER_ACTION>(Event.iPayload);

        __super::On_ActionFinished();

        // DASH/BACK_DASH 종료 -> 입력 있으면 RUN_FAST, 없으면 발 위치 분기
        const _bool bDashFinished =
            (CHARACTER_ACTION::DASH == eFinished) ||
            (CHARACTER_ACTION::BACK_DASH == eFinished);

        if (bDashFinished)
        {
            if (true == m_bLastHasMoveIntent)
            {
                Try_Transition(ETOUI(CHARACTER_ACTION::RUN_FAST));
            }
            else
            {
                Try_Transition(ETOUI(CHARACTER_ACTION::IDLE));
            }

            break;
        }

        // RUN_END_LEFT / RIGHT 종료 -> IDLE
        const _bool bRunEndFinished =
            (CHARACTER_ACTION::RUN_END == eFinished) ||
            (CHARACTER_ACTION::RUN_END_LEFT == eFinished) ||
            (CHARACTER_ACTION::RUN_END_RIGHT == eFinished);

        if (bRunEndFinished)
        {
            Try_Transition(ETOUI(CHARACTER_ACTION::IDLE));
            break;
        }

        const _bool bAttackFinished =
            (CHARACTER_ACTION::BASIC_ATTACK_01 == eFinished) ||
            (CHARACTER_ACTION::BASIC_ATTACK_02 == eFinished) ||
            (CHARACTER_ACTION::BASIC_ATTACK_03 == eFinished);
        
        if (bAttackFinished)
        {
            m_iComboStep = 0;
            m_bComboWindowOpen = false;

            Try_Transition(ETOUI(CHARACTER_ACTION::IDLE));
            break;
        }

        if (CHARACTER_ACTION::GUARD_START == eFinished)
        {
            if (true == m_bLastGuardHeld)
                Try_Transition(ETOUI(CHARACTER_ACTION::GUARD_LOOP));
            else
                Try_Transition(ETOUI(CHARACTER_ACTION::GUARD_END));
            break;
        }
        else if (CHARACTER_ACTION::GUARD_END == eFinished)
        {
            Try_Transition(ETOUI(CHARACTER_ACTION::IDLE));
            break;
        }

        // UNDRAW 완료 -> HIDDEN
        if (CHARACTER_ACTION::UNDRAW == eFinished)
        {
            if (nullptr != m_pOwner)
                m_pOwner->Set_WeaponsVisible(false);
            break;
        }

        break;
    }
    case NOTIFY_TYPE::ANIM_EVENT:
    {
        const ANIM_NOTIFY_TYPE eAnim = static_cast<ANIM_NOTIFY_TYPE>(Event.iPayload);

        switch (eAnim)
        {
        case ANIM_NOTIFY_TYPE::FOOTSTEP_L:
        case ANIM_NOTIFY_TYPE::FOOTSTEP_R:
            // Step E (Audio): 발소리 재생 위치. 현재는 무시.
            break;

        case ANIM_NOTIFY_TYPE::ATTACK_HIT:
            // Step C-6 (BASIC_ATTACK 콤보): 적 피격 박스 활성화 트리거 위치. 현재는 무시.
            break;

        case ANIM_NOTIFY_TYPE::COMBO_WINDOW_OPEN:
            m_bComboWindowOpen = true;
            break;

        case ANIM_NOTIFY_TYPE::COMBO_WINDOW_CLOSE:
            m_bComboWindowOpen = false;
            break;

        case ANIM_NOTIFY_TYPE::NONE:
        case ANIM_NOTIFY_TYPE::END:
        default:
            break;
        }
        break;
    }
    default:
        break;
    }
}

void CPlayer_StateMachine::On_Transition(_uint iFrom, _uint iTo, _bool bInitial)
{
    if (nullptr == m_pOwner)
        return;

    m_pOwner->Handle_ActionTransition(
        static_cast<CHARACTER_ACTION>(iFrom),
        static_cast<CHARACTER_ACTION>(iTo),
        bInitial);

    const CHARACTER_ACTION eTo = static_cast<CHARACTER_ACTION>(iTo);

    switch (eTo)
    {
    case CHARACTER_ACTION::WALK:
        m_pOwner->Set_SpeedCoeff(1.0f);
        break;
    case CHARACTER_ACTION::RUN:
        m_pOwner->Set_SpeedCoeff(1.5f);
        break;
    case CHARACTER_ACTION::RUN_FAST:
    case CHARACTER_ACTION::RUN_FAST_LEFT:
    case CHARACTER_ACTION::RUN_FAST_RIGHT:
        m_pOwner->Set_SpeedCoeff(2.0f);
        break;
    case CHARACTER_ACTION::IDLE:
    case CHARACTER_ACTION::RUN_END:
    case CHARACTER_ACTION::RUN_END_LEFT:
    case CHARACTER_ACTION::RUN_END_RIGHT:
        m_pOwner->Set_SpeedCoeff(0.f);
        break;
    case CHARACTER_ACTION::DASH:
    case CHARACTER_ACTION::BACK_DASH:
        m_pOwner->Set_SpeedCoeff(0.f);
        break;
    case CHARACTER_ACTION::BASIC_ATTACK_01:
    case CHARACTER_ACTION::BASIC_ATTACK_02:
    case CHARACTER_ACTION::BASIC_ATTACK_03:
        m_pOwner->Set_SpeedCoeff(0.f);
        break;
    case CHARACTER_ACTION::GUARD_START:
    case CHARACTER_ACTION::GUARD_LOOP:
    case CHARACTER_ACTION::GUARD_END:
        m_pOwner->Set_SpeedCoeff(0.f);
        break;
    case CHARACTER_ACTION::UNDRAW:
        m_pOwner->Set_SpeedCoeff(0.f);
        break;
    default:
        m_pOwner->Set_SpeedCoeff(0.f);
        break;
    }

    switch (eTo)
    {
    case CHARACTER_ACTION::WALK:
    case CHARACTER_ACTION::RUN:
    case CHARACTER_ACTION::RUN_FAST:
    case CHARACTER_ACTION::RUN_FAST_LEFT:
    case CHARACTER_ACTION::RUN_FAST_RIGHT:
    case CHARACTER_ACTION::RUN_END:
    case CHARACTER_ACTION::RUN_END_LEFT:
    case CHARACTER_ACTION::RUN_END_RIGHT:
    case CHARACTER_ACTION::DASH:
    case CHARACTER_ACTION::BACK_DASH:
        m_pOwner->Set_WeaponsVisible(false);
        break;
    
    case CHARACTER_ACTION::BASIC_ATTACK_01:
    case CHARACTER_ACTION::BASIC_ATTACK_02:
    case CHARACTER_ACTION::BASIC_ATTACK_03:
        m_pOwner->Set_WeaponsVisible(true);
        break;
    case CHARACTER_ACTION::GUARD_START:
    case CHARACTER_ACTION::GUARD_LOOP:
    case CHARACTER_ACTION::GUARD_END:
        m_pOwner->Set_WeaponsVisible(true);
        break;
    case CHARACTER_ACTION::IDLE:
    case CHARACTER_ACTION::UNDRAW:
        break;

    default:
        break;
    }
}

CPlayer_StateMachine* CPlayer_StateMachine::Create(const CHARACTER_ANIM_TABLE_DESC* pAnimTable)
{
    CPlayer_StateMachine* pInstance = new CPlayer_StateMachine();

    if (FAILED(pInstance->Initialize(pAnimTable)))
    {
        MSG_BOX("Failed to Created : CPlayer_StateMachine");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CPlayer_StateMachine::Free()
{
    __super::Free();
}