#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer abstract : public CComponent
{
protected:
	CVIBuffer(ID3D11Device * pDevice, ID3D11DeviceContext * pContext);
	CVIBuffer(const CVIBuffer& Prototype);
	virtual ~CVIBuffer() = default;

public:
	// CPU 스키닝
	PICK_DATA*					Get_PickData() const { return m_pPickData; }

public:
	virtual HRESULT				Initialize_Prototype();
	virtual HRESULT				Initialize(void* pArg);

	virtual HRESULT				Bind_Resources();			// IA 스테이지에 버퍼 바인딩
	virtual HRESULT				Render();					// DrawIndexed 호출

public:
	_bool						Pick(_fvector vRayOrigin, _fvector vRayDir, _fmatrix matWorld, _float& fDist);

protected:
	ID3D11Buffer*				m_pVB = { nullptr };		//  정점 버퍼 (GPU)
	_uint						m_iNumVertexBuffers = {};	// VB 개수 (보통 1)
	_uint						m_iNumVertices = {};		// 정점 수 
	_uint						m_iVertexStride = {};		// 정점 1개의 바이트 크기

	ID3D11Buffer*				m_pIB = { nullptr };		// 인덱스 버퍼 (GPU)
	_uint						m_iNumIndices = {};			// 인덱스 수
	_uint						m_iIndexStride = {};		// 인덱스 1개의 바이트 크기
	DXGI_FORMAT					m_eIndexFormat = {};		// 인덱스 포맷 (R16_UINT 등)

	D3D11_PRIMITIVE_TOPOLOGY	m_ePrimitiveType = {};

	PICK_DATA*					m_pPickData = { nullptr };



public:
	virtual CComponent*			Clone(void* pArg) PURE;
	virtual void				Free() override;
};

NS_END