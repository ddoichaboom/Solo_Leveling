#include "EditorApp.h"
#include "GameInstance.h"
#include "Panel_Manager.h"

// 임시 TestScene을 위해 추가
#include "Terrain.h"
#include "Camera_Free.h"
#include "Level_Editor.h"
#include "Model.h"
#include "Player.h"
#include "Body_Player.h"
#include "Weapon.h"
#include "MapObject.h"
#include "MapStaticObject.h"




CEditorApp::CEditorApp()
	: m_pGameInstance { CGameInstance::GetInstance() }
{
	Safe_AddRef(m_pGameInstance);
}
#pragma region EDITOR

HRESULT	CEditorApp::Initialize(HWND hWnd, HINSTANCE hInstance, _uint iWinSizeX, _uint iWinSizeY)
{
	// 엔진 초기화
	ENGINE_DESC		EngineDesc{};
	EngineDesc.hWnd				= hWnd;
	EngineDesc.hInstance		= hInstance;
	EngineDesc.eWinMode			= WINMODE::WIN;
	EngineDesc.iViewportWidth	= iWinSizeX;
	EngineDesc.iViewportHeight	= iWinSizeY;
	EngineDesc.iNumLevels		= 5; 

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

	if (FAILED(Ready_Panels()))
	{
		MSG_BOX("Failed to Initialize : Panels");
		return E_FAIL;
	}

	if (FAILED(Ready_TestScene()))
	{
		MSG_BOX("Failed to Initialize : TestScene");
		return E_FAIL;
	}

	return S_OK;
}

void	CEditorApp::Update(_float fTimeDelta)
{
	m_pGameInstance->Update_Engine(fTimeDelta);
	m_pPanel_Manager->Update_Panels(fTimeDelta);
}

HRESULT	CEditorApp::Render()
{
	// (1) 별도 RT에 3D Scene 렌더
	if (FAILED(Render_Scene()))
		return E_FAIL;

	// (2) BackBuffer Clear -> ImGui 전용
	if (FAILED(m_pGameInstance->Begin_Draw()))
		return E_FAIL;

	// (3) ImGui 프레임 시작
	Begin_ImGuiFrame();

	// (4) DockSpace + MenuBar
	Render_DockSpace();

	// (5) 패널 UI 렌더 (Viewport 패널에서 ImGui::Image(SRV)로 3D 표시
	m_pPanel_Manager->Render_Panels();

	// (6) ImGui Draw
	Render_ImGui();

	// (7) Present
	if (FAILED(m_pGameInstance->End_Draw()))
		return E_FAIL;

	return S_OK;
}

#pragma endregion

#pragma region IMGUI

HRESULT CEditorApp::Ready_ImGui(HWND hWnd)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	// ImGuizmo가 사용할 ImGui 컨텍스트 지정 
	ImGuizmo::SetImGuiContext(ImGui::GetCurrentContext());

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

	// ImGuizmo 프레임 시작 - 매 프레임 NewFrame 직후 반드시 호출
	ImGuizmo::BeginFrame();
}

void CEditorApp::Render_DockSpace()
{
	ImGuiViewport* viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);

	ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_MenuBar;
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

	// 최초 1회만 레이아웃 설정 (imgui.ini가 없을 때)
	if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr)
	{
		ImGui::DockBuilderRemoveNode(dockspace_id);
		ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspace_id, viewport->WorkSize);

		ImGuiID left, center, right, bottom;
		ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Left, 0.15f, &left, &center);
		ImGui::DockBuilderSplitNode(center, ImGuiDir_Right, 0.2f, &right, &center);
		ImGui::DockBuilderSplitNode(center, ImGuiDir_Down, 0.25f, &bottom, &center);

		ImGui::DockBuilderDockWindow("Hierarchy", left);
		ImGui::DockBuilderDockWindow("Viewport", center);
		ImGui::DockBuilderDockWindow("Inspector", right);
		ImGui::DockBuilderDockWindow("Content Browser", bottom);
		ImGui::DockBuilderDockWindow("Log", bottom);  
		ImGui::DockBuilderDockWindow("Shortcuts", bottom);  

		ImGui::DockBuilderFinish(dockspace_id);
	}

	ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

	// Menu Bar (2-4)
	if (ImGui::BeginMenuBar())
	{
		if (ImGui::BeginMenu("Window"))
		{
			ToggleMenuItem(TEXT("Panel_Viewport"), MENUTYPE::PANEL);
			ToggleMenuItem(TEXT("Panel_Hierarchy"), MENUTYPE::PANEL);
			ToggleMenuItem(TEXT("Panel_Inspector"), MENUTYPE::PANEL);
			ToggleMenuItem(TEXT("Panel_ContentBrowser"), MENUTYPE::PANEL);
			ToggleMenuItem(TEXT("Panel_Log"), MENUTYPE::PANEL);
			ToggleMenuItem(TEXT("Panel_Shortcuts"), MENUTYPE::PANEL);

			ImGui::EndMenu();
		}

		ImGui::EndMenuBar();
	}

	ImGui::End();
}

void CEditorApp::Render_ImGui()
{
	ImGui::Render();
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
}

void CEditorApp::ToggleMenuItem(const _wstring& strMenuTag, MENUTYPE eType)
{
	
	if (MENUTYPE::PANEL == eType)
	{
		CPanel* pPanel = m_pPanel_Manager->Get_Panel(strMenuTag);
		if (ImGui::MenuItem(pPanel->Get_Name(), nullptr, pPanel->Is_Open()))
			pPanel->Set_Open(!pPanel->Is_Open());
	}
	//else if (MENUTYPE::TOOL == eType)
	//{
	//	// 
	//}
		
}
#pragma endregion

#pragma region PANEL_MANAGER

HRESULT CEditorApp::Ready_Panels()
{
	m_pPanel_Manager = CPanel_Manager::GetInstance();
	Safe_AddRef(m_pPanel_Manager);

	m_pViewport = CPanel_Viewport::Create(m_pDevice, m_pContext);

	// Panel_Viewport
	if (FAILED(m_pPanel_Manager->Add_Panel(TEXT("Panel_Viewport"), m_pViewport)))
		return E_FAIL;

	Safe_AddRef(m_pViewport);

	// Panel_Hierarchy
	if (FAILED(m_pPanel_Manager->Add_Panel(TEXT("Panel_Hierarchy"), CPanel_Hierarchy::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Panel_Inspector
	if (FAILED(m_pPanel_Manager->Add_Panel(TEXT("Panel_Inspector"), CPanel_Inspector::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Panel_ContentBrowser
	if (FAILED(m_pPanel_Manager->Add_Panel(TEXT("Panel_ContentBrowser"), CPanel_ContentBrowser::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Panel_Log
	if (FAILED(m_pPanel_Manager->Add_Panel(TEXT("Panel_Log"), CPanel_Log::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// Panel_Shortcuts
	if (FAILED(m_pPanel_Manager->Add_Panel(TEXT("Panel_Shortcuts"), CPanel_Shortcuts::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	return S_OK;
}

#pragma endregion

#pragma region SCENE_RENDER

HRESULT CEditorApp::Render_Scene()
{
	// Viewport 패널의 별도 RenderTarget으로 전환 + Clear
	if (FAILED(m_pViewport->Begin_RT()))
		return S_OK;		// RT 없으면 스킵

	// 3D 오브젝트 렌더
	m_pGameInstance->Draw();

	// RT 바인딩 해제
	m_pViewport->End_RT();

	return S_OK;
}

HRESULT CEditorApp::Ready_TestScene()
{
	_uint iLevel = 3;    // LEVEL::GAMEPLAY

	// (1) 빈 레벨 활성화
	if (FAILED(m_pGameInstance->Change_Level(iLevel, CLevel_Editor::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// (2) 컴포넌트 프로토타입
	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_Component_Texture_Terrain"),
		CTexture::Create(m_pDevice, m_pContext, TEXT("../../Resources/Textures/Terrain/Tile0.jpg"), 1))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_Component_Shader_VtxNorTex"),
		CShader::Create(m_pDevice, m_pContext, TEXT("../../Resources/ShaderFiles/Shader_VtxNorTex.hlsl"),
			VTXNORTEX::Elements, VTXNORTEX::iNumElements))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_Component_VIBuffer_Terrain"),
		CVIBuffer_Terrain::Create(m_pDevice, m_pContext, TEXT("../../Resources/Textures/Terrain/Height.bmp")))))
		return E_FAIL;

	// (3) 오브젝트 프로토타입
	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Terrain"),
		CTerrain::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Camera_Free"),
		CCamera_Free::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// (4) 카메라
	CCamera_Free::CAMERA_FREE_DESC CameraDesc{};
	CameraDesc.vEye = _float3(0.f, 10.f, -7.f);
	CameraDesc.vAt = _float3(0.f, 0.f, 0.f);
	CameraDesc.fFovy = XMConvertToRadians(60.f);
	CameraDesc.fNear = 0.1f;
	CameraDesc.fFar = 500.f;
	CameraDesc.fSpeedPerSec = 10.f;
	CameraDesc.fRotationPerSec = XMConvertToRadians(180.f);
	CameraDesc.fMouseSensor = 0.05f;

	m_pViewport->Begin_RT();

	if (FAILED(m_pGameInstance->Add_GameObject(iLevel, TEXT("Prototype_GameObject_Camera_Free"),
		iLevel, TEXT("Layer_Camera"), &CameraDesc)))
		return E_FAIL;

	m_pViewport->End_RT();

	 //(5) 터레인
	//if (FAILED(m_pGameInstance->Add_GameObject(iLevel, TEXT("Prototype_GameObject_Terrain"),
	//	iLevel, TEXT("Layer_BackGround"))))
	//	return E_FAIL;

	// (6) 라이트
	LIGHT_DESC LightDesc{};
	LightDesc.eType = LIGHT::DIRECTIONAL;
	LightDesc.vDiffuse = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vAmbient = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vSpecular = _float4(1.f, 1.f, 1.f, 1.f);
	LightDesc.vDirection = _float4(1.f, -1.f, 1.f, 0.f);

	if (FAILED(m_pGameInstance->Add_Light(LightDesc)))
		return E_FAIL;

	// (7) 모델 셰이더 프로토타입
	if (FAILED(m_pGameInstance->Add_Prototype(iLevel,
		TEXT("Prototype_Component_Shader_VtxMesh"),
		CShader::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/ShaderFiles/Shader_VtxMesh.hlsl"),
			VTXMESH::Elements, VTXMESH::iNumElements))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(iLevel,
		TEXT("Prototype_Component_Shader_VtxAnimMesh"),
		CShader::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/ShaderFiles/Shader_VtxAnimMesh.hlsl"),
			VTXANIMMESH::Elements, VTXANIMMESH::iNumElements))))
		return E_FAIL;

	// (8) 모델 프로토타입 (.bin)
	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_Component_Model_SungJinWoo"),
		CModel::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/Models/hunter/SungJinWoo_ERank/SungJinWooERank.bin")))))
				return E_FAIL;
	
	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_Component_Model_Weapon01"),
		CModel::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/Models/weapons/Weapon01/SungJinWoo_ERank_Weapon01.bin")))))
		return E_FAIL;


	// (9) 게임오브젝트 프로토타입
	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Player"),
		CPlayer::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Body_Player"),
		CBody_Player::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_Weapon"),
		CWeapon::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	// (10) Player 인스턴스 배치
	if (FAILED(m_pGameInstance->Add_GameObject(iLevel, TEXT("Prototype_GameObject_Player"),
		iLevel, TEXT("Layer_Player"))))
		return E_FAIL;

	// (11) 맵 모델 프로토타입 (.bin)
	//if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_Component_Model_Map_Lobby_Static"),
	//	CModel::Create(m_pDevice, m_pContext,
	//		TEXT("../../Resources/Models/map/Lobby/Lobby_Static.bin")))))
	//	return E_FAIL;
	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_Component_Model_Map_Lobby_Static"),
		CModel::Create(m_pDevice, m_pContext,
			TEXT("../../Resources/Models/map/Hapjung_Station_1F/Hapjung_Station_1F.bin")))))
		return E_FAIL;

	// (12) 맵 게임오브젝트 프로토타입
	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_MapObject"),
		CMapObject::Create(m_pDevice, m_pContext))))
		return E_FAIL;

	if (FAILED(m_pGameInstance->Add_Prototype(iLevel, TEXT("Prototype_GameObject_MapStaticObject"),
		CMapStaticObject::Create(m_pDevice, m_pContext))))
		return E_FAIL;
	
	// (13) 맵 인스턴스 배치
	if (FAILED(m_pGameInstance->Add_GameObject(iLevel, TEXT("Prototype_GameObject_MapObject"),
		iLevel, TEXT("Layer_Map"))))
		return E_FAIL;

	return S_OK;
}

#pragma endregion

CEditorApp* CEditorApp::Create(HWND hWnd, HINSTANCE hInstance, _uint iWinSizeX, _uint iWinSizeY)
{
	CEditorApp* pInstance = new CEditorApp();

	if (FAILED(pInstance->Initialize(hWnd, hInstance, iWinSizeX, iWinSizeY)))
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

	Safe_Release(m_pViewport);

	m_pPanel_Manager->Release_Panels();
	Safe_Release(m_pPanel_Manager);

	Safe_Release(m_pDevice);
	Safe_Release(m_pContext);

	m_pGameInstance->Release_Engine();
	Safe_Release(m_pGameInstance);
}
