#pragma once

#include "Client_Defines.h"
#include "PartObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CWeapon final : public CPartObject
{
public:
    typedef struct tagWeaponDesc : public CPartObject::PARTOBJECT_DESC
    {
        const _float4x4*    pSocketBoneMatrix = { nullptr };
        const _tchar*       pModelPrototypeTag = { nullptr };
        _bool               bInitiallyVisible = { true };
    }WEAPON_DESC;

private:
    CWeapon(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CWeapon(const CWeapon& Prototype);
    virtual ~CWeapon() = default;

public:
    virtual HRESULT             Initialize_Prototype() override;
    virtual HRESULT             Initialize(void* pArg) override;
    virtual void                Priority_Update(_float fTimeDelta) override;
    virtual void                Update(_float fTimeDelta) override;
    virtual void                Late_Update(_float fTimeDelta) override;
    virtual HRESULT             Render() override;


public:
    void                        Set_Visible(_bool bVisible) { m_bVisible = bVisible; }
    _bool                       Is_Visible() const { return m_bVisible; }
    HRESULT                     Set_Model(const _tchar* pModelPrototypeTag);

private:
    CShader*                    m_pShaderCom = { nullptr };
    CModel*                     m_pModelCom = { nullptr };

private:
    const _float4x4*            m_pSocketBoneMatrix = { nullptr };
    const _tchar*               m_pModelPrototypeTag = { nullptr };
    _bool                       m_bVisible = { true };

private:
    HRESULT                     Ready_Components();
    HRESULT                     Bind_ShaderResources();

public:
    static CWeapon*             Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*        Clone(void* pArg) override;
    virtual void                Free() override;
};

NS_END