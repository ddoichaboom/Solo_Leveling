#ifndef Client_Enum_h__
#define Client_Enum_h__

namespace Client
{
	enum class CHARACTER_ACTION : unsigned int
	{
		// 기본 이동 모션
		IDLE,
		WALK,	
		RUN,	// DASH 이후에 이동은 RUN 처리(속도 빨라짐)

		// 회피
		DASH,
		BACK_DASH,

		// 기본 공격
		BASIC_ATTACK_01,
		BASIC_ATTACK_02,
		BASIC_ATTACK_03,
		CORE_ATTACK_01,

		// 스킬 공격
		SKILL_01,
		SKILL_02,
		U_SKILL,

		// 방어 모션
		GUARD_START,
		GUARD_LOOP,
		GUARD_END,

		// 패링(필요시 더 추가)
		PARRY_WEAK,
		PARRY_STRONG,
		

		// 전투 피격/상태이상
		BREAKFALL,
		HIT,
		STUN,
		DEATH,

		END

	};

	enum class CHARACTER_WEAPON_STATE : unsigned int
	{
		COMMON,
		UNARMED,
		FIGHTER,
		ONE_HAND_SWORD,
		DAGGER,

		END
	};

	enum class CHARACTER_ANIM_SET : unsigned int
	{
		SUNGJINWOO_ERANK,

		END
	};

	enum class CHARACTER_CONTROL_TYPE : unsigned int
	{
		PLAYER, 
		ALLY_AI,
		NPC_AI,

		END
	};

	enum class MONSTER_ACTION : unsigned int
	{
		IDLE,
		WALK,
		RUN,

		ATTACK_01,
		ATTACK_02,
		SKILL_01,

		HIT,
		STUN,
		DEATH,

		END
	};

	enum class MONSTER_PHASE : unsigned int
	{
		COMMON,
		PHASE_01,
		PHASE_02,
		ENRAGED,

		END
	};

	enum class MONSTER_ANIM_SET : unsigned int
	{
		NONE,

		END
	};
}
#endif // Client_Enum_h__
