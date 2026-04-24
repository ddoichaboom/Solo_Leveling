# 단계 3 — Player 게임플레이 세부 계획

> 작성일: 2026-04-22
> 상위 문서: `명세서/통합_구현계획_v2.md` §4.1
> 선행 문서: `명세서/4월 21일(화) 진행사항.md` (Step 2.5 런타임 테스트베드)
> 본 문서는 단계 3 진입 시점의 **세부 설계 논의 문서**이다. v2 문서가 목차 중심이라면 본 문서는 구현 경계/책임 분리/데이터 흐름을 결정한다. 결정되지 않은 항목은 "결정 대기"로 표기한다.

---

## 1. 범위와 비범위

### 1.1 이번 단계에서 다루는 범위

- 플레이어 입력 레이어링 (raw → Intent → Action)
- `CPlayer` 상태머신 (Current / Pending / Priority 가드 / AutoReturn 정책)
- `Current/Pending WeaponState` 분리 및 무기 스왑(`C`) 해석
- **쿨다운 로직 인프라** (`CHARACTER_ACTION_POLICY::fCooldown` + StateMachine `m_CooldownRemaining` + `Try_Transition` 체크). 실사용은 WEAPON_SWAP 1건
- 이동 정책 통합 (In-place WASD ↔ Root Motion Dash)
- 기본 콤보 (BASIC_ATTACK_01 → 02 → 03) 체이닝 윈도우 (AnimNotify 전제)
- 카메라 yaw 기반 이동 방향 해석 (비락온 상태 한정)
- **최소 추적 카메라 신설** (Player 등 뒤 고정 오프셋, Mouse X → Player Yaw 회전 시 카메라 따라 돔)
- **Camera_Free 를 Editor 전용으로 분리** — Client 런타임 경로(`Loader`, `Level_GamePlay`)에서 제거
- 애니메이션 전환 블렌딩의 **설계 접점만 정의** (실제 블렌딩 구현은 본 단계 **비범위**)

### 1.2 이번 단계에서 **다루지 않는** 범위

- 락온 / 타겟 추적 카메라 — 이후 단계
- 추적 카메라의 Pitch 제한 / 거리 조절(줌) / 충돌 회피 — 필요해지면 후속 보강
- **실제 블렌딩 구현** — 접점/API 시그니처만 합의. IDLE 하드컷은 본 단계 종료 시점에도 남는다 (§3.1 재설계 단계에서 해소)
- **스킬 Intent 인프라** — Q/E/R/F/V/T/LEFT SHIFT/G/Z 는 Intent enum 에도 넣지 않음. 필요해지는 단계에서 Intent/키 매핑을 함께 확장
- 히트박스 / 피격 판정 — 단계 5 (Collider/DebugDraw) 선행 필요
- NavMesh 제약 이동 — 단계 4 선행 필요
- HUD / 무기 UI / 쿨타임 **UI 시각화** — 단계 6 (쿨타임 **로직** 은 단계 3 포함, §1.1 참조)
- **스킬별 쿨다운 값/매핑** — 스킬 인프라 도입 단계에서 연결. 본 단계 실사용은 WEAPON_SWAP 1건
- 그림자 소환 (`1/2/3`) / 동료 헌터 태그 / 물약(`Z`) — 이번 범위 아님

### 1.3 §1 결정사항 (2026-04-22 합의)

| 항목 | 결정 |
|---|---|
| 카메라 | **최소 추적 카메라 구현** (Camera_Free 는 Editor 전용으로 분리). 필요 시 Pitch 제한까지 확장 |
| 스킬 Intent 인프라 | **연결 안 함** — Intent enum/InputMap 에도 넣지 않는다 |
| 블렌딩 | **접점/API 시그니처만 합의**. 실제 블렌딩 구현은 본 단계 비범위 |

### 1.4 이번 단계의 "성공" 정의

- 단계 2.5 의 임시 블록 2개(`CPlayer::Update` WASD 직결 + `Body_Player::KeyState('1','2')`)를 **Intent 레이어 경유로 전부 이관 완료**
- Dash/BackDash 가 키 조합(`W+SPACE`, `S+SPACE`)으로 동작
- 가드/공격/콤보가 마우스 입력으로 동작 (실 히트 판정은 단계 5 대기)
- 무기 스왑(`C`) 시 CurrentWeaponState 와 PendingWeaponState 분리 동작 확인
- **Body_Player 의 AutoReturn 경로 제거** 및 StateMachine 으로 이관 완료 (§3.3). IDLE 복귀 하드컷 자체는 본 단계 종료 시점에도 **허용됨 (§1.2)**. `fEnterBlendTime` 는 전부 0.f 유지, 실제 블렌딩 제거는 후속 단계

---

## 2. 레이어 구성

### 2.1 전체 흐름

```
CInput_Device (raw keyboard/mouse)
      │
      ▼
InputMap                         ─ 키/버튼 ↔ Intent 매핑 테이블
      │
      ▼
IntentResolver                   ─ 조합/모달/상태 기반 해석
      │                            (W+SPACE → DashForward 등)
      ▼
ActionPolicy / StateMachine      ─ Current / Pending / Priority 판정
      │
      ▼
CBody_Player (실행기)            ─ Play_Action(CHARACTER_ACTION)
      │
      ▼
CAnimController.Play(key)        ─ 클립 재생, 완료 통지
```

### 2.2 레이어별 책임 경계 (2026-04-22 확정)

| 레이어 | 책임 | 소유 위치 |
|---|---|---|
| InputMap | 키/버튼 → Intent 매핑 테이블 | `CPlayer` 내부 static table (하드코딩) |
| IntentResolver | 조합키/모달 해석, LookDelta/MoveVector 생산, 문맥 재해석(§4.4) | `CIntentResolver` (CPlayer 멤버, CBase 비상속) |
| ActionPolicy 커널 | Priority/Reject/Cooldown 가드, Pending/AutoReturn 처리 | `CStateMachine` (**Engine DLL**, `_uint` opaque action ID) |
| ActionPolicy 구체 | CHARACTER_ACTION 매핑, Player 고유 상태(RunGrace, WeaponTransition) | `CPlayer_StateMachine : public CStateMachine` (Client) |
| 실행기 | 결정된 Action 을 AnimController 에 흘림 | `CBody_Player::Play_Action` (기존 유지) |

**§2 결정사항 (2026-04-22 합의)**

| 항목 | 결정 |
|---|---|
| InputMap | **하드코딩** (C++ 정적 테이블). 외부 데이터화는 키 리바인딩 UI 필요 시점에 이관 |
| IntentResolver | **별 클래스 `CIntentResolver` 로 분리** — RMB-hold / 콤보 윈도우 / chord 감지가 프레임 간 상태를 가짐. **CBase 파생** (repo 컨벤션 일관성), `Create()`/`Safe_Release` 패턴. CPlayer 가 단독 소유 |
| StateMachine | **Engine `CStateMachine` (CBase 파생, ENGINE_DLL) + Client `CPlayer_StateMachine : public CStateMachine`** 2-계층 구조. Engine 커널은 `_uint` opaque action ID 로만 동작, Client 파생이 `CHARACTER_ACTION` 매핑과 Player 고유 상태(RunGrace, WeaponTransition) 소유. 단계 7 Monster 는 `CMonster_StateMachine` 으로 같은 베이스 상속 |
| StateMachine 소유/생명주기 | `CPlayer_StateMachine` 은 CBase 파생 → `Safe_Release` 사용 (AnimController 와 동일 패턴) |

**StateMachine 설계 근거**:
- `CAnimController` 가 이미 "Engine 은 `_uint64` key 로만 동작, Action 의미는 Client 가 소유" 원칙으로 구현됨 → StateMachine 이 같은 원칙 확장
- CLAUDE.md 의 SSOT / Engine ↔ Client 격리 원칙과 일관
- Monster 재사용(단계 7) 시 별도 추출 리팩터링 없이 같은 베이스 상속 가능

**Engine 측 책임 (`CStateMachine`)**:
- `_uint` 형 opaque action ID 단위 상태머신 커널
- Priority / Reject 리스트 / Cooldown 가드
- Pending 큐, AutoReturn 처리
- `Try_Transition(iNext)`, `Update(fTimeDelta)`, `On_ActionFinished()` 공용 API
- 가상 훅: `virtual void On_Transition(iFrom, iTo)` — 파생 클래스 확장 포인트

**Client 측 책임 (`CPlayer_StateMachine`)**:
- `CHARACTER_ACTION` enum ↔ `_uint` ID 매핑 (주로 `static_cast` 수준)
- `CharacterAnimTable` 순회하여 `Register_Policy` / `Register_Reject` 호출
- Player 고유 확장: `m_fRunGraceTimer`, `m_WeaponTransition`
- `On_Transition` 오버라이드로 DASH 완료 시 RunGrace 시작 등

### 2.3 Intent 의 최소 세트 (초안)

- `Intent::MoveVector` — 2D (forward/right 성분)
- `Intent::LookDelta` — Mouse X/Y
- `Intent::DashForward` / `Intent::DashBackward`
- `Intent::BasicAttackChain` — 연속 입력 윈도우 포함
- `Intent::LinkAttack` — DASH/BACK_DASH 중 LMB (§4.4.2 문맥 재해석)
- `Intent::ChainSmash` — LINK_ATTACK 중 Notify 윈도우 내 LMB
- `Intent::GuardHold` — 누르고 있는 동안 유지
- `Intent::WeaponSwap` — `C`
- (상호작용 `G` / 스킬 Intent 는 이번 범위 아님, §1.2)

**원칙**: Intent 는 **단계 3 범위에서 실제로 소비되는 것만** 정의한다. 스킬 Q/E/R/F/V/T/LEFT SHIFT/G/Z 등의 Intent 는 인프라가 수용할 수 있게 열어두되 초기 enum 에는 넣지 않는다 (§1.2 / §1.3 일관).

---

## 3. CPlayer 책임 경계

### 3.1 CPlayer 가 소유하는 것

- 현재 입력 상태 (KeyState 조회는 CPlayer 에서만)
- `Intent` 생산 (IntentResolver)
- Current / Pending Action, Current / Pending WeaponState
- 상태머신 전이 판정 (Priority 가드, AutoReturn 조건)
- 이동 정책 실행 (In-place 이동 시 Transform 직접 조작, Root Motion 시 Body 델타 수용)

### 3.2 CBody_Player 가 소유하는 것 (Step 2.5 자산 유지 + §3.3 책임 재분배)

- AnimTable 바인딩, Clip 등록
- `Play_Action(CHARACTER_ACTION)` 실행 — StateMachine 의 명령을 받아 AnimController.Play 호출
- `AnimController::Update` 구동 + Notify 이벤트 수집
- **완료/Notify 이벤트를 리스너(`IActionNotifyListener`)로 발화** — AutoReturn 결정은 **하지 않음** (§3.3 에서 StateMachine 으로 이관)
- Root Motion delta 제공 (`Get_LastRootMotionDelta`)
- Editor 프리뷰 모드 (기존, §2.9)

### 3.3 책임 경계 결정 (2026-04-22)

- **AutoReturn 주체**: **StateMachine 으로 이관 (B)**. `Body_Player::Update` 의 AutoReturn 블록 제거. Body 는 `ActionFinished` 콜백만 발화, StateMachine 이 Pending 큐 → AutoReturn 정책 → IDLE 순으로 다음 Action 결정
- **완료 신호 전달**: **이벤트/콜백 (B-2)**. `IActionNotifyListener` 인터페이스로 `ActionFinished` + AnimNotify 이벤트를 동일 경로로 수신 (§4.3 참조)
- **재진입 가드**: Body 는 콜백을 **`AnimController::Update` 직후 한 곳에서만** 발화. 콜백 안에서 `Play_Action` 을 다시 호출해도 Body Update 중간 상태가 깨지지 않도록 단일 경로 유지
- **이동 시 Action 선택**: StateMachine 이 `MoveVector` 크기 기반으로 WALK/RUN 결정. CPlayer 는 MoveVector 만 채워주고 StateMachine 이 Action 판정

---

## 4. 상태머신 설계

### 4.1 기본 골격

- **Current Action**: 지금 재생 중인 Action
- **Pending Action**: 다음 프레임에 전이될 Action (콤보 윈도우 입력, 버퍼링)
- **Priority 가드**: 더 높은 우선도 Action 이 들어오면 현재 Action 을 중단하고 전이
- **AutoReturn**: Current Action 종료 시 지정된 ReturnAction 으로 복귀 (기존 `CHARACTER_ACTION_POLICY::bAutoReturn` 재사용)

### 4.2 Priority 등급 + 예외 규칙 (2026-04-22 확정)

**근거**: 실제 게임 거동(Solo Leveling) 조사 결과 Priority 단순 비교로는 표현 못 하는 케이스(LINK_ATTACK, DASH→GUARD 거부, SPACE 문맥 해석 등) 가 다수 존재. **Priority(베이스) + 예외 규칙(오버라이드) + Intent 문맥 재해석(§4.4)** 3층 구조로 구성.

#### 4.2.1 Priority 표

구간 체계 (0~1000). 큰 간격으로 중간 삽입 여유 확보. 실제 `CharacterAnimTable.cpp::s_SungJinWooERankPolicies` 와 일치.

| Priority | Actions | 주석 |
|---|---|---|
| **0** | IDLE, WALK, RUN | 기본 이동. 동일 등급 자유 전이 (§7.4 Walk/Run Grace 로직) |
| **400** | GUARD_START, GUARD_LOOP, GUARD_END | 가드 계열 |
| **500** | DASH, BACK_DASH | |
| **550** | PARRY_WEAK, PARRY_STRONG | |
| **600** | BASIC_ATTACK_01/02/03, **LINK_ATTACK**, **CHAIN_SMASH** | 일반 공격/콤보. 동일 등급 전이는 AnimNotify `ComboWindow_Open/Close` 구간에서만 |
| **650** | **CORE_ATTACK_01** | 무기 스왑 시그니처 공격. WEAPON_SWAP Notify 로 자동 전이(§5.2.2) |
| **700** | SKILL_01, SKILL_02 | 스킬 (단계 3 비범위, 테이블만) |
| **750** | U_SKILL | 궁극기 (단계 3 비범위) |
| **850** | STUN | 향후 (피격 도입 시) |
| **900** | HIT | 향후 |
| **1000** | DEATH | 최상위. 어떤 상태에서도 전이, 빠져나올 수 없음 |

**미배치 Action** (테이블 추가 대기):
- `WEAPON_SWAP` — Step D 진입 시 650~700 구간에서 결정
- `BREAKFALL` — 현재 테이블에 없음. 피격 시스템과 함께 결정

**핵심 규칙**: `IDLE/WALK/RUN` 은 **모두 Priority 0** — WALK→IDLE 자유 전이를 위해 동일 등급 유지. 이동 간 우선순위 구분은 Priority 가 아닌 StateMachine 의 이동 정책 로직(§7.4 Walk/Run Grace)이 담당.

#### 4.2.2 기본 전이 규칙

- 현재 Action 의 Priority 보다 **높거나 같은** Action 만 전이 허용
- 같은 등급 내부는 AutoReturn 또는 AnimNotify 윈도우에서만 전이
- 낮은 등급 입력은 Pending 큐로도 받지 않는다 (무시)

#### 4.2.3 예외 규칙 (Priority 보다 우선 적용)

**거부(Reject) 리스트** — Priority 상 허용되지만 예외로 거부:

| 전이 | 근거 |
|---|---|
| DASH → GUARD_START | 게임 거동: DASH/BACK_DASH 는 GUARD 캔슬 불가 |
| BACK_DASH → GUARD_START | 동일 |

**조건부 치환(Rewrite) 리스트** — 같은 입력이 문맥에 따라 다른 Action 으로 해석:

| 현재 Action | 입력 | 치환 결과 |
|---|---|---|
| DASH, BACK_DASH | LMB (`BasicAttack` Intent) | → **LINK_ATTACK** (일반 콤보 아님) |
| LINK_ATTACK | LMB (ComboWindow 내) | → **CHAIN_SMASH** |

**허용 보강** — 실제 게임 거동에 맞춰 기본 규칙 외 명시적 허용:

| 전이 | 근거 |
|---|---|
| 모든 Action (1~3) → GUARD_START | 게임 거동: 모든 공격/스킬은 GUARD 캔슬 가능 (Priority 규칙으로 자동 성립) |
| 모든 Action → LINK_ATTACK | DASH/BACK_DASH 중 LMB 입력 시. Priority 치환 경로 |

#### 4.2.4 신규 Action 추가

`CHARACTER_ACTION` enum 에 다음 추가:

- `LINK_ATTACK` — DASH/BACK_DASH 중 LMB 입력으로 발동하는 돌진 공격
- `CHAIN_SMASH` — LINK_ATTACK 중 Notify 윈도우에서 LMB 입력으로 발동하는 후속 공격
- `WEAPON_SWAP` — `C` 입력으로 발동하는 무기 스왑 (§5.2)
- `CORE_ATTACK_01` — WEAPON_SWAP 애니메이션 Notify 시점에 **자동 전이** 되는 목표 무기 기준 시그니처 공격 (§5.2.2)

각각 `CharacterAnimTable` 에 애니메이션 매핑 엔트리 추가 필요 (무기별 구분 포함).

#### 4.2.5 구현 힌트

- `CHARACTER_ACTION_POLICY` (Client) 에 현재 `_uint iPriority` 존재. `_float fCooldown` / `_float fEnterBlendTime` 필드는 **Step D 에서 추가** (현재 Initialize 시 0 하드코딩)
- Engine `ACTION_POLICY_BASE` 는 `iAction / iPriority / bAutoReturn / iReturnAction / fCooldown / fEnterBlendTime` 전부 보유
- Client Policy → Base 변환 후 `CStateMachine::Register_Policy` 호출 (`CPlayer_StateMachine::Initialize` 참조)
- Engine `CStateMachine::Try_Transition(_uint iNext)` 흐름 (공용 커널, 현재 구현):
  1. **등록 검사** → `m_Policies.find(iNext)` 실패 시 거부
  2. **Current 없음 경로** → 즉시 전이 (초기 상태만 해당. Reject/Priority 검사 생략)
  3. **[동일 Action no-op 가드]** → Step A 마감 버그 수정으로 추가 예정
  4. **Reject 리스트 검사** → 해당 시 거부
  5. **Priority 비교** → `Get_Priority(iCurrent) <= Get_Priority(iNext)` 검사
  6. **Cooldown 검사** → `m_CooldownRemaining[iNext] > 0` 이면 거부 (§5.5)
  7. **전이 수행** → `m_iCurrentAction = iNext`, Pending 플래그 클리어, 쿨다운 세팅, `On_Transition(iFrom, iNext, bInitial)` 훅
  - **Pending 큐 적립 경로는 아직 없음** — Step C 콤보 진입 시 `Enqueue_Pending` API 추가 예정
- **Rewrite 리스트는 StateMachine 이 아닌 `CIntentResolver` 가 담당** (§4.4)
- Reject 리스트 등록 위치: `CPlayer_StateMachine::Initialize` 에서 `Register_Reject(DASH, GUARD_START)` 식으로. Step C 에서 GUARD 도입 시 등록

### 4.3 콤보 윈도우 & AnimNotify (2026-04-22 결정)

**결정: 단계 3 에 AnimNotify 최소 인프라 포함 (후보 B)**

- BASIC_ATTACK 01/02/03 은 **`ComboWindow_Open` / `ComboWindow_Close` Notify 구간**에서만 다음 단계 Pending 허용
- 윈도우 밖 입력은 버려진다 (버퍼링은 UX 튜닝 후순위)

#### 4.3.1 AnimNotify 설계 결정 (N-1 ~ N-4)

| 항목 | 결정 |
|---|---|
| **N-1. 데이터 저장** | **SLMD v3** — `.bin` per-animation 에 notify 배열 통합. v2 파일 로드 시 빈 배열 폴백 (기존 .bin 재변환 불필요) |
| **N-2. Editor UI** | **최소형** — Inspector 에 현재 애니메이션 Notify 리스트 + "Add Notify" 버튼. `{ 이름, 키프레임 비율 }` 한 줄. 타임라인 시각화는 후순위 |
| **N-3. 초기 이벤트 타입** | `ComboWindow_Open`, `ComboWindow_Close`, **`CoreAttack_Trigger`** (무기 스왑 CoreAttack 즉시 발동, §5.2.2) 총 3개 실제 소비. 나머지 이름 규약(`HitBox_On/Off`, `HitStop_Begin/End`, `Footstep`, `SFX_Play`, `VFX_Spawn`, `Weapon_Toggle_Visible`, `Invuln_Begin/End`, `Slowmo_Begin/End`)은 문서 규약으로만 확보, 소비자 연결은 후속 단계 |
| **N-4. 디스패처 위치** | **`CAnimController::Update` 내부** — 현재 재생 중인 Animation 의 notify 를 시간 경과에 따라 발화. Body 는 리스너 등록/해제만 |

#### 4.3.2 확장성 담보 원칙

- **이름(string) 기반 디스패치** — enum 하드코딩 금지. 신규 Notify 추가 시 Engine 재빌드 불필요
- **Notify 페이로드에 인자 슬롯 예약** — `{ name, ratio, param_int, param_float, param_string }`. 이번 단계는 param 빈 값이지만 포맷에 필드 확보 (예: `Footstep` L/R 구분, `VFX_Spawn` 소켓명)
- **리스너 이름 기반 구독** — 브로드캐스트(모든 리스너에 모든 Notify 전달) 대신 `Subscribe_Notify("ComboWindow_Open", pListener)` 형태. 신규 소비자가 관심 이름만 붙이면 됨

#### 4.3.3 미래 소비자 매트릭스 (문서 규약, 단계 3 비범위)

| Notify 이름 | 소비자 | 도입 단계 |
|---|---|---|
| `HitBox_On` / `HitBox_Off` | Collider | 단계 5 |
| `HitStop_Begin` / `HitStop_End` | TimeScale | 전투 시스템 |
| `Footstep` | Sound | 사운드 단계 |
| `SFX_Play` | Sound | 사운드 단계 |
| `VFX_Spawn` | 파티클/이펙트 | 이펙트 단계 |
| `Weapon_Toggle_Visible` | Body/Weapon 가시성 | 무기 스왑 연출 확장 |
| `Invuln_Begin` / `Invuln_End` | Player 무적 플래그 | 전투 시스템 |
| `Slowmo_Begin` / `Slowmo_End` | TimeScale | 패리/궁극기 연출 |

### 4.4 Intent 문맥 재해석 (IntentResolver 책임)

`CIntentResolver` 가 매 프레임 `CurrentAction` 을 참조해서 동일 raw 키 입력을 다른 Intent 로 해석. 이 문맥 해석이 **IntentResolver 를 별 클래스로 분리한(§2 결정 C) 주된 근거**.

#### 4.4.1 SPACE 문맥 해석

```
if (CurrentAction ∈ {BASIC_ATTACK_01~03, LINK_ATTACK, CHAIN_SMASH}):
    → Intent::DashBackward                      // 공격 중 SPACE = 자동 백대시
elif (CurrentAction ∈ {IDLE, WALK, RUN}):
    if (MoveVector.forward < 0):  → Intent::DashBackward
    else:                         → Intent::DashForward
else:
    → 무시 (GUARD/DASH/HIT 중 SPACE 는 무효)
```

#### 4.4.2 LMB 문맥 해석

```
if (CurrentAction ∈ {DASH, BACK_DASH}):
    → Intent::LinkAttack                        // DASH 중 LMB = LINK_ATTACK
elif (CurrentAction == LINK_ATTACK) and (Notify ComboWindow 열림):
    → Intent::ChainSmash
elif (CurrentAction ∈ {BASIC_ATTACK_01~03}) and (Notify ComboWindow 열림):
    → Intent::BasicAttackChain                  // 다음 콤보 단계
else:
    → Intent::BasicAttackChain                  // 새 콤보 시작 (_01)
```

#### 4.4.3 미래 확장 훅 (단계 3 비범위)

`CPlayer` 에 플래그 멤버만 선언해 두고, 세팅/소비 로직은 후속 단계에서:

| 플래그 | 세팅 조건 | 소비 단계 |
|---|---|---|
| `m_bShadowStepAvailable` | Perfect Dodge 성공 (DASH/BACK_DASH 가 적 공격 타이밍 일치) | 단계 7 Monster 공격 타이밍 감지 필요 |
| `m_bParrySkillEnabled` | PARRY 성공 (GUARD 가 적 공격 타이밍 일치) | 단계 7 + QTE 스킬 인프라 |

**원칙**: 단계 3 에선 훅만 확보, 실제 발화는 적(단계 7) + 스킬 인프라가 갖춰진 후.

---

## 5. Current / Pending WeaponState

### 5.1 모델

- `CurrentWeaponState`: 현재 활성 무기 (공격/스킬 판정 기준)
- `PendingWeaponState`: 스왑 요청 후 전환 중 목표 무기
- 기본 슬롯: `COMMON` / `DUAL_BLADE` (현재 `CHARACTER_WEAPON_STATE` enum 을 기준으로 확장)

### 5.2 무기 스왑 (`C`) 해석 및 Action 구조 (2026-04-22 확정)

#### 5.2.1 Action enum 결정: `WEAPON_SWAP` + `CORE_ATTACK_01` 2종 추가

**결정 (v2 §4.1 원문 준수)**:
- **`CHARACTER_ACTION::WEAPON_SWAP`** — 단일 Action. 목표 무기 정보는 `m_WeaponTransition.eTo` 로 보유. 목표 무기별 클립은 `Resolve_ActionKey(WEAPON_SWAP, eTo)` 로 선택
- **`CHARACTER_ACTION::CORE_ATTACK_01`** — WEAPON_SWAP 애니메이션 중 `AnimNotify_CoreAttack_Trigger` 키프레임 발화 시점에 **자동 전이** 되는 새 Action. LMB 입력 불필요. 목표 무기 기준 클립을 `Resolve_ActionKey(CORE_ATTACK_01, eTo)` 로 선택

**BasicAttack 과 분리 이유**:
- 이름/의미 구분 명확 — "CoreAttack" 은 무기 스왑 직후의 시그니처 공격 (v2 §4.1 용어). BasicAttackChain 과 동일 Intent 로 묶으면 의미 혼동
- Priority/Policy 를 별도로 조정 가능해짐 (예: CORE_ATTACK 은 콤보 윈도우 없이 단발)
- 단계 3 범위에선 `CORE_ATTACK_01` 1종. 후속에서 `_02` / `_03` 확장은 필요 시

#### 5.2.2 상태 전이 다이어그램

```
이벤트                            상태 변화
──────────────────────────────────────────────────────────────
현재 활성 무기 = A
C 입력                           m_WeaponTransition = { eFrom=A, eTo=B, bActive=true }
                                 Try_Transition(WEAPON_SWAP)
                                 Play_Action(WEAPON_SWAP)
                                 → Resolve_ActionKey(WEAPON_SWAP, eTo=B) 로 클립 선택
                                 ▼
(전환 액션 재생 중)               CurrentWeaponState 는 아직 A
                                 ▼
Notify "CoreAttack_Trigger"      StateMachine 이 수신 → **자동 전이** (LMB 불필요)
키프레임 도달                        Try_Transition(CORE_ATTACK_01)
                                    Play_Action(CORE_ATTACK_01)
                                    → Resolve_ActionKey(CORE_ATTACK_01, eTo=B) 로 클립 선택
                                 ▼
CORE_ATTACK_01 ActionFinished   CurrentWeaponState := B
                                 m_WeaponTransition.bActive = false
                                 WEAPON_SWAP 쿨다운 시작 (§5.5, POLICY.fCooldown)
                                 AutoReturn → IDLE
```

**핵심 흐름**: `C` 한 번만 눌러도 자동으로 `WEAPON_SWAP → CORE_ATTACK_01 → IDLE` 이 이어짐. LMB 는 필요 없음. 이는 v2 §4.1 "C 입력 → 전환 액션 → B 무기 기준 CoreAttack 이 **즉시** 발동" 원문과 일치.

#### 5.2.3 CharacterAnimTable 확장 필요

```
{ WEAPON_SWAP,    COMMON,     "Anim_WeaponSwap_ToCommon"     }   // Dual → Common
{ WEAPON_SWAP,    DUAL_BLADE, "Anim_WeaponSwap_ToDualBlade"  }   // Common → Dual
{ CORE_ATTACK_01, COMMON,     "Anim_CoreAttack_Common_01"    }
{ CORE_ATTACK_01, DUAL_BLADE, "Anim_CoreAttack_Dual_01"      }
```

실제 클립 이름은 `.bin` 확인 후 확정 (§13.3). 없으면 임시 대체 + TODO 주석.

`WEAPON_SWAP` 애니메이션에 `AnimNotify_CoreAttack_Trigger` 키프레임을 Editor Inspector 에서 추가 (§4.3 N-2 최소 UI). 일반적으로 스왑 애니 후반 60~80% 지점.

**핵심**: `C` 는 "현재 무기 기준 액션"이 아니라 **목표 무기 기준 클립 선택**. `Resolve_ActionKey` 호출 시 WeaponState 인자를 `Current` 가 아닌 `m_WeaponTransition.eTo` 로 넘기는 경로가 관건.

### 5.3 비주얼 표현과의 분리

- 무기 메쉬 토글 타이밍은 **논리 상태 전이와 독립** (AnimNotify 또는 전환 액션의 특정 비율 시점)
- HUD 스킬 UI 변경은 후속 (단계 6)
- 본 단계에서는 **논리 상태 전이 파이프라인만** 구축

### 5.4 WeaponState 구조 결정 (2026-04-22)

**결정: `WEAPON_TRANSITION` 구조체 + 취소 불가 (쿨다운으로 자연스럽게 강제)**

```cpp
struct WEAPON_TRANSITION
{
    CHARACTER_WEAPON_STATE  eFrom;
    CHARACTER_WEAPON_STATE  eTo;
    _bool                   bActive;      // 전환 중 여부
    // (옵션) _float fProgress — AnimNotify 로 충분하면 생략
};

class CPlayer
{
    CHARACTER_WEAPON_STATE  m_eCurrentWeaponState = COMMON;
    WEAPON_TRANSITION       m_WeaponTransition{};  // bActive=false 면 전환 없음
};
```

**근거**:
- 전환 상태는 From/To/Active 가 한 묶음으로 의미를 가짐
- 별 필드(A) 의 "Pending == NONE 이면 전환 아님" 식 sentinel 패턴은 "무기 없음" 의미와 혼동 여지
- 향후 "취소 가능 구간", "전환 불가 상태" 같은 속성을 구조체에 추가만 하면 확장 쉬움

**전환 중 재입력 정책: 취소 불가**
- 쿨다운(§5.5)이 재스왑을 원천 차단 → "취소 불가 vs 역전환" 논의는 실무적으로 해소
- Priority 검사 + 쿨다운 검사 이중 가드로 자연 거부

### 5.5 쿨다운 인프라 (2026-04-22 결정, 구현은 Step D)

**범위 분리**
- **로직 (타이머 + 전이 가드)**: 단계 3 포함 (Step D). 무기 스왑 `C` 가 실제 동작하려면 필요
- **UI (HUD 쿨다운 시각화)**: 단계 6 (§1.2)
- **스킬별 쿨다운 값/매핑**: 스킬 인프라 단계에서 연결. 본 단계 실사용은 WEAPON_SWAP 1건

**현재 진행 상태 (2026-04-23)**:
- Engine `ACTION_POLICY_BASE` 에 `fCooldown` 필드 **있음** ✅
- `CStateMachine::Try_Transition` 쿨다운 가드 코드 **있음** ✅ (POLICY.fCooldown > 0 이면 설정)
- Client `CHARACTER_ACTION_POLICY` 에 `fCooldown` 필드 **없음** ❌ — `CPlayer_StateMachine::Initialize` 에서 0 하드코딩 중
- 즉 **로직 골격은 준비됐으나 값을 채우는 경로가 막혀있음**. Step D 에서 Client POLICY 에 필드 추가 + CharacterAnimTable 에 WEAPON_SWAP 쿨다운 값 기입 시 활성화

**저장/관리: StateMachine 내부 map (권장안 A)**

```cpp
class CPlayer_StateMachine
{
    unordered_map<CHARACTER_ACTION, _float> m_CooldownRemaining;
    // Update 에서 전체 감소, Try_Transition 에서 검사
};
```

- 단계 3 실사용 1건이라 별 클래스(`CCooldown_Manager`)는 과투자
- 스킬 다수 도입 시점에 별 클래스로 승격 리팩터링

**쿨다운 값 저장 위치: `CHARACTER_ACTION_POLICY::fCooldown`**

```cpp
struct CHARACTER_ACTION_POLICY
{
    CHARACTER_ACTION  eAction;
    _uint             iPriority;
    _bool             bAutoReturn;
    CHARACTER_ACTION  eReturnAction;
    _float            fCooldown;       // 신규 (기본 0.f = 쿨다운 없음)
};
```

- SSOT: Action 별 쿨다운 값은 `CharacterAnimTable` 이 보유. 런타임 잔여 시간은 StateMachine

**전이 체크 순서 (§4.2.5 확장)**

```
Try_Transition(eNext):
  1. Reject 리스트 검사
  2. Priority 비교
  3. Cooldown 검사         ← 신규: m_CooldownRemaining[eNext] > 0 이면 거부
  4. Pending 적립 또는 즉시 전이
  5. 전이 성공 시 → m_CooldownRemaining[eNext] = policy.fCooldown
```

**쿨다운 중 재입력 처리: 무시 (Ignore)**
- 콤보 Pending 과 쿨다운 대기 Pending 의 의미 충돌 방지 (콤보는 "곧 소비", 쿨다운은 "몇 초 후 소비")
- 필요 시 후속 단계에서 Pending 큐잉으로 확장

---

## 6. 콤보 시스템

### 6.1 기본 동작

- `MOUSE_LB` 입력 → `BASIC_ATTACK_01` 재생
- 재생 중 콤보 윈도우 안에서 `MOUSE_LB` 추가 입력 → `BASIC_ATTACK_02` Pending
- 현재 Action 종료 시 Pending 소비 → `BASIC_ATTACK_02` 재생
- 최대 `_03` 까지. 이후는 AutoReturn 으로 IDLE 복귀

### 6.2 WeaponState 와의 상호작용

- AnimTable 의 `Resolve_ActionKey(BASIC_ATTACK_01, CurrentWeaponState)` 로 키 결정
- 같은 `BASIC_ATTACK_01` Intent 도 WeaponState 가 다르면 다른 Clip 재생 (v2 예시: `Fighter_Attack_01` vs `Dualwield_Attack_01`)
- 이 부분은 이미 `CharacterAnimTable + CAnimController` 레벨에서 지원됨 — CPlayer 는 단순히 Action + WeaponState 쌍을 넘기기만 하면 됨

### 6.3 결정 대기

- 콤보 윈도우 구현 방식 (§4.3 후보 A/B)
- 콤보 중 이동 입력 해석 (제자리 콤보 / 이동 중단 / 경량 이동 허용)

---

## 7. 이동 정책

### 7.1 두 가지 이동 소스

| 소스 | 설명 | 대표 Action |
|---|---|---|
| **In-place** | Transform 을 직접 이동. 애니메이션은 제자리 재생 | WALK, RUN |
| **Root Motion** | 애니메이션 루트 본 delta 를 Transform 에 누적. 키 입력은 **방향 결정에만** 관여 | DASH, BACK_DASH, 공격 일부 |

### 7.2 카메라 기준 방향 해석 — 부드러운 회전 이동 (비락온)

**2026-04-24 개정** — 초기 스트레이프형 계획을 폐기하고 **Style C (부드러운 회전 이동)** 으로 확정. v2 §4.1 비락온 이동 정책과 일관.

```
Mouse X Delta → 카메라 Yaw 회전          (소비자: 추적 카메라. Player Yaw 에 직결되지 않음)
W/A/S/D       → Intent::vMoveDirWorld    (카메라 Yaw 기준 월드 방향벡터, 정규화)
               → Player 목표 Yaw 계산     (atan2(dir.x, dir.z) 형태)
               → 현 Yaw 에서 목표 Yaw 쪽으로 회전 속도 한계 내 보간 (SlerpLimited 또는 각도 차 클램프)
               → 동시에 현 facing 방향으로 전진 이동 (회전이 끝나기를 기다리지 않음)
```

- 카메라는 Player 등 뒤 상단에 추적 배치. 카메라 Yaw 는 Mouse X 가 직접 제어
- **WASD 는 Player 로컬 축이 아닌 카메라 Yaw 기준 월드축** 으로 해석. 예: 카메라가 북쪽을 바라보면 `W`=북쪽, `A`=서쪽. Player 가 남쪽을 보고 있었다면 180° 회전하며 북쪽으로 달림
- **스트레이프 없음** (비락온). A/D 도 "왼쪽/오른쪽으로 회전하며 전진"
- 조합 입력(W+A 등)은 두 방향 벡터의 합 정규화
- 회전 속도 제한: `CPlayer::m_fRotationPerSec` (Initialize 에서 180°/s 디폴트) 재사용. 매 프레임 `min(회전_필요량, m_fRotationPerSec * fTimeDelta)` 만큼만 회전
- 락온 상태의 스트레이프 정책은 **이번 범위 제외** (단계 3 이후 락온 도입 시 별도 설계)

### 7.3 Root Motion 과 WASD 의 충돌

- Dash 중에는 WASD In-place 이동 블록 → Root Motion 만 반영
- Dash 종료 후 후속 이동은 §7.4 Walk/Run 전이 모델로 처리

### 7.4 Walk/Run 전이 모델 (2026-04-24 갱신)

**근거**: 실제 게임(Solo Leveling) 거동 조사 결과 — 기본 이동은 WALK, RUN 은 DASH 의 **모멘텀 보상 상태**. Shift 는 QTE 스킬 키로 예약되어 있어 Walk/Run 토글 modifier 로 사용 불가.

**2026-04-24 변경**: Grace window 제거 → DASH 종료 시점 WASD 홀드 여부로 즉시 결정. Dash 차지 시스템 도입 (§7.5 신설).

#### 7.4.1 상태 전이 다이어그램

```
IDLE ──(WASD 누름)──────→ WALK
WALK ──(WASD 뗌)────────→ IDLE
WALK/IDLE ──(SPACE, 차지 소비)─→ DASH / BACK_DASH

DASH/BACK_DASH 종료 시점:
  WASD 홀드 중? → RUN 즉시 승격
  WASD 안 눌림  → IDLE (AutoReturn)

RUN 상태:
  WASD 유지          → RUN 유지
  WASD 뗌            → IDLE (다시 달리려면 DASH 재입력 필요)
  WASD 방향 급변경    → RUN 유지 (방향만 변경)
  SPACE              → DASH (차지 소비 시)
```

#### 7.4.2 세부 결정사항

| 항목 | 결정 |
|---|---|
| **R-1. RUN 승격 조건** | **DASH 종료 시점 WASD 홀드 → 즉시 RUN**. Grace 타이머 없음. WASD 뗀 채 종료하면 IDLE |
| **R-2. RUN 중 WASD 뗌** | **IDLE 로 복귀** — 다시 달리려면 DASH 재사용 필요. DASH 의 가치 유지 |
| **R-3. RUN 중 방향 급변경** | **RUN 유지** — 방향만 변경, 상태는 불변 |
| Shift 키 | **QTE 스킬 예약** 유지 (단계 3 비범위). Walk/Run 토글로 사용하지 않음 |

#### 7.4.3 구현 힌트

- `CPlayer_StateMachine` 에 `_bool m_bLastHasMoveIntent` 캐시 (OnNotify 시점에 WASD 홀드 여부 판단용)
- `Update_LocoMotion` 진입 시 `m_bLastHasMoveIntent = Intent.bHasMoveIntent` 갱신
- `OnNotify(ACTION_FINISHED)` 에서 payload 가 DASH/BACK_DASH 일 때:
  1. `__super::On_ActionFinished()` — AutoReturn 으로 IDLE 전환
  2. `m_bLastHasMoveIntent == true` 면 `Try_Transition(RUN)` — IDLE/RUN 동일 priority 0 이라 통과
- 이동 속도: `_float m_fWalkSpeed`, `_float m_fRunSpeed` 분리 (Step E 에서 적용). DASH 는 Root Motion 이므로 속도 필드 없음

### 7.5 Dash 차지 시스템 (2026-04-24 신설)

**개념**: DASH/BACK_DASH 는 **공유 차지** 로 제한. 시간 기반 재충전.

#### 7.5.1 결정사항

| 항목 | 결정 |
|---|---|
| 최대 차지 수 | **3** |
| 재충전 인터벌 | **3.0s** (튜닝값) |
| 재충전 방식 | **항상 일정 주기** — 풀 차지여도 타이머 진행, 1 소비 시 즉시 다음 +1 까지의 대기 단축 |
| 공유 범위 | DASH / BACK_DASH **공유 풀** |
| 연타 | **허용** — 차지 남아있으면 DASH/BACK_DASH 도중 SPACE 재입력으로 즉시 재진입 |
| 소유 위치 | **`CPlayer`** — UI/HUD 의 차지 게이지 표시 결합도 최소화 |

#### 7.5.2 구현 힌트

- `CPlayer`: `_int m_iDashChargeMax{3}`, `_int m_iDashChargeCurrent{3}`, `_float m_fDashRegenInterval{3.f}`, `_float m_fDashRegenTimer{0.f}`
- `CPlayer::Tick_DashRegen(fTimeDelta)` — 매 프레임 호출, `while (timer >= interval) { timer -= interval; if (current < max) ++current; }` 패턴 (carry-over 유지)
- `CPlayer::Can_ConsumeDashCharge() / Consume_DashCharge()` 인터페이스
- `CPlayer_StateMachine::Update_LocoMotion` 의 Dash 분기에서 `Can_ConsumeDashCharge()` 체크 → `Try_Transition` 성공 시 `Consume_DashCharge()`
- HUD 는 `Get_DashCharge() / Get_DashChargeMax()` 로 조회 (Step E 이후 UI 작업 시)

#### 7.4.4 미결 — Mouse 축 감도

- Mouse X → Player Yaw (확정)
- Mouse Y 사용 여부 (카메라 Pitch 제한용). **단계 3 범위 추적 카메라 수준에서는 Mouse Y 무시해도 무방** — Pitch 제한은 §1 결정 시 "필요해지면 확장" 으로 미뤘음

---

## 8. 블렌딩 연동 (§3.1 재설계)

### 8.1 접점 API 결정 (2026-04-22)

**결정: `Play(key, fBlendTime)` 파라미터 확장 (후보 A) + `POLICY.fEnterBlendTime` 으로 값 관리**

```cpp
class CAnimController
{
    HRESULT Play(_uint64 iKey, _float fBlendTime = 0.f);   // 기본 0 = 하드컷
};

struct CHARACTER_ACTION_POLICY
{
    CHARACTER_ACTION  eAction;
    _uint             iPriority;
    _bool             bAutoReturn;
    CHARACTER_ACTION  eReturnAction;
    _float            fCooldown;
    _float            fEnterBlendTime;   // 신규. 이 Action 으로 들어올 때 블렌드 시간
};
```

**근거**:
- 단계 3 목적은 "시그니처 예약". 최소 구현량 우선 → (B) Begin_Transition / (C) 엣지 테이블은 과투자
- BlendTime 은 "전이 엣지 속성" → 호출 지점에서 함께 넘기는 게 자연스러움
- `fEnterBlendTime` 을 Action 단위로 두면 "이 Action 에 들어올 때" 라는 의미가 명확. 엣지별(from→to 페어) 블렌드가 필요해지는 시점에 (C) 테이블 방식으로 승격

### 8.2 단계 3 실제 동작

- `CAnimController::Play(key, 0.f)` 만 호출됨 → 내부는 기존 하드컷 로직 그대로
- 모든 `POLICY.fEnterBlendTime` 초기값 0.f
- IDLE 하드컷 등 증상은 단계 3 종료 시점에도 유지 (§1.2 명시)

### 8.3 이번 단계에서 결정하지 않는 것

- 실제 블렌딩 알고리즘 (Freeze A → Lerp → Play B 등)
- 비채널 본 처리 (1차 CrossFade 실패 원인 영역)
- 루트 모션과 블렌딩 구간의 위치 보간
- 엣지별(from→to 페어) 블렌드 테이블 승격

### 8.4 후속 블렌딩 단계에서 할 일

1. `POLICY.fEnterBlendTime` 값을 Action 별로 채움 (IDLE 복귀 등 주요 전이)
2. `CAnimController::Play` 내부에 Lerp 로직 추가 (Freeze A → Lerp → Play B 패턴 유력)
3. 루트 모션 + 블렌딩 구간 위치 보간 (§3.1 1차 CrossFade 실패 교훈 반영)
4. 엣지별 블렌드 시간이 필요해지면 `ANIM_TRANSITION_EDGE` 테이블 도입 (후보 C 승격)

---

## 9. 임시 블록 이관 경로

### 9.1 제거 대상

1. `Client/Private/Player.cpp` `CPlayer::Update` 의 WASD → Transform 직결 블록 (`// TEMP: ...`)
2. `Client/Private/Body_Player.cpp` `CBody_Player::Update` 의 `KeyState('1') / ('2')` 스모크 블록 (`// TEMP: Step 3...`)

### 9.2 이관 순서 (권장)

```
Step A. Intent/InputMap/IntentResolver 골격만 먼저 넣고
        기존 WASD 블록을 Intent 경유로 "형태만" 교체 (동작 동일 유지)
        ─ 회귀 없는지 확인

Step B. 상태머신 골격 도입. IDLE/WALK/RUN/DASH/BACK_DASH 만 우선.
        W+SPACE / S+SPACE 조합 Dash 를 Intent 로 해석.
        Body_Player 의 '1','2' 블록 제거 가능 시점이 여기.

Step C. 공격/가드/콤보. MOUSE_LB / MOUSE_RB Intent 추가.
        AnimNotify 결정(§4.3) 반영.

Step D. 무기 스왑 (C). Current/Pending WeaponState 분리.

Step E. 블렌딩 접점 반영 (§8).
```

### 9.3 회귀 방지

- 각 Step 완료 시 §10 검증 체크리스트 전체 재실행
- Step A 에서 "동작 동일" 을 못 지키면 이후 Step 전개가 모두 오염됨 — **Step A 의 동작 동일성이 최우선**

---

## 10. 검증 체크리스트

런타임(GameApp) 실행 후. **블록별로 해당 Step 에서 검증** (§12.1 Step 매핑 참조):

- 기본 회귀 / 카메라·이동 기본 → **Step A** (일부) + **Step E** (카메라)
- Walk/Run 전이 / Dash / 공격 중 SPACE → **Step B**
- 공격/콤보 / Guard / Notify / AnimNotify 인프라 → **Step C**
- 무기 스왑 + 쿨다운 → **Step D**
- 상태머신 / 임시 블록 이관 → **Step A~B 전반**

**기본 회귀**
- [ ] GAMEPLAY 진입 / 맵 / Player 스폰 (Step 2.5 회귀 없음)
- [ ] Camera_Free 가 Client 런타임 경로에서 제거되고 추적 카메라로 교체됨
- [ ] Player 뒤 상단 추적 카메라 동작 (§1 결정)

**카메라/이동 기본** (Style C — 부드러운 회전 이동, §7.2)
- [ ] Mouse X 로 **카메라 Yaw** 회전 (Player facing 은 직접 따라오지 않음)
- [ ] `W` 만 눌러도 Player 가 **카메라 정면** 방향으로 회전하며 전진
- [ ] `A` / `D` 가 스트레이프가 아닌 **회전하며 달림** (Player facing 이 이동 방향으로 보간 회전)
- [ ] 급 반전(`W` 유지 중 `S` 로 전환) 시 **스냅 회전 아님** — 회전 속도 한계 내 원호 궤적
- [ ] WASD 이동이 Intent 경유로 동작 (기본 상태 = WALK)

**Walk/Run 전이 (§7.4)**
- [ ] DASH 종료 후 0.3s 내 WASD 유지 → RUN 진입
- [ ] RUN 중 WASD 뗌 → IDLE (WALK 아님)
- [ ] RUN 중 방향 급변경 → RUN 유지
- [ ] Grace 창 만료 후 WASD 입력 → WALK (RUN 아님)

**Dash/콤보/LINK/CHAIN**
- [ ] `W + SPACE` → DASH, Root Motion 누적
- [ ] `S + SPACE` → BACK_DASH, Root Motion 누적
- [ ] 공격 중 SPACE → 자동 BACK_DASH (§4.4.1)
- [ ] `MOUSE_LB` 단발 → `BASIC_ATTACK_01`
- [ ] `MOUSE_LB` 콤보 윈도우 내 연속 → `_02` → `_03` (AnimNotify 기반)
- [ ] DASH/BACK_DASH 중 `MOUSE_LB` → **LINK_ATTACK** (일반 콤보 아님)
- [ ] LINK_ATTACK 중 ComboWindow 내 `MOUSE_LB` → **CHAIN_SMASH**

**Guard**
- [ ] `MOUSE_RB` 누름 유지 → `GUARD_START → LOOP`, 뗌 → `GUARD_END`
- [ ] DASH/BACK_DASH 중 `MOUSE_RB` → **거부** (§4.2.3 Reject 리스트)
- [ ] 공격 중 `MOUSE_RB` → GUARD 캔슬 허용

**무기 스왑 + 쿨다운**
- [ ] `C` → WEAPON_SWAP 액션 재생. CurrentWeaponState 는 전환 중 유지 (eTo 기준 클립 선택)
- [ ] WEAPON_SWAP 애니 내부 `CoreAttack_Trigger` Notify 시점 → **자동 CORE_ATTACK_01 전이** (LMB 입력 불필요, v2 §4.1 규약)
- [ ] CORE_ATTACK_01 재생 시 목표 무기(`eTo`) 기준 클립이 선택됨
- [ ] CORE_ATTACK_01 ActionFinished 시 `CurrentWeaponState` = eTo 로 갱신, WEAPON_SWAP 쿨다운 시작
- [ ] 쿨다운 중 `C` 재입력 → **무시** (재스왑 차단, §5.5)

**AnimNotify 인프라**
- [ ] SLMD v3 로드 (v2 파일은 빈 notify 배열로 폴백)
- [ ] Inspector 에 현재 애니메이션의 Notify 리스트 표시 + Add Notify 동작
- [ ] `ComboWindow_Open/Close` Notify 가 정상 발화 (콤보 윈도우 판정에 반영)
- [ ] `ActionFinished` 콜백이 StateMachine 에서 수신됨

**상태머신**
- [ ] 액션 종료 시 StateMachine 의 AutoReturn → IDLE 복귀 (Body_Player AutoReturn 경로 아님)
- [ ] IDLE 복귀 시 **하드컷** 유지 (블렌딩은 단계 3 비범위, `fEnterBlendTime = 0.f`)

**임시 블록 이관 확인**
- [ ] `Body_Player::Update` 의 `KeyState('1','2')` 스모크 블록 제거
- [ ] `CPlayer::Update` 의 임시 WASD → Transform 직결 블록 제거

---

## 11. 참고

| 자료 | 경로 |
|------|------|
| 상위 계획 | `명세서/통합_구현계획_v2.md` §4.1 |
| Step 2.5 런타임 테스트베드 | `명세서/4월 21일(화) 진행사항.md` |
| AnimController 완료 기록 | `명세서/4월 20일 진행 사항.md` |
| 수업 코드 9개월차 | `C:\Users\chaho\ddoichaboom\CPP_STUDY_3D\수업 파일\UnZips\9개월차\` |

---

## 12. 다음 액션 (본 문서 기준)

설계 논의 완료 (2026-04-22). 구현 진행 중 (2026-04-23 갱신 — 실제 진행률 반영하여 Step 재분할).

### 12.1 구현 순서 & 진행 상태

> Codex 슬라이싱 검토 결과 기존 Step A 가 너무 컸음. Input/Intent 경로 + StateMachine 골격 + IDLE↔WALK 까지를 Step A 로, AutoReturn 이관 + Dash/Run 을 Step B 로 재분할.

#### Step A — Input/Intent 경로 + StateMachine 골격 + IDLE↔WALK (✅ 거의 완료)

**완료**:
- ✅ `CInput_Device` Edge-trigger (`Get_KeyDown/Up`, `Get_MouseBtnDown/Up`) + prev state 버퍼
- ✅ `CGameInstance` 래퍼 확장 (동일 API 일대일 노출)
- ✅ `PLAYER_RAW_INPUT_FRAME`, `PLAYER_INTENT_FRAME` 구조체
- ✅ `CIntentResolver` (CBase 파생) — `Resolve(Raw → Intent)`, MoveAxis/LookDelta 계산
- ✅ Engine `CStateMachine` 베이스 (CBase 파생, ENGINE_DLL, abstract)
  - `Register_Policy / Register_Reject / Try_Transition / Update / On_ActionFinished`
  - `ACTION_POLICY_BASE` 구조체 (`Engine_Struct.h`)
  - PURE `On_Transition(iFrom, iTo, bInitial)` 훅
  - `m_bHasCurrentAction` / `m_bHasPendingAction` 플래그 분리
- ✅ Client `CPlayer_StateMachine : public CStateMachine` 골격
  - `Initialize(pAnimTable)` — CharacterAnimTable Policy 순회 → Register_Policy
  - `Bind_Owner(CPlayer*)` typed 방식
  - `Enter_InitialState(IDLE)`
  - `Update_LocoMotion(Intent)` — IDLE ↔ WALK 전이
  - `On_Transition` 오버라이드 → `CPlayer::Handle_ActionTransition` 호출
- ✅ `CPlayer::Update` 가 `Gather_RawInput → IntentResolver → StateMachine.Update_LocoMotion → StateMachine.Update → Apply_MoveIntent` 경로 사용
- ✅ Body_Player `KeyState('1','2')` 스모크 블록 제거
- ✅ Body_Player `Play_Action` public 노출
- ✅ `CharacterAnimTable.cpp` Policy 테이블 전체 Action 커버 (DEATH 까지)

**잔여 (Step A 마감 전에 해결)** — *2026-04-24 갱신*:
- ✅ **Priority 버그 해결됨**: `CharacterAnimTable.cpp` L79~81 IDLE/WALK/RUN 모두 priority 0 통일 완료
- ✅ **Try_Transition 동일 Action no-op 가드 구현됨**: `StateMachine.cpp` L30~31 `if (m_bHasCurrentAction && m_iCurrentAction == iNext) return true;`
- ✅ **Pending 체크 플래그화 완료**: `StateMachine.cpp::On_ActionFinished` L113 `if (m_bHasPendingAction)` 로 이미 반영
- 🟡 **Step A 런타임 검증** (3건): WASD 이동 / RMB 중 이동 가드 / Root Motion 누적 회귀 없음
- 🟡 **커밋**: `Step A: Input/Intent 경로 + StateMachine 골격 (IDLE↔WALK)`

> **실측 전 검증 경로 (2026-04-24 합의)**: 🔴 3건이 전부 코드에 이미 반영돼 있으므로 **Step A 남은 일은 런타임 GAMEPLAY 진입 → WASD/Mouse 입력 회귀 실측**뿐. 실측 통과 시 바로 Step A 커밋 → Step B 진입.

#### Step B — AutoReturn 이관 + 이동/대시 상태머신 (🔵 진행 예정)

**선행 Engine 확장**:
- `IActionNotifyListener` 인터페이스 신설 (Engine, `OnActionFinished` 만 단계 3 에서 실사용. `OnNotify(name, payload)` 는 Step C 대비 시그니처만 예약)

**작업**:
- `CBody_Player::Set_Listener(IActionNotifyListener*)` + 내부 `bFinished` 시 `listener->OnActionFinished()` 호출
- **`CBody_Player::Update` 자체 AutoReturn 블록 제거** (§3.3 B-2 결정)
- `CPlayer_StateMachine` 이 `IActionNotifyListener` 구현 → `OnActionFinished()` 내부에서 베이스 `CStateMachine::On_ActionFinished()` 호출
- `CPlayer::Ready_StateMachine` 에서 `m_pBody->Set_Listener(m_pStateMachine)` 등록
- IntentResolver 에 SPACE Edge-trigger Intent 추가 (`Get_KeyDown('SPACE')` 활용)
- DASH / BACK_DASH 전이 (§4.4.1 SPACE 문맥 재해석)
- `CPlayer_StateMachine` 에 RunGrace 타이머 + R-1/R-2/R-3 로직 (§7.4)
- `Update_LocoMotion` 확장 — IDLE/WALK/RUN/DASH/BACK_DASH 5개 처리
- Reject 리스트 등록 (현재 Step B 범위에선 아직 DASH↔GUARD 없음, Step C 에서 등록)

**검증**:
- §10 체크리스트 "이동/대시" + "Dash/콤보" 의 대시 부분 + "Walk/Run 전이" 블록 실측
- 커밋: `Step B: AutoReturn 이관 + Dash/Run + Grace 타이머`

#### Step C — 공격/가드/콤보 + AnimNotify 인프라 (⬜ 대기)

**선행 Engine 확장**:
- `CAnimController::Play(_uint64 key, _float fBlendTime = 0.f)` 시그니처 확장 (실 블렌드 로직은 0 일 때 하드컷 유지)
- SLMD v3 포맷 확장 + v2 폴백 로드 (`CModel::Load_Binary_Desc` 에서 version 분기)
- `CModel_Converter` v3 쓰기로 전환
- `CAnimController` 에 Notify 리스너 구독/해제 + Update 시 Notify 발화
- `IActionNotifyListener::OnNotify(name, payload)` 실제 구현
- Inspector 에 Notify 최소 저작 UI (§4.3 N-2)
- `CStateMachine` 에 Pending Enqueue API (콤보 윈도우 버퍼링용)

**작업**:
- `ComboWindow_Open` / `ComboWindow_Close` Notify 소비 (StateMachine)
- `BASIC_ATTACK_01 → 02 → 03` 콤보 체이닝
- `LINK_ATTACK` / `CHAIN_SMASH` (DASH+LMB, LINK+LMB)
- `GUARD_START/LOOP/END` + **Reject 리스트 (DASH→GUARD, BACK_DASH→GUARD) 등록**
- IntentResolver LMB / RMB Edge + 문맥 재해석 완성

**검증**:
- §10 체크리스트 공격/가드/콤보 블록 실측

#### Step D — 무기 스왑 + 쿨다운 (⬜ 대기)

**선행 확장**:
- `CHARACTER_ACTION_POLICY` 에 `_float fCooldown`, `_float fEnterBlendTime` 필드 추가 (현재 POLICY_BASE 에만 있고 Client POLICY 는 없음 — Initialize 시 0 하드코딩 중)
- `WEAPON_TRANSITION` 구조체 정의
- `CHARACTER_ACTION` enum: `WEAPON_SWAP`, `CORE_ATTACK_01` 추가 (이미 CharacterAnimTable 엔트리는 존재)

**작업**:
- `CPlayer_StateMachine::Begin_WeaponSwap(eTo)` API
- `m_WeaponTransition` 멤버 + `m_eCurrentWeaponState` 관리
- `CoreAttack_Trigger` Notify → 자동 `Try_Transition(CORE_ATTACK_01)`
- `StateMachine::Try_Transition` 쿨다운 가드 실활성화 (현재는 POLICY.fCooldown 이 0 이라 무효)
- `Weapon_Toggle_Visible` Notify → 무기 메쉬 가시성 토글
- CharacterAnimTable 에 Priority/Cooldown 값 채움 (`WEAPON_SWAP` 쿨다운 등)

**검증**:
- §10 체크리스트 무기 스왑 + 쿨다운 블록

#### Step E — 추적 카메라 + Camera_Free Editor 전용화 + Style C 이동 해석 (⬜ 대기)

**선행 확장**:
- Mouse X Delta 는 **카메라** 로 들어가야 함. 현재 `CPlayer::Gather_RawInput` 의 `lMouseDeltaX` 는 소비처 없음 — Step E 에서 추적 카메라가 소비
- Player 의 현 "스트레이프형 Apply_MoveIntent" 를 Style C 로 전환

**작업**:
- Client Loader/Level_GamePlay 에서 `Prototype_GameObject_Camera_Free` 제거
- 추적 카메라 신규 클래스 (가칭 `CCamera_Follow`):
  - Player Transform 포인터 구독 (또는 ParentMatrix 주입)
  - 등 뒤 고정 오프셋 (거리 + 높이)
  - Mouse X 입력으로 **카메라 Yaw** 회전 (Player 아님)
  - Mouse Y 는 단계 3 범위에선 무시 (Pitch 제한은 후속)
- Camera_Free 프로토타입은 Editor 테스트 씬에만 유지
- `PLAYER_INTENT_FRAME` 필드 교체:
  - (제거) `vMoveAxis` / `bHasMoveIntent`
  - (신설) `_float3 vMoveDirWorld` — 카메라 Yaw 기준 월드 방향벡터 (정규화, 크기 0 이면 이동 없음)
  - `bHasMoveIntent` 는 `LengthSquared(vMoveDirWorld) > 0` 로 대체
- `CIntentResolver::Resolve` 갱신:
  - 카메라 Yaw 취득 (`m_pGameInstance->Get_CamLook()` 에서 XZ 평면 투영 + 정규화 → Yaw forward)
  - WASD 입력을 (forward, right) 벡터 조합으로 → 월드 방향 생산
- `CPlayer::Apply_MoveIntent` 교체:
  - 목표 Yaw = `atan2(vMoveDirWorld.x, vMoveDirWorld.z)`
  - 현재 Yaw 와 차이를 회전 속도(`m_fRotationPerSec`) 한계 내에서 보간 (각도 차 래핑 주의: `-π ~ π`)
  - 회전 후 `Go_Straight` (Player facing 기준 전진) 또는 직접 `vMoveDirWorld * fSpeed * fTimeDelta` 로 Position 갱신 (둘 중 일관된 방법 선택)
- 이동 블록 / Dash 중 이동 가드 정책 유지 (§7.3)

**검증**:
- 카메라 시점/이동 해석 블록 (§10 기본 회귀 + 카메라/이동 기본)
- `A` 만 눌러도 Player 가 왼쪽으로 **회전하며 달림** (스트레이프 아님)
- 급 반전(`W` → `S`) 시 원호 궤적으로 회전하며 진행 (스냅 회전 아님)
- Mouse X 로 카메라 Yaw 가 돌고 Player facing 은 **입력이 있을 때만** 그쪽으로 따라옴
- 커밋: `Step E: 추적 카메라 + Style C 이동 해석`

### 12.2 각 Step 종료 기준

- Step 완료 시 §10 검증 체크리스트 해당 블록 전체 통과
- 각 Step 끝나면 **커밋 분리** — 빌드/런타임 깨진 상태 저장소 반영 금지 (§13.7)

### 12.3 남아있는 문서↔코드 불일치 (본 갱신 후)

- ✅ §4.2 Priority 값 실제 테이블 반영 (0~1000 구간)
- ✅ §2 / §13.5 IntentResolver CBase 파생으로 정정
- ✅ 클래스명 `CStateMachine` 일관화
- ⏳ `CHARACTER_ACTION_POLICY` 에 `fCooldown/fEnterBlendTime` 필드 — Step D 에서 추가 예정. 지금은 의도적 누락 상태
- ⏳ WEAPON_SWAP enum — Step D 에서 추가

---

## 13. 구현 핸드오프 메모 (Codex / 후속 세션)

### 13.1 세션 진입 시 필독 자료 (순서)

1. `CLAUDE.md` — 프로젝트 컨벤션, Gotchas 전부
2. `명세서/통합_구현계획_v2.md` §4.1 — 키 매핑 / 이동 해석 / 무기 스왑 원칙
3. **본 문서 전체** — 8블록 결정사항
4. `명세서/4월 21일(화) 진행사항.md` — Step 2.5 회귀 방지 기준
5. 최근 커밋 `7b79389` (Step 2.5 완료 상태)

### 13.2 기존 자산 — 절대 건드리지 말 것

- `CBody_Player::Begin_Preview / Restart_Preview / End_Preview` + `m_bPreviewMode` (§2.9 완료 자산)
  - Editor Inspector Animation List 경로가 이걸 사용. 제거/이름 변경 금지
- `CModel::Get_LastRootMotionDelta` + `CBody_Player::Get_LastRootMotionDelta` (preview 시 0 반환 보장)
- `CPlayer::Apply_RootMotion` (로컬→월드 회전 적용 완료)
- SLMD v2 포맷 (PreTransform 포함). v3 로 확장 시 **version 필드만 증가**, v2 파일도 여전히 로드 가능해야 함

### 13.3 현재 AnimTable 엔트리 상태 (2026-04-23 기준)

`CharacterAnimTable.cpp::s_SungJinWooERankClips` 에 등록된 현황:

**등록 완료**:
- IDLE, WALK, RUN, DASH, BACK_DASH
- BASIC_ATTACK_01/02/03 (UNARMED / FIGHTER)
- CORE_ATTACK_01 (UNARMED / FIGHTER)
- SKILL_01/02 (UNARMED / FIGHTER / ONE_HAND_SWORD)
- U_SKILL (UNARMED)
- GUARD_START/LOOP/END (UNARMED / FIGHTER)
- PARRY_WEAK/STRONG (UNARMED / FIGHTER / DAGGER)
- BREAKFALL, STUN

**미등록 (추가 필요)**:
- `LINK_ATTACK` — Step C 진입 시 추가 필요. SungJinWoo `.bin` 에서 돌진공격 클립 확인 후 매핑
- `CHAIN_SMASH` — Step C 진입 시. Line 28 에 `GS_ChainSmash_01_OneHandSword` 클립이 SKILL_02 로 잘못 매핑된 것 확인 — Step C 에서 `CHAIN_SMASH` 로 재매핑 권장
- `WEAPON_SWAP` — Step D 진입 시

**주의**: 위 미등록 Action 이 `s_SungJinWooERankPolicies` 에도 없음 (Policy 테이블). Step C/D 진입 시 Clip + Policy 양쪽 모두 추가 필요.

**실제 클립 이름 확정 원칙** (§13.9):
- `.bin` 확인 없이 자율 판단 금지
- 없으면 Step 내부 임시 대체 (Dash 애니 재사용 등) + TODO 주석, 실제 클립 확보 후 교체

### 13.4 SLMD v3 마이그레이션 체크

- 기존 `.bin` 파일 3개(`SungJinWooERank.bin`, `SungJinWoo_ERank_Weapon01.bin`, `Hapjung_Station_1F.bin`) 는 모두 v2
- v3 로 구조 변경 시 `CModel::Load_Binary_Desc` 에서 **version 분기 로드** 필수
  - v2 읽을 때 notify 배열을 빈 vector 로 초기화
  - v3 읽을 때 notify 배열 파싱
- `Model_Converter::Convert` 는 v3 쓰기로 전환 (기존 FBX 재변환 시 자동 v3)
- 재변환 없이 v2 `.bin` 만으로도 **단계 3 모든 기능 동작** 해야 함. 단 ComboWindow Notify 는 Inspector 에서 사용자가 추가/저장해야 기능 — 이건 UX 기대치로 명시

### 13.5 의존성 주의

- `CIntentResolver` 는 **CBase 파생** — `Create()` / `Safe_Release` 패턴 사용. 초기 설계는 비상속이었으나 repo 컨벤션(모든 주요 클래스 CBase 파생) 일관성을 위해 파생으로 구현 (§2 결정사항 반영)
- `CStateMachine` (Engine) / `CPlayer_StateMachine` (Client, 파생) 은 **CBase 파생** → `Safe_Release` 사용 (AnimController 와 동일 패턴)
- `CAnimController` 는 `CComponent` (`CBase` 파생) → 기존대로 `Safe_Release`
- `IActionNotifyListener` 는 순수 가상 인터페이스. 구현체는 CBase 계열이 아니어도 무방 (StateMachine 이 구현). 단 Body 가 리스너 포인터를 들 때 **소유하지 않음** — CPlayer 가 StateMachine 과 Body 모두 소유하므로 수명 보장
- `CStateMachine` 은 `ENGINE_DLL` export 필요. 가상 함수(`On_Transition`, PURE) 가 DLL 경계 넘어가도 안전

### 13.6 재진입 가드 (§3.3 에서 언급)

```
CBody_Player::Update 흐름:
  1. m_pAnimController->Update(fTimeDelta)  → Finished/Notify 이벤트 수집
  2. "이벤트 발화 블록" (한 곳에서만)
     - for each Notify: m_pListener->OnNotify(...)
     - if Finished: m_pListener->OnActionFinished(...)
  3. 이벤트 핸들러 내부에서 Play_Action 재호출 가능. 단 이 재호출은
     Update 가 다시 돌 때 반영 (같은 프레임에 추가 Update 호출 금지)
```

### 13.7 커밋 단위

각 Step 끝나면 **Step 단위로 커밋**. Step 중간 멈춘 상태 커밋 금지 (빌드/런타임 깨진 상태 저장소 반영 방지).

커밋 메시지 패턴:
- `Step 3-A: Intent 골격 + WASD 이관`
- `Step 3-B: 이동 상태머신 (WALK/RUN/DASH)`
- `Step 3-C: 공격/가드 + LINK_ATTACK/CHAIN_SMASH`
- `Step 3-D: 무기 스왑 + 쿨다운`
- `Step 3-E: 추적 카메라 + Camera_Free Editor 전용화`

각 커밋 직전에 §10 체크리스트 해당 블록 실측.

### 13.8 사용자 협업 규약 (CLAUDE.md 와 일관)

- **코드 편집 금지** — Claude Code 는 직접 Edit/Write 로 소스 수정 안 함. 마크다운 코드블록으로 제시, 사용자가 수동 반영
- **명세서 파일만 직접 수정 허용** (본 문서 포함)
- **Bash 로 파일 복사/이동 금지** — 명령 텍스트만 안내, 사용자가 실행
- **레이어 이름 범용 유지** — `Layer_BackGround` 같은 역할명 선호 (`feedback_layer_naming.md`)

### 13.9 경계선 판단 (막히면 사용자에게)

다음 상황은 자율 결정하지 말고 사용자에게 확인:

- 애니메이션 클립 이름 선정 (LINK_ATTACK 에 쓸 실제 `.bin` 내 이름)
- 쿨다운 시간 / 이동 속도 / Grace 타이머 초기값 (0.3s 외 다른 값)
- 기존 `CHARACTER_ACTION` enum 값 재배치/재번호 (enum 값 변화는 .bin 호환성에 영향 가능)
- Editor UI 레이아웃 확장 (Inspector 어디에 Notify 리스트 배치?)
- Step 범위 초과 판단 (예: "이 수정이 Step C 인데 Step D 에 들어갈 내용까지 건드려야 하는가?")

### 13.10 설계 재검토가 필요한 시그널

구현 중 다음 징후가 보이면 **즉시 멈추고 설계 문서 재검토**:

- §4.2 Priority 표로 표현 안 되는 전이 요구가 2건 이상 누적 → Reject/Rewrite 리스트 확장 vs Priority 체계 재검토
- AnimNotify 이름 규약이 실제 사용 시 부족 → §4.3.3 미래 소비자 매트릭스 보강
- `fEnterBlendTime` 하드컷 상태에서 IDLE 복귀 말고 다른 전이가 시각적으로 치명적이면 → §8 블렌딩 단계 조기 착수 판단
