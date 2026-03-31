#include "UIObject.h"
#include "GameInstance.h"

CUIObject::CUIObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CGameObject { pDevice, pContext }
{

}

CUIObject::CUIObject(const CUIObject& Prototype)
    : CGameObject{ Prototype }
{

}

HRESULT CUIObject::Initialize_Prototype()
{
    return S_OK;
}

HRESULT CUIObject::Initialize(void* pArg)
{
    if (nullptr == pArg)
        return E_FAIL;

    auto pDesc  = static_cast<UIOBJECT_DESC*>(pArg);

    m_fCenterX  = pDesc->fCenterX;
    m_fCenterY  = pDesc->fCenterY;
    m_fSizeX    = pDesc->fSizeX;
    m_fSizeY    = pDesc->fSizeY;

    // Transform Component 생성
    if (FAILED(__super::Initialize(pArg)))
        return E_FAIL;

    _uint           iNumViewport = { 1 };     // 뷰포트 개수 1개
    D3D11_VIEWPORT  ViewportDesc = {};
    m_pContext->RSGetViewports(&iNumViewport, &ViewportDesc);

    m_fViewWidth    = ViewportDesc.Width;
    m_fViewHeight   = ViewportDesc.Height;

    // 직교 투영으로 그려주기 위한 월드 행렬 세팅
    Update_UIState();

    // 직교 투영으로 그려주기 위한 뷰 행렬 세팅
    XMStoreFloat4x4(&m_TransformMatrices[ETOUI(D3DTS::VIEW)], XMMatrixIdentity());

    // 직교 투영으로 그려주기 위한 투영 행렬 세팅
    XMStoreFloat4x4(&m_TransformMatrices[ETOUI(D3DTS::PROJ)], XMMatrixOrthographicLH(ViewportDesc.Width, ViewportDesc.Height, 0.f, 1.f));

    return S_OK;
}

void CUIObject::Priority_Update(_float fTimeDelta)
{
}

void CUIObject::Update(_float fTimeDelta)
{
}

void CUIObject::Late_Update(_float fTimeDelta)
{
}

HRESULT CUIObject::Render()
{
    return S_OK;
}

void CUIObject::Update_UIState()
{
    m_pTransformCom->Set_Scale(m_fSizeX, m_fSizeY, 1.f);
    m_pTransformCom->Set_State(STATE::POSITION,
        XMVectorSet(m_fCenterX - m_fViewWidth * 0.5f, -m_fCenterY + m_fViewHeight * 0.5f, 0.f, 1.f));

}

HRESULT CUIObject::Bind_ShaderResource(CShader* pShader, const _char* pConstantName, D3DTS eType)
{
    return pShader->Bind_Matrix(pConstantName, &m_TransformMatrices[ETOUI(eType)]);
}

void CUIObject::Free()
{
    __super::Free();


}