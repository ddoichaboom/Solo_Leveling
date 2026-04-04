#include "VIBuffer_Rect.h"

CVIBuffer_Rect::CVIBuffer_Rect(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CVIBuffer{pDevice, pContext}
{

}

CVIBuffer_Rect::CVIBuffer_Rect(const CVIBuffer_Rect& Prototype)
	: CVIBuffer{ Prototype }
{

}

HRESULT CVIBuffer_Rect::Initialize_Prototype()
{
	// 1) 메타 데이터 설정
	m_iNumVertexBuffers = 1;
	m_iNumVertices = 4;
	m_iVertexStride = sizeof(VTXTEX);

	m_iNumIndices = 6;
	m_iIndexStride = 2;
	m_eIndexFormat = DXGI_FORMAT_R16_UINT;

	m_ePrimitiveType = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;

	// 2)  Vertex Buffer 생성

	// 2-1) 버퍼 설명 구조체
	D3D11_BUFFER_DESC VertexBufferDesc{};
	VertexBufferDesc.ByteWidth = m_iVertexStride * m_iNumVertices;		// 20 x 4 = 80바이트
	VertexBufferDesc.Usage = D3D11_USAGE_DEFAULT;						// GPU 읽기/쓰기 (CPU 접근 불가) - 정적으로 설정
	VertexBufferDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;				
	VertexBufferDesc.CPUAccessFlags = 0;								// CPU 접근 안함
	VertexBufferDesc.MiscFlags = 0;
	VertexBufferDesc.StructureByteStride = m_iVertexStride;

	// 2-2) CPU에서 정점 데이터 준비
	VTXTEX* pVertices = new VTXTEX[m_iNumVertices];
	ZeroMemory(pVertices, sizeof(VTXTEX) * m_iNumVertices);

	pVertices[0].vPosition = _float3(-0.5f, 0.5f, 0.f);					// 좌상단
	pVertices[0].vTexcoord = _float2(0.f, 0.f);

	pVertices[1].vPosition = _float3(0.5f, 0.5f, 0.f);					// 우상단
	pVertices[1].vTexcoord = _float2(1.f, 0.f);

	pVertices[2].vPosition = _float3(0.5f, -0.5f, 0.f);					// 우하단
	pVertices[2].vTexcoord = _float2(1.f, 1.f);

	pVertices[3].vPosition = _float3(-0.5f, -0.5f, 0.f);				// 좌하단
	pVertices[3].vTexcoord = _float2(0.f, 1.f);

	// 2-3) GPU에 업로드
	D3D11_SUBRESOURCE_DATA		VertexInitialData{};
	VertexInitialData.pSysMem = pVertices;								// CPU 데이터 포인터

	if (FAILED(m_pDevice->CreateBuffer(&VertexBufferDesc, &VertexInitialData, &m_pVB)))
		return E_FAIL;

	// PICK_DATA 생성
	m_pPickData = new PICK_DATA{};
	m_pPickData->iNumVertices = m_iNumVertices;

	m_pPickData->pVerticesPos = new _float3[m_iNumVertices];
	for (size_t i = 0; i < m_iNumVertices; i++)
		m_pPickData->pVerticesPos[i] = pVertices[i].vPosition;

	Safe_Delete_Array(pVertices);


	// 3) Index Buffer 생성
	D3D11_BUFFER_DESC			IndexBufferDesc{};
	IndexBufferDesc.ByteWidth = m_iIndexStride * m_iNumIndices;			// 2 x 6 = 12 바이트
	IndexBufferDesc.Usage = D3D11_USAGE_DEFAULT;
	IndexBufferDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	IndexBufferDesc.CPUAccessFlags = 0;
	IndexBufferDesc.MiscFlags = 0;
	IndexBufferDesc.StructureByteStride = m_iIndexStride;
	
	_ushort* pIndices = new _ushort[m_iNumIndices];
	ZeroMemory(pIndices, sizeof(_ushort) * m_iNumIndices);


	// 삼각형 1 : 좌상 -> 우상 -> 우하
	pIndices[0] = 0;
	pIndices[1] = 1;
	pIndices[2] = 2;

	// 삼각형 2 : 좌상 -> 우하 -> 좌하
	pIndices[3] = 0;
	pIndices[4] = 2;
	pIndices[5] = 3;

	D3D11_SUBRESOURCE_DATA		IndexInitialData{};
	IndexInitialData.pSysMem =	pIndices;

	if (FAILED(m_pDevice->CreateBuffer(&IndexBufferDesc, &IndexInitialData, &m_pIB)))
		return E_FAIL;

	// PICK_DATA 인덱스 복사
	m_pPickData->iNumIndices = m_iNumIndices;
	m_pPickData->pIndices = new _uint[m_iNumIndices];
	
	for (size_t i = 0; i < m_iNumIndices; i++)
		m_pPickData->pIndices[i] = static_cast<_uint>(pIndices[i]);

	Safe_Delete_Array(pIndices);

	return S_OK;
}

HRESULT CVIBuffer_Rect::Initialize(void* pArg)
{
	return S_OK;
}

CVIBuffer_Rect* CVIBuffer_Rect::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CVIBuffer_Rect* pInstance = new CVIBuffer_Rect(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CVIBuffer_Rect");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CVIBuffer_Rect::Clone(void* pArg)
{
	CVIBuffer_Rect* pInstance = new CVIBuffer_Rect(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CVIBuffer_Rect");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CVIBuffer_Rect::Free()
{
	__super::Free();
}