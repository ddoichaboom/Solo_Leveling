#pragma once

#include "Client_Defines.h"

NS_BEGIN(Client)

class CLIENT_DLL CSceneSerializer final
{
public:
    static HRESULT Save(const _tchar* pSceneDataPath, const SCENE_DATA& SceneData);
    static HRESULT Load(const _tchar* pSceneDataPath, SCENE_DATA* pOutSceneData);

    static const SPAWN_POINT* Find_FirstSpawnPoint(const SCENE_DATA& SceneData, SPAWN_TYPE eType);
};

NS_END
