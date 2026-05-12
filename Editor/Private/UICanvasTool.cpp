#include "UICanvasTool.h"
#include "UISceneSerializer.h"

CUICanvasTool::CUICanvasTool()
{
}

_int CUICanvasTool::Add_Element_Image()
{
    return Add_Element(UI_ELEMENT_TYPE::IMAGE);
}

_int CUICanvasTool::Add_Element_Text()
{
    return Add_Element(UI_ELEMENT_TYPE::TEXT);
}

_int CUICanvasTool::Add_Element_SpriteAnim()
{
    return Add_Element(UI_ELEMENT_TYPE::SPRITE_ANIM);
}

_int CUICanvasTool::Add_Element_Video()
{
    return Add_Element(UI_ELEMENT_TYPE::VIDEO);
}

HRESULT CUICanvasTool::Delete_Selected()
{
    if (m_iSelectedIndex < 0 || m_iSelectedIndex >= static_cast<_int>(m_SceneData.Elements.size()))
        return E_FAIL;

    m_SceneData.Elements.erase(m_SceneData.Elements.begin() + m_iSelectedIndex);
    Reindex_ZOrders();
    if (m_SceneData.Elements.empty())
        m_iSelectedIndex = -1;
    else if (m_iSelectedIndex >= static_cast<_int>(m_SceneData.Elements.size()))
        m_iSelectedIndex = static_cast<_int>(m_SceneData.Elements.size()) - 1;
    return S_OK;
}

HRESULT CUICanvasTool::Duplicate_Selected()
{
    UI_ELEMENT* pSrc = Get_SelectedElement();
    if (nullptr == pSrc)
        return E_FAIL;

    UI_ELEMENT Copy = *pSrc;
    _tchar szName[MAX_PATH] = {};
    swprintf_s(szName, TEXT("%s_copy%u"), pSrc->szName, m_iNameCounter++);
    wcscpy_s(Copy.szName, szName);
    Copy.iZOrder = static_cast<_uint>(m_SceneData.Elements.size());

    m_SceneData.Elements.push_back(Copy);
    Reindex_ZOrders();
    m_iSelectedIndex = static_cast<_int>(m_SceneData.Elements.size()) - 1;
    return S_OK;
}

HRESULT CUICanvasTool::Move_Selected_Up()
{
    if (m_iSelectedIndex <= 0)
        return E_FAIL;
    swap(m_SceneData.Elements[m_iSelectedIndex], m_SceneData.Elements[m_iSelectedIndex - 1]);
    Reindex_ZOrders();
    --m_iSelectedIndex;
    return S_OK;
}

HRESULT CUICanvasTool::Move_Selected_Down()
{
    if (m_iSelectedIndex < 0 || m_iSelectedIndex + 1 >= static_cast<_int>(m_SceneData.Elements.size()))
        return E_FAIL;
    swap(m_SceneData.Elements[m_iSelectedIndex], m_SceneData.Elements[m_iSelectedIndex + 1]);
    Reindex_ZOrders();
    ++m_iSelectedIndex;
    return S_OK;
}

HRESULT CUICanvasTool::Save(const _tchar* pFilePath)
{
    if (nullptr == pFilePath || 0 == pFilePath[0])
    {
        Log_Message(LOG_LEVEL::ERROR_, "[UI] Save: invalid path.");
        return E_FAIL;
    }

    if (FAILED(CUISceneSerializer::Save(pFilePath, m_SceneData)))
    {
        _char szPath[MAX_PATH] = {};
        WCharToChar(pFilePath, szPath, MAX_PATH);
        Log_Message(LOG_LEVEL::ERROR_, std::string("[UI] Save failed: ") + szPath);
        return E_FAIL;
    }

    _char szPath[MAX_PATH] = {};
    WCharToChar(pFilePath, szPath, MAX_PATH);
    _char szMsg[MAX_PATH + 64] = {};
    sprintf_s(szMsg, "[UI] Saved %u elements -> %s",
        static_cast<_uint>(m_SceneData.Elements.size()), szPath);
    Log_Message(LOG_LEVEL::INFO, szMsg);
    return S_OK;
}

HRESULT CUICanvasTool::Load(const _tchar* pFilePath)
{
    if (nullptr == pFilePath || 0 == pFilePath[0])
    {
        Log_Message(LOG_LEVEL::ERROR_, "[UI] Load: invalid path.");
        return E_FAIL;
    }

    UI_SCENE_DATA Loaded = {};
    if (FAILED(CUISceneSerializer::Load(pFilePath, &Loaded)))
    {
        _char szPath[MAX_PATH] = {};
        WCharToChar(pFilePath, szPath, MAX_PATH);
        Log_Message(LOG_LEVEL::ERROR_, std::string("[UI] Load failed: ") + szPath);
        return E_FAIL;
    }

    m_SceneData = Loaded;
    m_iSelectedIndex = m_SceneData.Elements.empty() ? -1 : 0;

    _char szPath[MAX_PATH] = {};
    WCharToChar(pFilePath, szPath, MAX_PATH);
    _char szMsg[MAX_PATH + 64] = {};
    sprintf_s(szMsg, "[UI] Loaded %u elements <- %s",
        static_cast<_uint>(m_SceneData.Elements.size()), szPath);
    Log_Message(LOG_LEVEL::INFO, szMsg);
    return S_OK;
}

void CUICanvasTool::Clear()
{
    const _uint iPrev = static_cast<_uint>(m_SceneData.Elements.size());
    m_SceneData.Elements.clear();
    m_iSelectedIndex = -1;

    _char szMsg[64] = {};
    sprintf_s(szMsg, "[UI] Cleared (%u elements).", iPrev);
    Log_Message(LOG_LEVEL::INFO, szMsg);
}

UI_ELEMENT* CUICanvasTool::Get_Element(_int iIndex)
{
    if (iIndex < 0 || iIndex >= static_cast<_int>(m_SceneData.Elements.size()))
        return nullptr;
    return &m_SceneData.Elements[iIndex];
}

UI_ELEMENT* CUICanvasTool::Get_SelectedElement()
{
    return Get_Element(m_iSelectedIndex);
}

void CUICanvasTool::Set_SelectedIndex(_int iIndex)
{
    if (iIndex < -1 || iIndex >= static_cast<_int>(m_SceneData.Elements.size()))
        return;
    m_iSelectedIndex = iIndex;
}

_int CUICanvasTool::Add_Element(UI_ELEMENT_TYPE eType)
{
    UI_ELEMENT Element = {};
    Init_Default(&Element, eType);
    m_SceneData.Elements.push_back(Element);
    Reindex_ZOrders();
    m_iSelectedIndex = static_cast<_int>(m_SceneData.Elements.size()) - 1;
    return m_iSelectedIndex;
}

void CUICanvasTool::Init_Default(UI_ELEMENT* pElement, UI_ELEMENT_TYPE eType)
{
    ZeroMemory(pElement, sizeof(UI_ELEMENT));
    pElement->eType = eType;
    pElement->fCenterX = 640.f;
    pElement->fCenterY = 360.f;
    pElement->fSizeX = 200.f;
    pElement->fSizeY = 200.f;
    pElement->iZOrder = static_cast<_uint>(m_SceneData.Elements.size());

    _tchar szName[MAX_PATH] = {};
    const _tchar* pPrefix = TEXT("Element");
    switch (eType)
    {
    case UI_ELEMENT_TYPE::IMAGE:            
        pPrefix = TEXT("Image");                
        break;
    case UI_ELEMENT_TYPE::TEXT:                     
        pPrefix = TEXT("Text");                 
        break;
    case UI_ELEMENT_TYPE::SPRITE_ANIM:      
        pPrefix = TEXT("SpriteAnim");   
        break;
    case UI_ELEMENT_TYPE::VIDEO:            
        pPrefix = TEXT("Video");                
        break;
    default: 
        break;
    }
    swprintf_s(szName, TEXT("%s_%u"), pPrefix, m_iNameCounter++);
    wcscpy_s(pElement->szName, szName);

    if (UI_ELEMENT_TYPE::TEXT == eType)
    {
        wcscpy_s(pElement->szFontTag, TEXT("Font_Default"));
        wcscpy_s(pElement->szText, TEXT("Text"));
    }
    if (UI_ELEMENT_TYPE::SPRITE_ANIM == eType)
    {
        pElement->iAtlasCols = 4;
        pElement->iAtlasRows = 4;
        pElement->fFrameDuration = 0.083f;
    }
    if (UI_ELEMENT_TYPE::VIDEO == eType)
    {
        // 영상은 풀스크린 기본 (1280x720 캔버스 중앙)
        pElement->fSizeX = 1280.f;
        pElement->fSizeY = 720.f;
    }
}

ImVec2 CUICanvasTool::Canvas_To_Screen(_float cx, _float cy, const ImVec2& vImagePos, _uint iViewportW, _uint iViewportH) const
{
    const _float fScaleX = static_cast<_float>(iViewportW) / m_SceneData.fAuthoringWidth;
    const _float fScaleY = static_cast<_float>(iViewportH) / m_SceneData.fAuthoringHeight;
    return ImVec2(vImagePos.x + cx * fScaleX, vImagePos.y + cy * fScaleY);
}

_int CUICanvasTool::Pick_Element(const ImVec2& vMouse, const ImVec2& vImagePos, _uint iViewportW, _uint iViewportH, DRAG_MODE* pOutMode) const
{
    if (nullptr != pOutMode) *pOutMode = DRAG_MODE::NONE;

    const _float fHandleHalf = 6.f;

    // 1) 선택된 엘리먼트 핸들 우선
    if (m_iSelectedIndex >= 0 && m_iSelectedIndex < static_cast<_int>(m_SceneData.Elements.size()))
    {
        const UI_ELEMENT& E = m_SceneData.Elements[m_iSelectedIndex];
        const _float fHalfX = E.fSizeX * 0.5f;
        const _float fHalfY = E.fSizeY * 0.5f;
        const ImVec2 vTL = Canvas_To_Screen(E.fCenterX - fHalfX, E.fCenterY - fHalfY, vImagePos, iViewportW, iViewportH);
        const ImVec2 vTR = Canvas_To_Screen(E.fCenterX + fHalfX, E.fCenterY - fHalfY, vImagePos, iViewportW, iViewportH);
        const ImVec2 vBL = Canvas_To_Screen(E.fCenterX - fHalfX, E.fCenterY + fHalfY, vImagePos, iViewportW, iViewportH);
        const ImVec2 vBR = Canvas_To_Screen(E.fCenterX + fHalfX, E.fCenterY + fHalfY, vImagePos, iViewportW, iViewportH);

        auto HitHandle = [&](const ImVec2& p) -> _bool
            {
                return fabsf(vMouse.x - p.x) <= fHandleHalf && fabsf(vMouse.y - p.y) <= fHandleHalf;
            };

        if (HitHandle(vTL)) { if (pOutMode) *pOutMode = DRAG_MODE::RESIZE_TL; return m_iSelectedIndex; }
        if (HitHandle(vTR)) { if (pOutMode) *pOutMode = DRAG_MODE::RESIZE_TR; return m_iSelectedIndex; }
        if (HitHandle(vBL)) { if (pOutMode) *pOutMode = DRAG_MODE::RESIZE_BL; return m_iSelectedIndex; }
        if (HitHandle(vBR)) { if (pOutMode) *pOutMode = DRAG_MODE::RESIZE_BR; return m_iSelectedIndex; }
    }

    // 2) 본체 hit test — 역순 (top z first)
    for (_int i = static_cast<_int>(m_SceneData.Elements.size()) - 1; i >= 0; --i)
    {
        const UI_ELEMENT& E = m_SceneData.Elements[i];
        const _float fHalfX = E.fSizeX * 0.5f;
        const _float fHalfY = E.fSizeY * 0.5f;
        const ImVec2 vTL = Canvas_To_Screen(E.fCenterX - fHalfX, E.fCenterY - fHalfY, vImagePos, iViewportW, iViewportH);
        const ImVec2 vBR = Canvas_To_Screen(E.fCenterX + fHalfX, E.fCenterY + fHalfY, vImagePos, iViewportW, iViewportH);

        if (vMouse.x >= vTL.x && vMouse.x <= vBR.x &&
            vMouse.y >= vTL.y && vMouse.y <= vBR.y)
        {
            if (pOutMode) *pOutMode = DRAG_MODE::MOVE;
            return i;
        }
    }

    return -1;
}

void CUICanvasTool::Apply_Drag(const ImVec2& vMouseDelta, _uint iViewportW, _uint iViewportH)
{
    UI_ELEMENT* pE = Get_SelectedElement();
    if (nullptr == pE) return;

    const _float fScaleX = static_cast<_float>(iViewportW) / m_SceneData.fAuthoringWidth;
    const _float fScaleY = static_cast<_float>(iViewportH) / m_SceneData.fAuthoringHeight;
    const _float fDx = vMouseDelta.x / fScaleX;
    const _float fDy = vMouseDelta.y / fScaleY;

    switch (m_eDragMode)
    {
    case DRAG_MODE::MOVE:
        pE->fCenterX = m_fStartCX + fDx;
        pE->fCenterY = m_fStartCY + fDy;
        break;
    case DRAG_MODE::RESIZE_TL:
        pE->fCenterX = m_fStartCX + fDx * 0.5f;
        pE->fCenterY = m_fStartCY + fDy * 0.5f;
        pE->fSizeX = max(1.f, m_fStartSX - fDx);
        pE->fSizeY = max(1.f, m_fStartSY - fDy);
        break;
    case DRAG_MODE::RESIZE_TR:
        pE->fCenterX = m_fStartCX + fDx * 0.5f;
        pE->fCenterY = m_fStartCY + fDy * 0.5f;
        pE->fSizeX = max(1.f, m_fStartSX + fDx);
        pE->fSizeY = max(1.f, m_fStartSY - fDy);
        break;
    case DRAG_MODE::RESIZE_BL:
        pE->fCenterX = m_fStartCX + fDx * 0.5f;
        pE->fCenterY = m_fStartCY + fDy * 0.5f;
        pE->fSizeX = max(1.f, m_fStartSX - fDx);
        pE->fSizeY = max(1.f, m_fStartSY + fDy);
        break;
    case DRAG_MODE::RESIZE_BR:
        pE->fCenterX = m_fStartCX + fDx * 0.5f;
        pE->fCenterY = m_fStartCY + fDy * 0.5f;
        pE->fSizeX = max(1.f, m_fStartSX + fDx);
        pE->fSizeY = max(1.f, m_fStartSY + fDy);
        break;
    default:
        break;
    }
}

void CUICanvasTool::Reindex_ZOrders()
{
    for (size_t i = 0; i < m_SceneData.Elements.size(); ++i)
        m_SceneData.Elements[i].iZOrder = static_cast<_uint>(i);
}

CUICanvasTool* CUICanvasTool::Create()
{
    CUICanvasTool* pInstance = new CUICanvasTool();
    wcscpy_s(pInstance->m_SceneData.szName, TEXT("UIScene"));
    pInstance->m_SceneData.fAuthoringWidth = 1280.f;
    pInstance->m_SceneData.fAuthoringHeight = 720.f;
    return pInstance;
}

void CUICanvasTool::Free()
{
    __super::Free();
}