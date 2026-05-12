#pragma once

#include "Client_Defines.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CUISceneLoader final
{
public:
    static HRESULT Load_Into_Layer(const _tchar* pFilePath, _uint iLayerLevelIndex,  const _wstring& strLayerTag);

    static HRESULT Apply_Into_Layer(const UI_SCENE_DATA& SceneData, _uint iLayerLevelIndex, const _wstring& strLayerTag);

private:
    static HRESULT Add_Image(CGameInstance* pInstance, const UI_ELEMENT& E, _float fScaleX, _float fScaleY,
                                _uint iLayerLevelIndex, const _wstring& strLayerTag);
    static HRESULT Add_Text(CGameInstance* pInstance, const UI_ELEMENT& E, _float fScaleX, _float fScaleY,
                                _uint iLayerLevelIndex, const _wstring& strLayerTag);
    static HRESULT Add_SpriteAnim(CGameInstance* pInstance, const UI_ELEMENT& E, _float fScaleX, _float fScaleY,
                                _uint iLayerLevelIndex, const _wstring& strLayerTag);
    static HRESULT Add_Video(CGameInstance* pInstance, const UI_ELEMENT& E, _float fScaleX, _float fScaleY,
                                _uint iLayerLevelIndex, const _wstring& strLayerTag);
};

NS_END

