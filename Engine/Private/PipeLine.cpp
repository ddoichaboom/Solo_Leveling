#include "PipeLine.h"

CPipeLine::CPipeLine()
{
	// 항등 행렬로 초기화
	for (size_t i = 0; i < ETOUI(D3DTS::END); i++)
	{
		XMStoreFloat4x4(&m_TransformStateMatrices[i], XMMatrixIdentity());
		XMStoreFloat4x4(&m_TransformStateInverseMatrices[i], XMMatrixIdentity());
	}
}

void CPipeLine::Compute_WorldRay(_float fViewportX, _float fViewportY, _float fViewportWidth, _float fViewportHeight, _float4* pRayOrigin, _float4* pRayDir)
{
	// (1) Viewport 좌표 -> View Space 방향
	_matrix matProj = XMLoadFloat4x4(&m_TransformStateMatrices[ETOUI(D3DTS::PROJ)]);

	_float fX = (2.f * fViewportX / fViewportWidth - 1.f) / XMVectorGetX(matProj.r[0]);
	_float fY = (-2.f * fViewportY / fViewportHeight + 1.f) / XMVectorGetY(matProj.r[1]);

	// (2) View Space -> World Space
	_matrix matViewInverse = XMLoadFloat4x4(&m_TransformStateInverseMatrices[ETOUI(D3DTS::VIEW)]);

	_vector vRayDir = XMVector3Normalize(
		XMVector3TransformNormal(XMVectorSet(fX, fY, 1.f, 0.f), matViewInverse));

	// (3) 결과 저장
	*pRayOrigin = m_vCamPosition;
	XMStoreFloat4(pRayDir, vRayDir);
}

void CPipeLine::Update()
{
	// 모든 변환 행렬의 역행렬 계산
	for (size_t i = 0; i < ETOUI(D3DTS::END); i++)
	{
		XMStoreFloat4x4(&m_TransformStateInverseMatrices[i],
			XMMatrixInverse(nullptr, XMLoadFloat4x4(&m_TransformStateMatrices[i])));
	}

	// View 역행렬의 4번째 행 = 카메라 월드 위치
	memcpy(&m_vCamPosition, &m_TransformStateInverseMatrices[ETOUI(D3DTS::VIEW)]._41, sizeof m_vCamPosition);
}

CPipeLine* CPipeLine::Create()
{
	return new CPipeLine();
}

void CPipeLine::Free()
{
	__super::Free();
}