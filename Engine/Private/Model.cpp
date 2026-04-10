#include "Model.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "Bone.h"
#include "Animation.h"

/* ============================================================================
   * Solo Leveling Model Data (.bin) 포맷 v1
   * ----------------------------------------------------------------------------
   * [Header]  (80 byte)
   *   char        magic[4]              = "SLMD"
   *   _uint       version               = 1
   *   _uint       modelType             (0=NONANIM, 1=ANIM)
   *   _uint       reserved              = 0
   *   _float4x4   PreTransformMatrix
   *
   * [Meshes]       _uint iNumMeshes + MESH_DESC[]
   *   char        szName[MAX_PATH]
   *   _uint       iMaterialIndex
   *   _uint       iVertexStride         (sizeof(VTXMESH) or VTXANIMMESH)
   *   _uint       iNumVertices
   *   byte        vertices[iNumVertices * iVertexStride]
   *   _uint       iNumIndices
   *   _uint       indices[iNumIndices]
   *   // ANIM 전용 본 바인딩:
   *   _uint       iNumBones (mesh-local)
   *   _uint       boneIndices[iNumBones]      (글로벌 본 인덱스)
   *   _float4x4   offsetMatrices[iNumBones]
   *
   * [Materials]    _uint iNumMaterials + MATERIAL_DESC[]
   *   _uint       iNumTextures
   *   for each tex: _uint eType + wchar_t szPath[MAX_PATH]   (Resources/ 루트 기준 상대)
   *
   * [Bones]        _uint iNumBones + BONE_DESC[]   (NONANIM 이면 0)
   *   char        szName[MAX_PATH]
   *   _int        iParentIndex
   *   _float4x4   TransformationMatrix
   *
   * [Animations]   _uint iNumAnimations + ANIMATION_DESC[] (NONANIM 이면 0)
   *   char        szName[MAX_PATH]
   *   _float      fDuration
   *   _float      fTickPerSecond
   *   _uint       bIsLoop (0/1)
   *   _uint       iNumChannels
   *   for each ch: _uint iBoneIndex + _uint iNumKeyFrames + KEYFRAME[iNumKeyFrames]
   * ============================================================================ */

namespace
{
	constexpr char SLMD_MAGIC[4] = { 'S', 'L', 'M', 'D' };
	constexpr _uint SLMD_VERSION = 1;
}

CModel::CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent { pDevice, pContext }
{
}

CModel::CModel(const CModel& Prototype)
	: CComponent{ Prototype }
	, m_eModelType { Prototype.m_eModelType }
	, m_PreTransformMatrix { Prototype.m_PreTransformMatrix }
	, m_iNumMeshes { Prototype.m_iNumMeshes }
	, m_Meshes {Prototype.m_Meshes }
	, m_iNumMaterials { Prototype.m_iNumMaterials }
	, m_Materials { Prototype.m_Materials }
	, m_iNumBones { Prototype.m_iNumBones }
	, m_iNumAnimations { Prototype.m_iNumAnimations }
	, m_AnimationIndices { Prototype.m_AnimationIndices }
{
	for (auto& pMesh : m_Meshes)
		Safe_AddRef(pMesh);

	for (auto& pMaterial : m_Materials)
		Safe_AddRef(pMaterial);

	m_Bones.reserve(m_iNumBones);
	for (auto& pBone : Prototype.m_Bones)
		m_Bones.push_back(pBone->Clone());

	m_Animations.reserve(m_iNumAnimations);
	for (auto& pAnim : Prototype.m_Animations)
		m_Animations.push_back(pAnim->Clone());
}

const _char* CModel::Get_AnimationName(_uint iIndex) const
{
	if (iIndex >= m_iNumAnimations)
		return "";

	return m_Animations[iIndex]->Get_Name();
}

_float CModel::Get_TrackPosition() const
{
	if (m_iCurrentAnimationIndex >= m_iNumAnimations)
		return 0.f;

	return m_Animations[m_iCurrentAnimationIndex]->Get_CurrentTrackPosition();
}

_float CModel::Get_Duration() const
{
	if (m_iCurrentAnimationIndex >= m_iNumAnimations)
		return 0.f;

	return m_Animations[m_iCurrentAnimationIndex]->Get_Duration();
}

_int CModel::Get_BoneIndex(const _char* pBoneName) const
{
	_int iIndex = { 0 };

	auto iter = find_if(m_Bones.begin(), m_Bones.end(), [&](CBone* pBone)
		{
			if (true == pBone->Compare_Name(pBoneName))
				return true;
			++iIndex;
			return false;
		});

	if (iter == m_Bones.end())
		return -1;

	return iIndex;
}

const _float4x4* CModel::Get_BoneCombinedMatrixPtr(_uint iBoneIndex) const
{
	if (iBoneIndex >= m_iNumBones)
		return nullptr;

	return m_Bones[iBoneIndex]->Get_CombinedTransformMatrixPtr();
}

_int CModel::Get_AnimationIndex(const _char* pAnimationName) const
{
	auto iter = m_AnimationIndices.find(pAnimationName);
	if (iter == m_AnimationIndices.end())
		return -1;

	return static_cast<_int>(iter->second);
}

HRESULT CModel::Initialize_Prototype(const MODEL_DESC& Desc)
{
	m_eModelType = Desc.eModelType;
	m_PreTransformMatrix = Desc.PreTransformMatrix;

	if (FAILED(Ready_Meshes(Desc)))
		return E_FAIL;

	if (FAILED(Ready_Materials(Desc)))
		return E_FAIL;

	if (FAILED(Ready_Bones(Desc)))
		return E_FAIL;

	if (FAILED(Ready_Animations(Desc)))
		return E_FAIL;

	return S_OK;
}

HRESULT CModel::Initialize(void* pArg)
{
	return S_OK;
}

HRESULT CModel::Bind_Material(CShader* pShader, const _char* pConstantName, _uint iMeshIndex, TEXTURE_TYPE eType, _uint iTexIndex)
{
	if (iMeshIndex >= m_iNumMeshes)
		return E_FAIL;

	_uint iMaterialIndex = m_Meshes[iMeshIndex]->Get_MaterialIndex();
	if (iMaterialIndex >= m_iNumMaterials)
		return E_FAIL;

	return m_Materials[iMaterialIndex]->Bind_ShaderResource(pShader, pConstantName, eType, iTexIndex);
}

HRESULT CModel::Bind_BoneMatrices(CShader* pShader, const _char* pConstantName, _uint iMeshIndex)
{
	if (iMeshIndex >= m_iNumMeshes)
		return E_FAIL;

	return m_Meshes[iMeshIndex]->Bind_BoneMatrices(pShader, pConstantName, m_Bones);
}

HRESULT CModel::Render(_uint iMeshIndex)
{
	if (iMeshIndex >= m_iNumMeshes)
		return E_FAIL;

	m_Meshes[iMeshIndex]->Bind_Resources();
	m_Meshes[iMeshIndex]->Render();

	return S_OK;
}

_bool CModel::Play_Animation(_float fTimeDelta)
{
	if (m_iCurrentAnimationIndex >= m_iNumAnimations)
		return false;

	// 현재 애니메이션 : 각 채널이 자기 본의 LocalTransformMatrix 갱신
	_bool isFinished = m_Animations[m_iCurrentAnimationIndex]->Update_TransformationMatrix(m_Bones, fTimeDelta, m_isAnimLoop);

	// 모든 본 -> Combined = Local x Parent.Combined
	_matrix PreTransformMatrix = XMLoadFloat4x4(&m_PreTransformMatrix);

	for (auto& pBone : m_Bones)
		pBone->Update_CombinedTransformationMatrix(m_Bones, PreTransformMatrix);

	return isFinished;
}

void CModel::Set_AnimationIndex(_uint iIndex)
{
	if (iIndex >= m_iNumAnimations)
		return;

	if (iIndex == m_iCurrentAnimationIndex)
		return;

	m_Animations[m_iCurrentAnimationIndex]->Reset_TrackPosition();

	m_iCurrentAnimationIndex = iIndex;
	m_isAnimLoop = m_Animations[iIndex]->Get_IsLoop();
}

HRESULT CModel::Set_Animation(const _char* pAnimationName)
{
	_int iIndex = Get_AnimationIndex(pAnimationName);
	if (-1 == iIndex)
		return E_FAIL;

	Set_AnimationIndex(static_cast<_uint>(iIndex));

	return S_OK;
}

void CModel::Set_AnimationLoop(_bool bLoop)
{
	if (m_iCurrentAnimationIndex >= m_iNumAnimations)
		return;

	m_isAnimLoop = bLoop;
	m_Animations[m_iCurrentAnimationIndex]->Set_IsLoop(bLoop);
}

HRESULT CModel::Ready_Meshes(const MODEL_DESC& Desc)
{
	m_iNumMeshes = Desc.iNumMeshes;
	m_Meshes.reserve(m_iNumMeshes);

	for (size_t i = 0; i < m_iNumMeshes; i++)
	{
		CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, Desc.eModelType, Desc.pMeshes[i]);
		if (nullptr == pMesh)
			return E_FAIL;

		m_Meshes.push_back(pMesh);
	}

	return S_OK;
}

HRESULT CModel::Ready_Materials(const MODEL_DESC& Desc)
{
	m_iNumMaterials = Desc.iNumMaterials;
	m_Materials.reserve(m_iNumMaterials);

	for (size_t i = 0; i < m_iNumMaterials; i++)
	{
		CMaterial* pMaterial = CMaterial::Create(m_pDevice, m_pContext);
		if (nullptr == pMaterial)
			return E_FAIL;

		const MATERIAL_DESC& MaterialDesc = Desc.pMaterials[i];

		for (size_t j = 0; j < MaterialDesc.iNumTextures; j++)
		{
			if (FAILED(pMaterial->Add_Texture(MaterialDesc.pTextures[j])))
			{
				Safe_Release(pMaterial);
				return E_FAIL;
			}
		}

		m_Materials.push_back(pMaterial);
	}

	return S_OK;
}

HRESULT CModel::Ready_Bones(const MODEL_DESC& Desc)
{
	m_iNumBones = Desc.iNumBones;
	m_Bones.reserve(m_iNumBones);

	for (size_t i = 0; i < m_iNumBones; i++)
	{
		CBone* pBone = CBone::Create(Desc.pBones[i]);
		if (nullptr == pBone)
			return E_FAIL;

		m_Bones.push_back(pBone);
	}
	
	return S_OK;
}

HRESULT CModel::Ready_Animations(const MODEL_DESC& Desc)
{
	m_iNumAnimations = Desc.iNumAnimations;
	m_Animations.reserve(m_iNumAnimations);

	for (size_t i = 0; i < m_iNumAnimations; i++)
	{
		CAnimation* pAnimation = CAnimation::Create(Desc.pAnimations[i]);
		if (nullptr == pAnimation)
			return E_FAIL;

		m_Animations.push_back(pAnimation);

		// 이름 -> 벡터 인덱스 캐시 등록
		m_AnimationIndices.emplace(pAnimation->Get_Name(), i);
	}

	return S_OK;
}

HRESULT CModel::Load_Binary_Desc(const _tchar* pBinaryPath, MODEL_DESC* pOutDesc)
{

	FILE* fp = nullptr;
	if (0 != _wfopen_s(&fp, pBinaryPath, L"rb") || nullptr == fp)
		return E_FAIL;

	/* ===== Header ===== */
	char magic[4] = {};
	fread(magic, sizeof(char), 4, fp);
	if (0 != memcmp(magic, SLMD_MAGIC, 4))
	{
		fclose(fp);
		return E_FAIL;
	}

	_uint iVersion = 0;
	fread(&iVersion, sizeof(_uint), 1, fp);
	if (SLMD_VERSION != iVersion)
	{
		fclose(fp);
		return E_FAIL;
	}

	_uint iModelType = 0;
	fread(&iModelType, sizeof(_uint), 1, fp);
	pOutDesc->eModelType = static_cast<MODEL>(iModelType);

	_uint iReserved = 0;
	fread(&iReserved, sizeof(_uint), 1, fp);

	fread(&pOutDesc->PreTransformMatrix, sizeof(_float4x4), 1, fp);

	/* ===== Meshes ===== */
	fread(&pOutDesc->iNumMeshes, sizeof(_uint), 1, fp);
	pOutDesc->pMeshes = new MESH_DESC[pOutDesc->iNumMeshes]{};

	for (_uint i = 0; i < pOutDesc->iNumMeshes; ++i)
	{
		MESH_DESC& Mesh = pOutDesc->pMeshes[i];

		fread(Mesh.szName, sizeof(char), MAX_PATH, fp);
		fread(&Mesh.iMaterialIndex, sizeof(_uint), 1, fp);
		fread(&Mesh.iVertexStride, sizeof(_uint), 1, fp);
		fread(&Mesh.iNumVertices, sizeof(_uint), 1, fp);

		// 정점 스트라이드 기반 raw byte 할당 (VTXMESH/VTXANIMMESH 구분 없음)
		Mesh.pVertices = new _ubyte[static_cast<size_t>(Mesh.iVertexStride) * Mesh.iNumVertices];
		fread(Mesh.pVertices, Mesh.iVertexStride, Mesh.iNumVertices, fp);

		fread(&Mesh.iNumIndices, sizeof(_uint), 1, fp);
		Mesh.pIndices = new _uint[Mesh.iNumIndices]{};
		fread(Mesh.pIndices, sizeof(_uint), Mesh.iNumIndices, fp);

		// Anim 전용 본 바인딩
		if (MODEL::ANIM == pOutDesc->eModelType)
		{
			fread(&Mesh.iNumBones, sizeof(_uint), 1, fp);
			if (Mesh.iNumBones > 0)
			{
				Mesh.pBoneIndices = new _uint[Mesh.iNumBones]{};
				fread(Mesh.pBoneIndices, sizeof(_uint), Mesh.iNumBones, fp);

				Mesh.pOffsetMatrices = new _float4x4[Mesh.iNumBones]{};
				fread(Mesh.pOffsetMatrices, sizeof(_float4x4), Mesh.iNumBones, fp);
			}
		}
	}

	/* ===== Materials ===== */
	fread(&pOutDesc->iNumMaterials, sizeof(_uint), 1, fp);
	pOutDesc->pMaterials = new MATERIAL_DESC[pOutDesc->iNumMaterials]{};

	for (_uint i = 0; i < pOutDesc->iNumMaterials; ++i)
	{
		MATERIAL_DESC& Mat = pOutDesc->pMaterials[i];

		fread(&Mat.iNumTextures, sizeof(_uint), 1, fp);
		Mat.pTextures = new MATERIAL_TEXTURE_DESC[Mat.iNumTextures]{};

		for (_uint j = 0; j < Mat.iNumTextures; ++j)
		{
			_uint iTexType = 0;
			fread(&iTexType, sizeof(_uint), 1, fp);
			Mat.pTextures[j].eType = static_cast<TEXTURE_TYPE>(iTexType);
			fread(Mat.pTextures[j].szPath, sizeof(wchar_t), MAX_PATH, fp);
		}
	}

	/* ===== Bones ===== */
	fread(&pOutDesc->iNumBones, sizeof(_uint), 1, fp);
	if (pOutDesc->iNumBones > 0)
	{
		pOutDesc->pBones = new BONE_DESC[pOutDesc->iNumBones]{};
		for (_uint i = 0; i < pOutDesc->iNumBones; ++i)
		{
			BONE_DESC& Bone = pOutDesc->pBones[i];
			fread(Bone.szName, sizeof(char), MAX_PATH, fp);
			fread(&Bone.iParentIndex, sizeof(_int), 1, fp);
			fread(&Bone.TransformationMatrix, sizeof(_float4x4), 1, fp);
		}
	}

	/* ===== Animations ===== */
	fread(&pOutDesc->iNumAnimations, sizeof(_uint), 1, fp);
	if (pOutDesc->iNumAnimations > 0)
	{
		pOutDesc->pAnimations = new ANIMATION_DESC[pOutDesc->iNumAnimations]{};
		for (_uint i = 0; i < pOutDesc->iNumAnimations; ++i)
		{
			ANIMATION_DESC& Anim = pOutDesc->pAnimations[i];

			fread(Anim.szName, sizeof(char), MAX_PATH, fp);
			fread(&Anim.fDuration, sizeof(_float), 1, fp);
			fread(&Anim.fTickPerSecond, sizeof(_float), 1, fp);

			_uint iIsLoop = 0;
			fread(&iIsLoop, sizeof(_uint), 1, fp);
			Anim.bIsLoop = (0 != iIsLoop);

			fread(&Anim.iNumChannels, sizeof(_uint), 1, fp);
			Anim.pChannels = new CHANNEL_DESC[Anim.iNumChannels]{};

			for (_uint j = 0; j < Anim.iNumChannels; ++j)
			{
				CHANNEL_DESC& Ch = Anim.pChannels[j];
				fread(&Ch.iBoneIndex, sizeof(_uint), 1, fp);
				fread(&Ch.iNumKeyFrames, sizeof(_uint), 1, fp);
				Ch.pKeyFrames = new KEYFRAME[Ch.iNumKeyFrames]{};
				fread(Ch.pKeyFrames, sizeof(KEYFRAME), Ch.iNumKeyFrames, fp);
			}
		}
	}

	fclose(fp);
	return S_OK;
}

void CModel::Free_Binary_Desc(MODEL_DESC* pDesc)
{
	// Animations
	if (nullptr != pDesc->pAnimations)
	{
		for (_uint i = 0; i < pDesc->iNumAnimations; ++i)
		{
			ANIMATION_DESC& Anim = pDesc->pAnimations[i];
			if (nullptr != Anim.pChannels)
			{
				for (_uint j = 0; j < Anim.iNumChannels; ++j)
					Safe_Delete_Array(Anim.pChannels[j].pKeyFrames);

				Safe_Delete_Array(Anim.pChannels);
			}
		}
		Safe_Delete_Array(pDesc->pAnimations);
	}

	// Bones
	Safe_Delete_Array(pDesc->pBones);

	// Materials
	if (nullptr != pDesc->pMaterials)
	{
		for (_uint i = 0; i < pDesc->iNumMaterials; ++i)
			Safe_Delete_Array(pDesc->pMaterials[i].pTextures);

		Safe_Delete_Array(pDesc->pMaterials);
	}

	// Meshes
	if (nullptr != pDesc->pMeshes)
	{
		for (_uint i = 0; i < pDesc->iNumMeshes; ++i)
		{
			MESH_DESC& Mesh = pDesc->pMeshes[i];

			// pVertices 는 _ubyte 배열로 할당했으므로 _ubyte* 로 캐스팅 후 delete[]
			if (nullptr != Mesh.pVertices)
			{
				delete[] static_cast<_ubyte*>(Mesh.pVertices);
				Mesh.pVertices = nullptr;
			}

			Safe_Delete_Array(Mesh.pIndices);
			Safe_Delete_Array(Mesh.pBoneIndices);
			Safe_Delete_Array(Mesh.pOffsetMatrices);
		}
		Safe_Delete_Array(pDesc->pMeshes);
	}
}

CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const MODEL_DESC& Desc)
{
	CModel* pInstance = new CModel(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(Desc)))
	{
		MSG_BOX("Failed to Created : CModel");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, const _tchar* pBinaryPath)
{
	MODEL_DESC Desc{};

	if (FAILED(Load_Binary_Desc(pBinaryPath, &Desc)))
	{
		Free_Binary_Desc(&Desc);    // 부분 할당된 상태 해제
		MSG_BOX("Failed to Load Binary : CModel");
		return nullptr;
	}

	CModel* pInstance = new CModel(pDevice, pContext);

	HRESULT hr = pInstance->Initialize_Prototype(Desc);

	// Initialize_Prototype 가 내부 구조체로 복사 완료했으므로 임시 Desc 해제
	Free_Binary_Desc(&Desc);

	if (FAILED(hr))
	{
		MSG_BOX("Failed to Created : CModel (Binary)");
		Safe_Release(pInstance);
		return nullptr;
	}

	return pInstance;
}

CComponent* CModel::Clone(void* pArg)
{
	CModel* pInstance = new CModel(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CModel");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CModel::Free()
{
	__super::Free();

	for (auto& pAnim : m_Animations)
		Safe_Release(pAnim);
	m_Animations.clear();
	m_AnimationIndices.clear();

	for (auto& pBone : m_Bones)
		Safe_Release(pBone);
	m_Bones.clear();

	for (auto& pMaterial : m_Materials)
		Safe_Release(pMaterial);
	m_Materials.clear();

	for (auto& pMesh : m_Meshes)
		Safe_Release(pMesh);
	m_Meshes.clear();
}
