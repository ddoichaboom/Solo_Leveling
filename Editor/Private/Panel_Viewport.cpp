#include "Panel_Viewport.h"

CPanel_Viewport::CPanel_Viewport(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Viewport::Initialize()
{
	strcpy_s(m_szName, "Viewport");

	return S_OK;
}

void CPanel_Viewport::Update(_float fTimeDelta)
{
}

void CPanel_Viewport::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	ImGui::Text("3D Viewport");

	ImGui::End();
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
}
