#pragma once
#include "Client_Defines.h"
#include "GameObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CModelObject final : public CGameObject
{
public:
    typedef struct tagModelObjectDesc : public CGameObject::GAMEOBJECT_DESC
    {
        const _tchar* pShaderProtoTag = { nullptr };
        const _tchar* pModelProtoTag = { nullptr };
    }MODELOBJECT_DESC;

private:
    CModelObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CModelObject(const CModelObject& Prototype);
    virtual ~CModelObject() = default;

public:
    virtual HRESULT         Initialize_Prototype() override;
    virtual HRESULT         Initialize(void* pArg) override;
    virtual void            Priority_Update(_float fTimeDelta) override;
    virtual void            Update(_float fTimeDelta) override;
    virtual void            Late_Update(_float fTimeDelta) override;
    virtual HRESULT         Render() override;

private:
    CShader* m_pShaderCom = { nullptr };
    CModel* m_pModelCom = { nullptr };

    _wstring                m_strShaderProtoTag;
    _wstring                m_strModelProtoTag;

private:
    HRESULT                 Ready_Components();
    HRESULT                 Bind_ShaderResources();

public:
    static CModelObject* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject* Clone(void* pArg) override;
    virtual void            Free() override;
};

NS_END