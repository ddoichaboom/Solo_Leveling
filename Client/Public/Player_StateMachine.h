#pragma once

#include "Client_Defines.h"
#include "StateMachine.h"

NS_BEGIN(Client)
class CPlayer;

class CLIENT_DLL CPlayer_StateMachine final : public CStateMachine
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
	void							Bind_Owner(CPlayer* pOwner);
	_bool							Enter_InitialState(CHARACTER_ACTION eInitialAction = CHARACTER_ACTION::IDLE);

protected:
	virtual void					On_Transition(_uint iFrom, _uint iTo, _bool bInitial) override;

private:
	CPlayer*						m_pOwner = { nullptr };

public:
	static CPlayer_StateMachine*	Create(const CHARACTER_ANIM_TABLE_DESC* pAnimTable);
	virtual void					Free() override;
};

NS_END