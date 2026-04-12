#pragma once

#include "Transform.h"

NS_BEGIN(Engine)

class ENGINE_DLL CTransform_3D final : public CTransform
{
private:
	CTransform_3D(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CTransform_3D(const CTransform_3D& Prototype);
	virtual ~CTransform_3D() = default;

public:
    void                        Go_Straight(_float fTimeDelta);
    void                        Go_Backward(_float fTimeDelta);
    void                        Go_Left(_float fTimeDelta);
    void                        Go_Right(_float fTimeDelta);
    void                        Go_Up(_float fTimeDelta);
    void                        Go_Down(_float fTimeDelta);

    void                        Rotation(_fvector vAxis, _float fRadian);
    void                        Turn(_fvector vAxis, _float fTimeDelta);

    void                        LookAt(_fvector vAt);

public:
	static CTransform_3D*       Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*         Clone(void* pArg) override;
	virtual void                Free() override;
};

NS_END

