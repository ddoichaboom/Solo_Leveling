#pragma once
#include "Editor_Defines.h"

NS_BEGIN(Editor)

class CModel_Converter final
{
public:
    // pFbxPath  : FBX 파일 절대 경로 (wchar_t)
    // pBinPath  : 출력 .bin 절대 경로 (wchar_t)
    // eModelType: MODEL::NONANIM or MODEL::ANIM
    static HRESULT      Convert(const _tchar* pFbxPath, const _tchar* pBinPath, MODEL eModelType);

private:
    // 본 노드 트리를 DFS 순서로 수집 — CModel::Ready_Bones 와 동일 순서
    static void         Collect_Bones(aiNode* pNode, _int iParentIndex,
                                        vector<pair<aiNode*, _int>>& outBones,
                                        unordered_map<string, _uint>& outBoneNameMap);

    // 섹션별 write 헬퍼
    static HRESULT      Write_Meshes(FILE* fp, const aiScene* pScene, MODEL eModelType,
                                        const unordered_map<string, _uint>& boneNameMap);

    static HRESULT      Write_Materials(FILE* fp, const aiScene* pScene,
                                    const _char* pFbxDirFull);

    static HRESULT      Write_Bones(FILE* fp,
                                    const vector<pair<aiNode*, _int>>& bones);

    static HRESULT      Write_Animations(FILE* fp, const aiScene* pScene,
                                            const unordered_map<string, _uint>& boneNameMap);

    // Assimp 텍스처 경로 → "../../Resources/..." 형태 wchar_t 상대 경로 변환
    static void         Build_TexturePath(const _char* pFbxDirFull, const char* pAiTexPath,
                                            _tchar* pOutWPath);
    static bool         Should_Skip_Mesh(const char* pMeshName);
};

NS_END

