#include "Panel_ContentBrowser.h"

CPanel_ContentBrowser::CPanel_ContentBrowser(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_ContentBrowser::Initialize()
{
	strcpy_s(m_szName, "Content Browser");

	return S_OK;
}

void CPanel_ContentBrowser::Update(_float fTimeDelta)
{
}

void CPanel_ContentBrowser::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	ImGui::Text("Resources");

	ImGui::End();
}

CPanel_ContentBrowser* CPanel_ContentBrowser::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_ContentBrowser* pInstance = new CPanel_ContentBrowser(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_ContentBrowser");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_ContentBrowser::Free()
{
	__super::Free();
}
