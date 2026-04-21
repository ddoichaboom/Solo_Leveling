#ifndef Client_Func_h
#define Client_Func_h

namespace Client
{
	inline _uint64 Make_CharacterAnimKey(CHARACTER_ACTION eAction, CHARACTER_WEAPON_STATE eWeapon)
	{
		return (ETOUI64(eAction) << 32) | ETOUI64(eWeapon);
	}

	inline _uint64 Make_MonsterAnimKey(MONSTER_ACTION eAction, MONSTER_PHASE ePhase)
	{
		return (ETOUI64(eAction) << 32) | ETOUI64(ePhase);
	}
}

#endif // Client_Func_h