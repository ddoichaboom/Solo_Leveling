#pragma once

#include "Bounding.h"

NS_BEGIN(Engine)

class CBounding_OBB final : public CBounding
{
public:
    typedef struct tagBoundingOBBDesc : public CBounding::BOUNDING_DESC
    {
        _float3     vSize;        
        _float3     vRadians;     
    } BOUNDING_OBB_DESC;

private:
    CBounding_OBB(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CBounding_OBB() = default;

public:
    const BoundingOrientedBox*  Get_Desc() const { return m_pDesc; }

public:
    virtual HRESULT             Initialize(const CBounding::BOUNDING_DESC* pBoundingDesc) override;
    virtual void                Update(_fmatrix WorldMatrix) override;

public:
    virtual _bool               Intersect(COLLIDER eTargetType, CBounding* pTarget) override;

#ifdef _DEBUG
public:
    virtual HRESULT             Render(PrimitiveBatch<VertexPositionColor>* pBatch) override;
#endif

private:
    BoundingOrientedBox*        m_pOriginalDesc = { nullptr };
    BoundingOrientedBox*        m_pDesc = { nullptr };

public:
    static CBounding_OBB*       Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const CBounding::BOUNDING_DESC* pBoundingDesc);
    virtual void                Free() override;
};

NS_END

