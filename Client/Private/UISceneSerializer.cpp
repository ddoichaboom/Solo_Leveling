#include "UISceneSerializer.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

static constexpr char  UISCENE_MAGIC[4] = { 'S', 'L', 'U', 'I' };
static constexpr _uint UISCENE_VERSION_MIN = { 1 };
static constexpr _uint UISCENE_VERSION_LATEST = { 2 };

HRESULT CUISceneSerializer::Save(const _tchar* pUISceneDataPath, const UI_SCENE_DATA& UISceneData)
{
    if (nullptr == pUISceneDataPath || 0 == pUISceneDataPath[0])
        return E_FAIL;

    CBinaryWriter Writer;

    if (FAILED(Writer.Open(pUISceneDataPath)))
        return E_FAIL;

    const _uint iVersion = UISCENE_VERSION_LATEST;
    const _uint iNumElements = static_cast<_uint>(UISceneData.Elements.size());

    if (FAILED(Writer.WriteMagic(UISCENE_MAGIC, 4)))                                
        return E_FAIL;

    if (FAILED(Writer.WriteVersion(iVersion)))                                      
        return E_FAIL;

    if (FAILED(Writer.WriteArray(UISceneData.szName, sizeof(_tchar), MAX_PATH)))    
        return E_FAIL;

    if (FAILED(Writer.Write(UISceneData.fAuthoringWidth)))                          
        return E_FAIL;

    if (FAILED(Writer.Write(UISceneData.fAuthoringHeight)))                         
        return E_FAIL;

    if (FAILED(Writer.Write(iNumElements)))                                         
        return E_FAIL;

    for (const UI_ELEMENT& Element : UISceneData.Elements)
    {
        if (FAILED(Write_Element(Writer, Element)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CUISceneSerializer::Load(const _tchar* pUISceneDataPath, UI_SCENE_DATA* pOutUISceneData)
{
    if (nullptr == pUISceneDataPath || 0 == pUISceneDataPath[0] || nullptr == pOutUISceneData)
        return E_FAIL;

    CBinaryReader Reader;

    if (FAILED(Reader.Open(pUISceneDataPath)))
        return E_FAIL;

    _uint iVersion = {};
    _uint iNumElements = {};

    if (FAILED(Reader.ReadMagic(UISCENE_MAGIC, 4)))
        return E_FAIL;

    if (FAILED(Reader.ReadVersion(&iVersion, UISCENE_VERSION_MIN, UISCENE_VERSION_LATEST)))
        return E_FAIL;

    UI_SCENE_DATA UISceneData{};

    if (FAILED(Reader.ReadArray(UISceneData.szName, sizeof(_tchar), MAX_PATH)))
        return E_FAIL;
    UISceneData.szName[MAX_PATH - 1] = 0;

    if (FAILED(Reader.Read(&UISceneData.fAuthoringWidth)))            
        return E_FAIL;

    if (FAILED(Reader.Read(&UISceneData.fAuthoringHeight)))                         
        return E_FAIL;

    if (FAILED(Reader.Read(&iNumElements)))                                         
        return E_FAIL;

    UISceneData.Elements.clear();
    UISceneData.Elements.resize(iNumElements);

    for (UI_ELEMENT& Element : UISceneData.Elements)
    {
        if (FAILED(Read_Element(Reader, &Element, iVersion)))
            return E_FAIL;
    }

    *pOutUISceneData = UISceneData;

    return S_OK;
}

HRESULT CUISceneSerializer::Write_Element(CBinaryWriter& Writer, const UI_ELEMENT& Element)
{
    const _uint iType = static_cast<_uint>(Element.eType);

    if (FAILED(Writer.Write(iType)))                                            
        return E_FAIL;
    if (FAILED(Writer.WriteArray(Element.szName, sizeof(_tchar), MAX_PATH))) 
        return E_FAIL;
    if (FAILED(Writer.WriteArray(Element.szTexturePath, sizeof(_tchar), MAX_PATH))) 
        return E_FAIL;

    if (FAILED(Writer.Write(Element.fCenterX)))                                 
        return E_FAIL;
    if (FAILED(Writer.Write(Element.fCenterY)))                                 
        return E_FAIL;
    if (FAILED(Writer.Write(Element.fSizeX)))                                   
        return E_FAIL;
    if (FAILED(Writer.Write(Element.fSizeY)))                                   
        return E_FAIL;
    if (FAILED(Writer.Write(Element.iZOrder)))                                  
        return E_FAIL;

    if (FAILED(Writer.WriteArray(Element.szFontTag, sizeof(_tchar), MAX_PATH))) 
        return E_FAIL;
    if (FAILED(Writer.WriteArray(Element.szText, sizeof(_tchar), MAX_PATH))) 
        return E_FAIL;
    if (FAILED(Writer.Write(Element.iAtlasCols)))                               
        return E_FAIL;
    if (FAILED(Writer.Write(Element.iAtlasRows)))                               
        return E_FAIL;
    if (FAILED(Writer.Write(Element.fFrameDuration)))                           
        return E_FAIL;

    const _uint iLoop = Element.bLoop ? 1u : 0u;

    if (FAILED(Writer.Write(iLoop)))                                                
        return E_FAIL;

    if (FAILED(Writer.Write(Element.fPlaybackSpeed)))                               
        return E_FAIL;

    return S_OK;
}

HRESULT CUISceneSerializer::Read_Element(CBinaryReader& Reader, UI_ELEMENT* pOutElement, _uint iVersion)
{
    if (nullptr == pOutElement)
        return E_FAIL;

    _uint iType = {};

    if (FAILED(Reader.Read(&iType)))                                                
        return E_FAIL;
    if (iType >= static_cast<_uint>(UI_ELEMENT_TYPE::END))                          
        return E_FAIL;

    if (FAILED(Reader.ReadArray(pOutElement->szName, sizeof(_tchar), MAX_PATH)))    
        return E_FAIL;
    if (FAILED(Reader.ReadArray(pOutElement->szTexturePath, sizeof(_tchar), MAX_PATH))) 
        return E_FAIL;

    if (FAILED(Reader.Read(&pOutElement->fCenterX)))                                
        return E_FAIL;
    if (FAILED(Reader.Read(&pOutElement->fCenterY)))                                
        return E_FAIL;
    if (FAILED(Reader.Read(&pOutElement->fSizeX)))                                  
        return E_FAIL;
    if (FAILED(Reader.Read(&pOutElement->fSizeY)))                                  
        return E_FAIL;
    if (FAILED(Reader.Read(&pOutElement->iZOrder)))                                 
        return E_FAIL;

    if (FAILED(Reader.ReadArray(pOutElement->szFontTag, sizeof(_tchar), MAX_PATH))) 
        return E_FAIL;
    if (FAILED(Reader.ReadArray(pOutElement->szText, sizeof(_tchar), MAX_PATH)))    
        return E_FAIL;
    if (FAILED(Reader.Read(&pOutElement->iAtlasCols)))                              
        return E_FAIL;
    if (FAILED(Reader.Read(&pOutElement->iAtlasRows)))                              
        return E_FAIL;
    if (FAILED(Reader.Read(&pOutElement->fFrameDuration)))                          
        return E_FAIL;

    if (iVersion >= 2)
    {
        _uint iLoop = 1;
        if (FAILED(Reader.Read(&iLoop)))                                            
            return E_FAIL;
        if (FAILED(Reader.Read(&pOutElement->fPlaybackSpeed)))                      
            return E_FAIL;
        pOutElement->bLoop = (0 != iLoop);
    }
    else
    {
        // v1 Æú¹é
        pOutElement->bLoop = true;
        pOutElement->fPlaybackSpeed = 1.f;
    }

    pOutElement->eType = static_cast<UI_ELEMENT_TYPE>(iType);
    pOutElement->szName[MAX_PATH - 1] = 0;
    pOutElement->szTexturePath[MAX_PATH - 1] = 0;
    pOutElement->szFontTag[MAX_PATH - 1] = 0;
    pOutElement->szText[MAX_PATH - 1] = 0;

    return S_OK;
}