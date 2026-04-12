#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Editor)

class CPanel_Shortcuts final : public CPanel
{
private:
    CPanel_Shortcuts(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_Shortcuts() = default;

public:
    virtual HRESULT             Initialize() override;
    virtual void                Update(_float fTimeDelta) override;
    virtual void                Render() override;

private:
    void                        Render_CameraShortcuts();
    void                        Render_GizmoShortcuts();

public:
    static CPanel_Shortcuts*    Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                Free() override;
};

NS_END