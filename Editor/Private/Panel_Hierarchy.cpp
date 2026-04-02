#include "Panel_Hierarchy.h"

CPanel_Hierarchy::CPanel_Hierarchy(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Hierarchy::Initialize()
{
	strcpy_s(m_szName, "Hierarchy");

	return S_OK;
}

void CPanel_Hierarchy::Update(_float fTimeDelta)
{
}

void CPanel_Hierarchy::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	ImGui::Text("Scene Objects");

	ImGui::End();
}

CPanel_Hierarchy* CPanel_Hierarchy::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_Hierarchy* pInstance = new CPanel_Hierarchy(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_Hierarchy");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_Hierarchy::Free()
{
	__super::Free();
}
