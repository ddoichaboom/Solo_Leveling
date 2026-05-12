#pragma once

#include "Client_Defines.h"
#include "UIObject.h"

NS_BEGIN(Client)

class CLIENT_DLL CUI_Text : public CUIObject
{
public:
    typedef struct tagUI_TextDesc : public CUIObject::UIOBJECT_DESC
    {
        const _tchar* pFontTag = { nullptr };
        const _tchar* pText = { TEXT("") };
        _float          fScale = { 1.f };
        _float4         vColor = { 1.f, 1.f, 1.f, 1.f };
        _float          fRotation = { 0.f };
        _uint           iZOrder = { 0 };
    }UI_TEXT_DESC;

protected:
    CUI_Text(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_Text(const CUI_Text& Prototype);
    virtual ~CUI_Text() = default;

public:
    virtual HRESULT         Initialize_Prototype() override;
    virtual HRESULT         Initialize(void* pArg) override;
    virtual void            Late_Update(_float fTimeDelta) override;
    virtual HRESULT         Render() override;

public:
    void                    Set_Text(const _tchar* pText) { m_strText = (pText ? pText : TEXT("")); }
    void                    Set_Color(const _float4& vColor) { m_vColor = vColor; }
    void                    Set_Scale(_float fScale) { m_fScale = fScale; }
    void                    Set_Rotation(_float fRotation) { m_fRotation = fRotation; }

private:
    _wstring                m_strFontTag;
    _wstring                m_strText;
    _float                  m_fScale = { 1.f };
    _float4                 m_vColor = { 1.f, 1.f, 1.f, 1.f };
    _float                  m_fRotation = { 0.f };
    _uint                   m_iZOrder = { 0 };

public:
    static CUI_Text*        Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*    Clone(void* pArg) override;
    virtual void            Free() override;
};

NS_END

