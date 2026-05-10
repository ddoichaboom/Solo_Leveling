#include "SceneSerializer.h"
#include "BinaryReader.h"
#include "BinaryWriter.h"

namespace
{
    static constexpr char SCENEDATA_MAGIC[4] = { 'S', 'L', 'S', 'C' };
    static constexpr _uint SCENEDATA_VERSION_MIN = { 1 };
    static constexpr _uint SCENEDATA_VERSION_LATEST = { 1 };

    HRESULT Write_SpawnPoint(CBinaryWriter& Writer, const SPAWN_POINT& Point)
    {
        const _uint iType = static_cast<_uint>(Point.eType);

        if (FAILED(Writer.Write(iType)))
            return E_FAIL;

        if (FAILED(Writer.Write(Point.iNavCellIndex)))
            return E_FAIL;

        if (FAILED(Writer.Write(Point.vPosition)))
            return E_FAIL;

        if (FAILED(Writer.Write(Point.vRotationDeg)))
            return E_FAIL;

        if (FAILED(Writer.WriteArray(Point.szName, sizeof(_tchar), MAX_PATH)))
            return E_FAIL;

        return S_OK;
    }

    HRESULT Read_SpawnPoint(CBinaryReader& Reader, SPAWN_POINT* pOutPoint)
    {
        if (nullptr == pOutPoint)
            return E_FAIL;

        _uint iType = {};

        if (FAILED(Reader.Read(&iType)))
            return E_FAIL;

        if (iType >= static_cast<_uint>(SPAWN_TYPE::END))
            return E_FAIL;

        if (FAILED(Reader.Read(&pOutPoint->iNavCellIndex)))
            return E_FAIL;

        if (FAILED(Reader.Read(&pOutPoint->vPosition)))
            return E_FAIL;

        if (FAILED(Reader.Read(&pOutPoint->vRotationDeg)))
            return E_FAIL;

        if (FAILED(Reader.ReadArray(pOutPoint->szName, sizeof(_tchar), MAX_PATH)))
            return E_FAIL;

        pOutPoint->eType = static_cast<SPAWN_TYPE>(iType);
        pOutPoint->szName[MAX_PATH - 1] = 0;

        return S_OK;
    }
}

HRESULT CSceneSerializer::Save(const _tchar* pSceneDataPath, const SCENE_DATA& SceneData)
{
    if (nullptr == pSceneDataPath || 0 == pSceneDataPath[0])
        return E_FAIL;

    CBinaryWriter Writer;

    if (FAILED(Writer.Open(pSceneDataPath)))
        return E_FAIL;

    const _uint iVersion = SCENEDATA_VERSION_LATEST;
    const _uint iNumSpawnPoints = static_cast<_uint>(SceneData.SpawnPoints.size());

    if (FAILED(Writer.WriteMagic(SCENEDATA_MAGIC, 4)))
        return E_FAIL;

    if (FAILED(Writer.WriteVersion(iVersion)))
        return E_FAIL;

    if (FAILED(Writer.WriteArray(SceneData.szNavDataPath, sizeof(_tchar), MAX_PATH)))
        return E_FAIL;

    if (FAILED(Writer.Write(iNumSpawnPoints)))
        return E_FAIL;

    for (const SPAWN_POINT& Point : SceneData.SpawnPoints)
    {
        if (FAILED(Write_SpawnPoint(Writer, Point)))
            return E_FAIL;
    }

    return S_OK;
}

HRESULT CSceneSerializer::Load(const _tchar* pSceneDataPath, SCENE_DATA* pOutSceneData)
{
    if (nullptr == pSceneDataPath || 0 == pSceneDataPath[0] || nullptr == pOutSceneData)
        return E_FAIL;

    CBinaryReader Reader;

    if (FAILED(Reader.Open(pSceneDataPath)))
        return E_FAIL;

    _uint iVersion = {};
    _uint iNumSpawnPoints = {};

    if (FAILED(Reader.ReadMagic(SCENEDATA_MAGIC, 4)))
        return E_FAIL;

    if (FAILED(Reader.ReadVersion(&iVersion, SCENEDATA_VERSION_MIN, SCENEDATA_VERSION_LATEST)))
        return E_FAIL;

    SCENE_DATA SceneData{};

    if (FAILED(Reader.ReadArray(SceneData.szNavDataPath, sizeof(_tchar), MAX_PATH)))
        return E_FAIL;

    SceneData.szNavDataPath[MAX_PATH - 1] = 0;

    if (FAILED(Reader.Read(&iNumSpawnPoints)))
        return E_FAIL;

    SceneData.SpawnPoints.clear();
    SceneData.SpawnPoints.resize(iNumSpawnPoints);

    for (SPAWN_POINT& Point : SceneData.SpawnPoints)
    {
        if (FAILED(Read_SpawnPoint(Reader, &Point)))
            return E_FAIL;
    }

    *pOutSceneData = SceneData;

    return S_OK;
}

const SPAWN_POINT* CSceneSerializer::Find_FirstSpawnPoint(const SCENE_DATA& SceneData, SPAWN_TYPE eType)
{
    for (const SPAWN_POINT& Point : SceneData.SpawnPoints)
    {
        if (Point.eType == eType)
            return &Point;
    }

    return nullptr;
}