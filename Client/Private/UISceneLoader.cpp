#include "UISceneLoader.h"
#include "UISceneSerializer.h"
#include "GameInstance.h"
#include "UI_Image.h"
#include "UI_Text.h"
#include "UI_SpriteAnim.h"
#include "UI_Video.h"

HRESULT CUISceneLoader::Load_Into_Layer(const _tchar* pFilePath, _uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    if (nullptr == pFilePath || 0 == pFilePath[0])
        return E_FAIL;

    UI_SCENE_DATA SceneData{};
    if (FAILED(CUISceneSerializer::Load(pFilePath, &SceneData)))
        return E_FAIL;

    return Apply_Into_Layer(SceneData, iLayerLevelIndex, strLayerTag);
}

HRESULT CUISceneLoader::Apply_Into_Layer(const UI_SCENE_DATA& SceneData, _uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    CGameInstance* pInstance = CGameInstance::GetInstance();
    if (nullptr == pInstance)
        return E_FAIL;

    const _float fAuthW = (SceneData.fAuthoringWidth > 0.f) ? SceneData.fAuthoringWidth : 1280.f;
    const _float fAuthH = (SceneData.fAuthoringHeight > 0.f) ? SceneData.fAuthoringHeight : 720.f;
    const _float fScaleX = static_cast<_float>(pInstance->Get_WinSizeX()) / fAuthW;
    const _float fScaleY = static_cast<_float>(pInstance->Get_WinSizeY()) / fAuthH;

    for (const UI_ELEMENT& E : SceneData.Elements)
    {
        HRESULT hr = S_OK;
        switch (E.eType)
        {
        case UI_ELEMENT_TYPE::IMAGE:
            hr = Add_Image(pInstance, E, fScaleX, fScaleY, iLayerLevelIndex, strLayerTag);
            break;
        case UI_ELEMENT_TYPE::TEXT:
            hr = Add_Text(pInstance, E, fScaleX, fScaleY, iLayerLevelIndex, strLayerTag);
            break;
        case UI_ELEMENT_TYPE::SPRITE_ANIM:
            hr = Add_SpriteAnim(pInstance, E, fScaleX, fScaleY, iLayerLevelIndex, strLayerTag);
            break;
        case UI_ELEMENT_TYPE::VIDEO:
            hr = Add_Video(pInstance, E, fScaleX, fScaleY, iLayerLevelIndex, strLayerTag);
            break;
        default: 
            break;
        }
        if (FAILED(hr))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CUISceneLoader::Add_Image(CGameInstance* pInstance, const UI_ELEMENT& E, _float fScaleX, _float fScaleY, _uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    CUI_Image::UI_IMAGE_DESC Desc{};
    Desc.fCenterX = E.fCenterX * fScaleX;
    Desc.fCenterY = E.fCenterY * fScaleY;
    Desc.fSizeX = E.fSizeX * fScaleX;
    Desc.fSizeY = E.fSizeY * fScaleY;
    Desc.pTexturePath = E.szTexturePath;
    Desc.iZOrder = E.iZOrder;
    Desc.pObjectName = E.szName;
    Desc.bVisible = E.bVisible;

    Desc.pTextureProtoTag = E.szTextureProtoTag;
    Desc.iTextureProtoLevel = (0 != E.iTextureProtoLevel) ? E.iTextureProtoLevel : ETOUI(LEVEL::STATIC);

    return CGameInstance::GetInstance()->Add_GameObject(
        ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_UI_Image"),
        iLayerLevelIndex, strLayerTag, &Desc);
}

HRESULT CUISceneLoader::Add_Text(CGameInstance* pInstance, const UI_ELEMENT& E, _float fScaleX, _float fScaleY, _uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    CUI_Text::UI_TEXT_DESC Desc{};
    Desc.fCenterX = E.fCenterX * fScaleX;
    Desc.fCenterY = E.fCenterY * fScaleY;
    Desc.fSizeX = E.fSizeX * fScaleX;
    Desc.fSizeY = E.fSizeY * fScaleY;
    Desc.pFontTag = E.szFontTag;
    Desc.pText = E.szText;
    Desc.fScale = 1.f;
    Desc.iZOrder = E.iZOrder;
    Desc.pObjectName = E.szName;

    Desc.eHAlign = E.eHAlign;
    Desc.eVAlign = E.eVAlign;
    Desc.bAutoFit = E.bAutoFit;
    Desc.bVisible = E.bVisible;

    return CGameInstance::GetInstance()->Add_GameObject(
        ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_UI_Text"),
        iLayerLevelIndex, strLayerTag, &Desc);
}

HRESULT CUISceneLoader::Add_SpriteAnim(CGameInstance* pInstance, const UI_ELEMENT& E, _float fScaleX, _float fScaleY, _uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    CUI_SpriteAnim::UI_SPRITEANIM_DESC Desc{};
    Desc.fCenterX = E.fCenterX * fScaleX;
    Desc.fCenterY = E.fCenterY * fScaleY;
    Desc.fSizeX = E.fSizeX * fScaleX;
    Desc.fSizeY = E.fSizeY * fScaleY;
    Desc.pTexturePath = E.szTexturePath;
    Desc.iZOrder = E.iZOrder;
    Desc.iAtlasCols = E.iAtlasCols ? E.iAtlasCols : 1;
    Desc.iAtlasRows = E.iAtlasRows ? E.iAtlasRows : 1;
    Desc.fFrameDuration = (E.fFrameDuration > 0.f) ? E.fFrameDuration : 0.1f;
    Desc.bLoop = E.bLoop;
    Desc.pObjectName = E.szName;
    Desc.bVisible = E.bVisible;


    return CGameInstance::GetInstance()->Add_GameObject(
        ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_UI_SpriteAnim"),
        iLayerLevelIndex, strLayerTag, &Desc);
}

HRESULT CUISceneLoader::Add_Video(CGameInstance* pInstance, const UI_ELEMENT& E, _float fScaleX, _float fScaleY, _uint iLayerLevelIndex, const _wstring& strLayerTag)
{
    CUI_Video::UI_VIDEO_DESC Desc{};
    Desc.fCenterX = E.fCenterX * fScaleX;
    Desc.fCenterY = E.fCenterY * fScaleY;
    Desc.fSizeX = E.fSizeX * fScaleX;
    Desc.fSizeY = E.fSizeY * fScaleY;
    Desc.pVideoPath = E.szTexturePath;
    Desc.iZOrder = E.iZOrder;
    Desc.bLoop = E.bLoop;
    Desc.fPlaybackSpeed = (E.fPlaybackSpeed > 0.f) ? E.fPlaybackSpeed : 1.f;
    Desc.pObjectName = E.szName;
    Desc.bVisible = E.bVisible;


    return CGameInstance::GetInstance()->Add_GameObject(
        ETOUI(LEVEL::STATIC), TEXT("Prototype_GameObject_UI_Video"),
        iLayerLevelIndex, strLayerTag, &Desc);
}
