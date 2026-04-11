# Editor 전체 구현 계획

## 개요

Editor.exe의 전체 기능 구현 방향과 단계별 계획을 정리한 문서.
오늘(2026-04-04) 논의한 아키텍처 결정사항을 기반으로 하며, 각 단계 진입 시 수업 코드 및 상황에 맞춰 구체화한다.

---

## 구체화 원칙

이 계획서의 각 항목을 구체화할 때 다음 원칙을 따른다:

### 내용 원칙
1. **근거(Why)**: 왜 이 작업이 필요한지, 어떤 문제를 해결하는지 명시
2. **이유(Rationale)**: 여러 선택지 중 왜 이 방식을 택했는지, 대안 대비 장단점 설명
3. **개념(Concept)**: 사용되는 패턴, 기술, 구조에 대한 배경 설명 (예: Reflection, Prototype 패턴, DLL export 경계 등)

→ 미래 세션에서 이 문서를 읽었을 때 "왜 이렇게 했는지"를 코드 없이도 이해할 수 있어야 한다.

### 진행 원칙
- **순차적 대화 방식**: 구체화 내용을 한번에 작성하지 않는다. 목차별로 사용자와 대화하며 순차적으로 진행한다.
- 각 목차(근거→이유→개념→파일목록→세부구현)를 하나씩 제시하고, 사용자의 확인/피드백을 받은 후 다음으로 넘어간다.
- 이유: 한번에 대량의 내용을 제시하면 검토 부담이 크고, 중간에 방향 수정이 어렵다.

---

## 핵심 아키텍처 결정사항

| 항목 | 결정 |
|------|------|
| Assimp 격리 | Editor 전용. Engine/Client는 Assimp 무의존 |
| Engine 구조체 | VTXMESH, VTXANIMMESH, KEYFRAME, BONE_DATA 등 Engine이 소유 |
| 모델 초기화 | Engine CModel은 바이너리(fread) + raw 데이터(메모리) 이중 경로 |
| 씬 포맷 | JSON(원본, 편집/확인용) + 바이너리(Export, 런타임용) |
| 씬 로더 위치 | Client DLL (Engine 무수정, Editor에서도 사용 가능) |
| UI 씬 단위 | 혼합 — Level당 Base UI + 상황별 오버레이 UI |
| 2D Viewport | 2D 캔버스 패널(편집) + 3D Viewport 오버레이 프리뷰(확인) |
| 그리드 | 토글 — 스냅 ON 시 그리드 단위 정렬 배치, OFF 시 자유 배치 |
| 마우스 피킹 | Engine에 구현 (DirectX::TriangleTests::Intersects 기반) |
| 사운드 | FMOD (구현 보류, 라이브러리만 확정) |
| 이펙트 | 보류 — 추후 언급 시 논의 재개 |

---

## 데이터 흐름

```
[Editor]
  편집 → JSON 저장 (원본, git diff 가능, 직접 열어서 확인 가능)
  Export 버튼 → JSON → 바이너리 변환 (.scene_bin, .model 등)

[Client DLL]
  CScene_Loader::Load_Scene() — 바이너리 fread → Add_GameObject + Transform 세팅

[GameApp]
  Level::Initialize() → CScene_Loader::Load_Scene("파일.scene_bin")

[Editor 런타임에서도]
  Client.dll의 CScene_Loader 호출 가능 → 에디터에서 저장된 씬 로드/확인
```

---

## 단계별 계획

### Layer 0: 공통 인프라

**목적:** 이후 모든 편집 기능의 기반

- ~~Engine API 확장~~ — 오브젝트/레이어/컴포넌트 접근 getter 추가 ✓
- ~~오브젝트 이름/태그 시스템~~ — Hierarchy 표시용 ✓
- ~~RTTR 라이브러리 세팅~~ — Editor 전용 (헤더, lib, dll). Engine/Client 무의존 ✓
- ~~RTTR_Registration.cpp~~ — CTransform/CGameObject 멤버 함수 getter/setter 등록 ✓
- ~~Hierarchy 패널~~ — 레이어별 오브젝트 목록 표시, 선택 ✓
- ~~Inspector 패널~~ — RTTR 리플렉션 기반 프로퍼티 자동 UI 생성 ✓
- ~~Log 패널~~ — 로그 버퍼, 자동 스크롤, 레벨 필터 (Info/Warning/Error) ✓
- ~~마우스 피킹~~ — Engine Ray 교차 검사 + Editor Panel_Viewport 피킹 + CGameObject VIBuffer 멤버 ✓

**RTTR 적용 방침:**
- Editor 전용. Engine/Client 코드에 RTTR 의존성 없음
- 외부 등록 방식: Editor/Private/RTTR_Registration.cpp에서 Engine/Client 클래스 프로퍼티 등록
- **멤버 함수 패턴**: RTTR `.property()`는 getter(인자 0개)/setter(인자 1개) 멤버 함수 포인터를 요구
  - 자유 함수(free function) 래퍼는 RTTR이 멤버 함수로 인식 못함 → 사용 불가
  - CTransform에 `_float3` 기반 Get/Set 멤버 함수 추가하여 해결
- 타입→위젯 매핑: float3→DragFloat3, float→DragFloat, bool→Checkbox 등 자동
- 컴포넌트 추가 시 RTTR_Registration.cpp에 등록만 추가하면 Inspector 자동 반영
- **등록 제약**: private/protected 소멸자인 concrete 클래스는 RTTR 등록 불가 (소멸자 접근 시도 → 컴파일 에러). abstract 클래스는 가능
- 상세 설계: `명세서/Editor_ImGui_구현계획.md` Phase 4 참조

**패널 세부:**
- Hierarchy: TreeNode(레이어) → Selectable(오브젝트), 선택 상태 관리 ✓
- Inspector: RTTR로 등록된 프로퍼티 순회 → 타입별 ImGui 위젯 자동 생성, 컴포넌트별 CollapsingHeader ✓
- Log: 정적 로그 버퍼 + 필터 토글 + 자동 스크롤 + 색상 구분 ✓

**마우스 피킹 세부 (완료):**
- Engine: PICK_DATA 구조체, CVIBuffer::Pick() (TriangleTests::Intersects), CPU 정점 보존 ✓
- Engine: CPipeLine::Compute_WorldRay() — Viewport→NDC→View→World 변환, CGameInstance 래퍼 ✓
- Engine: CGameObject에 CVIBuffer* m_pVIBufferCom 공통 멤버 + Get_VIBuffer() getter ✓
- Editor: Panel_Viewport 클릭 감지 + WorldRay + 오브젝트 순회 + 최소 거리 피킹 ✓
- Editor: Clear_Selection() 댕글링 포인터 해결 (Safe_Release 후 명시적 nullptr 대입) ✓

**순환 참조 해결:**
- CPanel↔CPanel_Manager 순환 참조 발생 → Release_Panels() 수동 해제 패턴으로 해결 (Release_Engine()과 동일 원리) ✓

**Rotation 표현 설계:**
- Inspector: **오일러 각도(Degree)** 표시 — 사람이 숫자로 읽고 입력하기 직관적
- Viewport Gizmo(ImGuizmo): 내부적으로 **쿼터니언 연산** — Gimbal Lock 없이 정밀 회전
- 역할 분담: Inspector는 "확인/미세 조정", Gizmo는 "정밀 회전 조작"
- **Gimbal Lock**: 오일러 각도의 고유 한계. 한 축이 90도 회전 시 나머지 두 축이 겹쳐 자유도 상실
  - Inspector에서는 허용 — 숫자 입력은 대략적 용도이고, 정밀 작업은 Gizmo로 처리
  - Unity/Unreal도 동일 방식 (Inspector=오일러 표시, 내부=쿼터니언)
- CTransform 멤버 함수에서 WorldMatrix ↔ 오일러 변환 처리 (XMVECTOR는 SIMD 레지스터 타입이라 RTTR variant에 직접 담을 수 없음)
- Set_Scale 오버로드 시 RTTR이 함수 포인터를 구분 못하므로 별도 이름(Set_ScaleF3) 사용

**QA:**
- Hierarchy 선택 → Inspector 반영 → Viewport 실시간 연동 확인
- 패널 간 데이터 흐름 정상 동작 검증

**구조 점검:**
- Engine getter 추가 시 DLL export 누락 없는지 확인
- 패널 간 통신 방식 (Panel_Manager 경유) 일관성 점검

---

### Layer 1: 모델 파이프라인 (Assimp)

**목적:** 3D 모델 로드/렌더링/내보내기 파이프라인 구축

- Engine 구조체 확장 — VTXANIMMESH, KEYFRAME, BONE_DATA, ANIMATION_DATA 등
- Engine 클래스 신규 — CModel, CMesh, CMaterial, CBone, CAnimation, CChannel
  - Assimp 타입 파라미터 없음, Engine 자체 구조체만 사용
  - 이중 초기화: 바이너리(fread) / raw 데이터(메모리)
- Editor Assimp 통합 — Assimp 라이브러리 세팅, 변환 로직
  - Assimp 타입 → Engine 구조체 변환은 모두 Editor 내부에서 수행
- 바이너리 포맷 설계 — 모델/메시/머티리얼/본/애니메이션 포함
- 바이너리 내보내기/로드 검증
- 셰이더 — Shader_VtxAnimMesh.hlsl (스키닝)

**참조:** 수업 코드(15일차) Engine의 Model/Mesh/Material/Bone/Animation/Channel 구조 참고. Assimp 사용부를 Editor로 이동하는 것이 핵심.

**QA:**
- FBX 로드 → Editor Viewport에서 렌더링 확인
- 애니메이션 재생 미리보기 확인
- 바이너리 Export → GameApp에서 로드 → 렌더링 일치 확인
- NonAnim / Anim 모델 모두 테스트

**구조 점검:**
- Engine에 Assimp 의존성 유입 없는지 확인
- CModel 이중 초기화 경로가 동일한 렌더 결과를 내는지 검증

#### Layer 1 진행 현황 (2026-04-09 갱신)

**Engine 측 완료:**
- CBone / CChannel / CAnimation / CMesh / CMaterial / **CModel** 작성 완료
- Engine 구조체 전체: VTXMESH, VTXANIMMESH, KEYFRAME, MESH_DESC, MATERIAL_DESC, BONE_DESC, CHANNEL_DESC, ANIMATION_DESC, **MODEL_DESC**
- MODEL enum { NONANIM, ANIM, END }, TEXTURE_TYPE 17종
- `g_iNumMeshBones = 512` (Mesh.h 의 `m_BoneMatrices` 크기와 일관)
- Engine.vcxproj / filters 등록 완료
- **원칙 유지**: Engine 에 Assimp 의존성 유입 없음

**CModel 주요 설계:**
- Clone 전략: Mesh/Material = 공유 (AddRef), Bone/Animation = 깊은 복사 (Clone)
- Loop 정책 = 데이터 기반 (ANIMATION_DESC.bIsLoop) → CModel 은 현재 애니메이션의 loop 속성을 m_isAnimLoop 에 캐시
- Play_Animation → `_bool` 반환 (현재 애니메이션 끝나면 true)
- Set_AnimationIndex / Set_Animation — 단일 시그니처 (오버로드 제거), Set 시 캐시 갱신

**Editor 측 — 6 sub-step 진행 상황:**

| # | 단계 | 상태 | 산출물 |
|---|---|---|---|
| 1 | Engine .bin 로더 + 포맷 설계 | **완료** ✓ | Load_Binary_Desc / Free_Binary_Desc / Create(path) 오버로드 |
| 2 | Editor Assimp 통합 | **완료** ✓ | Editor/Public/assimp/, ThirdPartyLib/assimp-vc143-mt(d).lib, Bin/assimp-vc143-mt(d).dll |
| 3 | Editor Model_Converter 클래스 | **완료** ✓ | Convert() + Collect_Bones + Write 4섹션 + Should_Skip_Mesh + Build_TexturePath |
| 4 | FBX 변환 트리거 | **완료** ✓ | Content Browser .fbx 우클릭 → "Convert to .bin" 팝업 (NONANIM/ANIM 선택) — 계획의 MenuBar 방식에서 **컨텍스트 메뉴 방식으로 변경** |
| **5** | **Inspector 확장** | **진행 중** (5-A,5-B,5-D 완료 / 5-C 코드 제시 완료, 적용 전) | 아래 상세 |
| 6 | Viewport 모델 렌더 + Shader_VtxAnimMesh.hlsl | **완료** ✓ | CModelObject (Client DLL) + 테스트 씬 SungJinWoo_ERank .bin 배치 + 애니메이션 재생 |

**진행 순서 변경**: Step 6을 Step 5보다 먼저 구현함 (렌더링 검증 후 Inspector UI 작업이 자연스러운 흐름)

**완료 Step 상세:**

- **Step 1**: SLMD v1 포맷 확정, Model.cpp에 Load_Binary_Desc/Free_Binary_Desc 구현. DBG_NEW 매크로 충돌 학습 (`::operator new` 금지, `new _ubyte[]` 패턴)
- **Step 2**: Assimp 5.x 헤더/lib/dll을 Editor 프로젝트에만 배치. Engine/Client 무의존 유지
- **Step 3**: `CModel_Converter` static 유틸리티 클래스. NONANIM(aiProcess_PreTransformVertices)/ANIM 분기, LowMesh/NLOD 필터(Should_Skip_Mesh), 0-bone 폴백, Assimp 열우선→DirectX 행우선 전치, DIFFUSE 없을 때 _CO 텍스처 자동 탐색
- **Step 4**: Content Browser에서 .fbx 파일 우클릭 컨텍스트 메뉴 → 모달 팝업 (모델 타입 선택 + Convert 버튼). 변환 성공/실패 Log 패널 출력 + Content Browser 자동 새로고침
- **Step 6**: `Shader_VtxAnimMesh.hlsl` (4본 스키닝 + Phong), `CModelObject` (Client DLL, MODELOBJECT_DESC 기반), `EditorApp::Ready_TestScene()`에서 Shader_VtxMesh/VtxAnimMesh 프로토타입 + SungJinWoo_ERank .bin 로드 + ModelObject 인스턴스 배치. Update에서 ANIM 타입이면 Play_Animation 호출
- **변환된 .bin 파일**: SungJinWoo_ERank, Fiona, ForkLift, Rock (4개)

**Step 5 미구현 — 확장된 범위 (2026-04-09 논의 결과):**

기존 계획은 "모델 정보 표시 + 애니메이션 리스트/재생 버튼"이었으나, 다음으로 확장:

| 항목 | 내용 | 분류 | 상태 |
|---|---|---|---|
| 5-A | 모델 기본 정보 (타입, 메시/머티리얼/본/애니메이션 개수) | Inspector UI | **완료** ✅ |
| 5-B-1 | 애니메이션 리스트 (이름 목록, 클릭 전환) | Inspector UI | **완료** ✅ |
| 5-B-2 | Loop 토글 (현재 애니메이션 bIsLoop 표시 + 런타임 변경) | Inspector UI + Engine getter/setter | **완료** ✅ |
| 5-B-3 | 타임라인 정보 (CurrentTrackPosition / Duration, 진행률 바) | Inspector UI | **완료** ✅ |
| 5-C | **애니메이션 전환 보간 (CrossFade 블렌딩)** | **Engine 기능 + Inspector UI** | **구현 중** — 코드 제시 완료, 적용 전 |
| 5-D | 모델 피킹 + CPU 스키닝 피킹 + 애니메이션 재생/정지 | Engine CModel::Pick() + Inspector UI | **완료** ✅ |

**5-A 설계 결정사항 (논의 완료):**

- **Inspector가 CModel에 접근하는 방법** — **A안 채택**: CGameObject에 `Get_Component(const _tchar* pTag)` public getter 추가 (Engine API 확장)
  - 대안 B (CModelObject 전용 getter): Inspector가 Client 클래스에 의존 → 부적절
  - 대안 C (RTTR 프로퍼티): CModel* 반환 타입을 RTTR variant로 다루기 복잡
  - A안은 범용적이고 Layer 2 이후에도 계속 필요한 Engine API
- **모델 섹션 구현 방식** — **수동 ImGui** (RTTR 아님)
  - RTTR getter는 인자 0개 필수 → `Get_Component(tag)`는 RTTR 프로퍼티로 등록 불가
  - 모델 정보는 구조가 특수 (애니 리스트, 타임라인 바 등) → RTTR 자동 UI 부적합
  - Inspector::Render()에 `Render_Model(pSelected)` 수동 섹션 추가
  - 내부 흐름: `pSelected->Get_Component(TEXT("Com_Model"))` → CModel* 캐스팅 → nullptr이면 return → ImGui 직접 호출

**5-C 애니메이션 전환 블렌딩 — 설계 결정사항 (논의 완료):**

- **현재 상태**: `Set_AnimationIndex`는 즉시 전환 (하드 컷). 수업 코드(16/17일차)에도 전환 보간 없음 확인 완료 (4월 7일)
- **필요성**: 게임플레이에서 Idle→Run→Attack 전환 시 뚝뚝 끊기는 문제. 에디터 프리뷰 용도로는 없어도 동작하지만, 최종 게임에는 필수

- **보간 방식 — B안 채택 (SQT 분해 후 보간, 업계 표준)**:
  - 대안 A (행렬 직접 Lerp): 회전 보간 시 스케일 왜곡 발생 (candy wrapper 현상). 상용 엔진에서 사용하지 않음 → 탈락
  - KEYFRAME에 이미 vScale/vRotation(Quat)/vTranslation이 분리되어 있으므로 자연스럽게 적용 가능
  - Scale/Translation → Lerp, Rotation → Slerp (또는 Nlerp + 정규화)
  - 보간 후 행렬 재조합 → 계층 전파 1회 (Combined = Local × Parent.Combined)

- **CrossFade 흐름**:
  ```
  A 애니메이션 (fadeout)  ──┐
                            ├──  본별 SQT 보간 (ratio: 0→1)
  B 애니메이션 (fadein)   ──┘
  ratio = m_fBlendElapsed / m_fBlendDuration (clamp 0~1)
  ```
  - 이전 애니메이션은 계속 재생 (멈추지 않음). 두 애니메이션 모두 fTimeDelta 진행
  - ratio 1.0 도달 시 이전 애니메이션 해제, 현재만 재생
  - 블렌딩 중 새 전환 요청: 현재 블렌드 결과를 "이전"으로 스냅, 새 타겟으로 블렌드 시작

- **단계적 확장 계획**:
  1. **5-C (현재)**: CModel에 CrossFade 구현 — SQT 보간 (저수준 "어떻게 섞을 것인가")
  2. **이후 (Layer 2+)**: Animation State Machine — 상태 그래프 기반 전환 제어 (고수준 "언제, 어떤 애니메이션으로 전환할 것인가"). CrossFade 위에 얹는 구조

- **Engine 변경 범위 (구현 완료/적용 전)**:
  - CChannel: `Get_SQT()` 추가 — 키프레임 보간 후 SQT 값만 출력 (Bone 세팅 안 함) **✅ 구현 완료**
  - CAnimation: `Update_SQT()` 추가 — TrackPosition 전진 + Channel::Get_SQT → 본별 SQT 벡터 채움 **코드 제시 완료**
  - CModel: `m_iPrevAnimIndex`, `m_fBlendElapsed`, `m_fBlendDuration(0.2f)`, `m_isBlending` 추가 **코드 제시 완료**
  - CModel::Play_Animation 수정: 블렌딩 중이면 양쪽 Update_SQT → 본별 Lerp/Slerp → AffineTransformation → Set_TransformationMatrix **코드 제시 완료**
  - CModel::Set_AnimationIndex 수정: m_fBlendDuration > 0 이면 블렌드 시작 트리거 (prev 보존, curr Reset) **코드 제시 완료**
- **Inspector 노출**: Blend Duration 슬라이더(0~1초), 블렌딩 상태 ProgressBar + "Blending..." 텍스트 **코드 제시 완료**
- **미해결 이슈 — 루트 모션 위치 동기화**: 본 애니메이션은 로컬 공간 동작, CTransform 월드 위치는 애니메이션이 갱신하지 않음. 루트 본에 Translation 키프레임이 있으면 메쉬만 이동하고 CTransform은 그대로 → 애니메이션 전환 시 메쉬 스냅백 발생. **해결책**: 루트 모션 추출 (매 프레임 루트 본 delta → CTransform 적용, 루트 본은 원점 고정). 5-C CrossFade와 독립적 기능. **다음 세션에서 애니메이션 루트 본 Translation 유무 확인 후 구현 여부 결정**

**5-D 모델 피킹 — 설계 결정사항 (논의 완료, 구현 완료):**

- **기존 인프라 재사용**: CMesh는 CVIBuffer를 상속하며, PICK_DATA(pVerticesPos + pIndices CPU 사본) 및 CVIBuffer::Pick()(TriangleTests::Intersects)을 상속
- **NONANIM**: 기존 CVIBuffer::Pick() 그대로 사용. aiProcess_PreTransformVertices로 정점에 노드 변환이 미리 적용되어 PICK_DATA와 렌더 결과 일치
- **ANIM — CPU 스키닝 피킹**: 바인드 포즈 PICK_DATA와 스키닝된 렌더 결과 불일치 문제 해결
  - CMesh에 정점별 BlendIndex/BlendWeight 보존 (Ready_AnimMesh에서 저장, vector 멤버)
  - CModel::Pick() — ANIM일 때 메시별 본 행렬 계산 → 바인드 포즈 정점에 CPU 스키닝 → 임시 정점 배열로 삼각형 테스트 → 해제
  - PICK_DATA 원본 불변, 클릭 시 1회만 실행
- **피킹 경로**: Panel_Viewport에서 Get_VIBuffer() nullptr이면 Com_Model 검색 → CModel::Pick() 호출
- **애니메이션 재생/정지**: CModel에 m_isAnimPlaying 플래그, Inspector에 Playing 체크박스

**원칙 확정**: Assimp 는 Editor 만, 런타임(Engine/Client/GameApp)은 .bin 파일 I/O 만 사용. Inspector 확장은 패널 신설이 아니라 기존 Panel_Inspector 에 RTTR 외 수동 섹션 추가 방식.

**SLMD 바이너리 포맷 v1 결정사항:**

| 항목 | 결정 |
|---|---|
| 확장자 | `.bin` |
| 저장 위치 | FBX 원본 옆 (`Resources/Models/<name>/<name>.bin`) |
| 문자열 | 고정 길이 `char[MAX_PATH]` / `wchar_t[MAX_PATH]` |
| 텍스처 경로 | **Resources/ 루트 기준 상대 경로** (`Models/Fiona/Fiona_Diffuse.png` 형태) |
| Magic | `"SLMD"` (Solo Leveling Model Data) |
| Version | 1 |
| Header 크기 | 80 byte (magic 4 + version 4 + modelType 4 + reserved 4 + _float4x4 64) |
| 섹션 순서 | Header → Meshes → Materials → Bones → Animations |
| KEYFRAME 직렬화 | POD 배열 일괄 `fread` (대용량 섹션 빠른 경로) |

**포맷 스펙 전문**: `Engine/Private/Model.cpp` 상단 주석 블록에 박제됨. 자세한 섹션별 필드는 `명세서/4월7일 진행 사항.md` 의 "4월 8일 이어서" 섹션 참조.

**학습 사항 (DBG_NEW 매크로 충돌):**
- Engine_Defines.h 의 `#define new DBG_NEW` 가 `::operator new` 같은 저수준 호출을 깨뜨림
- **Engine 코드에서는 `::operator new` / `::operator delete` 금지**
- `void*` 에 raw byte 할당 시: `new _ubyte[size]` + `delete[] static_cast<_ubyte*>(ptr)` 패턴 사용
- `Safe_Delete_Array<T>` 템플릿은 `void*` 에 사용 불가

---

### Layer 2: 3D 편집 기능

**목적:** 3D 오브젝트 배치/편집 + 씬 저장/로드

**Layer 2-a: 기본 배치**
- 오브젝트 선택 (피킹으로 클릭)
- ImGuizmo Transform 조작 (이동/회전/스케일)
- 배치 결과 씬 저장 (JSON) / 내보내기 (바이너리)
- Client CScene_Loader 구현

**Layer 2-b: 고급 배치**
- 그리드 렌더링 + 스냅 토글 (ON 시 그리드 단위 정렬)
- Content Browser에서 D&D로 배치
- 조명 배치 — 라이트 오브젝트 배치 (위치, 타입, 색상, 범위)
- 조명 매핑 — 특정 오브젝트/영역에 조명 연결, 씬 정보에 포함

**3D 배치 대상 전체 목록:**

| 대상 | 필요도 | 우선순위 |
|------|--------|---------|
| 맵 구조물 | 가능성 있음 (리소스 추출 결과에 따라) | 후순위 |
| 소품 | 가능성 있음 (리소스 추출 결과에 따라) | 후순위 |
| 몬스터 스폰 포인트 | 필요 | 중순위 |
| NPC 위치 | 필요 | 중순위 |
| 카메라 | 필요 | 중순위 |
| 라이트 | 필요 | 중순위 |
| 조명 매핑 | 필요 | 중순위 |
| 트리거 영역 | 필요 | 중순위 |
| 네비메시/이동 경로 | 필요 | 후순위 (구현 난이도 높음) |

**패널 세부:**
- Inspector 확장: 모델 컴포넌트 정보 (Mesh 개수, Material, Bone 구조), 라이트 속성 편집
- Hierarchy 확장: 배치된 오브젝트 타입별 아이콘/분류
- Content Browser: 모델/텍스처 선택 → Viewport로 D&D 배치

**있으면 추가할 기능:**
- Undo/Redo 시스템 (Command 패턴)
- 다중 선택 + 그룹 이동
- 오브젝트 복제 (Ctrl+D)
- Viewport 카메라 북마크 (자주 보는 위치 저장)

**QA:**
- 배치 → 저장 → 로드 → 위치 일치 확인
- JSON ↔ 바이너리 변환 정합성
- Client CScene_Loader로 GameApp에서 씬 복원 확인
- ImGuizmo 조작 → Inspector 값 동기화 확인

**구조 점검:**
- 씬 파일 포맷 버전 관리 — 구조 변경 시 하위 호환
- 씬 파일 내 리소스 경로의 상대/절대 경로 통일

---

### Layer 3: UI 에디터

**목적:** Screen Space UI 배치/편집 + 저장/로드

- 2D 캔버스 패널 — 직교 투영, 게임 해상도 기준 좌표계
- UI 오브젝트 배치 — UIOBJECT_DESC 기반 (CenterX/Y, SizeX/Y, 텍스처 경로, 프로토타입 태그, 렌더 순서)
- 3D Viewport 오버레이 프리뷰 — 토글로 실제 게임 화면처럼 UI 겹쳐서 확인
- UI 씬 저장/로드 — 혼합 구조 (Base UI + 오버레이 UI)
- Client UI 로더

**UI 씬 구조:**
```
Level 진입 → Base UI 자동 로드 (항상 보이는 HUD)
이벤트 발생 → 오버레이 UI 추가 로드 (인벤토리, 보스 HP 등)
이벤트 종료 → 오버레이 UI 해제
```

**World Space UI (3D 공간 UI):**
- 몬스터 HP바, NPC 이름표 등은 에디터에서 배치하는 대상이 아님
- 코드 로직으로 오브젝트에 바인딩
- 에디터에서는 매핑 정보 설정 정도 (어떤 오브젝트에 어떤 UI 타입 연결)

**패널 세부:**
- 2D 캔버스: UI 오브젝트 선택/이동/리사이즈, 정렬 가이드라인
- Inspector: UI 속성 편집 (위치, 크기, 텍스처, 렌더 순서)
- Content Browser: 텍스처 선택 → 2D 캔버스로 D&D

**있으면 추가할 기능:**
- 앵커 시스템 (해상도 변경 대응)
- UI 애니메이션 미리보기 (페이드, 슬라이드 등)
- UI 프리셋 템플릿 (HP바, 버튼 등 자주 쓰는 형태)

**QA:**
- 2D 캔버스 배치 → 오버레이 프리뷰 → 실제 게임 화면 일치 확인
- Base + 오버레이 로드/해제 정상 동작
- 다양한 해상도에서 UI 위치 확인

---

### Layer 4: 부가 기능 (보류)

**이펙트 시스템** — 보류. 수업 코드 또는 구현 계획 언급 시 논의 재개.

**FMOD 사운드** — 라이브러리 확정(FMOD). 구현 보류.
- 에디터에서 사운드 파일 재생 테스트
- 오브젝트/이벤트에 사운드 매핑
- 매핑 정보를 씬 데이터에 포함하여 내보내기

**Content Browser 확장**
- 텍스처 썸네일 미리보기
- 모델 3D 프리뷰
- 사운드 재생 버튼
- 파일 탐색기 수준의 기능 (검색, 필터, 정렬)
- Drag & Drop → Viewport/Hierarchy에 배치

---

## 패널 기능 로드맵 요약

| 패널 | Layer 0 | Layer 1 | Layer 2 | Layer 3 |
|------|---------|---------|---------|---------|
| Viewport | (기존 완료) | 모델 렌더 | ImGuizmo, 그리드, 피킹 | 오버레이 프리뷰 |
| 2D Canvas | - | - | - | 신규 패널, UI 배치 |
| Hierarchy | 오브젝트 목록, 선택 | 모델 표시 | 타입별 분류 | UI 오브젝트 목록 |
| Inspector | Transform 편집 | 모델/Material 정보 | 라이트, 배치 속성 | UI 속성 편집 |
| Content Browser | (기존 완료) | 모델 미리보기 | D&D 배치 | 텍스처 D&D |
| Log | 로그 기본 기능 | 로드 결과 로그 | 저장/로드 로그 | UI 편집 로그 |

---

## 비고

- RTTR 리플렉션: Layer 0에서 도입. Inspector는 처음부터 RTTR 기반으로 구현
- 맵 리소스 추출: 블렌더에서 시행착오 후 맵 구조물/소품 배치 필요 여부 결정
- 각 Layer 진입 시 해당 시점의 수업 코드를 참조하여 세부 구현 구체화
- 이 문서는 방향성과 결정사항 기록 용도. 세부 구현 명세는 각 Layer 착수 시 별도 작성
