#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Editor)

class CPanel_Inspector final : public CPanel
{
private:
    CPanel_Inspector(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_Inspector() = default;

public:
    virtual HRESULT         Initialize() override;
    virtual void            Update(_float fTimeDelta) override;
    virtual void            Render() override;

public:
    static CPanel_Inspector* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void            Free() override;

};

NS_END