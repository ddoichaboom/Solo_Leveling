#pragma once
#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Engine)
class CTexture;
NS_END

NS_BEGIN(Editor)

class CUICanvasTool;

class CPanel_2DCanvas final : public CPanel
{
private:
    CPanel_2DCanvas(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_2DCanvas() = default;

public:
    virtual HRESULT                         Initialize() override;
    virtual void                            Update(_float fTimeDelta) override;
    virtual void                            Render() override;

    CUICanvasTool*                          Get_Tool() const { return m_pTool; }

private:
    void                                    Render_ElementList();
    void                                    Render_PropertyEditor();
    void                                    Render_Common_Properties(UI_ELEMENT* pElement);
    void                                    Render_Image_Properties(UI_ELEMENT* pElement);
    void                                    Render_Text_Properties(UI_ELEMENT* pElement);
    void                                    Render_SpriteAnim_Properties(UI_ELEMENT* pElement);
    void                                    Render_Video_Properties(UI_ELEMENT* pElement);

private:
    const _char*                            Get_ElementTypeLabel(UI_ELEMENT_TYPE eType);

private:
    CUICanvasTool*                          m_pTool = { nullptr };
    _char                                   m_szFilePath[MAX_PATH] = { };

    unordered_map<_wstring, CTexture*>      m_PreviewCache;


public:
    static CPanel_2DCanvas*                 Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                            Free() override;
};

NS_END
