#pragma once

#include "Client_Defines.h"
#include "PartObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
class CCollider;
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
        COLLISION_GROUP     eAttackGroup = { COLLISION_GROUP::END };
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

    void                        Set_AttackHitboxActive(_bool bActive) { m_bAttackHitboxActive = bActive; }
    _bool                       Is_AttackHitboxActive() const { return m_bAttackHitboxActive; }
    CCollider*                  Get_BladeCollider() const { return m_pBladeCollider; }

private:
    CShader*                    m_pShaderCom = { nullptr };
    CModel*                     m_pModelCom = { nullptr };
    CCollider*                  m_pBladeCollider = { nullptr };

private:
    const _float4x4*            m_pSocketBoneMatrix = { nullptr };
    const _tchar*               m_pModelPrototypeTag = { nullptr };
    _bool                       m_bVisible = { true };

    _bool                       m_bAttackHitboxActive = { false };
    COLLISION_GROUP             m_eAttackGroup = { COLLISION_GROUP::END };

private:
    HRESULT                     Ready_Components();
    HRESULT                     Bind_ShaderResources();
    HRESULT                     Ready_BladeCollider();
    void                        Update_BladeHitbox();

public:
    static CWeapon*             Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*        Clone(void* pArg) override;
    virtual void                Free() override;
};

NS_END