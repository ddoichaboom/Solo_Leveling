#pragma once
#include "UI_Image.h"

NS_BEGIN(Client)

class CUI_Cursor final : public CUI_Image
{
protected:
    CUI_Cursor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    CUI_Cursor(const CUI_Cursor& Prototype);
    virtual ~CUI_Cursor() = default;

public:
    virtual HRESULT                 Initialize_Prototype() override;
    virtual HRESULT                 Initialize(void* pArg) override;
    virtual void                    Update(_float fTimeDelta) override;

public:
    static  CUI_Cursor*             Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual CGameObject*            Clone(void* pArg) override;
    virtual void                    Free() override;

};

NS_END