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
	_bool							Is_AttackHitboxActive() const { return m_bAttackHitboxActive; }
	_uint							Get_AttackHitboxWindowSerial() const { return m_iAttackHitboxWindowSerial; }

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

	_bool                           Is_ReactionLocked() const;
	void                            Enter_FloatReaction(CHARACTER_ACTION eFloatAction);
	void                            Update(_float fTimeDelta);
	void                            Update_Reaction(const PLAYER_INTENT_FRAME& Intent);
protected:
	virtual void					On_Transition(_uint iFrom, _uint iTo, _bool bInitial) override;

private:
	CPlayer*						m_pOwner = { nullptr };

private:
	_bool							m_bLastHasMoveIntent = { false };
	_bool							m_bComboWindowOpen = { false };
	_int							m_iComboStep = { 0 };
	_bool							m_bLastGuardHeld = { false };
	_bool							m_bAttackHitboxActive = { false };
	_uint                           m_iAttackHitboxWindowSerial = { 0 };
	_float							m_fDownRecoverTimer = { 0.f };
	static constexpr _float			DOWN_RECOVER_DELAY = { 0.5f };



public:
	static CPlayer_StateMachine*	Create(const CHARACTER_ANIM_TABLE_DESC* pAnimTable);
	virtual void					Free() override;
};

NS_END