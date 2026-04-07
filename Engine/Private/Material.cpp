#include "Material.h"
#include "Shader.h"

CMaterial::CMaterial(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: m_pDevice{ pDevice }
	, m_pContext{ pContext }
{
	Safe_AddRef(m_pDevice);
	Safe_AddRef(m_pContext);
}

HRESULT CMaterial::Initialize()
{
	return S_OK;
}

HRESULT CMaterial::Add_Texture(const MATERIAL_TEXTURE_DESC& Desc)
{
    ID3D11ShaderResourceView* pSRV = { nullptr };

    // .dds 먼저 시도 -> 실패하면 WIC
    if (FAILED(CreateDDSTextureFromFile(m_pDevice, Desc.szPath, nullptr, &pSRV)))
    {
        if (FAILED(CreateWICTextureFromFile(m_pDevice, Desc.szPath, nullptr, &pSRV)))
            return E_FAIL;
    }

    m_Textures[ETOUI(Desc.eType)].push_back(pSRV);

    return S_OK;
}

HRESULT CMaterial::Bind_ShaderResource(CShader* pShader, const _char* pConstantName, TEXTURE_TYPE eType, _uint iIndex)
{
    _uint iTypeIndex = ETOUI(eType);

    if (iIndex >= m_Textures[iTypeIndex].size())
        return E_FAIL;

    return pShader->Bind_SRV(pConstantName, m_Textures[iTypeIndex][iIndex]);
}

CMaterial* CMaterial::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CMaterial* pInstance = new CMaterial(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CMaterial");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CMaterial::Free()
{
    __super::Free();

    for (auto& SRVs : m_Textures)
    {
        for (auto& pSRV : SRVs)
            Safe_Release(pSRV);
        SRVs.clear();
    }

    Safe_Release(m_pDevice);
    Safe_Release(m_pContext);
}
