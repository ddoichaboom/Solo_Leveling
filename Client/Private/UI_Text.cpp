#include "UI_Text.h"
#include "GameInstance.h"

CUI_Text::CUI_Text(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUIObject{ pDevice, pContext }
{
}

CUI_Text::CUI_Text(const CUI_Text& Prototype)
    : CUIObject{ Prototype }
{
}

HRESULT CUI_Text::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUI_Text::Initialize(void* pArg)
{
    if (nullptr == pArg)
        return E_FAIL;

    auto pDesc = static_cast<UI_TEXT_DESC*>(pArg);

    m_strFontTag = (pDesc->pFontTag ? pDesc->pFontTag : TEXT(""));
    m_strText = (pDesc->pText ? pDesc->pText : TEXT(""));
    m_fScale = pDesc->fScale;
    m_vColor = pDesc->vColor;
    m_fRotation = pDesc->fRotation;
    m_iZOrder = pDesc->iZOrder;

    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    return S_OK;
}

void CUI_Text::Late_Update(_float fTimeDelta)
{
    m_pGameInstance->Add_RenderGroup(RENDERID::UI, this);
}

HRESULT CUI_Text::Render()
{
    if (m_strFontTag.empty() || m_strText.empty())
        return S_OK;

    // 중심 정렬: Measure → top-left = Center - (size * scale) / 2
    _float2 vTextSize{};
    if (FAILED(m_pGameInstance->Measure_Font(m_strFontTag, m_strText.c_str(), &vTextSize)))
        return E_FAIL;

    _float2 vPosition;
    vPosition.x = m_fCenterX - vTextSize.x * m_fScale * 0.5f;
    vPosition.y = m_fCenterY - vTextSize.y * m_fScale * 0.5f;

    return m_pGameInstance->Render_Font(
        m_strFontTag,
        m_strText.c_str(),
        vPosition,
        XMLoadFloat4(&m_vColor),
        m_fRotation,
        _float2(0.f, 0.f),
        _float2(m_fScale, m_fScale));
}

CUI_Text* CUI_Text::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUI_Text* pInstance = new CUI_Text(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CUI_Text");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CUI_Text::Clone(void* pArg)
{
    CUI_Text* pInstance = new CUI_Text(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CUI_Text");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CUI_Text::Free()
{
    __super::Free();
}