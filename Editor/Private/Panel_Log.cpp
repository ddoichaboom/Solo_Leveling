#include "Panel_Log.h"

CPanel_Log::CPanel_Log(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CPanel{ pDevice, pContext }
{
}

HRESULT CPanel_Log::Initialize()
{
	strcpy_s(m_szName, "Log");

	return S_OK;
}

void CPanel_Log::Update(_float fTimeDelta)
{
}

void CPanel_Log::Render()
{
	ImGui::Begin(m_szName, &m_bOpen);

	ImGui::Text("Log Output");

	ImGui::End();
}

CPanel_Log* CPanel_Log::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CPanel_Log* pInstance = new CPanel_Log(pDevice, pContext);

	if (FAILED(pInstance->Initialize()))
	{
		MSG_BOX("Failed to Create : CPanel_Log");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CPanel_Log::Free()
{
	__super::Free();
}
