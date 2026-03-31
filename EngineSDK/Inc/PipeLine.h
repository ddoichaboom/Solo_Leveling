#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CPipeLine final : public CBase
{
private:
	CPipeLine();
	virtual ~CPipeLine() = default;

public:
	// View/Proj 행렬 가져오기 
	const _float4x4*		Get_Transform(D3DTS eState) const {
		return &m_TransformStateMatrices[ETOUI(eState)];
	}

	// View/Proj 역행렬 가져오기
	const _float4x4*		Get_Transform_Inverse(D3DTS eState) const {
		return &m_TransformStateInverseMatrices[ETOUI(eState)];
	}

	// 카메라 월드 위치 가져오기
	const _float4*			Get_CamPosition() const {
		return &m_vCamPosition;
	}

public:
	// 카메라가 View/Proj 행렬을 세팅
	void					Set_Transform(D3DTS eState, _fmatrix StateMatrix) {
		XMStoreFloat4x4(&m_TransformStateMatrices[ETOUI(eState)], StateMatrix);
	}

public:
	void					Update();

private:
	_float4x4				m_TransformStateMatrices[ETOUI(D3DTS::END)] = {};
	_float4x4				m_TransformStateInverseMatrices[ETOUI(D3DTS::END)] = {};
	_float4					m_vCamPosition = {};

public:
	static CPipeLine*		Create();
	virtual void			Free() override;
};

NS_END