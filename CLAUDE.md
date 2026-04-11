# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

- 참고 프로젝트 1 (4-Project DLL 구조 원본): `C:\Users\chaho\ddoichaboom\CPP_STUDY_3D\Framework_4Proj\Framework\CLAUDE.md` 참조
- 참고 프로젝트 2 (3-Project 구조 원본): `C:\Users\chaho\ddoichaboom\CPP_STUDY_3D\Framework\CLAUDE.md` 참조

### 수업 코드 참조 (Assimp/Model/Animation 파이프라인 원본)

아래 경로에서 최신 수업 코드 및 수업 메모를 확인한다. **경로가 유효하지 않을 경우**, `C:\Users\denni\ddoichaboom\CPP_STUDY_3D\` 하위에서 최신 일차 폴더 또는 명세서 파일을 탐색할 것.

- **수업 코드 (최신: 15일차)**: `C:\Users\denni\ddoichaboom\CPP_STUDY_3D\수업 파일\Files\15일차`
  - Engine 내 Model/Mesh/Material/Bone/Animation/Channel 클래스가 Assimp을 직접 사용하는 원본
  - Solo_Leveling에서는 이 Assimp 사용부를 **Editor로 이동**하여 격리해야 함
- **수업 메모 (9~14일차)**: `C:\Users\denni\ddoichaboom\CPP_STUDY_3D\명세서\`
  - 9일차: Model & Mesh 로드 (Assimp 기초)
  - 10일차: Material (텍스처 타입, aiTextureType)
  - 11일차: 애니메이션 기초 & Bone 구조 (CBone, aiNode/aiBone/aiAnimNode 구분)
  - 12일차: Bone-Mesh 연결 & OffsetMatrix
  - 13일차: Animation & Channel 클래스 (KEYFRAME, Duration, TickPerSecond)
  - 14일차: 키프레임 보간 & 깊은 복사 문제 (Lerp/Slerp, Clone 이슈)
  - **주의**: 메모 내 세부 코드는 오류가 있을 수 있음. 주제 참고 용도로만 사용하고, 실제 구현은 수업 코드(15일차)를 기준으로 할 것

## Build System

- **Visual Studio 2022** solution: `Framework.sln` (MSBuild, v143 toolset)
- Four projects: **Engine** (DLL), **Client** (DLL), **GameApp** (EXE), **Editor** (EXE)
- Dependency chain: Engine ← Client ← GameApp / Editor
- Primary configuration: Debug|x64
- Post-build: `EngineSDK_Update.bat` (Engine), `ClientSDK_Update.bat` (Client)
- Build: `msbuild Framework.sln /p:Configuration=Debug /p:Platform=x64`
- **CRT 통일 필수**: 모든 프로젝트 동적 CRT (`/MDd` Debug, `/MD` Release). `/MT` 혼용 시 힙 충돌

## Architecture

```
Engine.dll  ←  Client.dll  ←  GameApp.exe
                            ←  Editor.exe
```

**4-Project 분리**: Engine(코어) → Client(게임 로직 DLL, CLIENT_DLL export) → GameApp(얇은 진입점) / Editor(ImGui 에디터)
Client를 DLL로 만든 이유: Editor에서 Client의 GameObject를 직접 Clone하여 사용하기 위함.

**Class hierarchy:**
```
CBase (ref counting)
├── CGameInstance (singleton) — 전 서브시스템 래퍼 (PipeLine/Input/Light 등), OnResize
├── CGraphic_Device — D3D11 device, swap chain, OnResize
├── CInput_Device (final) — Raw Input 키보드/마우스, 2단계 static→프레임 구조
├── CPipeLine (final) — View/Proj 행렬 저장, 역행렬, 카메라 위치, Compute_WorldRay
├── CComponent (abstract) — Prototype/Clone
│   ├── CTransform (abstract) — world matrix, STATE 접근자, Bind_ShaderResource, _float3 기반 Position/Scale/Rotation 접근자
│   │   ├── CTransform_3D (final) — 3D 이동/회전/LookAt
│   │   └── CTransform_2D (final) — 2D 이동
│   ├── CShader (final) — FX11 Effect, Bind_Matrix/Bind_SRV/Bind_RawValue
│   ├── CTexture (final) — multi-SRV, DDS/WIC 로드. Clone 시 각 SRV Safe_AddRef 필수
│   ├── CVIBuffer (abstract) — VB/IB, DrawIndexed, PICK_DATA(CPU 정점 사본), Pick()(TriangleTests::Intersects)
│   │   ├── CVIBuffer_Rect (final) — VTXTEX 쿼드, PICK_DATA 생성 (_ushort→_uint 인덱스 변환)
│   │   └── CVIBuffer_Terrain (final) — BMP heightmap, VTXNORTEX, PICK_DATA 생성
│   └── CModel (final) — 모델 컨테이너, vector<CMesh/CMaterial/CBone/CAnimation>, MODEL_DESC 초기화
│       ├── CMesh — VTXMESH/VTXANIMMESH, 본 바인딩(BoneIndices/OffsetMatrices)
│       ├── CMaterial — 텍스처 타입별 SRV vector (aiTextureType 대응)
│       ├── CBone — Transform/Combined 계층 갱신
│       ├── CAnimation — Duration/Channel 배열, Play_Animation _bool 반환, loop 데이터 기반
│       └── CChannel — KEYFRAME 배열, Update 시 Scale/Rot/Translation 보간
├── CLevel (abstract)
├── CGameObject (abstract) — TRANSFORMTYPE으로 3D/2D Transform 자동 생성
│   ├── CCamera (abstract) — CAMERA_DESC, Update_PipeLine
│   └── CUIObject (abstract) — 직교 투영, CTransform_2D 자동
├── CLayer, CLevel_Manager, CTimer_Manager, CPrototype_Manager, CObject_Manager, CRenderer
├── CLight (final) — LIGHT_DESC (DIRECTIONAL/POINT). CBase 상속, CComponent 아님
├── CLight_Manager (final) — CGameInstance 소유, 비싱글톤
│
├── [Client.dll]
├── CMainApp, CBackGround, CCamera_Free, CTerrain, CModelObject (CLIENT_DLL export)
├── CLevel_Logo, CLevel_Loading, CLevel_GamePlay, CLoader (internal)
│   └── CLevel_GamePlay: Initialize() 비어있음 (오브젝트 미생성 상태)
│
├── [Editor.exe]
├── CEditorApp (final) — ImGui DockSpace + MenuBar + DockBuilder 레이아웃, Ready_Panels/ToggleMenuItem/Render_Scene
├── CPanel (abstract) — Initialize/Update/Render, Is_Open/Set_Open/Get_Name
│   ├── CPanel_Viewport (final) — "Viewport", 별도 RenderTarget 소유 (RTV/SRV/DSV), Begin_RT/End_RT, 리사이즈 대응
│   ├── CPanel_Hierarchy (final) — "Hierarchy", TreeNode(레이어)→Selectable(오브젝트), 레벨 전환 감지
│   ├── CPanel_Inspector (final) — "Inspector", RTTR 기반 프로퍼티 자동 UI (타입→위젯 매핑)
│   ├── CPanel_ContentBrowser (final) — "Content Browser", std::filesystem 기반 Resources/ 탐색, Breadcrumb/파일타입 아이콘
│   └── CPanel_Log (final) — "Log", 정적 로그 버퍼, 필터 토글(Info/Warning/Error), 자동 스크롤
└── CPanel_Manager (final, singleton) — map<wstring, CPanel*>, Update/Render_Panels, 선택 상태(Get/Set_SelectedObject), Release_Panels()
```

**Key patterns:**
- **Singletons**: `DECLARE_SINGLETON` / `IMPLEMENT_SINGLETON`
- **Prototype/Clone**: `Add_Prototype()` → `Clone_Prototype(PROTOTYPE::GAMEOBJECT or COMPONENT)`
- **Descriptor chain**: `TRANSFORM_DESC` → `GAMEOBJECT_DESC` (+ eTransformType) → `CAMERA_DESC` / `UIOBJECT_DESC` → Client DESC
- **Update pipeline**: `Input_Device::Update` → `Priority_Update` → `Update` → `PipeLine::Update` → `Late_Update` → `Level_Manager::Update`
- **Render groups**: Late_Update에서 `Add_RenderGroup(RENDERID, this)` → Renderer가 Priority→NonBlend→Blend→UI 순서로 Draw. AddRef/Release 사이클
- **DLL export**: `ENGINE_DLL` (Engine), `CLIENT_DLL` (Client 외부 사용 클래스)

**Vertex Format ↔ Shader:**
| 정점 포맷 | 셰이더 | 용도 |
|-----------|--------|------|
| VTXTEX | Shader_VtxTex.hlsl | BackGround (UI) |
| VTXNORTEX | Shader_VtxNorTex.hlsl | Terrain (Phong) |
| VTXMESH | Shader_VtxMesh.hlsl | Static Model (NonAnim) |
| VTXANIMMESH | Shader_VtxAnimMesh.hlsl | Skinned Model (Anim, BoneIndices/Weights) |

## Coding Conventions

- Class: `C` prefix, Member: `m_` prefix, Method: PascalCase
- Namespace: `Engine`, `Client`, `Editor` via `NS_BEGIN` / `NS_END`
- Singleton classes use `final`, Return `HRESULT` from init functions
- `Safe_Delete`, `Safe_Release`, `Safe_AddRef` — never raw `delete`
- `ETOI` (to int) / `ETOUI` (to unsigned int) for enum casting

## Project File Tree

```
Framework/
├── Engine/
│   ├── Public/                     — 39 headers
│   │   ├── Engine_Defines/Enum/Function/Macro/Struct/Typedef.h
│   │   ├── Base, GameInstance, Graphic_Device, Input_Device, PipeLine
│   │   ├── Timer, Timer_Manager, Level, Level_Manager
│   │   ├── GameObject, UIObject, Camera, Layer, Object_Manager, Prototype_Manager, Renderer
│   │   ├── Component, Transform, Transform_3D, Transform_2D
│   │   ├── Shader, Texture, VIBuffer, VIBuffer_Rect, VIBuffer_Terrain
│   │   ├── Model, Mesh, Material, Bone, Animation, Channel
│   │   ├── Light, Light_Manager
│   │   ├── fx11/, DirectXTK/
│   │   └── Engine_Struct.h         — ENGINE_DESC, LIGHT_DESC, VTXTEX, VTXNORTEX, VTXMESH, VTXANIMMESH, PICK_DATA, MODEL_DESC, MESH_DESC, MATERIAL_DESC, BONE_DESC, ANIMATION_DESC, CHANNEL_DESC, KEYFRAME
│   ├── Private/                    — 33 cpp (Public와 1:1 대응)
│   ├── ThirdPartyLib/              — Effects11, DirectXTK (Debug/Release)
│   └── Bin/
│
├── Client/
│   ├── Public/                     — 10 headers (Client_Defines, MainApp, BackGround, Camera_Free, Terrain, ModelObject, Level_Logo/Loading/GamePlay, Loader)
│   ├── Private/                    — 9 cpp
│   └── Bin/
│
├── GameApp/Default/                — GameApp.cpp (wWinMain, 60FPS loop), GameApp_Defines.h
├── Editor/
│   ├── Default/Editor.cpp          — wWinMain + ImGui WndProc, WS_OVERLAPPEDWINDOW + SW_MAXIMIZE
│   ├── Public/                     — EditorApp.h, Editor_Defines.h, Editor_Enum.h
│   │                                 Panel.h, Panel_Manager.h
│   │                                 Panel_Viewport.h, Panel_Hierarchy.h, Panel_Inspector.h
│   │                                 Panel_ContentBrowser.h, Panel_Log.h
│   │                                 Model_Converter.h
│   │                                 assimp/                  — Assimp 5.x 헤더 (Editor 전용)
│   ├── Private/                    — EditorApp.cpp, Panel.cpp, Panel_Manager.cpp
│   │                                 Panel_Viewport.cpp, Panel_Hierarchy.cpp, Panel_Inspector.cpp
│   │                                 Panel_ContentBrowser.cpp, Panel_Log.cpp
│   │                                 RTTR_Registration.cpp, Model_Converter.cpp
│   ├── ThirdPartyLib/              — assimp-vc143-mt(d).lib, RTTR/
│   ├── ImGui/                      — docking 브랜치 + backends/ (DX11, Win32)
│   └── ImGuizmo/
│
├── Resources/
│   ├── ShaderFiles/                — Shader_VtxTex, Shader_VtxNorTex, Shader_VtxMesh, Shader_VtxAnimMesh (.hlsl)
│   ├── Textures/                   — Default, Terrain/, Logo/, Player/, SkyBox/, Explosion/
│   └── Models/                     — Fiona/, ForkLift/, Rock/, SungJinWoo_ERank/ 등 (FBX + 변환된 .bin)
│
├── EngineSDK/, ClientSDK/          — SDK 배포 (bat으로 자동 복사)
├── 명세서/
│   ├── 개발_진행_가이드.md           — Phase A~G 이식 가이드
│   ├── Editor_ImGui_구현계획.md     — Editor Phase 1~5 계획
│   └── Editor_전체_구현계획.md     — Layer 0~4 전체 계획
└── Framework.sln
```

## Important Implementation Notes

### DLL Export
- `ENGINE_DLL` / `CLIENT_DLL`: Engine_Macro.h 정의, `_EXPORTS` 전처리기로 export/import 전환
- Client 전역변수 없음: `g_hWnd` 등은 CGameInstance가 관리, GameApp.cpp에서만 전역

### 핵심 주의사항 (Gotchas)
- **m_WorldMatrix 초기화**: 반드시 Identity. 영행렬이면 WVP 변환 시 렌더링 안됨
- **CTexture Clone**: 복사 생성자에서 각 SRV에 `Safe_AddRef()` 필수. 미적용 시 combase.dll 크래시
- **CVIBuffer Clone**: 복사 생성자에서 `PICK_DATA` 깊은 복사 필수. 얕은 복사 시 이중 해제 크래시
- **InputLayout 매칭**: 정점 구조체와 D3D11_INPUT_ELEMENT_DESC 1:1 대응 필수 (불일치 시 크래시)
- **Level Transition**: `SUCCEEDED()` 사용, `FAILED()` 아님
- **CLoader 스레드**: WIC 텍스처 로딩 시 `CoInitializeEx(nullptr, 0)` 필수
- **Release 순서**: CMainApp — Device → Context → Release_Engine → GameInstance
- **WM_INPUT**: WndProc에서 `CGameInstance::Process_RawInput(lParam)` 호출
- **Safe_Release 동작**: RefCount가 0이 될 때만 포인터를 nullptr로 설정. RefCount > 0이면 포인터 유지 → 댕글링 위험. 선택 해제 등에서 Safe_Release 후 반드시 수동 `= nullptr` 필요
- **VIBuffer_Terrain PICK_DATA**: Initialize_Prototype에서 pVerticesPos 할당을 정점 값 쓰기 전에 수행해야 함
- **`::operator new` / `::operator delete` 금지**: Engine_Defines.h에서 `#define new DBG_NEW`로 치환되기 때문에 `::operator new(size)` 형태는 컴파일 오류. void* 바이트 버퍼가 필요하면 `new _ubyte[size]` 로 할당하고, 해제는 `delete[] static_cast<_ubyte*>(ptr)` 로 캐스팅 후 배열 delete. `Safe_Delete_Array<T>`는 void*에 쓸 수 없음. (CModel Load_Binary_Desc에서 겪음)

### Runtime Resource Paths
- 런타임 파일 I/O (프로젝트 속성에 Resources/ 추가 불필요)
- 경로 패턴: `TEXT("../../Resources/ShaderFiles/...")`, `TEXT("../../Resources/Textures/...")`

### Editor Implementation
- **기초 구현 계획**: `명세서/Editor_ImGui_구현계획.md` (Phase 1~5, 패널/Viewport 기초)
- **전체 구현 계획**: `명세서/Editor_전체_구현계획.md` (Layer 0~4, 아키텍처 결정사항, 배치/UI/모델 파이프라인)
- **현재 상태**: Phase 1~3 + Layer 0 전체 완료. Layer 1 — Engine 측 전체 완료 + Editor Step 1~4, 6 완료. Step 5 진행 중: **5-A(모델 기본 정보) ✓, 5-B(애니메이션 컨트롤) ✓, 5-D(모델 피킹+CPU스키닝+재생/정지) ✓, 5-C(CrossFade 블렌딩) 구현 중** — Channel::Get_SQT 구현 완료, Animation::Update_SQT / Model Play_Animation 블렌딩 / Inspector 블렌딩 UI 코드 제시 완료(사용자 적용 전)
- **패널 시스템**: CPanel (abstract) → 파생 5개 (Viewport, Hierarchy, Inspector, ContentBrowser, Log)
  - `CPanel_Manager` (싱글톤, `map<wstring, CPanel*>`): 이름 기반 접근, Update/Render_Panels, 선택 상태 관리
  - 자주 접근하는 패널은 멤버 포인터로 캐싱 (예: `m_pViewport`)
  - CPanel 생성자에서 GameInstance/Panel_Manager Safe_AddRef → 순환 참조 발생
  - **순환 참조 해결**: `Release_Panels()` 수동 해제 (Release_Engine() 패턴). EditorApp::Free()에서 호출
  - 캐싱 포인터는 Safe_AddRef 필요
- **Viewport 렌더 파이프라인** (Phase 1 완료):
  - CPanel_Viewport가 별도 RenderTarget 소유 (Texture2D + RTV + SRV + DSV)
  - `Render_Scene()` → `Begin_RT(별도RT전환)` → `Draw(3D)` → `End_RT(해제)` → `Begin_Draw(BackBuffer재바인딩)` → ImGui → Present
  - `CGraphic_Device::Clear_BackBuffer_View()`에서 `OMSetRenderTargets` 재바인딩 추가 (별도RT→BackBuffer 복원)
  - 패널 리사이즈 시 `GetContentRegionAvail()` 크기 비교 → RT 재생성
- **Content Browser** (Phase 3 완료):
  - `std::filesystem` + C++17 필수 (`/std:c++17` 프로젝트 설정)
  - Resources/ 루트 기준 디렉토리 순회, 캐싱 (매 프레임 순회 방지)
  - Breadcrumb 경로 (클릭 가능) + `<-` 뒤로가기
  - 파일 타입별 아이콘 ([D]폴더 [T]텍스처 [S]셰이더 [M]모델 [B]바이너리)
  - 더블클릭 폴더 진입, Selectable 파일 선택
  - .fbx 우클릭 → "Convert to .bin" 컨텍스트 메뉴 → 모달 팝업 (NONANIM/ANIM 선택 + Convert). 변환 결과 Log 출력 + 자동 새로고침
- **Editor_Enum.h**: `MENUTYPE { PANEL, TOOL, END }`, `LOG_LEVEL { INFO, WARNING, ERROR_, END }`
- **DockBuilder 레이아웃**: 최초 1회 자동 배치 (imgui.ini 없을 때), 이후 imgui.ini로 저장/복원
- **MenuBar**: Window 메뉴에서 패널 표시/숨기기 토글 (ToggleMenuItem 헬퍼 메서드)
- **ImGui 충돌**: `#define new DBG_NEW`가 ImGui/RTTR와 충돌 → `#undef new` 필요. RTTR registration.h는 Engine 헤더 전에 include하거나 `#undef new` 후 include
- **외부 라이브러리 격리**: Assimp(FBX→바이너리), RTTR(Inspector 리플렉션) 모두 Editor에만. Engine/Client 무의존
- **RTTR 세팅** (Layer 0 Step 3~4 완료):
  - 헤더: `Editor/Public/rttr/`, Lib: `Editor/ThirdPartyLib/RTTR/lib/Debug|Release/`, DLL: `Editor/Bin/rttr_core_d.dll`
  - 전처리기: `NOMINMAX` (모든 구성), 표준 준수 모드: `/permissive` (아니요) — RTTR 0.9.6 호환 필수
  - RTTR_Registration.cpp: CTransform, CGameObject 프로퍼티 등록 (멤버 함수 getter/setter 패턴)
  - RTTR getter/setter 규칙: getter 인자 0개, setter 인자 1개 (멤버 함수, this 암시적)
  - abstract 클래스(CTransform)는 등록 가능, private/protected 소멸자인 concrete 클래스(CTransform_3D/2D)는 등록 불가
- **CTransform _float3 접근자** (Layer 0 Step 4에서 추가):
  - `Get_Position()` / `Set_Position(_float3)` — WorldMatrix에서 Position 추출/설정
  - `Get_Rotation()` / `Set_Rotation(_float3)` — 오일러 각도(Degree) 추출/설정, Gimbal Lock 허용
  - `Set_ScaleF3(_float3)` — 기존 Set_Scale(3인자) 오버로드 대신 별도 이름 (RTTR 모호성 회피)
- **Editor_Function.h**: Editor inline 유틸리티 헤더 — Get_MonitorResolution, WTOA/ATOW, Log 시스템
- **Hierarchy**: TreeNode(레이어)→Selectable(오브젝트), 레벨 전환 감지→Clear_Selection
- **Inspector**: RTTR 프로퍼티 순회→타입별 위젯 자동 생성, 컴포넌트별 CollapsingHeader, Render_Property()
- **Log**: Editor 전용 정적 로그 버퍼 (Meyers' Singleton), LOG_ENTRY, 필터 토글/색상/자동 스크롤
- **마우스 피킹** (완료):
  - Engine: PICK_DATA 구조체, CVIBuffer::Pick() (TriangleTests::Intersects), CPU 정점 보존
  - Engine: CPipeLine::Compute_WorldRay() — Viewport→NDC→View→World Ray 변환, CGameInstance 래퍼
  - Engine: CGameObject에 CVIBuffer* m_pVIBufferCom 공통 멤버 (파생 shadowing 해결)
  - Editor: Panel_Viewport 클릭 감지 + WorldRay + 전체 오브젝트 순회 + 최소 거리 피킹
  - **Safe_Release 주의**: RefCount > 0이면 포인터를 nullptr로 만들지 않음 → 수동 nullptr 대입 필요
- **Editor 테스트 씬** (임시): CLevel_Editor (빈 CLevel 셸) + Ready_TestScene() (Camera_Free/Terrain/Light)
  - Client 게임 레벨은 CLIENT_DLL export 없음 → Editor 전용 빈 레벨로 대체
- **Phase (기초)**: ~~1.Viewport 분리~~ → ~~2.패널 구조~~ → ~~3.Content Browser~~ → ~~4.Inspector+RTTR~~ → 5.Model Converter+Assimp (진행 중)
- **Layer (전체)**: ~~0.공통 인프라(완료)~~ → 1.모델 파이프라인 (Engine 완료 / Editor Step 1~4,6 완료, Step 5: 5-A,5-B,5-D 완료, **5-C 구현 중**) → 2.3D 편집 → 3.UI 에디터 → 4.부가 기능(보류)
- **5-C CrossFade 구현 진행 상태**: Channel::Get_SQT 구현 완료. Animation::Update_SQT, Model::Play_Animation 블렌딩 분기, Model::Set_AnimationIndex 블렌드 트리거, Inspector 블렌딩 UI(SliderFloat+ProgressBar) — 코드 제시 완료, 사용자 적용 전
- **미해결 이슈 — 루트 모션 위치 동기화**: 본 애니메이션은 로컬 공간(BoneMatrix × WorldMatrix), CTransform 월드 위치는 애니메이션이 갱신 안 함. 루트 본 Translation 키프레임이 있으면 메쉬만 이동 → 전환 시 스냅백. 해결: 루트 모션 추출(매 프레임 루트 본 delta → CTransform 적용, 루트 본 원점 고정). 5-C와 독립 기능. 다음 세션에서 루트 본 Translation 유무 확인 후 결정
- **Layer 1 아키텍처 결정 (B안)**: Editor 만 Assimp 의존. FBX → MODEL_DESC → SLMD v1 바이너리(.bin) 저장 → Engine/Client 는 `CModel::Create(..., const _tchar* pBinaryPath)` 로 .bin 만 읽음. Engine 의 Assimp 의존 완전 제거
- **CModel 설계**:
  - `MODEL { NONANIM, ANIM }` 구분. MODEL_DESC 는 Mesh/Material/Bone/Animation 배열 pointer 컨테이너 (POD, 직렬화 친화)
  - `Ready_Meshes / Ready_Materials / Ready_Bones / Ready_Animations` 로 분할 초기화. 각 파생 클래스가 DESC 값을 복사 (Mesh 는 D3D11_USAGE_DEFAULT + pSysMem 내부 복사, Bone/Channel 는 memcpy/strcpy_s) → DESC free 안전
  - `Play_Animation(fTimeDelta)` 은 `_bool` 반환 (완료 여부). loop 여부는 `ANIMATION_DESC::bIsLoop` 에 저장 (SSOT, CModel 은 m_isAnimLoop 캐시 없음)
  - 바이너리 포맷 `SLMD` magic + version 1: Header → Meshes → Materials → Bones → Animations 순서. 텍스처 경로는 상대 `_char[MAX_PATH]` 문자열 (옵션 b)
  - `Load_Binary_Desc` 는 `new _ubyte[]` 로 정점 버퍼 할당 → `fread` → Initialize_Prototype 호출 → `Free_Binary_Desc` 로 역순 해제 (DBG_NEW 매크로 호환 위해 `::operator new` 금지)
- **CModelObject** (Client DLL): `MODELOBJECT_DESC` (ShaderProtoTag + ModelProtoTag) 기반 초기화, Update에서 ANIM이면 Play_Animation, Render에서 메시별 BoneMatrices/Material 바인딩 + 셰이더 Begin + Render
- **CModel_Converter** (Editor): Assimp aiScene → SLMD v1 .bin 변환 static 유틸리티. Collect_Bones(DFS), Write_Meshes/Materials/Bones/Animations, Should_Skip_Mesh(LowMesh/NLOD 필터), Build_TexturePath(상대경로), 0-bone 폴백, DIFFUSE 없을 때 _CO 텍스처 자동 탐색
- **Shader_VtxAnimMesh.hlsl**: VTXANIMMESH 입력 (BLENDINDEX uint4 + BLENDWEIGHT float4), 4본 스키닝 (가중합 BoneMatrix), Phong 라이팅 (Diffuse + Ambient + Specular)

## Working with Claude Code in This Repository

**Communication:** Korean and English are both acceptable.

**File Modification Policy:**
- **CRITICAL:** Claude Code must NEVER directly modify files using Edit/Write tools (encoding issues occur)
- **EXCEPTION:** CLAUDE.md file can be directly modified by Claude Code
- Always present code changes in markdown code blocks
- User will review and manually apply changes after understanding them

**명세서 관리:**
- 명세서 파일은 반드시 `명세서/` 폴더에 생성 (`C:\Users\chaho\ddoichaboom\Solo_Leveling\명세서\`)
- CLAUDE.md와 마찬가지로 명세서 파일(.md)은 Claude Code가 직접 생성/수정 가능

**SubAgent Usage:**
- Use SubAgents (Explore, Task, etc.) to maintain context and handle complex work
- Always review SubAgent outputs before presenting to user
- If output quality is insufficient, re-run the SubAgent task
