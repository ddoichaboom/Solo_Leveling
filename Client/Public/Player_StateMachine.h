#pragma once

#include "Client_Defines.h"
#include "StateMachine.h"
#include "NotifyListener.h"

NS_BEGIN(Client)
class CPlayer;

class CLIENT_DLL CPlayer_StateMachine final : public CStateMachine, public INotifyListener
{
private:
	CPlayer_StateMachine();
	virtual ~CPlayer_StateMachine() = default;

public:
	CHARACTER_ACTION				Get_CurrentCharacterAction() const
	{
		return static_cast<CHARACTER_ACTION>(Get_CurrentAction());
	}

public:
	HRESULT							Initialize(const CHARACTER_ANIM_TABLE_DESC* pAnimTable);

public:
	void							Update_LocoMotion(const PLAYER_INTENT_FRAME& Intent);
	void							Update_Combat(const PLAYER_INTENT_FRAME& Intent);
	void							Update_Guard(const PLAYER_INTENT_FRAME& Intent);

	_bool							Is_GuardLocked() const;
	_bool							Is_AttackLocked() const;
	void							Bind_Owner(CPlayer* pOwner);
	_bool							Enter_InitialState(CHARACTER_ACTION eInitialAction = CHARACTER_ACTION::IDLE);
	virtual void					OnNotify(const NOTIFY_EVENT& Event) override;

protected:
	virtual void					On_Transition(_uint iFrom, _uint iTo, _bool bInitial) override;

private:
	CPlayer* m_pOwner = { nullptr };
	_bool							m_bLastHasMoveIntent = { false };
	_bool							m_bComboWindowOpen = { false };
	_int							m_iComboStep = { 0 };
	_bool							m_bLastGuardHeld = { false };


public:
	static CPlayer_StateMachine*	Create(const CHARACTER_ANIM_TABLE_DESC* pAnimTable);
	virtual void					Free() override;
};

NS_END