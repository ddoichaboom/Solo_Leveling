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

    return S_OK;
}

void CPlayer_StateMachine::Update_LocoMotion(const PLAYER_INTENT_FRAME& Intent)
{
    m_bLastHasMoveIntent = Intent.bHasMoveIntent;

    // (1) DASH 입력 처리
    if (true == Intent.bDashRequested && nullptr != m_pOwner)
    {
        if (true == m_pOwner->Can_ConsumeDashCharge())
        {
            const CHARACTER_ACTION eDashAction = (false == Intent.bHasMoveIntent)
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
        if (true == Intent.bHasMoveIntent && nullptr != m_pOwner)
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

    // (2)  RUN / RUN_FAST 중 입력 종료 -> 발위 치 분기
    if (bIsRunAction)
    {
        if (false == Intent.bHasMoveIntent && nullptr != m_pOwner)
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

    // (3) RUN_END 재생 중 다시 이동 입력 -> RUN으로 전환
    if (CHARACTER_ACTION::RUN_END == eCurrent ||
        CHARACTER_ACTION::RUN_END_LEFT == eCurrent ||
        CHARACTER_ACTION::RUN_END_RIGHT == eCurrent)
    {
        if (true == Intent.bHasMoveIntent)
            Try_Transition(ETOUI(CHARACTER_ACTION::RUN));
        return;
    }

    // (4) IDLE 등에서 이동 입력 -> RUN 전환
    if (true == Intent.bHasMoveIntent)
        Try_Transition(ETOUI(CHARACTER_ACTION::RUN));
    else
        Try_Transition(ETOUI(CHARACTER_ACTION::IDLE));
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
        // Step C 이후 처리
        break;
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