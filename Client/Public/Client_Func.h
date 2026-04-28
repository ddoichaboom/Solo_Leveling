#ifndef Client_Func_h
#define Client_Func_h

namespace Client
{
	inline _uint64 Make_CharacterAnimKey(CHARACTER_STATE eState, CHARACTER_ACTION eAction, WEAPON_TYPE eWeapon)
	{
		return (static_cast<_uint64>(eState) << 32) |
			(static_cast<_uint64>(eAction) << 16) |
			static_cast<_uint64>(eWeapon);
	}

	inline _uint64 Make_MonsterAnimKey(MONSTER_ACTION eAction, MONSTER_PHASE ePhase)
	{
		return (ETOUI64(eAction) << 32) | ETOUI64(ePhase);
	}
}

#endif // Client_Func_h