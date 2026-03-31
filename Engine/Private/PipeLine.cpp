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