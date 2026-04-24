#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CStateMachine abstract :public CBase
{
protected:
	CStateMachine();
	virtual ~CStateMachine() = default;

public:
	_bool										Has_CurrentAction() const { return m_bHasCurrentAction; }
	_uint										Get_CurrentAction() const { return m_iCurrentAction; }
	_uint										Get_PendingAction() const { return m_iPendingAction; }

public:
	HRESULT										Register_Policy(const ACTION_POLICY_BASE& Policy);
	HRESULT										Register_Reject(_uint iFrom, _uint iTo);

	_bool										Try_Transition(_uint iNext);
	void										Update(_float fTimeDelta);
	void										On_ActionFinished();

protected:
	virtual void								On_Transition(_uint iFrom, _uint iTo, _bool bInitial) PURE;

protected:
	unordered_map<_uint, ACTION_POLICY_BASE>	m_Policies;
	unordered_map<_uint, _float>				m_CooldownRemaining;
	vector<pair<_uint, _uint>>					m_Rejects;

	_uint										m_iCurrentAction = {};
	_bool										m_bHasCurrentAction = { false };

	_uint										m_iPendingAction = {};
	_bool										m_bHasPendingAction = { false };

public:
	virtual void								Free() override;
};

NS_END