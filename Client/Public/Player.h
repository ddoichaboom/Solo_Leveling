#pragma once

#include "Client_Defines.h"
#include "ContainerObject.h"

NS_BEGIN(Client)

class CLIENT_DLL CPlayer final : public CContainerObject
{
public:
    // юс╫ц
    enum PLAYER_STATE {
        IDLE    = 0x00000001,
        RUN     = 0x00000002,
        ATTACK  = 0x00000004,
    };

private:
    CPlayer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CPlayer(const CPlayer& Prototype);
    virtual ~CPlayer() = default;

public:
    virtual HRESULT         Initialize_Prototype() override;
    virtual HRESULT         Initialize(void* pArg) override;
    virtual void            Priority_Update(_float fTimeDelta) override;
    virtual void            Update(_float fTimeDelta) override;
    virtual void            Late_Update(_float fTimeDelta) override;
    virtual HRESULT         Render() override;

private:
    _uint                   m_iState = { PLAYER_STATE::IDLE };

private:
    HRESULT                 Ready_PartObjects();

public:
    static CPlayer*         Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*    Clone(void* pArg) override;
    virtual void            Free() override;


};

NS_END