#ifndef Client_Enum_h__
#define Client_Enum_h__


namespace Client
{
	enum class LEVEL { STATIC, LOADING, LOGO, GAMEPLAY, END };

	enum class CHARACTER_STATE { LOCOMOTION, COMBAT, GUARD, HIT_STAGGER, AIRBORNE, DOWN, STUN, ACTION_LOCKED, DEAD, END };

	enum class WEAPON_TYPE { DEFAULT, DAGGER, SWORD, BOW, PISTOL, POLE_WEAPON, MAGICAL_WEAPON, TWO_HANDED_WEAPON, END };

	enum class CHARACTER_ACTION
	{
		// LOCOMOTION 
		IDLE,
		WALK,
		RUN,
		RUN_FAST,
		RUN_FAST_LEFT,
		RUN_FAST_RIGHT,
		DASH,
		BACK_DASH,
		RUN_END,		// 애니메이션이 LEFT랑 비슷하지만 클립이 존재하므로 발 비교가 불가능할 때 분기하기 위해 바인딩
		RUN_END_LEFT,
		RUN_END_RIGHT,
		END
	};

	enum class CHARACTER_TYPE
	{
		// SungJinWoo
		SUNGJINWOO_ERANK,
		SUNGJINWOO_OVERDRIVE,
		
		END
	};

	//enum class CHARACTER_ACTION { };

	//enum class CHARACTER_ACTION : unsigned int
	//{
	//	// 플레이어 이동 (WASD)
	//	// 기본 상태 
	//	// ( 손에 무기가 부착되어 있는 경우 = 공격/스킬 사용 후 -> UnDraw Weapon 애니메이션 호출 후 IDLE로 복귀)
	//	IDLE,

	//	WALK,			 // 플레이어 이동 입력 직후는 WALK
	//	RUN,			// 이동 입력이 지속될 경우 RUN으로 전환
	//	RUN_FAST,		// DASH 이후의 이동 입력은 RUN_FAST로 진행.

	//	// 회피 (Space)
	//	DASH,
	//	BACK_DASH,

	//	// 기본 공격 - (LMB) 
	//	// 세부 로직은 차차 구현할 예정인데 해당 BASIC_ATTACK의 애니메이션은 현재 장착된/보이고 있는 Weapon Type에 의존 (DAGGER/PISTOL/...)
	//	BASIC_ATTACK_01,
	//	BASIC_ATTACK_02,
	//	BASIC_ATTACK_03,

	//	// 무기 고유 스킬 (C)
	//	CORE_ATTACK,

	//	// 일반 스킬 (Q/E)
	//	// 스킬 트리 - 일반 스킬 세트 A/B 세트에 각각 두개의 스킬들을 매핑해놓는다. 
	//	// 1번 무기 사용 중 - A 세트 -> Core Attack 사용 ->  2번 무기 전환 - B 세트 전환 
	//	NORMAL_SKILL,

	//	// 궁극기 (R)
	//	ULTIMATE,

	//	// 방어 (RMB)
	//	GUARD,
	//	
	//	// 전투 피격/상태이상
	//	BREAKFALL,
	//	HIT,
	//	STUN,
	//	DEATH,

	//	LINK_ATTACK,
	//	CHAIN_SMASH,

	//	END

	//};

	//enum class CHARACTER_TYPE { SUNGJINWOO_OVERDRIVE, END };

	//enum class CHARACTER_CONTROL_TYPE : unsigned int
	//{
	//	PLAYER, 
	//	ALLY_AI,
	//	NPC_AI,

	//	END
	//};

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
