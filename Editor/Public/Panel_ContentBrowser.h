#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Editor)

class CPanel_ContentBrowser final : public CPanel
{
private:
    CPanel_ContentBrowser(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_ContentBrowser() = default;

public:
    virtual HRESULT         Initialize() override;
    virtual void            Update(_float fTimeDelta) override;
    virtual void            Render() override;

public:
    static CPanel_ContentBrowser* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void            Free() override;

};

NS_END