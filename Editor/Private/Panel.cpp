#include "Panel.h"
#include "GameInstance.h"
#include "Panel_Manager.h"

CPanel::CPanel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice(pDevice)
	, m_pContext(pContext)
	, m_pGameInstance(CGameInstance::GetInstance())
	, m_pPanel_Manager(CPanel_Manager::GetInstance())
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
	Safe_AddRef(m_pGameInstance);
	Safe_AddRef(m_pPanel_Manager);
}

void CPanel::Free()
{
	__super::Free();

	Safe_Release(m_pPanel_Manager);
	Safe_Release(m_pGameInstance);
	Safe_Release(m_pContext);
	Safe_Release(m_pDevice);
}