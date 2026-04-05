#include "Panel_Viewport.h"
#include "GameInstance.h"
#include "Layer.h"
#include "GameObject.h"
#include "Panel_Manager.h"

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

		if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
		{
			ImVec2 vMousePos = ImGui::GetMousePos();
			m_fPickX = vMousePos.x - vImagePos.x;
			m_fPickY = vMousePos.y - vImagePos.y;
			Pick_Object();
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
	if (0 == m_iRTWidth || 0 == m_iRTHeight)
		return;

	// (1) Screen → World Ray
	_float4 vRayOrigin = {};
	_float4 vRayDir = {};

	m_pGameInstance->Compute_WorldRay(
		m_fPickX, m_fPickY,
		static_cast<_float>(m_iRTWidth), static_cast<_float>(m_iRTHeight),
		&vRayOrigin, &vRayDir);

	_vector vOrigin = XMLoadFloat4(&vRayOrigin);
	_vector vDir	= XMLoadFloat4(&vRayDir);

	//_vector vOrigin = XMVectorSet(64.f, 100.f, 64.f, 1.f);
	//_vector vDir = XMVectorSet(0.f, -1.f, 0.f, 0.f);

	// (2) 현재 레벨의 오브젝트 순회
	_int iLevelIndex = m_pGameInstance->Get_CurrentLevelIndex();
	if (iLevelIndex < 0)
		return;

	const auto* pLayers = m_pGameInstance->Get_Layers(static_cast<_uint>(iLevelIndex));
	if (nullptr == pLayers)
		return;

	CGameObject* pPicked = { nullptr };
	_float fMinDist		= FLT_MAX;

	for (auto& LayerPair : *pLayers)
	{
		for (auto& pObject : LayerPair.second->Get_GameObjects())
		{
			CVIBuffer* pVIBuffer = pObject->Get_VIBuffer();
			if (nullptr == pVIBuffer)
				continue;

			_float fDist = { 0.f };
			if (pVIBuffer->Pick(vOrigin, vDir,
				XMLoadFloat4x4(pObject->Get_Transform()->Get_WorldMatrixPtr()), fDist))
			{
				if (fDist < fMinDist)
				{
					fMinDist = fDist;
					pPicked = pObject;
				}
			}
		}
	}

	// (3) 결과 반영
	if (nullptr != pPicked)
		m_pPanel_Manager->Set_SelectedObject(pPicked);
	else
		m_pPanel_Manager->Clear_Selection();
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
