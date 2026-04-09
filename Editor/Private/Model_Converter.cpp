#include "Model_Converter.h"

HRESULT CModel_Converter::Convert(const _tchar* pFbxPath, const _tchar* pBinPath, MODEL eModelType)
{
    // 1. wchar_t → char (Assimp는 char 경로 사용)
    _char szFbxPathA[MAX_PATH] = {};
    WideCharToMultiByte(CP_ACP, 0, pFbxPath, -1, szFbxPathA, MAX_PATH, nullptr,
        nullptr);

    // 2. FBX 디렉토리 추출 (텍스처 경로 계산용)
    _char szDrive[MAX_PATH] = {};
    _char szDir[MAX_PATH] = {};
    _splitpath_s(szFbxPathA, szDrive, MAX_PATH, szDir, MAX_PATH, nullptr, 0, nullptr, 0);
    _char szFbxDirFull[MAX_PATH] = {};
    strcpy_s(szFbxDirFull, szDrive);
    strcat_s(szFbxDirFull, szDir);

    // 3. Assimp 로드
    Assimp::Importer Importer;
    _uint iFlag = aiProcess_ConvertToLeftHanded |
        aiProcessPreset_TargetRealtime_Fast;

    if (MODEL::NONANIM == eModelType)
        iFlag |= aiProcess_PreTransformVertices;   // 비애니: 정점에 노드 변환 미리 적용

        const aiScene* pScene = Importer.ReadFile(szFbxPathA, iFlag);
    if (nullptr == pScene)
        return E_FAIL;

    // 4. 본 트리 수집 (ANIM 전용)
    vector<pair<aiNode*, _int>> Bones;
    unordered_map<string, _uint> BoneNameMap;
    if (MODEL::ANIM == eModelType)
        Collect_Bones(pScene->mRootNode, -1, Bones, BoneNameMap);

    // 5. 출력 파일 열기
    FILE* fp = nullptr;
    if (0 != _wfopen_s(&fp, pBinPath, L"wb") || nullptr == fp)
        return E_FAIL;

    // 6. Header (80 bytes)
    {
        const char szMagic[4] = { 'S','L','M','D' };
        _uint iVersion = 1u;
        _uint iModelType = static_cast<_uint>(eModelType);
        _uint iReserved = 0u;
        fwrite(szMagic, sizeof(char), 4, fp);
        fwrite(&iVersion, sizeof(_uint), 1, fp);
        fwrite(&iModelType, sizeof(_uint), 1, fp);
        fwrite(&iReserved, sizeof(_uint), 1, fp);

        _float4x4 PreTransform;
        XMStoreFloat4x4(&PreTransform, XMMatrixIdentity());
        fwrite(&PreTransform, sizeof(_float4x4), 1, fp);
    }

    // 7. 섹션 순서대로 write
    HRESULT hr = S_OK;
    if (SUCCEEDED(hr)) 
        hr = Write_Meshes(fp, pScene, eModelType, BoneNameMap);

    if (SUCCEEDED(hr)) 
        hr = Write_Materials(fp, pScene, szFbxDirFull);

    if (SUCCEEDED(hr)) 
        hr = Write_Bones(fp, Bones);

    if (SUCCEEDED(hr)) 
        hr = Write_Animations(fp, pScene, BoneNameMap);

    fclose(fp);

    if (FAILED(hr))
        _wremove(pBinPath);   // 불완전한 파일 제거

    return hr;
}

void CModel_Converter::Collect_Bones(aiNode* pNode, _int iParentIndex, vector<pair<aiNode*, _int>>& outBones, unordered_map<string, _uint>& outBoneNameMap)
{
    _uint iMyIndex = static_cast<_uint>(outBones.size());
    outBones.emplace_back(pNode, iParentIndex);
    outBoneNameMap.emplace(pNode->mName.data, iMyIndex);

    for (size_t i = 0; i < pNode->mNumChildren; i++)
    {
        Collect_Bones(pNode->mChildren[i], static_cast<_int>(iMyIndex),
                        outBones, outBoneNameMap);
    }
}

HRESULT CModel_Converter::Write_Meshes(FILE* fp, const aiScene* pScene, MODEL eModelType, const unordered_map<string, _uint>& boneNameMap)
{
    const bool bAnim = (MODEL::ANIM == eModelType);

    for (_uint m = 0; m < pScene->mNumMeshes; ++m)
    {
        char szBuf[512] = {};
        sprintf_s(szBuf, "[Mesh %u] %s %s\n", m,
            pScene->mMeshes[m]->mName.data,
            Should_Skip_Mesh(pScene->mMeshes[m]->mName.data) ?
            "(SKIP)" : "(KEEP)");
        OutputDebugStringA(szBuf);
    }

    vector<_uint> validMeshIndices;
    for (_uint m = 0; m < pScene->mNumMeshes; m++)
    {
        if (!Should_Skip_Mesh(pScene->mMeshes[m]->mName.data))
            validMeshIndices.push_back(m);
    }

    _uint iNumMeshes = static_cast<_uint>(validMeshIndices.size());
    fwrite(&iNumMeshes, sizeof(_uint), 1, fp);

    for (_uint i = 0; i < iNumMeshes; ++i)
    {
        const aiMesh* pMesh = pScene->mMeshes[validMeshIndices[i]];
        OutputDebugStringA(pMesh->mName.data);
        OutputDebugStringA("\n");

        const _uint   iNumVerts = pMesh->mNumVertices;

        // szName[MAX_PATH]
        _char szName[MAX_PATH] = {};
        strcpy_s(szName, pMesh->mName.data);
        fwrite(szName, sizeof(_char), MAX_PATH, fp);

        // iMaterialIndex
        _uint iMatIdx = pMesh->mMaterialIndex;
        fwrite(&iMatIdx, sizeof(_uint), 1, fp);

        // iVertexStride, iNumVertices
        _uint iStride = bAnim ? sizeof(VTXANIMMESH) : sizeof(VTXMESH);
        fwrite(&iStride, sizeof(_uint), 1, fp);
        fwrite(&iNumVerts, sizeof(_uint), 1, fp);

        // 정점 배열 
        if (!bAnim)
        {
            // VTXMESH
            vector<VTXMESH> Verts(iNumVerts);
            for (_uint v = 0; v < iNumVerts; ++v)
            {
                memcpy(&Verts[v].vPosition, &pMesh->mVertices[v], sizeof(_float3));
                if (pMesh->mNormals)
                    memcpy(&Verts[v].vNormal, &pMesh->mNormals[v],
                        sizeof(_float3));
                if (pMesh->mTextureCoords[0])
                    memcpy(&Verts[v].vTexcoord, &pMesh->mTextureCoords[0][v],
                        sizeof(_float2));
                if (pMesh->mTangents)
                    memcpy(&Verts[v].vTangent, &pMesh->mTangents[v],
                        sizeof(_float3));
                if (pMesh->mBitangents)
                    memcpy(&Verts[v].vBinormal, &pMesh->mBitangents[v],
                        sizeof(_float3));
            }
            fwrite(Verts.data(), iStride, iNumVerts, fp);
        }
        else
        {
            // VTXANIMMESH — 정점 기본 데이터
            vector<VTXANIMMESH> Verts(iNumVerts);   // zero-init: vBlendIndex/Weight= 0
                for (_uint v = 0; v < iNumVerts; ++v)
                {
                    memcpy(&Verts[v].vPosition, &pMesh->mVertices[v], sizeof(_float3));
                    if (pMesh->mNormals)
                        memcpy(&Verts[v].vNormal, &pMesh->mNormals[v],
                            sizeof(_float3));
                    if (pMesh->mTextureCoords[0])
                        memcpy(&Verts[v].vTexcoord, &pMesh->mTextureCoords[0][v],
                            sizeof(_float2));
                    if (pMesh->mTangents)
                        memcpy(&Verts[v].vTangent, &pMesh->mTangents[v],
                            sizeof(_float3));
                    if (pMesh->mBitangents)
                        memcpy(&Verts[v].vBinormal, &pMesh->mBitangents[v],
                            sizeof(_float3));
                }

            // 본 가중치 채우기 (mesh-local 인덱스 b)
            _uint               iNumMeshBones = pMesh->mNumBones;
            vector<_uint>       GlobalBoneIdx;
            vector<_float4x4>   OffsetMats;
            GlobalBoneIdx.reserve(iNumMeshBones);
            OffsetMats.reserve(iNumMeshBones);

            for (_uint b = 0; b < iNumMeshBones; ++b)
            {
                const aiBone* pBone = pMesh->mBones[b];

                auto it = boneNameMap.find(pBone->mName.data);
                if (it == boneNameMap.end())
                    return E_FAIL;
                GlobalBoneIdx.push_back(it->second);

                // OffsetMatrix: Assimp 열우선 → DirectX 행우선 (전치)
                _float4x4 Offset;
                memcpy(&Offset, &pBone->mOffsetMatrix, sizeof(_float4x4));
                XMStoreFloat4x4(&Offset, XMMatrixTranspose(XMLoadFloat4x4(&Offset)));
                OffsetMats.push_back(Offset);

                // 정점별 가중치 할당 (빈 슬롯 순서대로)
                for (_uint w = 0; w < pBone->mNumWeights; ++w)
                {
                    _uint  vIdx = pBone->mWeights[w].mVertexId;
                    _float weight = pBone->mWeights[w].mWeight;
                    auto& vtx = Verts[vIdx];

                    if (0.f == vtx.vBlendWeight.x) {
                        vtx.vBlendIndex.x = b;
                        vtx.vBlendWeight.x = weight;
                    }
                    else if (0.f == vtx.vBlendWeight.y) {
                        vtx.vBlendIndex.y = b;
                        vtx.vBlendWeight.y = weight;
                    }
                    else if (0.f == vtx.vBlendWeight.z) {
                        vtx.vBlendIndex.z = b;
                        vtx.vBlendWeight.z = weight;
                    }
                    else {
                        vtx.vBlendIndex.w = b;
                        vtx.vBlendWeight.w = weight;
                    }
                }
            }

            // 0-bone 폴백: 메시 이름과 동일한 노드를 본으로 사용 (16일차 동일 처리)
            if (0 == iNumMeshBones)
            {
                auto it = boneNameMap.find(pMesh->mName.data);
                if (it == boneNameMap.end())
                    return E_FAIL;

                GlobalBoneIdx.push_back(it->second);
                _float4x4 Identity;
                XMStoreFloat4x4(&Identity, XMMatrixIdentity());
                OffsetMats.push_back(Identity);
                iNumMeshBones = 1;
            }

            fwrite(Verts.data(), iStride, iNumVerts, fp);

            // 인덱스
            _uint iNumIdx = pMesh->mNumFaces * 3;
            fwrite(&iNumIdx, sizeof(_uint), 1, fp);
            for (_uint f = 0; f < pMesh->mNumFaces; ++f)
                fwrite(pMesh->mFaces[f].mIndices, sizeof(_uint), 3, fp);

            // 본 데이터 (ANIM 전용)
            fwrite(&iNumMeshBones, sizeof(_uint), 1, fp);
            fwrite(GlobalBoneIdx.data(), sizeof(_uint), iNumMeshBones, fp);
            fwrite(OffsetMats.data(), sizeof(_float4x4), iNumMeshBones, fp);

            continue;   // ANIM은 위에서 인덱스+본 data 기록 완료
        }

        // NONANIM 인덱스 (공통)
        _uint iNumIdx = pMesh->mNumFaces * 3;
        fwrite(&iNumIdx, sizeof(_uint), 1, fp);
        for (_uint f = 0; f < pMesh->mNumFaces; ++f)
            fwrite(pMesh->mFaces[f].mIndices, sizeof(_uint), 3, fp);
    }

    return S_OK;
}

HRESULT CModel_Converter::Write_Materials(FILE* fp, const aiScene* pScene, const _char* pFbxDirFull)
{
    _uint iNumMaterials = pScene->mNumMaterials;
    fwrite(&iNumMaterials, sizeof(_uint), 1, fp);

    for (_uint i = 0; i < iNumMaterials; ++i)
    {
        const aiMaterial* pMat = pScene->mMaterials[i];

        // ── Phase 1: Assimp 텍스처 수집 ──
        struct TexEntry { _uint eType; _char szAiPath[MAX_PATH]; };
        vector<TexEntry> entries;
        _bool bHasDiffuse = false;

        for (_uint aiType = aiTextureType_DIFFUSE; aiType <
            AI_TEXTURE_TYPE_MAX; ++aiType)
        {
            _uint iCount =
                pMat->GetTextureCount(static_cast<aiTextureType>(aiType));
            for (_uint j = 0; j < iCount; ++j)
            {
                aiString aiTexPath;
                pMat->GetTexture(static_cast<aiTextureType>(aiType), j,
                    &aiTexPath);

                TexEntry entry{};
                entry.eType = aiType - 1u;
                strcpy_s(entry.szAiPath, aiTexPath.data);

                char szDebug[512] = {};
                sprintf_s(szDebug, "[Material %u] aiType=%u  path=%s\n", i, aiType, aiTexPath.data);
                OutputDebugStringA(szDebug);

                entries.push_back(entry);

                if (aiTextureType_DIFFUSE == aiType)
                    bHasDiffuse = true;
            }
        }

        // ── Phase 2: DIFFUSE 없으면 _CO 파일 탐색 ──
        if (!bHasDiffuse && !entries.empty())
        {
            _char szFname[MAX_PATH] = {}, szExt[MAX_PATH] = {};
            _splitpath_s(entries[0].szAiPath, nullptr, 0, nullptr, 0,
                szFname, MAX_PATH, szExt, MAX_PATH);

            _char* pLastUnderscore = strrchr(szFname, '_');
            if (nullptr != pLastUnderscore)
            {
                strcpy_s(pLastUnderscore + 1,
                    MAX_PATH - static_cast<size_t>(pLastUnderscore
                        - szFname) - 1, "CO");

                _char szCOFull[MAX_PATH] = {};
                strcpy_s(szCOFull, pFbxDirFull);
                strcat_s(szCOFull, szFname);
                strcat_s(szCOFull, szExt);

                if (INVALID_FILE_ATTRIBUTES !=
                    GetFileAttributesA(szCOFull))
                {
                    TexEntry coEntry{};
                    coEntry.eType = 0u;   // TEXTURE_TYPE::DIFFUSE
                    strcpy_s(coEntry.szAiPath, szFname);
                    strcat_s(coEntry.szAiPath, szExt);
                    entries.push_back(coEntry);
                }
            }
        }

        // ── Phase 3: 기록 ──
        _uint iNumTextures = static_cast<_uint>(entries.size());
        fwrite(&iNumTextures, sizeof(_uint), 1, fp);

        for (const auto& entry : entries)
        {
            fwrite(&entry.eType, sizeof(_uint), 1, fp);

            _tchar szRelPath[MAX_PATH] = {};
            Build_TexturePath(pFbxDirFull, entry.szAiPath, szRelPath);
            fwrite(szRelPath, sizeof(_tchar), MAX_PATH, fp);
        }
    }

    return S_OK;
}

HRESULT CModel_Converter::Write_Bones(FILE* fp, const vector<pair<aiNode*, _int>>& bones)
{
    _uint iNumBones = static_cast<_uint>(bones.size());
    fwrite(&iNumBones, sizeof(_uint), 1, fp);

    for (const auto& [pNode, iParentIdx] : bones)
    {
        // szName[MAX_PATH]
        _char szName[MAX_PATH] = {};
        strcpy_s(szName, pNode->mName.data);
        fwrite(szName, sizeof(_char), MAX_PATH, fp);

        // iParentIndex
        fwrite(&iParentIdx, sizeof(_int), 1, fp);

        // TransformationMatrix (Assimp 열우선 → DirectX 행우선: 전치)
        _float4x4 Transform;
        memcpy(&Transform, &pNode->mTransformation, sizeof(_float4x4));
        XMStoreFloat4x4(&Transform, XMMatrixTranspose(XMLoadFloat4x4(&Transform)));
        fwrite(&Transform, sizeof(_float4x4), 1, fp);
    }

    return S_OK;
}

HRESULT CModel_Converter::Write_Animations(FILE* fp, const aiScene* pScene, const unordered_map<string, _uint>& boneNameMap)
{
    _uint iNumAnims = pScene->mNumAnimations;
    fwrite(&iNumAnims, sizeof(_uint), 1, fp);

    for (_uint i = 0; i < iNumAnims; ++i)
    {
        const aiAnimation* pAnim = pScene->mAnimations[i];

        // szName[MAX_PATH]
        _char szName[MAX_PATH] = {};
        strcpy_s(szName, pAnim->mName.data);
        fwrite(szName, sizeof(_char), MAX_PATH, fp);

        // fDuration, fTickPerSecond
        _float fDuration = static_cast<_float>(pAnim->mDuration);
        _float fTickPerSec = static_cast<_float>(pAnim->mTicksPerSecond);
        fwrite(&fDuration, sizeof(_float), 1, fp);
        fwrite(&fTickPerSec, sizeof(_float), 1, fp);

        // bIsLoop — 기본 false (0). 게임 로직에서 필요 시 변경
        _uint iIsLoop = 0u;
        fwrite(&iIsLoop, sizeof(_uint), 1, fp);

        // iNumChannels
        _uint iNumChannels = pAnim->mNumChannels;
        fwrite(&iNumChannels, sizeof(_uint), 1, fp);

        for (_uint c = 0; c < iNumChannels; ++c)
        {
            const aiNodeAnim* pNodeAnim = pAnim->mChannels[c];

            // iBoneIndex (global)
            auto it = boneNameMap.find(pNodeAnim->mNodeName.data);
            if (it == boneNameMap.end())
                return E_FAIL;
            _uint iBoneIndex = it->second;
            fwrite(&iBoneIndex, sizeof(_uint), 1, fp);

            // iNumKeyFrames = max(Scaling, Rotation, Position 키 개수)
            _uint iNumKeys = max(pNodeAnim->mNumScalingKeys,
                pNodeAnim->mNumRotationKeys);
            iNumKeys = max(iNumKeys, pNodeAnim->mNumPositionKeys);
            fwrite(&iNumKeys, sizeof(_uint), 1, fp);

            // KEYFRAME 배열 (POD 구조체, fread 한 번에 읽음)
            vector<KEYFRAME> KeyFrames(iNumKeys);
            for (_uint k = 0; k < iNumKeys; ++k)
            {
                KEYFRAME& kf = KeyFrames[k];

                if (k < pNodeAnim->mNumScalingKeys)
                {
                    memcpy(&kf.vScale, &pNodeAnim->mScalingKeys[k].mValue,
                        sizeof(_float3));
                    kf.fTrackPosition =
                        static_cast<_float>(pNodeAnim->mScalingKeys[k].mTime);
                }
                if (k < pNodeAnim->mNumRotationKeys)
                {
                    kf.vRotation.x = pNodeAnim->mRotationKeys[k].mValue.x;
                    kf.vRotation.y = pNodeAnim->mRotationKeys[k].mValue.y;
                    kf.vRotation.z = pNodeAnim->mRotationKeys[k].mValue.z;
                    kf.vRotation.w = pNodeAnim->mRotationKeys[k].mValue.w;
                    kf.fTrackPosition =
                        static_cast<_float>(pNodeAnim->mRotationKeys[k].mTime);
                }
                if (k < pNodeAnim->mNumPositionKeys)
                {
                    memcpy(&kf.vTranslation, &pNodeAnim->mPositionKeys[k].mValue,
                        sizeof(_float3));
                    kf.fTrackPosition =
                        static_cast<_float>(pNodeAnim->mPositionKeys[k].mTime);
                }
            }
            fwrite(KeyFrames.data(), sizeof(KEYFRAME), iNumKeys, fp);
        }
    }

    return S_OK;
}

void CModel_Converter::Build_TexturePath(const _char* pFbxDirFull, const char* pAiTexPath, _tchar* pOutWPath)
{
    // Assimp 경로에서 파일명+확장자만 추출
    _char szFileName[MAX_PATH] = {};
    _char szExt[MAX_PATH] = {};
    _splitpath_s(pAiTexPath, nullptr, 0, nullptr, 0, szFileName, MAX_PATH, szExt,
        MAX_PATH);

    // FBX 디렉토리 + 파일명 → 절대 경로
    _char szFull[MAX_PATH] = {};
    strcpy_s(szFull, pFbxDirFull);
    strcat_s(szFull, szFileName);
    strcat_s(szFull, szExt);

    // "Resources\" 위치를 찾아 그 이후 상대 경로 추출
    // 탐색: "Resources\\" 또는 "Resources/"
    const _char* pResources = strstr(szFull, "Resources\\");
    if (nullptr == pResources)
        pResources = strstr(szFull, "Resources/");

    if (nullptr == pResources)
    {
        // 못 찾으면 파일명만이라도 보존
        _char szFallback[MAX_PATH] = {};
        strcat_s(szFallback, szFileName);
        strcat_s(szFallback, szExt);
        MultiByteToWideChar(CP_ACP, 0, szFallback, -1, pOutWPath, MAX_PATH);
        return;
    }

    // "Resources\" 다음 문자부터가 상대 경로
    const _char* pRelative = pResources + strlen("Resources\\");

    // "../../Resources/" + 상대경로 조합
    _char szResult[MAX_PATH] = {};
    strcpy_s(szResult, "../../Resources/");
    strcat_s(szResult, pRelative);

    // 백슬래시 → 슬래시 통일 (옵션)
    for (_char* p = szResult; *p; ++p)
        if ('\\' == *p) *p = '/';

    MultiByteToWideChar(CP_ACP, 0, szResult, -1, pOutWPath, MAX_PATH);
}

bool CModel_Converter::Should_Skip_Mesh(const char* pMeshName)
{
    // LowMesh 제거
    char szLower[MAX_PATH] = {};
    strcpy_s(szLower, pMeshName);
    _strlwr_s(szLower);

    if (strstr(szLower, "lowmesh"))
        return true;

    const char* pNLOD = strstr(szLower, "_nlod");   
    if (nullptr != pNLOD)
    {
        char cLod = *(pNLOD + 5);
        if (cLod != '0')
            return true;
    }

    return false;
}
