#include "UI_Image.h"
#include "GameInstance.h"

CUI_Image::CUI_Image(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CUIObject{ pDevice, pContext }
{
}

CUI_Image::CUI_Image(const CUI_Image& Prototype)
	: CUIObject{ Prototype }
	, m_pShaderCom{ Prototype.m_pShaderCom }
	, m_pVIBufferCom{ Prototype.m_pVIBufferCom }
	, m_pTextureCom{ Prototype.m_pTextureCom }
{
}

void CUI_Image::Set_Progress(_float fProgress)
{
	if (!m_bBaseCached)
	{
		m_fBaseCenterX = m_fCenterX;
		m_fBaseSizeX = m_fSizeX;
		m_bBaseCached = true;
	}

	if (fProgress < 0.f) 
		fProgress = 0.f;
	if (fProgress > 1.f) 
		fProgress = 1.f;

	const _float fNewSizeX = m_fBaseSizeX * fProgress;
	// 좌측 앵커: 왼쪽 가장자리를 base 의 left 에 고정
	const _float fLeft = m_fBaseCenterX - m_fBaseSizeX * 0.5f;
	m_fSizeX = fNewSizeX;
	m_fCenterX = fLeft + fNewSizeX * 0.5f;
}

HRESULT CUI_Image::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CUI_Image::Initialize(void* pArg)
{
	auto pDesc = static_cast<UI_IMAGE_DESC*>(pArg);
	m_iZOrder = pDesc->iZOrder;

	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (FAILED(Ready_Components(pDesc)))
		return E_FAIL;

	return S_OK;
}

void CUI_Image::Late_Update(_float fTimeDelta)
{
	m_pGameInstance->Add_RenderGroup(RENDERID::UI, this);
}

HRESULT CUI_Image::Render()
{
	if (FAILED(Bind_ShaderResources()))
		return E_FAIL;

	if (FAILED(m_pShaderCom->Begin(1)))
		return E_FAIL;

	if (FAILED(m_pVIBufferCom->Bind_Resources()))
		return E_FAIL;

	if (FAILED(m_pVIBufferCom->Render()))
		return E_FAIL;

	return S_OK;
}

HRESULT CUI_Image::Ready_Components(const UI_IMAGE_DESC* pDesc)
{
	if (FAILED(__super::Add_Component(ETOUI(LEVEL::STATIC),
		TEXT("Prototype_Component_Shader_VtxTex"),
		TEXT("Com_Shader"), reinterpret_cast<CComponent**>(&m_pShaderCom))))
		return E_FAIL;

	if (FAILED(__super::Add_Component(ETOUI(LEVEL::STATIC),
		TEXT("Prototype_Component_VIBuffer_Rect"),
		TEXT("Com_VIBuffer"), reinterpret_cast<CComponent**>(&m_pVIBufferCom))))
		return E_FAIL;

	// Pattern A: 프로토타입 풀에서 Clone 우선
	const _bool bUseProto = (nullptr != pDesc->pTextureProtoTag) && (L'\0' != pDesc->pTextureProtoTag[0]);

	if (bUseProto)
	{
		if (FAILED(__super::Add_Component(pDesc->iTextureProtoLevel,
			pDesc->pTextureProtoTag,
			TEXT("Com_Texture"), reinterpret_cast<CComponent**>(&m_pTextureCom))))
			return E_FAIL;
	}
	else
	{
		// Pattern B: 경로 기반 직접 로드 (Editor 미리보기/일회성 용)
		m_pTextureCom = Engine::CTexture::Create(m_pDevice, m_pContext, pDesc->pTexturePath, 1);
		if (nullptr == m_pTextureCom)
			return E_FAIL;
	}

	return S_OK;
}

HRESULT CUI_Image::Bind_ShaderResources()
{
	if (FAILED(m_pTransformCom->Bind_ShaderResource(m_pShaderCom, "g_WorldMatrix")))
		return E_FAIL;

	if (FAILED(__super::Bind_ShaderResource(m_pShaderCom, "g_ViewMatrix", D3DTS::VIEW)))
		return E_FAIL;

	if (FAILED(__super::Bind_ShaderResource(m_pShaderCom, "g_ProjMatrix", D3DTS::PROJ)))
		return E_FAIL;

	if (FAILED(m_pTextureCom->Bind_ShaderResource(m_pShaderCom, "g_Texture", 0)))
		return E_FAIL;

	return S_OK;
}

CUI_Image* CUI_Image::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CUI_Image* pInstance = new CUI_Image(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CUI_Image");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CUI_Image::Clone(void* pArg)
{
	CUI_Image* pInstance = new CUI_Image(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CUI_Image");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CUI_Image::Free()
{
	__super::Free();

	Safe_Release(m_pShaderCom);
	Safe_Release(m_pTextureCom);
	Safe_Release(m_pVIBufferCom);
}
