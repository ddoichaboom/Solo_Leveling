#pragma once
#include "Bounding.h"

NS_BEGIN(Engine)

class CBounding_AABB final : public CBounding
{
public:
    typedef struct tagBoundingAABBDesc : public CBounding::BOUNDING_DESC
    {
        _float3     vSize;   
    } BOUNDING_AABB_DESC;

private:
    CBounding_AABB(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CBounding_AABB() = default;

public:
    const BoundingBox*      Get_Desc() const { return m_pDesc; }

public:
    virtual HRESULT         Initialize(const CBounding::BOUNDING_DESC* pBoundingDesc) override;
    virtual void            Update(_fmatrix WorldMatrix) override;

public:
    virtual _bool           Intersect(COLLIDER eTargetType, CBounding* pTarget) override;

#ifdef _DEBUG
public:
    virtual HRESULT         Render(PrimitiveBatch<VertexPositionColor>* pBatch) override;
#endif

private:
    BoundingBox*            m_pOriginalDesc = { nullptr };
    BoundingBox*            m_pDesc = { nullptr };

public:
    static CBounding_AABB*  Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CBounding::BOUNDING_DESC* pBoundingDesc);
    virtual void            Free() override;
};

NS_END

