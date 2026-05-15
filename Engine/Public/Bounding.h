#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CBounding abstract : public CBase
{
public:
	typedef struct tagBoundingDesc
	{
		_float3 vCenter;
	}BOUNDING_DESC;

protected:
	CBounding(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CBounding() = default;

public:
	virtual HRESULT			Initialize(const BOUNDING_DESC* pDesc);
	virtual void			Update(_fmatrix WorldMat) PURE;

public:
	virtual _bool			Intersect(COLLIDER eTargetType, CBounding* pTarget) PURE;

	_bool					Is_Coll() const { return m_bIsColl; }
	void					Set_Coll(_bool bValue) { m_bIsColl = bValue; }

#ifdef _DEBUG
public:
	virtual HRESULT			Render(PrimitiveBatch<VertexPositionColor>* pBatch) PURE;
#endif

protected:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	_bool					m_bIsColl = { false };

public:
	virtual void			Free() override;
};

NS_END