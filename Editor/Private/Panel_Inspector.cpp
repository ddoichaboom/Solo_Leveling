#include "Panel_Inspector.h"

CPanel_Inspector::CPanel_Inspector(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Inspector::Initialize()
{
	strcpy_s(m_szName, "Inspector");

	return S_OK;
}

void CPanel_Inspector::Update(_float fTimeDelta)
{
}

void CPanel_Inspector::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	ImGui::Text("Properties");

	ImGui::End();
}

CPanel_Inspector* CPanel_Inspector::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_Inspector* pInstance = new CPanel_Inspector(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_Inspector");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_Inspector::Free()
{
	__super::Free();
}
