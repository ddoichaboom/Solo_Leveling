#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CSpringArm final : public CComponent
{
public:
	typedef struct tagSpringArmDesc
	{
		const _float4x4*	pTargetWorldMatrix = { nullptr };		// 따라갈 Target
		_float3				vHeightOffset = { 0.f, 1.5f, 0.f };

		_float				fIdealDistance = { 5.f };
		_float				fArmLerpSpeed = { 8.f };		// 보면서 수정

		_float				fInitialYaw = { 0.f };
		_float				fInitialPitch = { -0.3f };
		_float				fPitchMin = { -1.4f };
		_float				fPitchMax = { 1.0f };
		_float				fMouseSensor = { 0.003f };
	}SPRING_ARM_DESC;

private:
	CSpringArm(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CSpringArm(const CSpringArm& Prototype);
	virtual ~CSpringArm() = default;

public:
	void				Set_TargetWorldMatrix(const _float4x4* pTargetMat) 
	{ 
		m_pTargetWorldMatrix = pTargetMat; 
	}
	_vector				Get_TargetPoint() const;
	_vector				Get_LookDirection() const;
	_vector				Get_EyePosition() const;
	_float				Get_Yaw() const;

	_float				Get_CurrentDistance() const { return m_fCurrentDistance; }
	_float				Get_IdealDistance() const { return m_fIdealDistance; }

public:
	virtual HRESULT		Initialize_Prototype() override;
	virtual HRESULT		Initialize(void* pArg) override;

public:
	void				Update_Rotation(_long lMouseDX, _long lMouseDY);
	void				Update_Arm(_float fTimeDelta);

private:
	const _float4x4*	m_pTargetWorldMatrix = { nullptr };
	_float3				m_vHeightOffset = {};
	_float				m_fIdealDistance = {};
	_float				m_fArmLerpSpeed = {};
	_float				m_fPitchMin = {};
	_float				m_fPitchMax = {};
	_float				m_fMouseSensor = {};

	_float4				m_qOrientation = {};
	_float				m_fCurrentDistance = {};

public:
	static	CSpringArm* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent* Clone(void* pArg) override;
	virtual void		Free() override;
};

NS_END