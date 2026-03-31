#include "Transform_2D.h"

CTransform_2D::CTransform_2D(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CTransform { pDevice, pContext }
{
}

CTransform_2D::CTransform_2D(const CTransform_2D& Prototype)
    : CTransform { Prototype }
{
}

void CTransform_2D::Move_X(_float fTimeDelta)
{
    _vector vPosition = Get_State(STATE::POSITION);

    vPosition += XMVectorSet(1.f, 0.f, 0.f, 0.f) * m_fSpeedPerSec * fTimeDelta;

    Set_State(STATE::POSITION, vPosition);
}

void CTransform_2D::Move_Y(_float fTimeDelta)
{
    _vector vPosition = Get_State(STATE::POSITION);

    vPosition += XMVectorSet(0.f, 1.f, 0.f, 0.f) * m_fSpeedPerSec * fTimeDelta;

    Set_State(STATE::POSITION, vPosition);
}

CTransform_2D* CTransform_2D::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CTransform_2D* pInstance = new CTransform_2D(pDevice, pContext);

    if (FAILED(pInstance->Initialize_Prototype()))
    {
        MSG_BOX("Failed to Created : CTransform_2D");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CComponent* CTransform_2D::Clone(void* pArg)
{
    CTransform_2D* pInstance = new CTransform_2D(*this);

    if (FAILED(pInstance->Initialize(pArg)))
    {
        MSG_BOX("Failed to Cloned : CTransform_2D");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CTransform_2D::Free()
{
    __super::Free();
}
