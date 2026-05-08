#include "Panel_Viewport.h"
#include "GameInstance.h"
#include "Layer.h"
#include "GameObject.h"
#include "Panel_Manager.h"
#include "Model.h"
#include "ContainerObject.h"
#include "PartObject.h"
#include "NavMeshObject.h"
#include "NavMesh.h"


CPanel_Viewport::CPanel_Viewport(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Viewport::Initialize()
{
	strcpy_s(m_szName, "Viewport");

	if (FAILED(Create_RenderTarget(1280, 720)))
		return E_FAIL;

	return S_OK;
}

void CPanel_Viewport::Update(_float fTimeDelta)
{
}

void CPanel_Viewport::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	//  패널 내부 가용 크기 획득
	ImVec2 vAvail = ImGui::GetContentRegionAvail();

	// 리사이즈 감지
	if (vAvail.x > 0.f && vAvail.y > 0.f)
	{
		_uint iNewWidth = static_cast<_uint>(vAvail.x);
		_uint iNewHeight = static_cast<_uint>(vAvail.y);

		if (iNewWidth != m_iRTWidth || iNewHeight != m_iRTHeight)
		{
			Release_RenderTarget();
			Create_RenderTarget(iNewWidth, iNewHeight);
		}
	}

	// SRV를 ImGui::Image()로 렌더링
	if (nullptr != m_pSRV)
	{
		ImVec2 vImagePos = ImGui::GetCursorScreenPos();		// Image 좌상단 좌표

		ImGui::Image(
			reinterpret_cast<ImTextureID>(m_pSRV),
			ImVec2(static_cast<_float>(m_iRTWidth), static_cast<_float>(m_iRTHeight)));

		const _bool bViewportImageHovered = ImGui::IsItemHovered();

		const _bool bWindowFocused =
			ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);

		const _bool bNavMeshToolbarHovered = Render_NavMeshEditToolbar(vImagePos);
		const _bool bNavMeshEditMode = m_pPanel_Manager->Is_NavMeshEditMode();

		// ImGuizmo 오버레이 세팅
		// (1) 기즈모 드로잉을 현재 Viewport 윈도우 drawlist에 연결
		ImGuizmo::SetDrawlist();

		// (2) 기즈모 수학이 사용할 스크린 공간 영역 = Image 위젯 영역과 일치
		ImGuizmo::SetRect(
			vImagePos.x, vImagePos.y,
			static_cast<_float>(m_iRTWidth),
			static_cast<_float>(m_iRTHeight));

		const _bool bNavMeshToggleKeyDown = ImGui::IsKeyDown(ImGuiKey_N);

		if (false == bNavMeshToggleKeyDown)
		{
			m_bNavMeshToggleKeyHeld = false;
		}
		else if (bWindowFocused &&
			false == m_bNavMeshToggleKeyHeld &&
			false == ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
			false == ImGui::IsAnyItemActive())
		{
			m_pPanel_Manager->Toggle_NavMeshEditMode();
			m_bNavMeshToggleKeyHeld = true;
		}

		if (bWindowFocused &&
			bNavMeshEditMode &&
			false == ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
			false == ImGui::IsAnyItemActive() &&
			false == ImGui::IsKeyDown(ImGuiMod_Ctrl) &&
			false == ImGui::IsKeyDown(ImGuiMod_Alt) &&
			false == ImGui::IsKeyDown(ImGuiMod_Shift) &&
			ImGui::IsKeyPressed(ImGuiKey_C))
		{
			Try_Create_NavMeshCell();
		}

		// 기즈모 단축키 처리
		// Viewport 포커스 상태 + RMB(카메라 모드) 비활성 시에만 반응
		if (bWindowFocused && !bNavMeshEditMode && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
		{
			if (ImGui::IsKeyPressed(ImGuiKey_W))
				m_eGizmoOperation = ImGuizmo::TRANSLATE;
			else if (ImGui::IsKeyPressed(ImGuiKey_E))
				m_eGizmoOperation = ImGuizmo::ROTATE;
			else if (ImGui::IsKeyPressed(ImGuiKey_R))
				m_eGizmoOperation = ImGuizmo::SCALE;

			if (ImGui::IsKeyPressed(ImGuiKey_X))
			{
				m_eGizmoMode = (m_eGizmoMode == ImGuizmo::LOCAL)
					? ImGuizmo::WORLD
					: ImGuizmo::LOCAL;
			}
		}

		_bool bGizmoBlocking = { false };

		if (false == bNavMeshEditMode)
		{
			// 선택 오브젝트에 대한 기즈모 조작
			CGameObject* pSelected = m_pPanel_Manager->Get_SelectedObject();
			if (nullptr != pSelected)
			{
				CTransform* pTransform = pSelected->Get_Transform();
				if (nullptr != pTransform)
				{
					// View/Proj 행렬
					const _float4x4* pViewMatrix = m_pGameInstance->Get_Transform(D3DTS::VIEW);
					const _float4x4* pProjMatrix = m_pGameInstance->Get_Transform(D3DTS::PROJ);

					// 대상 World 행렬을 스택 로컬로 복사
					_float4x4 worldMatrix = *pTransform->Get_WorldMatrixPtr();

					const _float* pSnap = { nullptr };
					_float3 vSnap = {};

					if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
					{
						switch (m_eGizmoOperation)
						{
						case ImGuizmo::TRANSLATE:
							vSnap = _float3(m_fSnapTranslate, m_fSnapTranslate, m_fSnapTranslate);
							break;
						case ImGuizmo::ROTATE:
							vSnap = _float3(m_fSnapRotate, m_fSnapRotate, m_fSnapRotate);
							break;
						case ImGuizmo::SCALE:
							vSnap = _float3(m_fSnapScale, m_fSnapScale, m_fSnapScale);
							break;
						}
						pSnap = reinterpret_cast<const _float*>(&vSnap);
					}

					// 기즈모 조작
					ImGuizmo::Manipulate(
						reinterpret_cast<const _float*>(pViewMatrix),
						reinterpret_cast<const _float*>(pProjMatrix),
						m_eGizmoOperation,
						m_eGizmoMode,
						reinterpret_cast<_float*>(&worldMatrix),
						nullptr,
						pSnap);

					// 조작이 발생했을 때만 Transform에 반영
					if (ImGuizmo::IsUsing())
						pTransform->Set_WorldMatrix(worldMatrix);

					// 이 프레임에 Manipulate를 호출했을 때만 IsOver/IsUsing 상태가 유효
					bGizmoBlocking = ImGuizmo::IsOver() || ImGuizmo::IsUsing();
				}
			}
		}

		if (bViewportImageHovered &&
			false == bNavMeshToolbarHovered &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			!bGizmoBlocking)
		{
			ImVec2 vMousePos = ImGui::GetMousePos();
			m_fPickX = vMousePos.x - vImagePos.x;
			m_fPickY = vMousePos.y - vImagePos.y;

			if (bNavMeshEditMode)
			{
				if (ImGui::IsKeyDown(ImGuiMod_Ctrl))
					Select_NavMeshCell();
				else if (ImGui::IsKeyDown(ImGuiMod_Alt))
					Select_NavMeshVertex();
				else if (ImGui::IsKeyDown(ImGuiMod_Shift))
					Move_SelectedNavMeshVertex();
				else
					Pick_NavMeshEditPoint();
			}
			else
			{
				Pick_Object();
			}
		}

		if (bNavMeshEditMode)
		{
			Render_SelectedNavMeshCell(vImagePos);
			Render_SelectedNavMeshVertex(vImagePos);
			Render_NavMesh_PickPreview(vImagePos);
		}
	}

	ImGui::End();
}

#pragma region RENDER_TARGET

HRESULT CPanel_Viewport::Begin_RT()
{
	if (nullptr == m_pRTV || nullptr == m_pDSV)
		return E_FAIL;

	// 별도 RT/DSV로 전환
	m_pContext->OMSetRenderTargets(1, &m_pRTV, m_pDSV);
	m_pContext->RSSetViewports(1, &m_Viewport);

	// Clear
	_float4 vCleanColor = _float4(0.2f, 0.2f, 0.2f, 1.f); // 어두운 회색
	m_pContext->ClearRenderTargetView(m_pRTV, reinterpret_cast<const _float*>(&vCleanColor));
	m_pContext->ClearDepthStencilView(m_pDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.f, 0);

	return S_OK;
}

HRESULT CPanel_Viewport::End_RT()
{

	/*BackBuffer 복원은 EditorApp에서 Begin_Draw() 호출 시 자동으로 됨.
	여기서는 RT 바인딩만 해제하여 안전하게 SRV로 읽을 수 있도록 함 */

	ID3D11RenderTargetView* pNullRTV = nullptr;
	m_pContext->OMSetRenderTargets(1, &pNullRTV, nullptr);

	return S_OK;
}

HRESULT CPanel_Viewport::Create_RenderTarget(_uint iWidth, _uint iHeight)
{
	if (0 == iWidth || 0 == iHeight)
		return E_FAIL;

	// (1) 렌더 대상 텍스처 (Texture2D + RTV + SRV)
	D3D11_TEXTURE2D_DESC TexDesc{};
	TexDesc.Width = iWidth;
	TexDesc.Height = iHeight;
	TexDesc.MipLevels = 1;
	TexDesc.ArraySize = 1;
	TexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	TexDesc.SampleDesc.Count = 1;
	TexDesc.SampleDesc.Quality = 0;
	TexDesc.Usage = D3D11_USAGE_DEFAULT;
	TexDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	TexDesc.CPUAccessFlags = 0;
	TexDesc.MiscFlags = 0;

	if (FAILED(m_pDevice->CreateTexture2D(&TexDesc, nullptr, &m_pRTTexture)))
		return E_FAIL;

	if (FAILED(m_pDevice->CreateRenderTargetView(m_pRTTexture, nullptr, &m_pRTV)))
		return E_FAIL;

	if (FAILED(m_pDevice->CreateShaderResourceView(m_pRTTexture, nullptr, &m_pSRV)))
		return E_FAIL;

	// (2) 깊이/스텐실 텍스처 (Texture2D + DSV) 
	D3D11_TEXTURE2D_DESC DSDesc{};
	DSDesc.Width = iWidth;
	DSDesc.Height = iHeight;
	DSDesc.MipLevels = 1;
	DSDesc.ArraySize = 1;
	DSDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DSDesc.SampleDesc.Count = 1;
	DSDesc.SampleDesc.Quality = 0;
	DSDesc.Usage = D3D11_USAGE_DEFAULT;
	DSDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	DSDesc.CPUAccessFlags = 0;
	DSDesc.MiscFlags = 0;

	if (FAILED(m_pDevice->CreateTexture2D(&DSDesc, nullptr, &m_pDSTexture)))
		return E_FAIL;

	if (FAILED(m_pDevice->CreateDepthStencilView(m_pDSTexture, nullptr, &m_pDSV)))
		return E_FAIL;

	// (3) Viewport 
	m_Viewport.TopLeftX = 0.f;
	m_Viewport.TopLeftY = 0.f;
	m_Viewport.Width = static_cast<_float>(iWidth);
	m_Viewport.Height = static_cast<_float>(iHeight);
	m_Viewport.MinDepth = 0.f;
	m_Viewport.MaxDepth = 1.f;

	// (4) 크기 기록
	m_iRTWidth = iWidth;
	m_iRTHeight = iHeight;

	return S_OK;
}

void CPanel_Viewport::Release_RenderTarget()
{
	Safe_Release(m_pDSV);
	Safe_Release(m_pDSTexture);
	Safe_Release(m_pSRV);
	Safe_Release(m_pRTV);
	Safe_Release(m_pRTTexture);

	m_iRTWidth = { 0 };
	m_iRTHeight = { 0 };
}

#pragma endregion

#pragma region PICKING

void CPanel_Viewport::Pick_Object()
{
	PICK_RESULT Result{};

	if (Pick_Surface(&Result, false) && nullptr != Result.pObject)
		m_pPanel_Manager->Set_SelectedObject(Result.pObject);
	else
		m_pPanel_Manager->Clear_Selection();
}

#pragma endregion

#pragma region NAVMESH

_bool CPanel_Viewport::Render_NavMeshEditToolbar(const ImVec2& vImagePos)
{
	ImGuiWindowFlags Flags =
		ImGuiWindowFlags_NoTitleBar |
		ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoMove |
		ImGuiWindowFlags_NoSavedSettings |
		ImGuiWindowFlags_AlwaysAutoResize;

	ImGui::SetNextWindowPos(ImVec2(vImagePos.x + 12.f, vImagePos.y + 12.f), ImGuiCond_Always);
	ImGui::SetNextWindowBgAlpha(0.72f);

	ImGui::Begin("NavMesh Edit##ViewportOverlay", nullptr, Flags);

	const _bool bHovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_RootAndChildWindows);

	_bool bNavMeshMode = m_pPanel_Manager->Is_NavMeshEditMode();

	if (ImGui::Checkbox("NavMesh Edit", &bNavMeshMode))
	{
		m_pPanel_Manager->Set_ToolMode(
			bNavMeshMode ? EDITOR_TOOL_MODE::NAVMESH : EDITOR_TOOL_MODE::OBJECT);
	}

	if (ImGui::Button("Clear Picks"))
	{
		Clear_NavMeshPickPoints();
		Log_EditStatus(LOG_LEVEL::INFO, "Cleared Picks");
	}

	const _bool bCreateCellDisabled = m_NavMeshPickedPoints.size() < 3;

	ImGui::SameLine();

	if (bCreateCellDisabled)
		ImGui::BeginDisabled();

	if (ImGui::Button("Create Cell"))
	{
		Try_Create_NavMeshCell();
	}

	if (bCreateCellDisabled)
		ImGui::EndDisabled();

	ImGui::SameLine();

	const _bool bDeleteCellDisabled = (NAVMESH_INVALID_INDEX == m_iSelectedNavMeshCellIndex);
	if (bDeleteCellDisabled)
		ImGui::BeginDisabled();

	if (ImGui::Button("Delete Cell"))
	{
		Delete_SelectedNavMeshCell();
	}

	if (bDeleteCellDisabled)
		ImGui::EndDisabled();

	const _bool bUndoDisabled = m_NavMeshUndoStack.empty();
	const _bool bRedoDisabled = m_NavMeshRedoStack.empty();

	if (bUndoDisabled)
		ImGui::BeginDisabled();

	if (ImGui::Button("Undo"))
	{
		Undo_NavMeshEdit();
	}

	if (bUndoDisabled)
		ImGui::EndDisabled();

	ImGui::SameLine();

	if (bRedoDisabled)
		ImGui::BeginDisabled();

	if (ImGui::Button("Redo"))
	{
		Redo_NavMeshEdit();
	}

	if (bRedoDisabled)
		ImGui::EndDisabled();

	if (ImGui::Button("Save NavData"))
	{
		Save_NavMeshData();
	}

	ImGui::SameLine();

	if (ImGui::Button("Load NavData"))
	{
		Load_NavMeshData();
	}

	ImGui::Text("Selected Cell : %d", m_iSelectedNavMeshCellIndex);
	ImGui::Text("Selected Vertex : %d", m_iSelectedNavMeshVertexIndex);
	ImGui::Text("Pending Points: %u", static_cast<_uint>(m_NavMeshPickedPoints.size()));


	for (_uint i = 0; i < static_cast<_uint>(m_NavMeshPickedPoints.size()); ++i)
	{
		const NAVMESH_PICK_POINT& Point = m_NavMeshPickedPoints[i];

		if (Point.bSnapped)
		{
			ImGui::Text("[%u] snapped: %d / %.2f, %.2f, %.2f",
				i,
				Point.iSnapVertexIndex,
				Point.vPreviewPosition.x,
				Point.vPreviewPosition.y,
				Point.vPreviewPosition.z);
		}
		else
		{
			ImGui::Text("[%u] new / %.2f, %.2f, %.2f",
				i,
				Point.vPreviewPosition.x,
				Point.vPreviewPosition.y,
				Point.vPreviewPosition.z);
		}
	}

	ImGui::End();

	return bHovered;
}

void CPanel_Viewport::Render_NavMesh_PickPreview(const ImVec2& vImagePos)
{
	if (m_NavMeshPickedPoints.empty())
		return;

	ImDrawList* pDrawList = ImGui::GetWindowDrawList();
	if (nullptr == pDrawList)
		return;

	ImVec2 FirstScreen{};
	ImVec2 PrevScreen{};

	_bool bHasFirst = false;
	_bool bHasPrev = false;
	_uint iVisibleCount = 0;

	for (_uint i = 0; i < static_cast<_uint>(m_NavMeshPickedPoints.size()); ++i)
	{
		const NAVMESH_PICK_POINT& Point = m_NavMeshPickedPoints[i];

		ImVec2 ScreenPos{};
		if (false == World_To_Viewport(Point.vPreviewPosition, vImagePos, &ScreenPos))
			continue;

		const ImU32 Color = Point.bSnapped
			? IM_COL32(255, 210, 64, 255)
			: IM_COL32(64, 180, 255, 255);

		pDrawList->AddCircleFilled(ScreenPos, 5.f, Color, 16);
		pDrawList->AddCircle(ScreenPos, 9.f, Color, 16, 2.f);

		if (false == bHasFirst)
		{
			FirstScreen = ScreenPos;
			bHasFirst = true;
		}

		if (bHasPrev)
			pDrawList->AddLine(PrevScreen, ScreenPos, IM_COL32(255, 255, 255, 220), 2.f);

		PrevScreen = ScreenPos;
		bHasPrev = true;
		++iVisibleCount;
	}

	if (3 == m_NavMeshPickedPoints.size() &&
		3 == iVisibleCount &&
		bHasFirst &&
		bHasPrev)
	{
		pDrawList->AddLine(PrevScreen, FirstScreen, IM_COL32(255, 255, 255, 220), 2.f);
	}
}

void CPanel_Viewport::Render_SelectedNavMeshCell(const ImVec2& vImagePos)
{
	if (NAVMESH_INVALID_INDEX == m_iSelectedNavMeshCellIndex)
		return;

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
		return;

	const vector<NAVMESH_CELL>& Cells = pNavMesh->Get_CellDescs();
	const vector<_float3>& Vertices = pNavMesh->Get_Vertices();

	if (m_iSelectedNavMeshCellIndex < 0 ||
		static_cast<size_t>(m_iSelectedNavMeshCellIndex) >= Cells.size())
		return;

	const NAVMESH_CELL& Cell = Cells[m_iSelectedNavMeshCellIndex];

	ImVec2 Screen[3]{};

	for (_uint i = 0; i < 3; ++i)
	{
		const _int iVertexIndex = Cell.iVertexIndices[i];

		if (iVertexIndex < 0 ||
			static_cast<size_t>(iVertexIndex) >= Vertices.size())
			return;

		if (false == World_To_Viewport(Vertices[iVertexIndex], vImagePos, &Screen[i]))
			return;
	}

	ImDrawList* pDrawList = ImGui::GetWindowDrawList();
	if (nullptr == pDrawList)
		return;

	pDrawList->AddTriangleFilled(
		Screen[0], Screen[1], Screen[2],
		IM_COL32(255, 96, 64, 55));

	pDrawList->AddTriangle(
		Screen[0], Screen[1], Screen[2],
		IM_COL32(255, 96, 64, 255),
		3.f);
}

void CPanel_Viewport::Render_SelectedNavMeshVertex(const ImVec2& vImagePos)
{
	if (NAVMESH_INVALID_INDEX == m_iSelectedNavMeshVertexIndex)
		return;

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
		return;

	const vector<_float3>& Vertices = pNavMesh->Get_Vertices();

	if (m_iSelectedNavMeshVertexIndex < 0 ||
		static_cast<size_t>(m_iSelectedNavMeshVertexIndex) >= Vertices.size())
		return;

	ImVec2 vScreenPosition = {};
	if (false == World_To_Viewport(Vertices[m_iSelectedNavMeshVertexIndex], vImagePos, &vScreenPosition))
		return;

	ImDrawList* pDrawList = ImGui::GetWindowDrawList();
	if (nullptr == pDrawList)
		return;

	pDrawList->AddCircleFilled(
		vScreenPosition,
		7.f,
		IM_COL32(255, 210, 64, 255));

	pDrawList->AddCircle(
		vScreenPosition,
		10.f,
		IM_COL32(255, 255, 255, 255),
		16,
		2.f);
}

void CPanel_Viewport::Select_NavMeshVertex()
{
	PICK_RESULT Result{};

	if (false == Pick_Surface(&Result, true))
	{
		m_iSelectedNavMeshVertexIndex = NAVMESH_INVALID_INDEX;
		Log_EditStatus(LOG_LEVEL::WARNING, "No map hit.");
		return;
	}

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
	{
		m_iSelectedNavMeshVertexIndex = NAVMESH_INVALID_INDEX;
		Log_EditStatus(LOG_LEVEL::ERROR_, "NavMeshObject not found.");
		return;
	}

	static constexpr _float fVertexPickRadius = { 0.35f };

	const _int iVertexIndex = pNavMesh->Find_Vertex(Result.vPosition, fVertexPickRadius);
	m_iSelectedNavMeshVertexIndex = iVertexIndex;

	if (NAVMESH_INVALID_INDEX == iVertexIndex)
	{
		Log_EditStatus(LOG_LEVEL::WARNING, "No vertex selected.");
		return;
	}

	_char szStatus[128] = {};
	sprintf_s(szStatus, "Selected Vertex: %d", iVertexIndex);
	Log_EditStatus(LOG_LEVEL::INFO, szStatus);
}

HRESULT CPanel_Viewport::Move_SelectedNavMeshVertex()
{
	if (NAVMESH_INVALID_INDEX == m_iSelectedNavMeshVertexIndex)
	{
		Log_EditStatus(LOG_LEVEL::WARNING, "No selected vertex.");
		return E_FAIL;
	}

	PICK_RESULT Result{};

	if (false == Pick_Surface(&Result, true))
	{
		Log_EditStatus(LOG_LEVEL::WARNING, "No map hit.");
		return E_FAIL;
	}

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
	{
		Log_EditStatus(LOG_LEVEL::ERROR_, "NavMeshObject not found.");
		return E_FAIL;
	}

	NAVMESH_SNAPSHOT Backup = pNavMesh->Capture_Snapshot();

	if (FAILED(pNavMesh->Move_Vertex(m_iSelectedNavMeshVertexIndex, Result.vPosition)))
	{
		pNavMesh->Restore_Snapshot(Backup);
		Log_EditStatus(LOG_LEVEL::ERROR_, "Failed to move vertex.");
		return E_FAIL;
	}

	Push_NavMeshUndoSnapshot(Backup);
	Clear_NavMeshPickPoints();

	_char szStatus[128] = {};
	sprintf_s(szStatus, "Moved Vertex: %d", m_iSelectedNavMeshVertexIndex);
	Log_EditStatus(LOG_LEVEL::INFO, szStatus);

	return S_OK;
}

void CPanel_Viewport::Select_NavMeshCell()
{
	PICK_RESULT Result{};

	if (false == Pick_Surface(&Result, true))
	{
		m_iSelectedNavMeshCellIndex = NAVMESH_INVALID_INDEX;
		Log_EditStatus(LOG_LEVEL::WARNING, "No map hit");
		return;
	}

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
	{
		m_iSelectedNavMeshCellIndex = NAVMESH_INVALID_INDEX;
		Log_EditStatus(LOG_LEVEL::ERROR_, "NavMeshObject not found.");
		return;
	}

	const _int iCellIndex = pNavMesh->Find_Cell(Result.vPosition);
	m_iSelectedNavMeshCellIndex = iCellIndex;

	if (NAVMESH_INVALID_INDEX == iCellIndex)
		Log_EditStatus(LOG_LEVEL::WARNING, "No cell selected.");
	else
	{
		_char szStatus[128] = {};
		sprintf_s(szStatus, "Selected Cell: %d", iCellIndex);
		Log_EditStatus(LOG_LEVEL::INFO, szStatus);
	}
}

HRESULT CPanel_Viewport::Delete_SelectedNavMeshCell()
{
	if (NAVMESH_INVALID_INDEX == m_iSelectedNavMeshCellIndex)
	{
		Log_EditStatus(LOG_LEVEL::WARNING, "No selected cell.");
		return E_FAIL;
	}

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
	{
		Log_EditStatus(LOG_LEVEL::ERROR_, "NavMeshObject not found.");
		return E_FAIL;
	}

	NAVMESH_SNAPSHOT Backup = pNavMesh->Capture_Snapshot();

	if (FAILED(pNavMesh->Remove_Cell(m_iSelectedNavMeshCellIndex)))
	{
		pNavMesh->Restore_Snapshot(Backup);
		Log_EditStatus(LOG_LEVEL::ERROR_, "Failed to delete cell.");
		return E_FAIL;
	}

	Push_NavMeshUndoSnapshot(Backup);

	m_iSelectedNavMeshCellIndex = NAVMESH_INVALID_INDEX;
	Clear_NavMeshPickPoints();

	Log_EditStatus(LOG_LEVEL::INFO, "Deleted selected cell.");

	return S_OK;
}

HRESULT CPanel_Viewport::Undo_NavMeshEdit()
{
	if (m_NavMeshUndoStack.empty())
		return E_FAIL;

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
		return E_FAIL;

	NAVMESH_SNAPSHOT Current = pNavMesh->Capture_Snapshot();
	NAVMESH_SNAPSHOT Previous = m_NavMeshUndoStack.back();
	m_NavMeshUndoStack.pop_back();

	if (FAILED(pNavMesh->Restore_Snapshot(Previous)))
	{
		pNavMesh->Restore_Snapshot(Current);
		Log_EditStatus(LOG_LEVEL::ERROR_, "Undo failed.");
		return E_FAIL;
	}

	m_NavMeshRedoStack.push_back(Current);
	Clear_NavMeshEditState();

	Log_EditStatus(LOG_LEVEL::INFO, "Undo.");

	return S_OK;
}

HRESULT CPanel_Viewport::Redo_NavMeshEdit()
{
	if (m_NavMeshRedoStack.empty())
		return E_FAIL;

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
		return E_FAIL;

	NAVMESH_SNAPSHOT Current = pNavMesh->Capture_Snapshot();
	NAVMESH_SNAPSHOT Next = m_NavMeshRedoStack.back();
	m_NavMeshRedoStack.pop_back();

	if (FAILED(pNavMesh->Restore_Snapshot(Next)))
	{
		pNavMesh->Restore_Snapshot(Current);
		Log_EditStatus(LOG_LEVEL::ERROR_, "Redo failed.");
		return E_FAIL;
	}

	m_NavMeshUndoStack.push_back(Current);
	Clear_NavMeshEditState();

	Log_EditStatus(LOG_LEVEL::INFO, "Redo.");

	return S_OK;
}

HRESULT CPanel_Viewport::Save_NavMeshData()
{
	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
	{
		Log_EditStatus(LOG_LEVEL::ERROR_, "NavMeshObject not found.");
		return E_FAIL;
	}

	std::error_code ErrorCode{};
	std::filesystem::create_directories(std::filesystem::path(TEXT("../../Resources/NavMesh")), ErrorCode);

	if (ErrorCode)
	{
		Log_EditStatus(LOG_LEVEL::ERROR_, "Failed to create NavMesh directory.");
		return E_FAIL;
	}

	if (FAILED(pNavMesh->Save_NavData(TEXT("../../Resources/NavMesh/ThroneRoom.navdata"))))
	{
		Log_EditStatus(LOG_LEVEL::ERROR_, "Failed to save NavData.");
		return E_FAIL;
	}

	Log_EditStatus(LOG_LEVEL::INFO, "Saved NavData: ../../Resources/NavMesh/ThroneRoom.navdata");

	return S_OK;
}

HRESULT CPanel_Viewport::Load_NavMeshData()
{
	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
	{
		Log_EditStatus(LOG_LEVEL::ERROR_, "NavMeshObject not found.");
		return E_FAIL;
	}

	NAVMESH_SNAPSHOT Backup = pNavMesh->Capture_Snapshot();

	if (FAILED(pNavMesh->Load_NavData(TEXT("../../Resources/NavMesh/ThroneRoom.navdata"))))
	{
		Log_EditStatus(LOG_LEVEL::ERROR_, "Failed to load NavData.");
		return E_FAIL;
	}

	Push_NavMeshUndoSnapshot(Backup);
	Clear_NavMeshEditState();

	Log_EditStatus(LOG_LEVEL::INFO, "Loaded NavData: ../../Resources/NavMesh/ThroneRoom.navdata");

	return S_OK;
}

void CPanel_Viewport::Push_NavMeshUndoSnapshot(const NAVMESH_SNAPSHOT& Snapshot)
{
	m_NavMeshUndoStack.push_back(Snapshot);
	m_NavMeshRedoStack.clear();

	static constexpr size_t iMaxUndoCount = 64;
	if (m_NavMeshUndoStack.size() > iMaxUndoCount)
		m_NavMeshUndoStack.erase(m_NavMeshUndoStack.begin());
}

void CPanel_Viewport::Clear_NavMeshEditState()
{
	Clear_NavMeshPickPoints();
	m_iSelectedNavMeshCellIndex = NAVMESH_INVALID_INDEX;
	m_iSelectedNavMeshVertexIndex = NAVMESH_INVALID_INDEX;
}

_bool CPanel_Viewport::World_To_Viewport(const _float3& vWorldPosition, const ImVec2& vImagePos, ImVec2* pOutScreenPosition) const
{
	if (nullptr == pOutScreenPosition || 0 == m_iRTWidth || 0 == m_iRTHeight)
		return false;

	const _float4x4* pViewMatrix = m_pGameInstance->Get_Transform(D3DTS::VIEW);
	const _float4x4* pProjMatrix = m_pGameInstance->Get_Transform(D3DTS::PROJ);
	if (nullptr == pViewMatrix || nullptr == pProjMatrix)
		return false;

	_matrix matView = XMLoadFloat4x4(pViewMatrix);
	_matrix matProj = XMLoadFloat4x4(pProjMatrix);

	_vector vClip = XMVector3TransformCoord(XMLoadFloat3(&vWorldPosition), matView * matProj);

	const _float fX = XMVectorGetX(vClip);
	const _float fY = XMVectorGetY(vClip);
	const _float fZ = XMVectorGetZ(vClip);

	if (fZ < 0.f || fZ > 1.f)
		return false;

	pOutScreenPosition->x = vImagePos.x + (fX + 1.f) * 0.5f * static_cast<_float>(m_iRTWidth);
	pOutScreenPosition->y = vImagePos.y + (1.f - fY) * 0.5f * static_cast<_float>(m_iRTHeight);

	return true;
}

CNavMesh* CPanel_Viewport::Find_NavMesh() const
{
	_int iLevelIndex = m_pGameInstance->Get_CurrentLevelIndex();
	if (iLevelIndex < 0)
		return nullptr;

	const auto* pLayers = m_pGameInstance->Get_Layers(static_cast<_uint>(iLevelIndex));
	if (nullptr == pLayers)
		return nullptr;

	auto iterLayer = pLayers->find(TEXT("Layer_NavMesh"));
	if (iterLayer == pLayers->end() || nullptr == iterLayer->second)
		return nullptr;

	for (auto& pObject : iterLayer->second->Get_GameObjects())
	{
		CNavMeshObject* pNavMeshObject = dynamic_cast<CNavMeshObject*>(pObject);
		if (nullptr == pNavMeshObject)
			continue;

		return pNavMeshObject->Get_NavMesh();
	}

	return nullptr;
}

_bool CPanel_Viewport::Pick_Surface(PICK_RESULT* pOutResult, _bool bMapOnly)
{
	if (nullptr == pOutResult || 0 == m_iRTWidth || 0 == m_iRTHeight)
		return false;

	_float4 vRayOrigin = {};
	_float4 vRayDir = {};

	m_pGameInstance->Compute_WorldRay(
		m_fPickX, m_fPickY,
		static_cast<_float>(m_iRTWidth),
		static_cast<_float>(m_iRTHeight),
		&vRayOrigin, &vRayDir);

	_vector vOrigin = XMLoadFloat4(&vRayOrigin);
	_vector vDir = XMLoadFloat4(&vRayDir);

	_int iLevelIndex = m_pGameInstance->Get_CurrentLevelIndex();
	if (iLevelIndex < 0)
		return false;

	const auto* pLayers = m_pGameInstance->Get_Layers(static_cast<_uint>(iLevelIndex));
	if (nullptr == pLayers)
		return false;

	CGameObject* pPicked = { nullptr };
	_float fMinDist = FLT_MAX;

	for (auto& LayerPair : *pLayers)
	{
		if (LayerPair.first == TEXT("Layer_NavMesh"))
			continue;

		if (bMapOnly && LayerPair.first != TEXT("Layer_BackGround"))
			continue;

		for (auto& pObject : LayerPair.second->Get_GameObjects())
		{
			if (nullptr == pObject || nullptr == pObject->Get_Transform())
				continue;

			_float fDist = {};
			_bool bHit = false;

			_matrix matWorld = XMLoadFloat4x4(pObject->Get_Transform()->Get_WorldMatrixPtr());

			CVIBuffer* pVIBuffer = pObject->Get_VIBuffer();
			if (nullptr != pVIBuffer)
			{
				bHit = pVIBuffer->Pick(vOrigin, vDir, matWorld, fDist);
			}
			else
			{
				auto& Components = pObject->Get_Components();
				auto iter = Components.find(TEXT("Com_Model"));
				if (iter != Components.end())
				{
					CModel* pModel = static_cast<CModel*>(iter->second);
					bHit = pModel->Pick(vOrigin, vDir, matWorld, fDist);
				}
			}

			if (bHit && fDist < fMinDist)
			{
				fMinDist = fDist;
				pPicked = pObject;
			}

			CContainerObject* pContainer = dynamic_cast<CContainerObject*>(pObject);
			if (nullptr == pContainer)
				continue;

			for (auto& PartPair : pContainer->Get_PartObjects())
			{
				CPartObject* pPartObject = PartPair.second;
				if (nullptr == pPartObject)
					continue;

				auto& PartComponents = pPartObject->Get_Components();
				auto itModel = PartComponents.find(TEXT("Com_Model"));
				if (itModel == PartComponents.end())
					continue;

				CModel* pModel = static_cast<CModel*>(itModel->second);
				if (nullptr == pModel)
					continue;

				_matrix matPartWorld = XMLoadFloat4x4(&pPartObject->Get_CombinedWorldMatrix());

				_float fPartDist = {};
				if (pModel->Pick(vOrigin, vDir, matPartWorld, fPartDist))
				{
					if (fPartDist < fMinDist)
					{
						fMinDist = fPartDist;
						pPicked = pPartObject;
					}
				}
			}
		}
	}

	if (nullptr == pPicked)
		return false;

	_vector vHitPosition = vOrigin + XMVector3Normalize(vDir) * fMinDist;

	pOutResult->pObject = pPicked;
	pOutResult->fDistance = fMinDist;
	XMStoreFloat3(&pOutResult->vPosition, vHitPosition);

	return true;
}

void CPanel_Viewport::Pick_NavMeshEditPoint()
{
	PICK_RESULT Result{};

	if (false == Pick_Surface(&Result, true))
	{
		m_bHasLastNavMeshPick = false;
		return;
	}

	NAVMESH_PICK_POINT PickPoint{};
	PickPoint.vRawPosition = Result.vPosition;
	PickPoint.vPreviewPosition = Result.vPosition;

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr != pNavMesh)
	{
		const _int iSnapVertexIndex = pNavMesh->Find_Vertex(Result.vPosition);
		const vector<_float3>& Vertices = pNavMesh->Get_Vertices();

		if (iSnapVertexIndex >= 0 &&
			static_cast<size_t>(iSnapVertexIndex) < Vertices.size())
		{
			PickPoint.iSnapVertexIndex = iSnapVertexIndex;
			PickPoint.vPreviewPosition = Vertices[iSnapVertexIndex];
			PickPoint.bSnapped = true;
		}
	}

	m_vLastNavMeshPick = PickPoint.vPreviewPosition;
	m_bHasLastNavMeshPick = true;

	m_NavMeshPickedPoints.push_back(PickPoint);

	if (m_NavMeshPickedPoints.size() > 3)
		m_NavMeshPickedPoints.erase(m_NavMeshPickedPoints.begin());
}

HRESULT CPanel_Viewport::Try_Create_NavMeshCell()
{
	if (3 != m_NavMeshPickedPoints.size())
	{
		Log_EditStatus(LOG_LEVEL::WARNING, "Need exactly 3 points.");
		return E_FAIL;
	}

	CNavMesh* pNavMesh = Find_NavMesh();
	if (nullptr == pNavMesh)
	{
		Log_EditStatus(LOG_LEVEL::ERROR_, "NavMeshObject not found.");
		return E_FAIL;
	}

	NAVMESH_SNAPSHOT Backup = pNavMesh->Capture_Snapshot();

	_int iVertexIndices[3] = {
			NAVMESH_INVALID_INDEX,
			NAVMESH_INVALID_INDEX,
			NAVMESH_INVALID_INDEX
	};

	for (_uint i = 0; i < 3; ++i)
	{
		const NAVMESH_PICK_POINT& Point = m_NavMeshPickedPoints[i];

		if (Point.bSnapped)
			iVertexIndices[i] = Point.iSnapVertexIndex;
		else
			iVertexIndices[i] = pNavMesh->Find_OrAddVertex(Point.vPreviewPosition);

		if (NAVMESH_INVALID_INDEX == iVertexIndices[i])
		{
			pNavMesh->Restore_Snapshot(Backup);
			Log_EditStatus(LOG_LEVEL::ERROR_, "Failed to create vertex.");
			return E_FAIL;
		}
	}

	if (iVertexIndices[0] == iVertexIndices[1] ||
		iVertexIndices[1] == iVertexIndices[2] ||
		iVertexIndices[2] == iVertexIndices[0])
	{
		pNavMesh->Restore_Snapshot(Backup);
		Log_EditStatus(LOG_LEVEL::WARNING, "Duplicated vertices.");
		return E_FAIL;
	}

	_int iCellIndex = NAVMESH_INVALID_INDEX;
	if (FAILED(pNavMesh->Try_AddCell(
		iVertexIndices[0],
		iVertexIndices[1],
		iVertexIndices[2],
		&iCellIndex)))
	{
		pNavMesh->Restore_Snapshot(Backup);
		Log_EditStatus(LOG_LEVEL::ERROR_, "Failed to create cell.");
		return E_FAIL;
	}


	Push_NavMeshUndoSnapshot(Backup);
	m_iSelectedNavMeshCellIndex = iCellIndex;

	Clear_NavMeshPickPoints();

	_char szStatus[128] = {};
	sprintf_s(szStatus, "Created Cell: %d", iCellIndex);
	Log_EditStatus(LOG_LEVEL::INFO, szStatus);

	return S_OK;
}

void CPanel_Viewport::Clear_NavMeshPickPoints()
{
	m_NavMeshPickedPoints.clear();
	m_bHasLastNavMeshPick = false;
	m_vLastNavMeshPick = {};
}

void CPanel_Viewport::Log_EditStatus(LOG_LEVEL eLevel, const string& strMessage) const
{
	Log_Message(eLevel, "[NavMesh] " + strMessage);
}

#pragma endregion


CPanel_Viewport* CPanel_Viewport::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_Viewport* pInstance = new CPanel_Viewport(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_Viewport");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_Viewport::Free()
{
	__super::Free();
	Release_RenderTarget();
}
