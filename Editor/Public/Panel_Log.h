#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Editor)

class CPanel_Log final : public CPanel
{
private:
    CPanel_Log(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_Log() = default;

public:
    virtual HRESULT         Initialize() override;
    virtual void            Update(_float fTimeDelta) override;
    virtual void            Render() override;

private:
    _bool                   m_bShowInfo     = { true };
    _bool                   m_bShowWarning  = { true };
    _bool                   m_bShowError    = { true };
    _bool                   m_bAutoScroll   = { true };

public:
    static CPanel_Log*      Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void            Free() override;

};

NS_END