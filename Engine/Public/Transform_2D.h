#pragma once

#include "Transform.h"

NS_BEGIN(Engine)

class ENGINE_DLL CTransform_2D final : public CTransform
{
private:
	CTransform_2D(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTransform_2D(const CTransform_2D& Prototype);
	virtual ~CTransform_2D() = default;

public:
	void						Move_X(_float fTimeDelta);
	void						Move_Y(_float fTimeDelta);

public:
	static CTransform_2D*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*			Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END