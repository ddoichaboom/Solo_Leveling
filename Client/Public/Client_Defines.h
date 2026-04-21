#pragma once
#include <Windows.h>
#include <process.h>

#include "Engine_Defines.h"
#include "Client_Enum.h"
#include "Client_Struct.h"
#include "Client_Func.h"

using namespace Client;

// 키 매핑 정보 ( 마우스 + 키보드 )
// 이동 
// : W / A / S / D
// 회피 (W + SPACE = DASH / S + SPACE = BACK DASH) / DASH,BACK_DASH 이후에는 일정 시간 동안 WALK -> RUN
// : SPACE
// 무기 교체 / 강공격(Core Attack) - 무기 교체하면서 해당 스킬이 나가면서 무기가 자연스럽게 교체됨
// : C 
// 상호작용 ( NPC와 대화, 특정 오브젝트와 상호 작용 등 )
// : G
// 일반 공격 (일정 시간 안에 누르면 다음 공격 진행 Base_1->Base_2->Base_3)
// : MOUSE_LB
// 일반 스킬 (Skill_01/Skill_02)
// : Q | E
// 궁극기 스킬 ( U_Skill )
// : R
// 막기 (Guard 모션 재생 - 적의 공격을 막으면 Parrying 판정)
// : MOUSE_RB
// 오버 드라이브 (특수 공격 - 기존 모델에는 해당 애니메이션 X)
// : V
// 무기 고유 스킬 
// : F 
// QTE 스킬 
// : LEFT SHIFT
// 군주화/싱크로 체인 (성진우의 일정 시간 동안의 진화 스킬 개념)
// : T
// 물약 ( 물약 먹으며 체력 회복 )
// : Z
// 동료 헌터 태그 ( 성진우가 아닌 헌터들 3명으로 태그하며 플레이하는 모드인 경우 - 헌터 모드 )
// 1 | 2 | 3
// 그림자 소환 ( 성진우가 메인 플레이어 인 경우 - 성진우 모드 )
// : 1
// 락온 ( 특정 개체에게 고정 )
// : Mouse_Wheel Click
// 락온 변경 
// : Mouse_Wheel Scroll

