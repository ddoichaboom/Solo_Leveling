#include "VIBuffer.h"

CVIBuffer::CVIBuffer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{

}

CVIBuffer::CVIBuffer(const CVIBuffer& Prototype)
	: CComponent{ Prototype }
	, m_pVB { Prototype.m_pVB }
	, m_pIB{ Prototype.m_pIB }
	, m_iNumVertexBuffers{ Prototype.m_iNumVertexBuffers }
	, m_iNumVertices{ Prototype.m_iNumVertices }
	, m_iVertexStride{ Prototype.m_iVertexStride }
	, m_iNumIndices{ Prototype.m_iNumIndices }
	, m_iIndexStride{ Prototype.m_iIndexStride }
	, m_eIndexFormat{ Prototype.m_eIndexFormat }
	, m_ePrimitiveType{ Prototype.m_ePrimitiveType }
{
	Safe_AddRef(m_pVB);
	Safe_AddRef(m_pIB);
}

HRESULT CVIBuffer::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CVIBuffer::Initialize(void* pArg)
{
	return S_OK;
}

// IA(Input Assembler) Stage에 버퍼를 세팅하는 핵심 함수
HRESULT CVIBuffer::Bind_Resources()
{
	// 1) Vertex Buffer 배열 준비
	// - 슬롯이 여러 개일 수 있으므로 배열로 선언
	ID3D11Buffer*	pVertexBuffers[] = {
		m_pVB,
	};

	_uint			iVertexStrides[] = {
		m_iVertexStride,	// 정점 1개의 바이트 크기
	};

	_uint			iOffsets[] = {
		0,					// 버퍼 시작 오프셋
	};

	// 2) IA 스테이지 세팅
	m_pContext->IASetVertexBuffers(0, m_iNumVertexBuffers, pVertexBuffers, iVertexStrides, iOffsets);
	m_pContext->IASetIndexBuffer(m_pIB, m_eIndexFormat, 0);
	m_pContext->IASetPrimitiveTopology(m_ePrimitiveType);

	return S_OK;
}

// 실제 GPU 드로우 콜
HRESULT CVIBuffer::Render()
{
	// DrawIndexed(인덱스 수, 시작 인덱스 위치, 정점 오프셋)
	m_pContext->DrawIndexed(m_iNumIndices, 0, 0);

	return S_OK;
}

void CVIBuffer::Free()
{
	__super::Free();

	Safe_Release(m_pVB);
	Safe_Release(m_pIB);
}