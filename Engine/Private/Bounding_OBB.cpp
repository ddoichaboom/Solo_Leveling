#include "Bounding_OBB.h"
#include "Bounding_AABB.h"
#include "Bounding_Sphere.h"
#include "DebugDraw.h"

CBounding_OBB::CBounding_OBB(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : CBounding{ pDevice, pContext }
{
}

HRESULT CBounding_OBB::Initialize(const CBounding::BOUNDING_DESC* pBoundingDesc)
{
    auto pDesc = static_cast<const BOUNDING_OBB_DESC*>(pBoundingDesc);

    _float4 vQuaternion = {};
    XMStoreFloat4(&vQuaternion,
        XMQuaternionRotationRollPitchYaw(pDesc->vRadians.x, pDesc->vRadians.y, pDesc->vRadians.z));

    const _float3 vHalfExtent = _float3(pDesc->vSize.x * 0.5f, pDesc->vSize.y * 0.5f, pDesc->vSize.z * 0.5f);

    m_pOriginalDesc = new BoundingOrientedBox(pDesc->vCenter, vHalfExtent, vQuaternion);
    m_pDesc = new BoundingOrientedBox(*m_pOriginalDesc);

    return S_OK;
}

void CBounding_OBB::Update(_fmatrix WorldMatrix)
{
    // OBB 는 회전 포함 그대로 변환 가능
    m_pOriginalDesc->Transform(*m_pDesc, WorldMatrix);
}

_bool CBounding_OBB::Intersect(COLLIDER eTargetType, CBounding* pTarget)
{
    m_bIsColl = false;

    // DirectXTK BoundingOrientedBox::Intersects 가 3타입 모두 오버로드 제공.
    // OBB-OBB 는 내부적으로 분리축 정리(SAT) 6축 검사를 수행한다.
    switch (eTargetType)
    {
    case COLLIDER::AABB:
        m_bIsColl = m_pDesc->Intersects(*dynamic_cast<CBounding_AABB*>(pTarget)->Get_Desc());
        break;
    case COLLIDER::OBB:
        m_bIsColl = m_pDesc->Intersects(*dynamic_cast<CBounding_OBB*>(pTarget)->Get_Desc());
        break;
    case COLLIDER::SPHERE:
        m_bIsColl = m_pDesc->Intersects(*dynamic_cast<CBounding_Sphere*>(pTarget)->Get_Desc());
        break;
    default:
        break;
    }

    return m_bIsColl;
}

#ifdef _DEBUG
HRESULT CBounding_OBB::Render(PrimitiveBatch<VertexPositionColor>* pBatch)
{
    const XMVECTOR vColor = m_bIsColl
        ? XMVectorSet(1.f, 0.f, 0.f, 1.f)
        : XMVectorSet(0.f, 1.f, 0.f, 1.f);

    DX::Draw(pBatch, *m_pDesc, vColor);
    return S_OK;
}
#endif

CBounding_OBB* CBounding_OBB::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CBounding::BOUNDING_DESC* pDesc)
{
    CBounding_OBB* pInstance = new CBounding_OBB(pDevice, pContext);

    if (FAILED(pInstance->Initialize(pDesc)))
    {
        MSG_BOX("Failed to Created : CBounding_OBB");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CBounding_OBB::Free()
{
    __super::Free();

    Safe_Delete(m_pDesc);
    Safe_Delete(m_pOriginalDesc);
}