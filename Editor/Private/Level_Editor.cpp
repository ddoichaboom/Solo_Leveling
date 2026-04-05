#include "Level_Editor.h"

CLevel_Editor::CLevel_Editor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CLevel { pDevice, pContext }
{

}

CLevel_Editor* CLevel_Editor::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	return new CLevel_Editor(pDevice, pContext);
}

void CLevel_Editor::Free()
{
	__super::Free();
}
