#include "UI_Cursor.h"
#include "GameInstance.h"

CUI_Cursor::CUI_Cursor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CUI_Image{ pDevice, pContext }
{
}

CUI_Cursor::CUI_Cursor(const CUI_Cursor& Prototype)
    : CUI_Image{ Prototype }
{
}

HRESULT CUI_Cursor::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUI_Cursor::Initialize(void* pArg)
{
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    Set_Alpha(1.f);
    return S_OK;
}

void CUI_Cursor::Update(_float fTimeDelta)
{
    __super::Update(fTimeDelta);

    const _bool bLocked = m_pGameInstance->Is_CursorLocked();
    Set_Visible(!bLocked);
    if (bLocked)
        return;

    POINT pt{};
    if (!GetCursorPos(&pt))
        return;
    ScreenToClient(m_pGameInstance->Get_hWnd(), &pt);

    m_fCenterX = static_cast<_float>(pt.x) + m_fSizeX * 0.5f;
    m_fCenterY = static_cast<_float>(pt.y) + m_fSizeY * 0.5f;
    Update_UIState();
}

CUI_Cursor* CUI_Cursor::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CUI_Cursor* pInstance = new CUI_Cursor(pDevice, pContext);
    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CUI_Cursor");
        Safe_Release(pInstance);
    }
    return pInstance;
}

CGameObject* CUI_Cursor::Clone(void* pArg)
{
    CUI_Cursor* pInstance = new CUI_Cursor(*this);
    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CUI_Cursor");
        Safe_Release(pInstance);
    }
    return pInstance;
}

void CUI_Cursor::Free()
{
    __super::Free();
}