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
├── CPipeLine (final) — View/Proj 행렬 저장, 역행렬, 카메라 위치
├── CComponent (abstract) — Prototype/Clone
│   ├── CTransform (abstract) — world matrix, STATE 접근자, Bind_ShaderResource, _float3 기반 Position/Scale/Rotation 접근자
│   │   ├── CTransform_3D (final) — 3D 이동/회전/LookAt
│   │   └── CTransform_2D (final) — 2D 이동
│   ├── CShader (final) — FX11 Effect, Bind_Matrix/Bind_SRV/Bind_RawValue
│   ├── CTexture (final) — multi-SRV, DDS/WIC 로드. Clone 시 각 SRV Safe_AddRef 필수
│   └── CVIBuffer (abstract) — VB/IB, DrawIndexed
│       ├── CVIBuffer_Rect (final) — VTXTEX 쿼드
│       └── CVIBuffer_Terrain (final) — BMP heightmap, VTXNORTEX
├── CLevel (abstract)
├── CGameObject (abstract) — TRANSFORMTYPE으로 3D/2D Transform 자동 생성
│   ├── CCamera (abstract) — CAMERA_DESC, Update_PipeLine
│   └── CUIObject (abstract) — 직교 투영, CTransform_2D 자동
├── CLayer, CLevel_Manager, CTimer_Manager, CPrototype_Manager, CObject_Manager, CRenderer
├── CLight (final) — LIGHT_DESC (DIRECTIONAL/POINT). CBase 상속, CComponent 아님
├── CLight_Manager (final) — CGameInstance 소유, 비싱글톤
│
├── [Client.dll]
├── CMainApp, CBackGround, CCamera_Free, CTerrain (CLIENT_DLL export)
├── CLevel_Logo, CLevel_Loading, CLevel_GamePlay, CLoader (internal)
│   └── CLevel_GamePlay: Initialize() 비어있음 (오브젝트 미생성 상태)
│
├── [Editor.exe]
├── CEditorApp (final) — ImGui DockSpace + MenuBar + DockBuilder 레이아웃, Ready_Panels/ToggleMenuItem/Render_Scene
├── CPanel (abstract) — Initialize/Update/Render, Is_Open/Set_Open/Get_Name
│   ├── CPanel_Viewport (final) — "Viewport", 별도 RenderTarget 소유 (RTV/SRV/DSV), Begin_RT/End_RT, 리사이즈 대응
│   ├── CPanel_Hierarchy (final) — "Hierarchy" (셸)
│   ├── CPanel_Inspector (final) — "Inspector" (셸)
│   ├── CPanel_ContentBrowser (final) — "Content Browser", std::filesystem 기반 Resources/ 탐색, Breadcrumb/파일타입 아이콘
│   └── CPanel_Log (final) — "Log" (셸)
└── CPanel_Manager (final, singleton) — map<wstring, CPanel*>, Update/Render_Panels
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
| VTXMESH | Shader_VtxMesh.hlsl | Model (향후) |

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
│   ├── Public/                     — 33 headers
│   │   ├── Engine_Defines/Enum/Function/Macro/Struct/Typedef.h
│   │   ├── Base, GameInstance, Graphic_Device, Input_Device, PipeLine
│   │   ├── Timer, Timer_Manager, Level, Level_Manager
│   │   ├── GameObject, UIObject, Camera, Layer, Object_Manager, Prototype_Manager, Renderer
│   │   ├── Component, Transform, Transform_3D, Transform_2D
│   │   ├── Shader, Texture, VIBuffer, VIBuffer_Rect, VIBuffer_Terrain
│   │   ├── Light, Light_Manager
│   │   ├── fx11/, DirectXTK/
│   │   └── Engine_Struct.h         — ENGINE_DESC, LIGHT_DESC, VTXTEX, VTXNORTEX, VTXMESH
│   ├── Private/                    — 27 cpp (Public와 1:1 대응)
│   ├── ThirdPartyLib/              — Effects11, DirectXTK (Debug/Release)
│   └── Bin/
│
├── Client/
│   ├── Public/                     — 9 headers (Client_Defines, MainApp, BackGround, Camera_Free, Terrain, Level_Logo/Loading/GamePlay, Loader)
│   ├── Private/                    — 8 cpp
│   └── Bin/
│
├── GameApp/Default/                — GameApp.cpp (wWinMain, 60FPS loop), GameApp_Defines.h
├── Editor/
│   ├── Default/Editor.cpp          — wWinMain + ImGui WndProc, WS_OVERLAPPEDWINDOW + SW_MAXIMIZE
│   ├── Public/                     — EditorApp.h, Editor_Defines.h, Editor_Enum.h
│   │                                 Panel.h, Panel_Manager.h
│   │                                 Panel_Viewport.h, Panel_Hierarchy.h, Panel_Inspector.h
│   │                                 Panel_ContentBrowser.h, Panel_Log.h
│   ├── Private/                    — EditorApp.cpp, Panel.cpp, Panel_Manager.cpp
│   │                                 Panel_Viewport.cpp, Panel_Hierarchy.cpp, Panel_Inspector.cpp
│   │                                 Panel_ContentBrowser.cpp, Panel_Log.cpp
│   │                                 RTTR_Registration.cpp
│   ├── ImGui/                      — docking 브랜치 + backends/ (DX11, Win32)
│   └── ImGuizmo/
│
├── Resources/
│   ├── ShaderFiles/                — Shader_VtxTex, Shader_VtxNorTex, Shader_VtxMesh (.hlsl)
│   ├── Textures/                   — Default, Terrain/, Logo/, Player/, SkyBox/, Explosion/
│   └── Models/                     — Fiona/, ForkLift/, map/, Rock/, Test/, Tong/ (FBX, Editor에서 변환 예정)
│
├── EngineSDK/, ClientSDK/          — SDK 배포 (bat으로 자동 복사)
├── 명세서/
│   ├── 개발_진행_가이드.md           — Phase A~G 이식 가이드
│   └── Editor_ImGui_구현계획.md     — Editor Phase 1~5 계획
└── Framework.sln
```

## Important Implementation Notes

### DLL Export
- `ENGINE_DLL` / `CLIENT_DLL`: Engine_Macro.h 정의, `_EXPORTS` 전처리기로 export/import 전환
- Client 전역변수 없음: `g_hWnd` 등은 CGameInstance가 관리, GameApp.cpp에서만 전역

### 핵심 주의사항 (Gotchas)
- **m_WorldMatrix 초기화**: 반드시 Identity. 영행렬이면 WVP 변환 시 렌더링 안됨
- **CTexture Clone**: 복사 생성자에서 각 SRV에 `Safe_AddRef()` 필수. 미적용 시 combase.dll 크래시
- **InputLayout 매칭**: 정점 구조체와 D3D11_INPUT_ELEMENT_DESC 1:1 대응 필수 (불일치 시 크래시)
- **Level Transition**: `SUCCEEDED()` 사용, `FAILED()` 아님
- **CLoader 스레드**: WIC 텍스처 로딩 시 `CoInitializeEx(nullptr, 0)` 필수
- **Release 순서**: CMainApp — Device → Context → Release_Engine → GameInstance
- **WM_INPUT**: WndProc에서 `CGameInstance::Process_RawInput(lParam)` 호출

### Runtime Resource Paths
- 런타임 파일 I/O (프로젝트 속성에 Resources/ 추가 불필요)
- 경로 패턴: `TEXT("../../Resources/ShaderFiles/...")`, `TEXT("../../Resources/Textures/...")`

### Editor Implementation
- **기초 구현 계획**: `명세서/Editor_ImGui_구현계획.md` (Phase 1~5, 패널/Viewport 기초)
- **전체 구현 계획**: `명세서/Editor_전체_구현계획.md` (Layer 0~4, 아키텍처 결정사항, 배치/UI/모델 파이프라인)
- **현재 상태**: Phase 1~3 완료 + Layer 0 Step 1~4 완료. RTTR 세팅 + 등록 완료, Transform _float3 접근자 추가
- **패널 시스템**: CPanel (abstract) → 파생 5개 (Viewport, Hierarchy, Inspector, ContentBrowser, Log)
  - `CPanel_Manager` (싱글톤, `map<wstring, CPanel*>`): 이름 기반 접근, Update_Panels + Render_Panels
  - 자주 접근하는 패널은 멤버 포인터로 캐싱 (예: `m_pViewport`)
  - 패널 소유권: Manager가 유일 소유자 (Add_Panel 시 AddRef 안함, Free에서 Release)
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
- **Editor_Enum.h**: `MENUTYPE { PANEL, TOOL, END }` — ToggleMenuItem에서 메뉴 타입 분기용
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
- **Phase (기초)**: ~~1.Viewport 분리~~ → ~~2.패널 구조~~ → ~~3.Content Browser~~ → 4.Inspector+RTTR → 5.Model Converter+Assimp
- **Layer (전체)**: 0.공통 인프라(Step 1~4 완료, Step 5~8 잔여) → 1.모델 파이프라인(Assimp) → 2.3D 편집 → 3.UI 에디터 → 4.부가 기능(보류)

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
