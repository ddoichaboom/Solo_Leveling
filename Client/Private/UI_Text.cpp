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

    m_eHAlign = pDesc->eHAlign;
    m_eVAlign = pDesc->eVAlign;
    m_bAutoFit = pDesc->bAutoFit;

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

    _float2 vTextSize{};
    if (FAILED(m_pGameInstance->Measure_Font(m_strFontTag, m_strText.c_str(), &vTextSize)))
        return E_FAIL;

    // 최종 스케일 결정 (AutoFit 시 박스 비율로 축소)
    _float fFinalScale = m_fScale;
    if (m_bAutoFit && vTextSize.x > 0.f && vTextSize.y > 0.f && m_fSizeX > 0.f && m_fSizeY > 0.f)
    {
        const _float fFitX = m_fSizeX / (vTextSize.x * m_fScale);
        const _float fFitY = m_fSizeY / (vTextSize.y * m_fScale);
        const _float fFit = (fFitX < fFitY) ? fFitX : fFitY;
        fFinalScale = m_fScale * fFit;
    }

    const _float fDrawW = vTextSize.x * fFinalScale;
    const _float fDrawH = vTextSize.y * fFinalScale;

    // 박스 좌측 상단
    const _float fBoxLeft = m_fCenterX - m_fSizeX * 0.5f;
    const _float fBoxTop = m_fCenterY - m_fSizeY * 0.5f;

    // 정렬에 따른 오프셋
    _float fOffsetX = 0.f;
    switch (m_eHAlign)
    {
    case UI_TEXT_HALIGN::LEFT:   fOffsetX = 0.f; 
        break;
    case UI_TEXT_HALIGN::CENTER: fOffsetX = (m_fSizeX - fDrawW) * 0.5f; 
        break;
    case UI_TEXT_HALIGN::RIGHT:  fOffsetX = m_fSizeX - fDrawW; 
        break;
    default: break;
    }

    _float fOffsetY = 0.f;
    switch (m_eVAlign)
    {
    case UI_TEXT_VALIGN::TOP:    fOffsetY = 0.f; 
        break;
    case UI_TEXT_VALIGN::MIDDLE: fOffsetY = (m_fSizeY - fDrawH) * 0.5f; 
        break;
    case UI_TEXT_VALIGN::BOTTOM: fOffsetY = m_fSizeY - fDrawH; 
        break;
    default: break;
    }

    _float2 vPosition;
    vPosition.x = fBoxLeft + fOffsetX;
    vPosition.y = fBoxTop + fOffsetY;

    return m_pGameInstance->Render_Font(
        m_strFontTag,
        m_strText.c_str(),
        vPosition,
        XMLoadFloat4(&m_vColor),
        m_fRotation,
        _float2(0.f, 0.f),
        _float2(fFinalScale, fFinalScale));
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