#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Editor)

class CPanel_Hierarchy final : public CPanel
{
private:
    CPanel_Hierarchy(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_Hierarchy() = default;

public:
    virtual HRESULT             Initialize() override;
    virtual void                Update(_float fTimeDelta) override;
    virtual void                Render() override;

private:
    _int                        m_iPrevLevelIndex = { -1 };

public:
    static CPanel_Hierarchy*    Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                Free() override;

};

NS_END