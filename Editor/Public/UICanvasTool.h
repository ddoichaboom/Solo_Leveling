#pragma once

#include "Editor_Defines.h"
#include "Client_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)
class CTexture;
NS_END

NS_BEGIN(Editor)

class CUICanvasTool final : public CBase
{
private:
    CUICanvasTool(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CUICanvasTool() = default;

public:
    _int                            Add_Element_Image();
    _int                            Add_Element_Text();
    _int                            Add_Element_SpriteAnim();
    _int                            Add_Element_Video();

    HRESULT                         Delete_Selected();
    HRESULT                         Duplicate_Selected();
    HRESULT                         Move_Selected_Up();
    HRESULT                         Move_Selected_Down();

    HRESULT                         Save(const _tchar* pFilePath);
    HRESULT                         Load(const _tchar* pFilePath);
    void                            Clear();

    UI_SCENE_DATA*                  Get_SceneData() { return &m_SceneData; }
    UI_ELEMENT*                     Get_Element(_int iIndex);
    UI_ELEMENT*                     Get_SelectedElement();
    _int                            Get_SelectedIndex() const { return m_iSelectedIndex; }
    void                            Set_SelectedIndex(_int iIndex);
    _uint                           Get_NumElements() const { return static_cast<_uint>(m_SceneData.Elements.size()); }

    void                            Render_Overlay(const ImVec2& vImagePos, _uint iViewportW, _uint iViewportH);
    void                            Handle_Interaction(const ImVec2& vImagePos, _uint iViewportW, _uint iViewportH, _bool bImageHovered);
    void                            Render_TextPreview_ToRT(_uint iRTWidth, _uint iRTHeight);

    ID3D11ShaderResourceView*       Get_PreviewSRV(const _tchar* pPath);

    void                            Set_ShowOverlayImages(_bool b) { m_bShowOverlayImages = b; }
    _bool                           Get_ShowOverlayImages() const { return m_bShowOverlayImages; }

private:
    _int                            Add_Element(UI_ELEMENT_TYPE eType);
    void                            Init_Default(UI_ELEMENT* pElement, UI_ELEMENT_TYPE eType);

    ImVec2                          Canvas_To_Screen(_float cx, _float cy, const ImVec2& vImagePos, _uint iViewportW, _uint iViewportH) const;
    _int                            Pick_Element(const ImVec2& vMouse, const ImVec2& vImagePos, _uint iViewportW, _uint iViewportH, DRAG_MODE* pOutMode) const;
    void                            Apply_Drag(const ImVec2& vMouseDelta, _uint iViewportW, _uint iViewportH);
    void                            Reindex_ZOrders();

private:
    ID3D11Device*                   m_pDevice = { nullptr };
    ID3D11DeviceContext*            m_pContext = { nullptr };
    UI_SCENE_DATA                   m_SceneData = {};
    _int                            m_iSelectedIndex = { -1 };
    _uint                           m_iNameCounter = { 0 };

    DRAG_MODE                       m_eDragMode = { DRAG_MODE::NONE };
    ImVec2                          m_vDragStart = {};
    _float                          m_fStartCX = { 0.f };
    _float                          m_fStartCY = { 0.f };
    _float                          m_fStartSX = { 0.f };
    _float                          m_fStartSY = { 0.f };
    _bool                           m_bShowOverlayImages = { true };

    unordered_map<_wstring, CTexture*>     m_PreviewCache;

public:
    static CUICanvasTool*           Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                    Free() override;
};

NS_END