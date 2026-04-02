# Editor ImGui 구현 계획

## 개요

Editor.exe의 ImGui 기반 UI 구조를 단계별로 구현하기 위한 명세서.
현재 기본 DockSpace 셸만 존재하며, 3D Viewport 분리 + 패널 시스템을 구축한다.

**핵심 설계 원칙:**
- BackBuffer에는 ImGui만 렌더 (DockSpace + 패널)
- 3D 씬은 별도 RenderTarget에 렌더 → SRV로 변환 → `ImGui::Image()`로 Viewport 패널 안에 표시
- 각 패널은 독립적인 ImGui 윈도우로, DockSpace 안에서 자유롭게 도킹/탭/리사이즈 가능

**레이아웃 참고 (Unreal/Unity 공통 패턴):**
```
+----------+------------------------+-----------+
| Menu Bar (File / Edit / Window / Help)        |
+----------+------------------------+-----------+
|          |                        |           |
|Hierarchy |      Viewport          | Inspector |
|(씬 객체  |   (3D 렌더링 영역)      | (선택 객체|
| 트리)    |                        |  속성)    |
|          |                        |           |
+----------+------------------------+-----------+
|  [Content Browser] [Log]  (탭으로 전환)        |
+-----------------------------------------------+
```

---

## Phase 1: Viewport 분리 (별도 RenderTarget)

**목표:** 3D 씬을 BackBuffer가 아닌 별도 RenderTarget에 렌더하고, ImGui Viewport 패널에 표시

### 1-1. RenderTarget 클래스 또는 구조 추가

Engine 또는 Editor 측에 별도 RenderTarget 관리 구조 필요:
- ID3D11Texture2D (렌더 대상 텍스처)
- ID3D11RenderTargetView (RTV) — 렌더링 출력용
- ID3D11ShaderResourceView (SRV) — ImGui::Image() 입력용
- ID3D11DepthStencilView (DSV) — 깊이 버퍼
- D3D11_VIEWPORT — Viewport 크기

**위치 결정:**
- (a) Engine에 범용 CRenderTarget 컴포넌트로 → 재사용성 높음
- (b) Editor 내부에 EditorViewport 전용으로 → 단순함

→ 우선 **(b) Editor 내부**에서 시작. 필요 시 Engine으로 승격.

### 1-2. CEditorApp 렌더 파이프라인 변경

**현재 흐름:**
```
Begin_Draw (BackBuffer Clear)
  → Renderer::Draw (3D 오브젝트 → BackBuffer)
  → Level_Manager::Render
  → ImGui 렌더 (BackBuffer 위에 오버레이)
End_Draw (Present)
```

**변경 후 흐름:**
```
[1] 씬 렌더 패스: RTV/DSV를 별도 RenderTarget으로 바인딩
    → OMSetRenderTargets(별도 RTV, 별도 DSV)
    → Clear 별도 RTV/DSV
    → Renderer::Draw (3D 오브젝트 → 별도 RenderTarget)
    → OMSetRenderTargets(BackBuffer RTV, BackBuffer DSV) 복원

[2] ImGui 렌더 패스: BackBuffer에 ImGui만 렌더
    → Begin_Draw (BackBuffer Clear)
    → ImGui NewFrame
    → DockSpace + 패널들
    → Viewport 패널: ImGui::Image(별도 SRV, 패널크기)
    → ImGui::Render + DrawData
    → End_Draw (Present)
```

### 1-3. Viewport 리사이즈 대응

ImGui Viewport 패널 크기가 변할 때마다 별도 RenderTarget도 재생성 필요:
- `ImGui::GetContentRegionAvail()`로 패널 내부 크기 획득
- 이전 크기와 비교 → 변경 시 RTV/SRV/DSV/Texture2D 재생성
- 매 프레임 체크하되, 실제 재생성은 크기 변경 시에만

### 1-4. 완료 기준

- [x] ImGui DockSpace가 전체 화면을 채움
- [ ] 별도 RenderTarget 생성/해제 정상 동작
- [ ] 3D 씬이 Viewport 패널 안에만 표시됨
- [ ] Viewport 패널 리사이즈 시 3D 씬 비율 유지
- [ ] 다른 패널이 Viewport를 가리지 않음

---

## Phase 2: 기본 패널 구조

**목표:** 핵심 패널 4개의 껍데기를 만들고 DockSpace에 배치

### 2-1. 패널 목록

| 패널 | ImGui 윈도우 이름 | 초기 위치 | 역할 |
|------|-------------------|-----------|------|
| Viewport | "Viewport" | 중앙 | Phase 1에서 구현 완료 |
| Hierarchy | "Hierarchy" | 좌측 | 씬 오브젝트 목록 (빈 리스트) |
| Inspector | "Inspector" | 우측 | 선택 객체 속성 (빈 패널) |
| Content Browser | "Content Browser" | 하단 | 리소스 파일 탐색 (빈 패널) |
| Log | "Log" | 하단 (Content Browser 탭) | 실시간 출력 로그 |

### 2-2. 패널 클래스 구조

각 패널을 별도 클래스로 분리한다. 패널이 복잡해질수록 CEditorApp이 비대해지는 것을 방지.

**베이스 클래스:**
```cpp
// Editor/Public/Panel.h
class CPanel abstract : public CBase
{
protected:
    CPanel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel() = default;

public:
    virtual HRESULT Initialize() PURE;
    virtual void    Update(_float fTimeDelta) PURE;
    virtual void    Render() PURE;          // ImGui::Begin/End 포함

    _bool           Is_Open() const { return m_bOpen; }
    void            Set_Open(_bool bOpen) { m_bOpen = bOpen; }
    const _char*    Get_Name() const { return m_szName; }

protected:
    ID3D11Device*           m_pDevice = { nullptr };
    ID3D11DeviceContext*    m_pContext = { nullptr };
    CGameInstance*          m_pGameInstance = { nullptr };

    _char                   m_szName[64] = {};  // ImGui 윈도우 이름
    _bool                   m_bOpen = { true };  // 패널 표시 여부

public:
    virtual void Free() override;
};
```

**파생 클래스:**

| 클래스 | 파일 | ImGui 윈도우 이름 |
|--------|------|-------------------|
| `CPanel_Viewport` | Editor/Public·Private/ | "Viewport" |
| `CPanel_Hierarchy` | Editor/Public·Private/ | "Hierarchy" |
| `CPanel_Inspector` | Editor/Public·Private/ | "Inspector" |
| `CPanel_ContentBrowser` | Editor/Public·Private/ | "Content Browser" |
| `CPanel_Log` | Editor/Public·Private/ | "Log" |

**CPanel_Manager (패널 매니저): ✅ 구현 완료**

패널을 `map<wstring, CPanel*>`로 관리. 이름 기반 접근으로 패널 간 통신 지원.
패널 소유권: Manager가 유일 소유자 (Add_Panel 시 AddRef 안함, Free에서 Release). 캐싱 포인터는 Safe_AddRef 필요.

```cpp
// Editor/Public/Panel_Manager.h
class CPanel_Manager final : public CBase
{
    DECLARE_SINGLETON(CPanel_Manager)
private:
    CPanel_Manager() = default;
    virtual ~CPanel_Manager() = default;

public:
    HRESULT     Add_Panel(const _wstring& strPanelTag, CPanel* pPanel);
    CPanel*     Get_Panel(const _wstring& strPanelTag);
    void        Update_Panels(_float fTimeDelta);   // 열린 패널만 순회 업데이트
    void        Render_Panels();                    // 열린 패널만 순회 렌더

private:
    map<_wstring, CPanel*>  m_Panels;

public:
    virtual void Free() override;
};
```

**CEditorApp에서의 사용: ✅ 구현 완료**
```cpp
// CEditorApp 멤버 — 자주 접근하는 패널은 포인터 캐싱
CPanel_Manager*         m_pPanel_Manager = { nullptr };
CPanel_Viewport*        m_pViewport = { nullptr };  // 렌더 파이프라인에서 직접 접근

// Ready_Panels()에서 생성 + 등록
m_pPanel_Manager = CPanel_Manager::GetInstance();
Safe_AddRef(m_pPanel_Manager);

m_pViewport = CPanel_Viewport::Create(m_pDevice, m_pContext);
m_pPanel_Manager->Add_Panel(TEXT("Panel_Viewport"), m_pViewport);
Safe_AddRef(m_pViewport);  // 캐싱 포인터는 AddRef 필요
// 나머지 4개는 Create → Add_Panel (Manager가 유일 소유자)

// Update()에서 일괄 업데이트
m_pPanel_Manager->Update_Panels(fTimeDelta);

// Render()에서 일괄 렌더
m_pPanel_Manager->Render_Panels();

// Menu Bar에서 토글 — ToggleMenuItem 헬퍼 메서드
// MENUTYPE 열거체(Editor_Enum.h)로 메뉴 타입 분기
if (ImGui::BeginMenu("Window"))
{
    ToggleMenuItem(TEXT("Panel_Viewport"), MENUTYPE::PANEL);
    // ...
    ImGui::EndMenu();
}

// Free()에서 정리
Safe_Release(m_pViewport);
CPanel_Manager::DestroyInstance();
Safe_Release(m_pPanel_Manager);
```

**패널 간 통신 예시:**
```cpp
// Inspector에서 Hierarchy의 선택 오브젝트 가져오기
CPanel_Hierarchy* pHierarchy = static_cast<CPanel_Hierarchy*>(
    m_pPanel_Manager->Get_Panel(TEXT("Panel_Hierarchy")));
CGameObject* pSelected = pHierarchy->Get_SelectedObject();

// 외부에서 Log 패널에 로그 추가
CPanel_Log* pLog = static_cast<CPanel_Log*>(
    m_pPanel_Manager->Get_Panel(TEXT("Panel_Log")));
pLog->AddLog(LOG_INFO, "FBX 로드 완료 (Mesh 12개)");
```

**장점:**
- 각 패널의 상태/로직이 자체 클래스에 캡슐화
- 이름 기반 접근으로 패널 간 데이터 교환 용이
- 자주 접근하는 패널은 멤버 포인터로 캐싱 (map 탐색 비용 없음)
- 패널 추가 시 CEditorApp 수정 최소화 (Create + Add_Panel)

### 2-3. 초기 DockSpace 레이아웃 (ImGui DockBuilder)

첫 실행 시 `ImGui::DockBuilderXXX` API로 기본 레이아웃 자동 배치:
```cpp
// 최초 1회만 실행 (imgui.ini가 없을 때)
ImGuiID dockspace_id = ImGui::GetID("EditorDockSpace");
ImGui::DockBuilderRemoveNode(dockspace_id);
ImGui::DockBuilderAddNode(dockspace_id, flags);
ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->Size);

// 분할
ImGuiID left, center, right, bottom;
ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, &left, &center);
ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.2f, &right, &center);
ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.25f, &bottom, &center);

// 도킹
ImGui::DockBuilderDockWindow("Hierarchy", left);
ImGui::DockBuilderDockWindow("Viewport", center);
ImGui::DockBuilderDockWindow("Inspector", right);
ImGui::DockBuilderDockWindow("Content Browser", bottom);
ImGui::DockBuilderDockWindow("Log", bottom);  // Content Browser와 같은 영역 (탭)
ImGui::DockBuilderFinish(dockspace_id);
```

이후에는 `imgui.ini`에 저장되어 사용자 커스텀 레이아웃 유지. **✅ 구현 완료**

### 2-4. Menu Bar ✅ 구현 완료

DockSpace 상단에 메뉴바 추가 (DockSpaceWindow에 `ImGuiWindowFlags_MenuBar` 플래그):
```
Window: Viewport / Hierarchy / Inspector / Content Browser / Log (토글) — ToggleMenuItem 헬퍼 메서드
File: New Scene / Open Scene / Save Scene / Exit (향후)
Edit: Undo / Redo (향후)
Tools: Model Converter (Phase 별도, 향후)
```

### 2-5. 완료 기준

- [x] 5개 패널이 DockSpace 내 기본 레이아웃으로 배치됨 (Log은 Content Browser와 탭)
- [x] 각 패널 자유롭게 도킹/탭/리사이즈 가능
- [x] Menu Bar에서 패널 표시/숨기기 토글
- [x] imgui.ini로 레이아웃 저장/복원
- [ ] Log 패널에 테스트 로그 출력 확인

### 2-6. Log 패널

실시간 로그 출력 패널. Content Browser와 하단 영역을 탭으로 공유.

**기능:**
- 로그 메시지 버퍼 (vector<string> 또는 ImGuiTextBuffer)
- 자동 스크롤 (최신 로그가 항상 보이도록)
- Clear 버튼
- 로그 레벨 필터 (Info / Warning / Error) — 색상 구분
- 로그 추가 API: `EditorLog(LEVEL, format, ...)` 형태

**활용 예시:**
- 엔진 초기화: "Engine initialized (1920x1080, Windowed)"
- 리소스 로드: "Texture 로드 성공: Tile0.jpg" / "Texture 로드 실패: 경로 없음"
- Model Converter: "FBX 로드 완료 (Mesh 12개, Material 5개)"
- 오브젝트 배치: "Terrain 생성 완료 (Layer_Environment)"

---

## Phase 3: Content Browser 구현

**목표:** Resources/ 폴더를 탐색하고 파일을 선택할 수 있는 브라우저

### 3-1. 기능

- Resources/ 하위 디렉토리 트리 표시
- 파일 아이콘 (폴더/FBX/텍스처/셰이더 구분)
- 파일 선택 시 Inspector에 정보 표시
- 더블클릭: FBX → Model Converter로 전달 (Phase 별도)

### 3-2. 구현 방식

- `std::filesystem`으로 디렉토리 순회
- `ImGui::Selectable()` 또는 `ImGui::TreeNode()`로 트리 구조
- 현재 경로 breadcrumb 표시
- 썸네일 표시는 후순위 (우선 텍스트 목록)

### 3-3. 완료 기준

- [ ] Resources/ 하위 폴더/파일 목록 표시
- [ ] 폴더 더블클릭으로 진입/상위 이동
- [ ] 파일 선택 상태 관리

---

## Phase 4: Hierarchy + Inspector + RTTR 리플렉션

**목표:** 씬에 배치된 오브젝트 목록 표시 + RTTR 리플렉션 기반 Inspector로 프로퍼티 자동 표시/편집

### 4-1. RTTR 환경 세팅 (Editor 전용)

Engine/Client 코드에는 RTTR 의존성을 넣지 않는다. Editor에서 외부 등록 방식으로 사용.

**라이브러리 배치:**
- `Editor/rttr/` — RTTR 헤더
- `Editor/ThirdPartyLib/` — rttr_core_d.lib (Debug), rttr_core.lib (Release)
- `Editor/Bin/` — rttr_core_d.dll (런타임)
- `Editor.vcxproj` — Additional Include/Lib 경로 추가

**외부 등록 방식 (Engine 무의존):**
Engine 클래스를 직접 수정하지 않고, Editor 측에서 등록 파일을 별도로 둠.
```
Editor/Private/RTTR_Registration.cpp  — 모든 Engine/Client 클래스의 리플렉션 등록
```

```cpp
// Editor/Private/RTTR_Registration.cpp
#include <rttr/registration>
#include "Transform_3D.h"
#include "Transform_2D.h"
#include "Shader.h"
#include "Texture.h"
// ...

RTTR_REGISTRATION
{
    using namespace rttr;

    registration::class_<CTransform_3D>("CTransform_3D")
        .property("Position", &CTransform_3D::Get_State, &CTransform_3D::Set_State)
        .property("Scale", &CTransform_3D::Get_Scale, &CTransform_3D::Set_Scale);

    registration::class_<CTransform_2D>("CTransform_2D")
        .property("Position", ...);

    // 컴포넌트/게임오브젝트 추가 시 여기에 등록 추가
}
```

### 4-2. Hierarchy 패널

- Object_Manager에서 현재 레벨의 레이어/오브젝트 목록 조회
- `ImGui::TreeNode(레이어명)` → `ImGui::Selectable(오브젝트명)`
- 선택 시 Inspector에 해당 오브젝트 전달

### 4-3. Inspector 패널 (RTTR 기반 자동 UI)

RTTR로 등록된 프로퍼티를 순회하며 타입별 ImGui 위젯을 자동 생성.

**타입 → 위젯 매핑:**
| RTTR 타입 | ImGui 위젯 |
|-----------|------------|
| `_float3` / `XMFLOAT3` | `DragFloat3` |
| `_float4` / `XMFLOAT4` | `DragFloat4` / `ColorEdit4` |
| `_float` | `DragFloat` |
| `_int` / `_uint` | `DragInt` |
| `_bool` | `Checkbox` |
| `string` / `wstring` | `InputText` (읽기 전용) |

**자동 순회 렌더:**
```cpp
void DrawInspector(CGameObject* pObj)
{
    // Transform
    DrawComponentProperties(pObj->Get_Transform());

    // 나머지 컴포넌트
    for (auto& [tag, pComp] : pObj->Get_Components())
        DrawComponentProperties(pComp);
}

void DrawComponentProperties(CComponent* pComp)
{
    rttr::type t = rttr::type::get(*pComp);  // 다형성: 실제 타입 조회
    if (ImGui::CollapsingHeader(t.get_name().data()))
    {
        for (auto& prop : t.get_properties())
            DrawProperty(prop, pComp);
    }
}
```

**장점:** 컴포넌트/게임오브젝트가 추가되어도 Inspector 코드 수정 불필요. RTTR_Registration.cpp에 등록만 추가하면 자동 반영.

### 4-4. 완료 기준

- [ ] RTTR 라이브러리 Editor에 통합, 빌드 성공
- [ ] RTTR_Registration.cpp에서 CTransform_3D/2D 등록 완료
- [ ] Hierarchy에서 오브젝트 목록 표시
- [ ] 오브젝트 선택 → Inspector에 RTTR 기반 프로퍼티 자동 표시
- [ ] DragFloat3로 Position/Rotation/Scale 편집
- [ ] 편집 결과가 Viewport에 실시간 반영
- [ ] 새 컴포넌트 등록 시 Inspector에 자동 반영되는지 검증

---

## Phase 5: Model Converter (Assimp → Binary)

**목표:** FBX 파일을 Assimp으로 로드하여 바이너리 포맷(.model)으로 export

### 5-1. Assimp 환경 세팅 (Editor 전용)

- `Editor/assimp/` — Assimp 헤더 (Engine에는 절대 추가하지 않음)
- `Editor/ThirdPartyLib/` — assimp-vc143-mtd.lib (Debug), assimp-vc143-mt.lib (Release)
- `Editor/Bin/` — assimp-vc143-mtd.dll (런타임)
- `Editor.vcxproj` — Additional Include/Lib 경로 추가

### 5-2. ModelConverter 클래스

```
Editor/Public/ModelConverter.h
Editor/Private/ModelConverter.cpp
```

- Assimp::Importer로 FBX 로드
- aiScene에서 Mesh, Material 데이터 추출
- 바이너리 포맷으로 직렬화 (fwrite)
- ImGui UI: 파일 선택 → 옵션 설정 → 변환 → 결과 표시

### 5-3. 바이너리 포맷 (.model)

```
[MODEL_FILE_HEADER]
  - iVersion, iNumMeshes, iNumMaterials, iVertexType
[MESH_DATA × iNumMeshes]
  - iNumVertices, iNumIndices, iMaterialIndex
  - Vertex 데이터 (VTXMESH[] 또는 VTXNORTEX[])
  - Index 데이터 (_uint[])
[MATERIAL_DATA × iNumMaterials]
  - 텍스처 경로 (Diffuse, Normal 등)
  - 머티리얼 속성 (Ambient, Diffuse, Specular, Shininess)
```

### 5-4. Engine 측 CModel/CMesh/CMaterial (바이너리 전용)

개발_진행_가이드.md의 Phase G와 연계:
- Engine의 CModel은 .model 바이너리만 읽음 (Assimp 없음)
- CMesh는 raw 정점/인덱스 데이터로 GPU 버퍼 생성
- CMaterial은 MATERIAL_DATA 구조체로 텍스처 로드

### 5-5. 완료 기준

- [ ] Editor에서 FBX 파일 로드 성공 (Assimp)
- [ ] 바이너리 포맷으로 export 성공
- [ ] Engine의 CModel이 바이너리에서 로드 성공
- [ ] GameApp에서 변환된 모델 렌더링 확인

---

## 작업 우선순위 요약

```
Phase 1: Viewport 분리 (별도 RenderTarget)     ← 가장 먼저, Editor 기반 구조
Phase 2: 기본 패널 구조 (5개 패널 껍데기)         ← UI 뼈대
Phase 3: Content Browser                       ← 리소스 탐색 도구
Phase 4: Hierarchy + Inspector + RTTR          ← RTTR 리플렉션 기반 오브젝트 편집
Phase 5: Model Converter (Assimp → Binary)     ← Assimp 통합 + 바이너리 파이프라인
```

각 Phase는 독립적으로 완료/검증 가능하도록 설계.
Phase 간 의존성: Phase 1 → Phase 2 → Phase 3/4 (병렬 가능) → Phase 5

**Editor 전용 외부 라이브러리 격리 원칙:**
- Assimp: Editor에만 (Phase 5)
- RTTR: Editor에만 (Phase 4)
- Engine/Client는 두 라이브러리 모두 무의존
