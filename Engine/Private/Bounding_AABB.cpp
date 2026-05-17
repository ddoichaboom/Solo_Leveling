#include "Bounding_AABB.h"
#include "Bounding_OBB.h"
#include "Bounding_Sphere.h"
#include "DebugDraw.h"


CBounding_AABB::CBounding_AABB(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CBounding{ pDevice, pContext }
{
}

HRESULT CBounding_AABB::Initialize(const CBounding::BOUNDING_DESC* pBoundingDesc)
{
	auto pDesc = static_cast<const BOUNDING_AABB_DESC*>(pBoundingDesc);

	const _float3 vHalfExtent = _float3(pDesc->vSize.x * 0.5f, pDesc->vSize.y * 0.5f, pDesc->vSize.z * 0.5f);

	m_pOriginalDesc = new BoundingBox(pDesc->vCenter, vHalfExtent);
	m_pDesc			= new BoundingBox(*m_pOriginalDesc);

	return S_OK;
}

void CBounding_AABB::Update(_fmatrix WorldMatrix)
{
	_matrix Matrix = WorldMatrix;
	Matrix.r[0] = XMVectorSet(1.f, 0.f, 0.f, 0.f) * XMVector3Length(Matrix.r[0]);
	Matrix.r[1] = XMVectorSet(0.f, 1.f, 0.f, 0.f) * XMVector3Length(Matrix.r[1]);
	Matrix.r[2] = XMVectorSet(0.f, 0.f, 1.f, 0.f) * XMVector3Length(Matrix.r[2]);

	m_pOriginalDesc->Transform(*m_pDesc, Matrix);
}

_bool CBounding_AABB::Intersect(COLLIDER eTargetType, CBounding* pTarget)
{
    m_bIsColl = false;

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
HRESULT CBounding_AABB::Render(PrimitiveBatch<VertexPositionColor>* pBatch)
{
    const XMVECTOR vColor = m_bIsColl
        ? XMVectorSet(1.f, 0.f, 0.f, 1.f)
        : XMVectorSet(0.f, 1.f, 0.f, 1.f);

    DX::Draw(pBatch, *m_pDesc, vColor);
    return S_OK;
}
#endif

CBounding_AABB* CBounding_AABB::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CBounding::BOUNDING_DESC* pBoundingDesc)
{
    CBounding_AABB* pInstance = new CBounding_AABB(pDevice, pContext);

    if (FAILED(pInstance->Initialize(pBoundingDesc)))
    {
        MSG_BOX("Failed to Created : CBounding_AABB");
        Safe_Release(pInstance);
    }

    return pInstance;
}
void CBounding_AABB::Free()
{
    __super::Free();

    Safe_Delete(m_pDesc);
    Safe_Delete(m_pOriginalDesc);
}
