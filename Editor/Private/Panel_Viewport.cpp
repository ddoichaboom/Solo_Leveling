#include "Panel_Viewport.h"
#include "GameInstance.h"
#include "Layer.h"
#include "GameObject.h"
#include "Panel_Manager.h"
#include "Model.h"
#include "ContainerObject.h"
#include "PartObject.h"
#include "VIBuffer.h"
#include "Panel_NavMeshEditor.h"
#include "NavMeshEditorTool.h"
#include "Panel_2DCanvas.h"
#include "UICanvasTool.h"


namespace
{
	CNavMeshEditorTool* Find_NavMeshEditorTool(CPanel_Manager* pPanelManager)
	{
		if (nullptr == pPanelManager)
			return nullptr;

		CPanel* pPanel = pPanelManager->Get_Panel(TEXT("Panel_NavMeshEditor"));
		CPanel_NavMeshEditor* pNavMeshEditor = dynamic_cast<CPanel_NavMeshEditor*>(pPanel);
		if (nullptr == pNavMeshEditor)
			return nullptr;

		return pNavMeshEditor->Get_Tool();
	}
}
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

		const _bool bNavMeshToolbarHovered = false;
		CNavMeshEditorTool* pNavMeshEditorTool = Find_NavMeshEditorTool(m_pPanel_Manager);
		const _bool bNavMeshEditMode = m_pPanel_Manager->Is_NavMeshEditMode();
		CUICanvasTool* pUICanvasTool = Find_UICanvasTool();
		const _bool bUICanvasMode = m_pPanel_Manager->Is_UICanvasMode();

		// ImGuizmo 오버레이 세팅
		// (1) 기즈모 드로잉을 현재 Viewport 윈도우 drawlist에 연결
		ImGuizmo::SetDrawlist();

		// (2) 기즈모 수학이 사용할 스크린 공간 영역 = Image 위젯 영역과 일치
		ImGuizmo::SetRect(
			vImagePos.x, vImagePos.y,
			static_cast<_float>(m_iRTWidth),
			static_cast<_float>(m_iRTHeight));

		if (bWindowFocused &&
			bNavMeshEditMode &&
			false == ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
			false == ImGui::IsAnyItemActive() &&
			false == ImGui::IsKeyDown(ImGuiMod_Ctrl) &&
			false == ImGui::IsKeyDown(ImGuiMod_Alt) &&
			false == ImGui::IsKeyDown(ImGuiMod_Shift) &&
			ImGui::IsKeyPressed(ImGuiKey_C))
		{
			if (nullptr != pNavMeshEditorTool)
				pNavMeshEditorTool->Create_NavMeshCell();
		}

		// 기즈모 단축키 처리
		// Viewport 포커스 상태 + RMB(카메라 모드) 비활성 시에만 반응
		if (bWindowFocused && !bNavMeshEditMode && !bUICanvasMode  && !ImGui::IsMouseDown(ImGuiMouseButton_Right))
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

		if (false == bNavMeshEditMode && false == bUICanvasMode)
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
			false == bUICanvasMode &&
			ImGui::IsMouseClicked(ImGuiMouseButton_Left) &&
			!bGizmoBlocking)
		{
			ImVec2 vMousePos = ImGui::GetMousePos();
			m_fPickX = vMousePos.x - vImagePos.x;
			m_fPickY = vMousePos.y - vImagePos.y;

			if (bNavMeshEditMode)
			{
				if (nullptr != pNavMeshEditorTool)
						pNavMeshEditorTool->Handle_ViewportClick(m_fPickX, m_fPickY, m_iRTWidth, m_iRTHeight);
			}
			else
			{
				Pick_Object();
			}
		}

		if (bNavMeshEditMode && nullptr != pNavMeshEditorTool)
		{
			pNavMeshEditorTool->Render_Overlay(vImagePos, m_iRTWidth, m_iRTHeight);
		}
		else if (bUICanvasMode && nullptr != pUICanvasTool)
		{
			pUICanvasTool->Handle_Interaction(vImagePos, m_iRTWidth, m_iRTHeight, bViewportImageHovered);
			pUICanvasTool->Render_Overlay(vImagePos, m_iRTWidth, m_iRTHeight);
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



CUICanvasTool* CPanel_Viewport::Find_UICanvasTool()
{
	if (nullptr == m_pPanel_Manager)
		return nullptr;

	CPanel* pPanel = m_pPanel_Manager->Get_Panel(TEXT("Panel_2DCanvas"));
	CPanel_2DCanvas* pCanvas = dynamic_cast<CPanel_2DCanvas*>(pPanel);
	if (nullptr == pCanvas)
		return nullptr;

	return pCanvas->Get_Tool();
}

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
