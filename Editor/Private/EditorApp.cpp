#include "EditorApp.h"
#include "GameInstance.h"

#ifdef _DEBUG
#undef new
#endif

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

CEditorApp::CEditorApp()
	: m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
}

HRESULT	CEditorApp::Initialize(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY)
{
	// 엔진 초기화
	ENGINE_DESC		EngineDesc{};
	EngineDesc.hWnd = hWnd;
	EngineDesc.eWinMode = WINMODE::WIN;
	EngineDesc.iViewportWidth = iWinSizeX;
	EngineDesc.iViewportHeight = iWinSizeY;
	EngineDesc.iNumLevels = 1; 

	if (FAILED(m_pGameInstance->Initialize_Engine(EngineDesc, &m_pDevice, &m_pContext)))
	{
		MSG_BOX("Failed to Initialize : Engine");
		return E_FAIL;
	}

	// ImGui 초기화 (엔진 초기화 이후)
	if (FAILED(Ready_ImGui(hWnd)))
	{
		MSG_BOX("Failed to Initialize : ImGui");
		return E_FAIL;
	}

	return S_OK;
}

void	CEditorApp::Update(_float fTimeDelta)
{
	m_pGameInstance->Update_Engine(fTimeDelta);
}

HRESULT	CEditorApp::Render()
{
	// 1. ImGui 프레임 시작
	Begin_ImGuiFrame();

	// 2. DockSpace 설정
	Render_DockSpace();

	// 3. 에디터 UI 패널 
	// TODO : 메뉴바, 하이어라키(계층 구조), 인스펙터 등 
#ifdef _DEBUG
	ImGui::ShowDemoWindow();		// 개발 중 참고용
#endif

	// 4. Scene 렌더링
	if(FAILED(m_pGameInstance->Begin_Draw()))
		return E_FAIL;

	//m_pGameInstance->Draw();
		

	// 5. ImGui 드로우 (씬 위에 오버레이)
	Render_ImGui();

	// 6. Present
	if (FAILED(m_pGameInstance->End_Draw()))
		return E_FAIL;

	return S_OK;
}

#pragma region IMGUI

HRESULT CEditorApp::Ready_ImGui(HWND hWnd)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO();
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	// io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // 멀티 뷰포트 (필요 시)

	ImGui::StyleColorsDark();

	if (!ImGui_ImplWin32_Init(hWnd))
		return E_FAIL;

	if (!ImGui_ImplDX11_Init(m_pDevice, m_pContext))
		return E_FAIL;

	return S_OK;
}

void CEditorApp::ShutDown_ImGui()
{
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void CEditorApp::Begin_ImGuiFrame()
{
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void CEditorApp::Render_DockSpace()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;
	window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
	window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
	window_flags |= ImGuiWindowFlags_NoBackground;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

	ImGui::Begin("DockSpaceWindow", nullptr, window_flags);
	ImGui::PopStyleVar(3);

	ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	ImGui::End();
}

void CEditorApp::Render_ImGui()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

#pragma endregion

CEditorApp* CEditorApp::Create(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY)
{
	CEditorApp* pInstance = new CEditorApp();

	if (FAILED(pInstance->Initialize(hWnd, iWinSizeX, iWinSizeY)))
	{
		MSG_BOX("Failed to Created : CEditorApp");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CEditorApp::Free() 
{
	__super::Free();

	ShutDown_ImGui();

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	m_pGameInstance->Release_Engine();
	Safe_Release(m_pGameInstance);
}
