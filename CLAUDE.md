# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build System

- **Visual Studio 2022** solution: `Framework.sln` (MSBuild, v143 toolset)
- Four projects: **Engine** (DLL), **Client** (DLL), **GameApp** (EXE), **Editor** (EXE)
- Dependency chain: Engine ← Client ← GameApp / Editor
- Primary configuration: Debug|x64 (Win32 configurations exist but settings are incomplete)
- Build order: Engine → Client → GameApp / Editor
- Post-build events execute `EngineSDK_Update.bat` (Engine) and `ClientSDK_Update.bat` (Client)
- Build from VS or command line: `msbuild Framework.sln /p:Configuration=Debug /p:Platform=x64`
- Output: `Engine/Bin/Engine.dll`, `Client/Bin/Client.dll`, `GameApp/Bin/GameApp.exe`

## Architecture

**4-Project separation:** Engine.dll provides core 3D framework. Client.dll contains game logic (MainApp, Levels, GameObjects). GameApp.exe is a thin entry point (window creation + game loop). Editor.exe is the ImGui-based editor tool (기본 구현 완료, WS_OVERLAPPEDWINDOW + SW_MAXIMIZE 최대화 창).

**Why Client is DLL (not EXE):** In the previous SR_Project (3-project: Engine.dll + Client.exe + Editor.exe), Editor couldn't use Client's GameObjects directly, requiring code duplication. By making Client a DLL, both GameApp.exe and Editor.exe can share the same game objects.

```
Engine.dll  ←  Client.dll  ←  GameApp.exe
                            ←  Editor.exe
```

**SDK distribution:** Engine headers/lib → `EngineSDK/` (via EngineSDK_Update.bat). Client headers/lib → `ClientSDK/` (via ClientSDK_Update.bat). Downstream projects reference SDK folders, not source folders directly.

**Class hierarchy:** All classes inherit from `CBase`, which provides COM-style reference counting (AddRef/Release). Never use raw `delete`; always use `Safe_Release`.

```
CBase (ref counting)
├── CGameInstance (singleton) — central coordinator, stores HWND/WinSize from ENGINE_DESC, OnResize 래퍼
├── CGraphic_Device — D3D11 device, context, swap chain initialization, OnResize (SwapChain/RTV/DSV/Viewport 재생성)
├── CComponent (abstract) — base for components, holds Device/Context, supports prototype/clone
│   ├── CTransform (final) — world matrix, STATE-based accessors, movement/rotation/scale/LookAt, Bind_ShaderResource
│   ├── CShader (final) — FX11 Effect shader, per-pass InputLayout, Begin(), Bind_Matrix/Bind_SRV
│   ├── CTexture (final) — multi-texture SRV management, DDS/WIC loading via DirectXTK, shader binding
│   └── CVIBuffer (abstract) — vertex/index buffer management, IA binding, DrawIndexed rendering
│       └── CVIBuffer_Rect (final) — quad geometry (4 vertices, 6 indices, VTXTEX format)
├── CLevel (abstract) — base for game levels (Logo, Loading, GamePlay)
├── CGameObject (abstract) — base for game entities, auto-creates CTransform, supports Clone pattern
├── CLayer — groups GameObjects at the same depth, delegates update/render
├── CLevel_Manager (singleton) — manages current level and transitions
├── CTimer_Manager (singleton) — named timers with delta time
├── CPrototype_Manager (singleton) — prototype registry, clones GameObjects by key
├── CObject_Manager (singleton) — level/layer-based GameObject lifecycle management
├── CRenderer (singleton-like) — render group management (Priority/NonBlend/Blend/UI)
│
├── [Client.dll]
├── CMainApp (CLIENT_DLL export) — engine init with HWND/size params, update/render delegation
├── CBackGround (CLIENT_DLL export) — background game object, Shader+Texture+VIBuffer rendering, BACKGROUND_DESC
├── CLevel_Logo (internal) — logo display, Enter key → loading transition
├── CLevel_Loading (internal) — threaded loading, Space key → next level
├── CLevel_GamePlay (internal) — gameplay stub
├── CLoader (internal) — multi-threaded asset loading (HANDLE + CRITICAL_SECTION + CoInitializeEx for WIC)
│
├── [Editor.exe] (기본 구현 완료)
└── CEditorApp (internal) — CBase 상속, Engine 초기화 + ImGui DX11 통합, DockSpace 기반 UI
```

**Key patterns:**
- **Singletons** via `DECLARE_SINGLETON` / `IMPLEMENT_SINGLETON` macros
- **Prototype/Clone** for GameObjects (register prototypes, clone instances)
- **Component system**: CComponent abstract base → CTransform (직접 생성) + CShader / CTexture / CVIBuffer 계열 (Prototype/Clone)
- **Descriptor chain**: TRANSFORM_DESC → GAMEOBJECT_DESC → client-specific DESC (e.g., BACKGROUND_DESC)
- **Threaded loading** via CLoader for level transitions
- **HRESULT error handling** with `FAILED_CHECK` / `NULL_CHECK` macros
- **3-stage update pipeline**: Priority_Update → Update → Late_Update
- **Render group system**: GameObjects register in Late_Update, CRenderer draws in order
- **Shader render pipeline**: Bind_ShaderResources() (WVP+Texture) → Shader::Begin(passIndex) → VIBuffer::Bind_Resources() → VIBuffer::Render()
- **DLL export**: `ENGINE_DLL` for Engine classes, `CLIENT_DLL` for Client export classes

**Execution flow:** `GameApp.cpp` wWinMain → window creation → `CMainApp::Create(hWnd, sizeX, sizeY)` → `CGameInstance::Initialize_Engine(ENGINE_DESC)` → PeekMessage game loop (60 FPS) → `CMainApp::Update/Render` → Shutdown: `CGameInstance::Release_Engine()` → `DestroyInstance()`.

## Coding Conventions

- Class names: `C` prefix (`CBase`, `CGameInstance`)
- Member variables: `m_` prefix (`m_pDevice`, `m_iRefCnt`)
- Methods: PascalCase (`Initialize`, `Priority_Update`, `Late_Update`)
- Namespaces: `Engine`, `Client` via `NS_BEGIN` / `NS_END` macros
- Singleton classes use `final` keyword
- Unicode throughout (wchar_t, `TEXT()`)
- Return `HRESULT` from initialization and setup functions
- Use `Safe_Delete`, `Safe_Release`, `Safe_AddRef` templates from `Engine_Function.h`
- Enum casting via `ETOI` (to int) / `ETOUI` (to unsigned int) macros

## Project File Tree

```
Framework/
├── Engine/                         — Core engine DLL project
│   ├── Default/
│   │   └── Engine.vcxproj          — DLL, ENGINE_EXPORTS, links d3d11.lib/dxguid.lib/Effects11d.lib
│   ├── Public/                     — Engine header files (24 files)
│   │   ├── Engine_Defines.h        — Master header: D3D11, DirectXMath, DirectXCollision, fx11/d3dx11effect.h, DirectXTK (DDS/WIC), STL includes, debug memory tracking
│   │   ├── Engine_Enum.h           — Enums: WINMODE, PROTOTYPE, RENDERID, STATE
│   │   ├── Engine_Function.h       — Template utilities: Safe_Delete, Safe_Release, Safe_AddRef
│   │   ├── Engine_Macro.h          — Macros: ETOI/ETOUI, SINGLETON, ENGINE_DLL, CLIENT_DLL, PURE
│   │   ├── Engine_Struct.h         — ENGINE_DESC, VTXTEX struct (XMFLOAT3 vPosition + XMFLOAT2 vTexcoord)
│   │   ├── Engine_Typedef.h        — Type aliases: _float, _int, _uint, _float2/3/4, _float4x4, _vector/_fvector, _matrix/_fmatrix
│   │   ├── Base.h                  — Abstract base class: reference counting (AddRef/Release)
│   │   ├── GameInstance.h          — Singleton: engine init, Get_hWnd/WinSizeX/WinSizeY, WINMODE tracking (Get/Set_WinMode), OnResize
│   │   ├── Graphic_Device.h        — D3D11 device, swap chain, render target, depth stencil
│   │   ├── Timer.h                 — High-resolution timer via QueryPerformanceCounter
│   │   ├── Timer_Manager.h         — Named timer map manager
│   │   ├── Level.h                 — Abstract base level class
│   │   ├── Level_Manager.h         — Level transition and current level management
│   │   ├── GameObject.h            — Abstract game object: GAMEOBJECT_DESC, prototype/clone
│   │   ├── Layer.h                 — Groups GameObjects at same depth
│   │   ├── Object_Manager.h        — Level/layer-indexed GameObject lifecycle manager
│   │   ├── Prototype_Manager.h     — Level-indexed prototype storage, type-aware cloning
│   │   ├── Renderer.h              — Render group manager: Priority/NonBlend/Blend/UI
│   │   ├── Component.h             — Abstract component base: Device/Context refs, clone support
│   │   ├── Transform.h             — Transform component: world matrix, Get/Set_State, Scale/Move/Rotate/LookAt, Bind_ShaderResource
│   │   ├── Shader.h                — FX11 Effect shader component: compile HLSL, per-pass InputLayout, Begin(), Bind_Matrix(), Bind_SRV()
│   │   ├── Texture.h               — Texture component: multi-SRV management, DDS/WIC file loading, shader binding
│   │   ├── VIBuffer.h              — Abstract geometry component: vertex/index buffer, IA binding, DrawIndexed
│   │   └── VIBuffer_Rect.h         — Concrete quad geometry: 4 vertices + 6 indices, VTXTEX format
│   ├── Private/                    — Engine implementation files (18 files)
│   │   ├── Base.cpp, GameInstance.cpp, Graphic_Device.cpp
│   │   ├── Timer.cpp, Timer_Manager.cpp
│   │   ├── Level.cpp, Level_Manager.cpp
│   │   ├── GameObject.cpp, Component.cpp, Transform.cpp
│   │   ├── Layer.cpp, Object_Manager.cpp, Prototype_Manager.cpp
│   │   ├── Renderer.cpp
│   │   ├── Shader.cpp              — FX11 Effect compile, InputLayout creation per pass, Begin(), Bind_Matrix/Bind_SRV
│   │   ├── Texture.cpp             — Texture loading (DDS/WIC format branch), SRV vector management, Clone with AddRef
│   │   ├── VIBuffer.cpp            — Geometry buffer: Bind_Resources(), Render(DrawIndexed)
│   │   └── VIBuffer_Rect.cpp       — Quad geometry: 4-vertex/6-index buffer creation
│   ├── Public/fx11/                — DirectX Effects 11 headers
│   │   ├── d3dx11effect.h          — Effect system interface
│   │   └── d3dxGlobal.h            — Global definitions
│   ├── Public/DirectXTK/           — DirectXTK headers for texture loading
│   │   ├── DDSTextureLoader.h      — DDS format texture loader
│   │   └── WICTextureLoader.h      — WIC format loader (JPG, PNG, BMP)
│   ├── ThirdPartyLib/              — Third-party precompiled libraries
│   │   ├── Effects11d.lib          — DirectX Effects 11 (Debug)
│   │   ├── Effects11.lib           — DirectX Effects 11 (Release)
│   │   ├── DirectXTKd.lib          — DirectXTK (Debug)
│   │   └── DirectXTK.lib           — DirectXTK (Release)
│   └── Bin/                        — Build output: Engine.dll, Engine.lib, Engine.pdb
│
├── Client/                         — Game logic DLL project (기존 EXE → DLL 전환)
│   ├── Default/
│   │   ├── Client.vcxproj          — DLL, CLIENT_EXPORTS, links Engine.Lib
│   │   └── Client_Defines.h        — LEVEL enum (STATIC/LOADING/LOGO/GAMEPLAY/END), 전역변수 없음
│   ├── Public/                     — Client header files (6 files)
│   │   ├── MainApp.h               — CLIENT_DLL export, Initialize(HWND, sizeX, sizeY)
│   │   ├── BackGround.h            — CLIENT_DLL export, BACKGROUND_DESC
│   │   ├── Level_Logo.h            — Internal: logo display, Enter key transition
│   │   ├── Level_Loading.h         — Internal: threaded loading, Space key transition
│   │   ├── Level_GamePlay.h        — Internal: gameplay stub
│   │   └── Loader.h                — Internal: multi-threaded asset loader
│   ├── Private/                    — Client implementation files (6 files)
│   │   ├── MainApp.cpp             — Engine init, Ready_Prototype_For_Static (VIBuffer_Rect + Shader_VtxTex), Start_Level
│   │   ├── BackGround.cpp          — Ready_Components (Shader+Texture+VIBuffer clone), Bind_ShaderResources (WVP+Texture), Render pipeline
│   │   ├── Level_Logo.cpp          — Ready_Layer_BackGround, Enter → Loading transition
│   │   ├── Level_Loading.cpp       — Spawns CLoader thread, Space → next level transition
│   │   ├── Level_GamePlay.cpp      — Gameplay placeholder
│   │   └── Loader.cpp              — Worker thread, CoInitializeEx for WIC, registers Texture + BackGround prototypes
│   └── Bin/                        — Build output: Client.dll, Client.lib (+ Engine.dll copied)
│
├── GameApp/                        — Main application EXE project (얇은 진입점)
│   ├── Default/
│   │   ├── GameApp.vcxproj         — EXE, links Engine.Lib + Client.Lib
│   │   ├── GameApp_Defines.h       — Window size 1280×720, extern HWND g_hWnd
│   │   ├── GameApp.cpp             — wWinMain, window creation, PeekMessage 60FPS loop, WM_SIZE→OnResize
│   │   ├── GameApp.h, framework.h, targetver.h, Resource.h
│   │   ├── GameApp.rc, GameApp.ico, small.ico
│   │   └── (Public/, Private/ — 비어있음, GameApp은 자체 클래스 없음)
│   └── Bin/                        — Build output: GameApp.exe (+ Engine.dll, Client.dll copied)
│
├── Editor/                         — Editor tool EXE project (ImGui 기반, 기본 구현 완료)
│   ├── Default/
│   │   ├── Editor.vcxproj          — EXE, links Engine.Lib + Client.Lib + d3d11.lib + dxguid.lib + dxgi.lib
│   │   ├── Editor.cpp              — wWinMain (GameApp 패턴 + ImGui WndProc, WS_OVERLAPPEDWINDOW + SW_MAXIMIZE, WM_SIZE→OnResize)
│   │   ├── imgui.ini               — ImGui 레이아웃 설정 (자동 생성)
│   │   ├── Editor.h, framework.h, targetver.h, Resource.h
│   │   └── Editor.rc, Editor.ico, small.ico
│   ├── Public/
│   │   ├── EditorApp.h             — CBase 상속, ImGui 초기화/DockSpace/렌더 메서드
│   │   └── Editor_Defines.h        — extern HWND g_hWnd, Get_MonitorResolution() (DXGI inline)
│   ├── Private/
│   │   └── EditorApp.cpp           — Engine 초기화 + ImGui DX11 백엔드 + DockSpace 렌더 루프
│   ├── ImGui/                      — ImGui docking 브랜치 소스 (루트 11개 파일)
│   │   └── backends/               — DX11/Win32 백엔드 (4개 파일: impl_dx11, impl_win32)
│   ├── ImGuizmo/                   — ImGuizmo master (ImGuizmo.h, ImGuizmo.cpp)
│   └── Bin/                        — Build output: Editor.exe (+ DLLs copied)
│
├── Resources/                      — Shared game resources (런타임 로드, 프로젝트 속성 불필요)
│   ├── ShaderFiles/
│   │   └── Shader_VtxTex.hlsl      — VTXTEX shader: WVP transform, texture2D sampling, sampler state, alpha discard
│   └── Textures/                   — Texture resource files
│       ├── Default0.jpg, Default1.JPG — BackGround 테스트용 기본 텍스처
│       ├── Logo/, Player/, SkyBox/  — 카테고리별 텍스처 폴더
│       └── Explosion/               — 이펙트 스프라이트 시퀀스 (90장)
│
├── EngineSDK/                      — Engine SDK distribution
│   ├── Inc/                        — Copied Engine headers (via EngineSDK_Update.bat, includes Shader/VIBuffer/Texture headers + DirectXTK/)
│   └── Lib/                        — Copied Engine.lib
│
├── ClientSDK/                      — Client SDK distribution
│   ├── Inc/                        — Copied Client headers (via ClientSDK_Update.bat)
│   └── Lib/                        — Copied Client.lib
│
├── 명세서/                          — 구현 명세서 (Claude Code 세션 간 작업 연속성)
│   ├── Editor_Plan.md              — Editor 세팅 전체 계획 (폴더구조, 클래스설계, vcxproj설정, ImGui파일목록)
│   └── Editor_수정안.md             — Editor 수정안 (DXGI 모니터 해상도, WS_POPUP 보더리스 전체화면)
│
├── EngineSDK_Update.bat            — Engine post-build: headers→EngineSDK, dll→Client/GameApp/Editor, lib→EngineSDK
├── ClientSDK_Update.bat            — Client post-build: headers→ClientSDK, dll→GameApp/Editor, lib→ClientSDK
└── Framework.sln                   — Visual Studio 2022 solution file (4 projects)
```

## Important Implementation Notes

### 4-Project DLL Architecture
- **Engine.dll**: 엔진 코어. ENGINE_DLL로 export. 모든 싱글톤의 IMPLEMENT_SINGLETON이 Engine.dll 내부에 위치
- **Client.dll**: 게임 로직. CMainApp, CBackGround 등 외부 사용 클래스만 CLIENT_DLL로 export
- **GameApp.exe**: 윈도우 생성 + 게임루프만 담당. CMainApp::Create() 호출하는 얇은 진입점
- **Editor.exe**: ImGui/ImGuizmo 기반 에디터 도구. Client.dll의 GameObject를 직접 Clone하여 사용

### DLL Export 전략
- `ENGINE_DLL`: Engine_Macro.h에 정의. ENGINE_EXPORTS 전처리기로 export/import 전환
- `CLIENT_DLL`: Engine_Macro.h에 정의. CLIENT_EXPORTS 전처리기로 export/import 전환
- Export 대상: CMainApp, CBackGround (및 향후 추가되는 CGameObject 파생 클래스)
- Export 제외 (내부용): CLevel_Logo, CLevel_Loading, CLevel_GamePlay, CLoader

### CRT 통일 (필수)
- 3개 DLL/EXE 프로젝트 모두 동적 CRT(`/MDd` Debug, `/MD` Release)로 통일 필수
- 하나라도 `/MT`(정적 CRT) 사용 시 DLL 경계에서 `delete this` 힙 충돌 발생

### 전역변수 제거 (기존 Framework 대비 변경점)
- `g_hWnd`, `g_iWinSizeX`, `g_iWinSizeY`는 Client에서 완전히 제거됨
- `CGameInstance`가 `Initialize_Engine()` 시점에 저장하고 `Get_hWnd()`, `Get_WinSizeX()`, `Get_WinSizeY()`로 제공
- `CMainApp::Initialize(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY)` — 매개변수로 전달받음
- `g_hWnd`는 GameApp.cpp에서만 전역으로 존재 (윈도우 생성 시 설정)

### ENGINE_DESC Initialization
CMainApp::Initialize()에서 매개변수를 통해 설정:
```cpp
HRESULT CMainApp::Initialize(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY)
{
    ENGINE_DESC EngineDesc{};
    EngineDesc.hWnd = hWnd;
    EngineDesc.eWinMode = WINMODE::WIN;
    EngineDesc.iViewportWidth = iWinSizeX;
    EngineDesc.iViewportHeight = iWinSizeY;
    EngineDesc.iNumLevels = ETOUI(LEVEL::END);  // CRITICAL: Required for Prototype_Manager
    ...
}
```

### Memory Management
- **Release order**: Device before Context (CMainApp::Free 기준)
- `CMainApp::Free()`: m_pDevice → m_pContext → Release_Engine() → m_pGameInstance
- `CEditorApp::Free()`: ShutDown_ImGui → m_pDevice → m_pContext → Release_Engine → m_pGameInstance
- Always use `Safe_Release()` — never raw `delete` on COM objects

### Level Transitions
- Use `SUCCEEDED()` when checking level change results, not `FAILED()`
```cpp
if (GetKeyState(VK_RETURN) & 0x8000) {
    if (SUCCEEDED(m_pGameInstance->Change_Level(...)))
        return;
}
```

### Build & SDK Distribution Flow
1. Engine.dll 빌드 → `EngineSDK_Update.bat` 실행 (헤더→EngineSDK/Inc, dll→Client/GameApp/Editor Bin, lib→EngineSDK/Lib)
2. Client.dll 빌드 → `ClientSDK_Update.bat` 실행 (헤더→ClientSDK/Inc, dll→GameApp/Editor Bin, lib→ClientSDK/Lib)
3. Client\Bin\에 Engine.dll이 이미 있으므로 `*.dll` 복사 시 Engine.dll + Client.dll 모두 전달됨
4. GameApp.exe / Editor.exe 빌드 → Bin에 이미 DLL 복사 완료

### Debug-Only Code
- `#ifdef _DEBUG` / `#endif`로 감싸기
- `CLoader::Show()`, Level Render()의 SetWindowText 등
- GameApp.cpp의 `_CrtSetDbgFlag` 메모리 누수 감지

### Render Group System
- GameObjects register in `Late_Update()` via `m_pGameInstance->Add_RenderGroup(RENDERID, this)`
- CRenderer draws in order: Priority → NonBlend → Blend → UI
- Each `Add_RenderGroup()` does `Safe_AddRef()`; each render pass does `Safe_Release()` after drawing

### Component System & Descriptor Chain
- `CComponent` (abstract): Device/Context 보유, Prototype/Clone 패턴 지원
- `CTransform` (final): 월드 행렬, STATE 기반 행/열 접근 (Get/Set_State), 이동(Go_Straight/Backward/Left/Right), 회전(Rotation/Turn), 스케일(Set_Scale/Scaling), LookAt. Bind_ShaderResource()로 월드 행렬을 셰이더에 전달. CGameObject::Initialize()에서 직접 Create (Prototype_Manager 미사용). **주의: m_WorldMatrix 반드시 Identity로 초기화 필요** (WVP 변환 시 영행렬이면 렌더링 안됨)
- `CShader` (final): FX11 Effect 기반 셰이더 컴포넌트. HLSL 파일 → D3DX11CompileEffectFromFile → ID3DX11Effect. Technique의 각 Pass별 InputLayout 생성. Begin(passIndex)로 IASetInputLayout + Pass::Apply. **Bind_Matrix()**: Effect 변수에 행렬 전달 (AsMatrix→SetMatrix). **Bind_SRV()**: Effect 변수에 텍스처 SRV 전달 (AsShaderResource→SetResource). 복사 생성자로 Effect/InputLayout 공유 (Safe_AddRef). Prototype/Clone 패턴 지원
- `CTexture` (final): 다중 텍스처 SRV 관리 컴포넌트. Initialize_Prototype(filePath, numTextures)에서 확장자별 분기 로드 (`.dds`→CreateDDSTextureFromFile, `.jpg/.png/.bmp`→CreateWICTextureFromFile, wsprintf로 `%d` 치환). Bind_ShaderResource(pShader, name, index)로 특정 인덱스 SRV를 셰이더에 바인딩. **복사 생성자에서 각 SRV에 Safe_AddRef 필수** (미적용 시 종료 크래시). Prototype/Clone 패턴 지원
- `CVIBuffer` (abstract): 정점/인덱스 버퍼 관리. Bind_Resources() (IA 세팅), Render() (DrawIndexed). 복사 생성자로 버퍼 공유 (Safe_AddRef)
- `CVIBuffer_Rect` (final): 4정점 + 6인덱스 쿼드. VTXTEX 포맷 (Position + Texcoord). D3D11_USAGE_DEFAULT. Prototype/Clone으로 버퍼 인스턴스 공유
- Descriptor 상속 체인: `TRANSFORM_DESC` → `GAMEOBJECT_DESC` (+ iFlag) → `BACKGROUND_DESC` (Client측 확장)

### Shader & Render Pipeline
- **HLSL 파일**: `Resources/ShaderFiles/Shader_VtxTex.hlsl` — VTXTEX용. `texture2D g_Texture` + `sampler DefaultSampler` (LINEAR/wrap). VS_MAIN: WVP 변환 적용 (World→View→Proj). PS_MAIN: `g_Texture.Sample()` 텍스처 샘플링, `discard` 알파 테스트 (a < 0.1f)
- **FX11 technique11**: DefaultTechnique → DefaultPass (vs_5_0, ps_5_0)
- **프로토타입 등록**: `CMainApp::Ready_Prototype_For_Static()`에서 STATIC 레벨에 VIBuffer_Rect + Shader_VtxTex 프로토타입 등록
- **컴포넌트 클론**: `CBackGround::Ready_Components()`에서 Shader/VIBuffer(STATIC) + Texture(LOGO) 클론
- **렌더 순서**: CBackGround::Render() → `Bind_ShaderResources()` (WVP행렬 + 텍스처 바인딩) → `Begin(0)` → `Bind_Resources()` → `Render()`
- **Bind_ShaderResources 흐름**: Transform→Bind_ShaderResource("g_WorldMatrix") → Shader::Bind_Matrix("g_ViewMatrix"/"g_ProjMatrix", Identity) → Texture→Bind_ShaderResource("g_Texture", index)
- **InputLayout 매칭**: D3D11_INPUT_ELEMENT_DESC 배열 (POSITION + TEXCOORD)을 Shader::Create() 시 전달, VTXTEX 구조체와 1:1 대응 필수

### Prototype Registration & Object Creation
- Register prototypes in `CLoader::Ready_Resources_For_XXX()` via `Add_Prototype(levelIndex, tag, Object::Create(...))`
- Static component prototypes (VIBuffer_Rect, Shader_VtxTex) are registered in `CMainApp::Ready_Prototype_For_Static()`
- Level-specific component prototypes (Texture_BackGround) are registered in `CLoader::Ready_Resources_For_Logo()`
- Create instances in Level's `Ready_Layer_XXX()` via `Add_GameObject(protoLevel, protoTag, layerLevel, layerTag)`
- `Clone_Prototype()` uses `PROTOTYPE` enum to dispatch: `GAMEOBJECT` → `CGameObject::Clone()`, `COMPONENT` → `CComponent::Clone()`

### Window Resize (OnResize)
- `WM_SIZE` → `CGameInstance::OnResize(width, height)` → `CGraphic_Device::OnResize(width, height)`
- CGraphic_Device::OnResize 처리 순서: OMSetRenderTargets 해제 → RTV/DSV Release → SwapChain::ResizeBuffers → RTV/DSV 재생성 → 재바인딩 → Viewport 갱신
- **Same-size skip guard**: CGameInstance::OnResize에서 `m_iWinSizeX == iWinSizeX && m_iWinSizeY == iWinSizeY`면 S_OK 즉시 반환 (중복 SwapChain 재생성 방지, 최대화 버튼 등에서 WM_SIZE + WM_EXITSIZEMOVE 동시 발생 시 유효)
- CGameInstance::OnResize에서 `m_pGraphic_Device` null 체크 필수 (윈도우 생성 시점에 WM_SIZE가 먼저 발생, 엔진 초기화 전)
- WndProc에서 `SIZE_MINIMIZED` 및 크기 0은 skip, GetInstance()로 싱글톤 접근 (AddRef 불필요)
- GameApp.cpp WndProc: WM_SIZE 핸들러 현재 주석 처리 상태 (드래그 리사이즈 시 흰색 배경 이슈 조사 중 보류)

### WINMODE Tracking
- `CGameInstance`에 `m_eWinMode` 멤버 추가 (WINMODE::WIN 기본값)
- `Initialize_Engine()`에서 `m_eWinMode = EngineDesc.eWinMode` 저장
- `Get_WinMode()` / `Set_WinMode(WINMODE)` 접근자 제공
- 향후 Set_WinMode() 확장 예정: 보더리스 전체화면 토글 (SetWindowLongPtr + SetWindowPos + OnResize), Alt+Enter 단축키 연동

### Texture System (CTexture + DirectXTK)
- **DirectXTK 연동**: `Engine/Public/DirectXTK/` 헤더 + `Engine/ThirdPartyLib/DirectXTKd.lib`(Debug)/`DirectXTK.lib`(Release)
- **Engine_Defines.h**에 `#include <DirectXTK/DDSTextureLoader.h>`, `#include <DirectXTK/WICTextureLoader.h>` 추가
- **파일 포맷 분기**: `_wsplitpath_s`로 확장자 추출 → `.dds`(CreateDDSTextureFromFile) / `.jpg .png .bmp`(CreateWICTextureFromFile) / `.tga`(미지원)
- **다중 텍스처**: `wsprintf(path, format, i)`로 `%d` 치환하여 순차 로드 (예: `Default%d.jpg` → Default0.jpg, Default1.jpg)
- **COM 초기화**: WIC는 COM 기반이므로 로딩 스레드에서 `CoInitializeEx(nullptr, 0)` / `CoUninitialize()` 필수 (CLoader::Loading()에서 호출)
- **Clone 시 주의**: 복사 생성자에서 SRV 벡터 복사 후 각 SRV에 `Safe_AddRef()` 필수. 미적용 시 이중 해제로 combase.dll 크래시 발생

### Runtime Resource Paths
- `Resources/` 폴더의 HLSL, 텍스처 등은 **런타임에 파일 경로로 직접 로드** (D3DX11CompileEffectFromFile, CreateWICTextureFromFile 등)
- **프로젝트 속성(Additional Include Directories)에 Resources/ 추가 불필요** — 컴파일 타임 #include가 아닌 런타임 파일 I/O
- 상대 경로 기준: 실행 파일 작업 디렉토리 (GameApp/Bin/ 또는 VS의 $(ProjectDir))
- 경로 패턴: `TEXT("../../Resources/ShaderFiles/...")`, `TEXT("../../Resources/Textures/...")`
- GameApp/Bin/ 및 GameApp/Default/ 모두 `../../` = `Framework/`로 동일하게 해석됨

### Editor Implementation (기본 구현 완료)
- **상세 명세**: `명세서/Editor_Plan.md` (전체 계획), `명세서/Editor_수정안.md` (DXGI 전체화면 수정)
- **CEditorApp**: CBase 상속, CMainApp과 유사하나 Level 시스템 대신 ImGui DockSpace 운영
  - `EngineDesc.iNumLevels = 1` (단일 레벨), CLIENT_DLL export 불필요 (Editor.exe 내부)
  - `Free()` 순서: ShutDown_ImGui → Device → Context → Release_Engine → GameInstance
- **ImGui 통합**: docking 브랜치 (DX11 백엔드), `Editor/ImGui/` + `Editor/ImGui/backends/`
  - 초기화: Engine 초기화 후 `ImGui_ImplDX11_Init(pDevice, pContext)`
  - 렌더 순서: ImGui NewFrame → DockSpace → UI패널 → Begin_Draw → ImGui Render → End_Draw
  - Draw() 미호출: Level_Manager::Render()가 레벨 없으면 E_FAIL 반환하므로 레벨 세팅 전까지 제외
  - WndProc: 시스템 메시지 먼저 처리 후 `ImGui_ImplWin32_WndProcHandler` 호출
- **윈도우**: DXGI로 모니터 해상도 획득 → WS_OVERLAPPEDWINDOW + SW_MAXIMIZE 최대화 창
  - `Get_MonitorResolution()` in Editor_Defines.h (inline, DXGI 사용)
  - `GetClientRect`로 실제 크기를 CEditorApp::Create에 전달
- **vcxproj 설정**: `/MDd`, EngineSDK/ClientSDK/ImGui/ImGuizmo include, ImGui .cpp는 PrecompiledHeader=NotUsing
- **ImGui + Engine 충돌 주의사항**:
  - `Engine_Defines.h`의 `#define new DBG_NEW` 매크로가 ImGui의 operator new와 충돌
  - EditorApp.cpp에서 ImGui include 전 `#undef new`, 후 `#define new` 복구 필요
  - `using namespace Editor;`는 Editor_Defines.h가 아닌 .cpp 파일에서 선언 (include 순서 문제)
  - WndProc 전방선언에서 `IMGUI_IMPL_API` 제거 (Editor.cpp에서 imgui.h 미포함)
- **향후 확장**: 맵툴, 오브젝트 배치, 텍스처 적용, 이펙트툴, UI 배치, 파싱용 데이터 생성 등

## CLAUDE.md Update Routine

**Trigger conditions (둘 다 적용):**
- **Auto**: When project files are added, removed, or significantly modified during a session, Claude Code proactively proposes CLAUDE.md updates after the main task is complete
- **Manual**: User can request updates at any time (e.g., "CLAUDE.md 갱신해")

**Update steps:**
1. **Detect changes**: Compare current file structure against the Project File Tree above
2. **Identify updates needed**: Check for:
   - New files added (update tree + add one-line description)
   - Files removed (remove from tree)
   - File role changed significantly (update description)
   - New implementation notes discovered (add to Important Implementation Notes)
   - Architecture changes (update Architecture section)
3. **Propose changes to user**: Present the CLAUDE.md diff in a code block and ask for confirmation
4. **Apply after approval**: Update CLAUDE.md only after user confirms

## Working with Claude Code in This Repository

**Communication:** Korean and English are both acceptable.

**File Modification Policy:**
- **CRITICAL:** Claude Code must NEVER directly modify files using Edit/Write tools (encoding issues occur)
- **EXCEPTION:** CLAUDE.md file can be directly modified by Claude Code
- Always present code changes in markdown code blocks
- User will review and manually apply changes after understanding them
- This prevents encoding corruption in Visual Studio projects

**SubAgent Usage:**
- Use SubAgents (Explore, Task, etc.) to maintain context and handle complex work
- Always review SubAgent outputs before presenting to user
- If output quality is insufficient, re-run the SubAgent task
- Request permission before using SubAgents beyond Explore and Task types
