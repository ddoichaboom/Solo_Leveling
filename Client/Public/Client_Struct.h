#ifndef Client_Struct_h
#define Client_Struct_h

namespace Client
{
	typedef struct tagCharacterAnimBindDesc
	{
		CHARACTER_STATE				eState			= { CHARACTER_STATE::END };
		CHARACTER_ACTION			eAction			= { CHARACTER_ACTION::END };
		WEAPON_TYPE					eWeapon			= { WEAPON_TYPE::END };
		const _char*				pAnimationName	= { nullptr };
		_bool						bRestartOnEnter = { true };
	}CHARACTER_ANIM_BIND_DESC;

	typedef struct tagCharacterActionPolicy
	{
		CHARACTER_ACTION			eAction = { CHARACTER_ACTION::END };
		_uint						iPriority = {};
		_bool						bAutoReturn = { false };
		CHARACTER_ACTION			eReturnAction = { CHARACTER_ACTION::IDLE };
		_float						fEnterBlendTime = { 0.f };
	} CHARACTER_ACTION_POLICY;

	typedef struct tagCharacterAnimTableDesc
	{
		CHARACTER_TYPE						eCharacterType		= { CHARACTER_TYPE::END };
		const _tchar*						pModelPrototypeTag	= { nullptr };
		const _char*						pRootBoneName		= { nullptr };

		const CHARACTER_ANIM_BIND_DESC*		pBinds				= { nullptr };
		_uint								iNumBinds			= {};

		const CHARACTER_ACTION_POLICY*		pPolicies			= { nullptr };
		_uint								iNumPolicies		= {};
	} CHARACTER_ANIM_TABLE_DESC;

	typedef struct tagMonsterAnimBindDesc
	{
		MONSTER_ACTION			eAction = { MONSTER_ACTION::END };
		MONSTER_PHASE			ePhase = { MONSTER_PHASE::COMMON };
		const _char* pAnimationName = { nullptr };
		_bool					bRestartOnEnter = { true };
	} MONSTER_ANIM_BIND_DESC;

	typedef struct tagMonsterActionPolicy
	{
		MONSTER_ACTION			eAction = { MONSTER_ACTION::END };
		_uint					iPriority = {};
		_bool					bAutoReturn = { false };
		MONSTER_ACTION			eReturnAction = { MONSTER_ACTION::IDLE };
	} MONSTER_ACTION_POLICY;

	typedef struct tagMonsterAnimTableDesc
	{
		MONSTER_ANIM_SET				eAnimSet = { MONSTER_ANIM_SET::END };
		const _tchar*					pModelPrototypeTag = { nullptr };
		const _char*					pRootBoneName = { nullptr };

		const MONSTER_ANIM_BIND_DESC*	pClips = { nullptr };
		_uint							iNumClips = {};

		const MONSTER_ACTION_POLICY*	pPolicies = { nullptr };
		_uint							iNumPolicies = {};
	} MONSTER_ANIM_TABLE_DESC;

	typedef struct tagPlayerRawInputFrame
	{
		_bool bMoveForwardHeld = { false };
		_bool bMoveBackwardHeld = { false };
		_bool bMoveLeftHeld = { false };
		_bool bMoveRightHeld = { false };

		_bool bRButtonHeld = { false };
		_bool bLButtonPressed = { false };
		_bool bDashPressed = { false };


		_long lMouseDeltaX = {};
		_long lMouseDeltaY = {};

	}PLAYER_RAW_INPUT_FRAME;

	typedef struct tagPlayerIntentFrame
	{
		_float2 vMoveAxis = {};
		_float3 vMoveDirWorld = {};
		_long	lLookDeltaX = {};
		_bool	bHasMoveIntent = { false };

		_bool bDashRequested = { false };		// 이번 프레임에 Dash 요청
		_bool bAttackRequested = { false };

		_bool bGuardHeld = { false };

	}PLAYER_INTENT_FRAME;

}

#endif // Client_Struct_h