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
	virtual HRESULT				Initialize_Prototype();
	virtual HRESULT				Initialize(void* pArg);

	virtual HRESULT				Bind_Resources();			// IA 스테이지에 버퍼 바인딩
	virtual HRESULT				Render();					// DrawIndexed 호출

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



public:
	virtual CComponent*			Clone(void* pArg) PURE;
	virtual void				Free() override;
};

NS_END