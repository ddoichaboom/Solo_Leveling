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
        DstPolicy.fEnterBlendTime = 0.f;

        if (FAILED(Register_Policy(DstPolicy)))
            return E_FAIL;
    }

    return S_OK;
}

void CPlayer_StateMachine::Update_LocoMotion(const PLAYER_INTENT_FRAME& Intent)
{
    if (true == Intent.bHasMoveIntent)
    {
        Try_Transition(ETOUI(CHARACTER_ACTION::WALK));
    }
    else
    {
        Try_Transition(ETOUI(CHARACTER_ACTION::IDLE));
    }
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
        __super::On_ActionFinished();
        break;

    case NOTIFY_TYPE::ANIM_EVENT:
        // Step C 에서 AnimNotify 처리 연결
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