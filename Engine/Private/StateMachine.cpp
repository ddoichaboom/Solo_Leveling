#include "StateMachine.h"

CStateMachine::CStateMachine()
{
}

HRESULT CStateMachine::Register_Policy(const ACTION_POLICY_BASE& Policy)
{
	m_Policies[Policy.iAction] = Policy;

	if (m_CooldownRemaining.end() == m_CooldownRemaining.find(Policy.iAction))
		m_CooldownRemaining.emplace(Policy.iAction, 0.f);

	return S_OK;
}

HRESULT CStateMachine::Register_Reject(_uint iFrom, _uint iTo)
{
	m_Rejects.emplace_back(iFrom, iTo);
	return S_OK;
}

_bool CStateMachine::Try_Transition(_uint iNext)
{
	// 등록되지 않은 Action은 거부
	auto iterNext = m_Policies.find(iNext);
	if (iterNext == m_Policies.end())
		return false;

	if (m_bHasCurrentAction && m_iCurrentAction == iNext)
		return true;

	// 현재 Action이 없으면 즉시 진입
	if (false == m_bHasCurrentAction)
	{
		const _uint iPrev = m_iCurrentAction;

		m_iCurrentAction = iNext;
		m_iPendingAction = 0;
		m_bHasPendingAction = false;
		m_bHasCurrentAction = true;

		if (iterNext->second.fCooldown > 0.f)
			m_CooldownRemaining[iNext] = iterNext->second.fCooldown;

		On_Transition(iPrev, iNext, true);
		return true;
	}

	// Reject 리스트 검사
	for (const auto& Reject : m_Rejects)
	{
		if (Reject.first == m_iCurrentAction && Reject.second == iNext)
			return false;
	}

	auto iterCurrent = m_Policies.find(m_iCurrentAction);

	if (iterCurrent == m_Policies.end())
		return false;

	const ACTION_POLICY_BASE& CurrentPolicy = iterCurrent->second;
	const ACTION_POLICY_BASE& NextPolicy = iterNext->second;

	// Priority 비교 : 현재보다 낮으면 거부
	if (NextPolicy.iPriority < CurrentPolicy.iPriority)
		return false;

	// CoolDown 검사
	auto iterCoolDown = m_CooldownRemaining.find(iNext);
	if (iterCoolDown != m_CooldownRemaining.end())
	{
		if (iterCoolDown->second > 0.f)
			return false;
	}

	// Priority가 높거나 같으면 즉시 전이 
	const _uint iPrev = m_iCurrentAction;

	m_iCurrentAction = iNext;
	m_bHasCurrentAction = true;
	m_iPendingAction = 0;
	m_bHasPendingAction = false;

	if (NextPolicy.fCooldown > 0.f)
		m_CooldownRemaining[iNext] = NextPolicy.fCooldown;

	On_Transition(iPrev, iNext, false);

	return true;
}

void  CStateMachine::Update(_float fTimeDelta)
{
	for (auto& Pair : m_CooldownRemaining)
	{
		if (Pair.second > 0.f)
		{
			Pair.second -= fTimeDelta;

			if (Pair.second < 0.f)
				Pair.second = 0.f;
		}
	}
}

void CStateMachine::On_ActionFinished()
{
	if (false == m_bHasCurrentAction)
		return;

	auto iterCurrent = m_Policies.find(m_iCurrentAction);

	m_bHasCurrentAction = false;

	// Pending Action  우선 소비
	if (m_bHasPendingAction)
	{
		_uint iPending = m_iPendingAction;
		m_iPendingAction = 0;
		m_bHasPendingAction = false;

		Try_Transition(iPending);
		return;
	}

	if (iterCurrent == m_Policies.end())
		return;

	const ACTION_POLICY_BASE& CurrentPolicy = iterCurrent->second;

	// AutoReturn 처리
	if (true == CurrentPolicy.bAutoReturn)
	{
		Try_Transition(CurrentPolicy.iReturnAction);
	}
}

void CStateMachine::Free()
{
	__super::Free();

	m_Policies.clear();
	m_CooldownRemaining.clear();
	m_Rejects.clear();


}
