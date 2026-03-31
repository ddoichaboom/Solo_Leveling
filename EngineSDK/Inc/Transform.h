#pragma once

#include "Component.h"

// 1. 객체의 월드 상태를 표현해주는 상태 변환 행렬을 보관한다 (WorldMatrix)
// 2. 월드 행렬의 상태 표현을 위한 여러 인터페이스를 보관한다.

NS_BEGIN(Engine)

class ENGINE_DLL CTransform final : public CComponent
{
public:
	typedef struct tagTransformDesc
	{
		_float		fScale = { 1.f };
		_float		fSpeedPerSec = {};
		_float		fRotationPerSec = {};
	}TRANSFORM_DESC;

protected:
	CTransform(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTransform(const CTransform& Prototype);
	virtual ~CTransform() = default;

public:
	_vector Get_State(STATE eState)
	{
		return XMLoadFloat4(reinterpret_cast<_float4*>(&m_WorldMatrix.m[ETOUI(eState)]));
	}

	_float3 Get_Scale()
	{
		return _float3{
			XMVectorGetX(XMVector3Length(Get_State(STATE::RIGHT))),
			XMVectorGetX(XMVector3Length(Get_State(STATE::UP))),
			XMVectorGetX(XMVector3Length(Get_State(STATE::LOOK)))
		};
	}

	void Set_State(STATE eState, _fvector vState)
	{
		XMStoreFloat4(reinterpret_cast<_float4*>(&m_WorldMatrix.m[ETOUI(eState)]), vState);
	}

public:
	virtual HRESULT			Initialize_Prototype();
	virtual HRESULT			Initialize(void* pArg);

public:
	HRESULT					Bind_ShaderResource(class CShader* pShader, const _char* pConstantName);


public:
	void					Set_Scale(_float fScaleX = 1.f, _float fScaleY = 1.f, _float fScaleZ = 1.f);
	void					Scaling(_float fScaleX = 1.f, _float fScaleY = 1.f, _float fScaleZ = 1.f);

	void					Go_Straight(_float fTimeDelta);
	void					Go_Backward(_float fTimeDelta);
	void					Go_Left(_float fTimeDelta);
	void					Go_Right(_float fTimeDelta);

	void					Rotation(_fvector vAxis, _float fRadian);
	void					Turn(_fvector vAxis, _float fTimeDelta);

	void					LookAt(_fvector vAt);

private:
	_float4x4				m_WorldMatrix = {};
	_float					m_fSpeedPerSec = {};
	_float					m_fRotationPerSec = {};

public:
	static	CTransform*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*		Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END