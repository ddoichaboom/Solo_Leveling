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

	// PICK_DATA 깊은 복사
	if (Prototype.m_pPickData)
	{
		m_pPickData = new PICK_DATA{};
		m_pPickData->iNumVertices = Prototype.m_pPickData->iNumVertices;
		m_pPickData->iNumIndices = Prototype.m_pPickData->iNumIndices;

		m_pPickData->pVerticesPos = new _float3[m_pPickData->iNumVertices];
		memcpy(m_pPickData->pVerticesPos, Prototype.m_pPickData->pVerticesPos, sizeof(_float3) * m_pPickData->iNumVertices);

		m_pPickData->pIndices = new _uint[m_pPickData->iNumIndices];
		memcpy(m_pPickData->pIndices, Prototype.m_pPickData->pIndices, sizeof(_uint) * m_pPickData->iNumIndices);
	}
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

_bool CVIBuffer::Pick(_fvector vRayOrigin, _fvector vRayDir, _fmatrix matWorld, _float& fDist)
{
	if (nullptr == m_pPickData)
		return false;

	// (1) Ray를 로컬 스페이스로 변환
	_matrix matWorldInverse = XMMatrixInverse(nullptr, matWorld);

	_vector vLocalOrigin = XMVector3TransformCoord(vRayOrigin, matWorldInverse);
	_vector vLocalDir = XMVector3Normalize(XMVector3TransformNormal(vRayDir, matWorldInverse));

	// (2) 삼각형 순회
	_float fMinDist = FLT_MAX;
	_bool bHit = { false };

	for (size_t i = 0; i < m_pPickData->iNumIndices; i +=3)
	{
		_vector v0 = XMLoadFloat3(&m_pPickData->pVerticesPos[m_pPickData->pIndices[i]]);
		_vector v1 = XMLoadFloat3(&m_pPickData->pVerticesPos[m_pPickData->pIndices[i + 1]]);
		_vector v2 = XMLoadFloat3(&m_pPickData->pVerticesPos[m_pPickData->pIndices[i + 2]]);

		_float fLocalDist = { 0.f };
		if (TriangleTests::Intersects(vLocalOrigin, vLocalDir, v0, v1, v2, fLocalDist))
		{
			if (fLocalDist < fMinDist)
			{
				fMinDist = fLocalDist;
				bHit = true;
			}
		}
	}

	if (!bHit)
		return false;

	// (3) 로컬 거리 -> 월드 거리 변환
	_vector vLocalHit	= vLocalOrigin + vLocalDir * fMinDist;
	_vector vWorldHit	= XMVector3TransformCoord(vLocalHit, matWorld);
	fDist				= XMVectorGetX(XMVector3Length(vWorldHit - vRayOrigin));

	return true;
}

void CVIBuffer::Free()
{
	__super::Free();

	Safe_Release(m_pVB);
	Safe_Release(m_pIB);

	if (m_pPickData)
	{
		Safe_Delete_Array(m_pPickData->pVerticesPos);
		Safe_Delete_Array(m_pPickData->pIndices);
		Safe_Delete(m_pPickData);
	}
}