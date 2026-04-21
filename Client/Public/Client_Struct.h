#ifndef Client_Struct_h
#define Client_Struct_h

namespace Client
{
	typedef struct tagCharacterAnimBindDesc
	{
		CHARACTER_ACTION			eAction			= { CHARACTER_ACTION::END };
		CHARACTER_WEAPON_STATE		eWeapon			= { CHARACTER_WEAPON_STATE::COMMON };
		const _char*				pAnimationName	= { nullptr };
		_bool						bRestartOnEnter = { true };
	}CHARACTER_ANIM_BIND_DESC;

	typedef struct tagCharacterActionPolicy
	{
		CHARACTER_ACTION			eAction = { CHARACTER_ACTION::END };
		_uint						iPriority = {};
		_bool						bAutoReturn = { false };
		CHARACTER_ACTION			eReturnAction = { CHARACTER_ACTION::IDLE };
	} CHARACTER_ACTION_POLICY;

	typedef struct tagCharacterAnimTableDesc
	{
		CHARACTER_ANIM_SET					eAnimSet			= { CHARACTER_ANIM_SET::END };
		const _tchar*						pModelPrototypeTag	= { nullptr };
		const _char*						pRootBoneName		= { nullptr };

		const CHARACTER_ANIM_BIND_DESC*		pClips				= { nullptr };
		_uint								iNumClips			= {};

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

}

#endif // Client_Struct_h