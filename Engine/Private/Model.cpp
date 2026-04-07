#include "Model.h"
#include "Mesh.h"
#include "Shader.h"
#include "Material.h"
#include "Bone.h"
#include "Animation.h"

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

HRESULT CModel::Initialize_Prototype(MODEL eType, const MODEL_DESC& Desc)
{
	m_eModelType = eType;
	m_PreTransformMatrix = Desc.PreTransformMatrix;

	if (FAILED(Ready_Meshes(eType, Desc)))
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

HRESULT CModel::Ready_Meshes(MODEL eType, const MODEL_DESC& Desc)
{
	m_iNumMeshes = Desc.iNumMeshes;
	m_Meshes.reserve(m_iNumMeshes);

	for (size_t i = 0; i < m_iNumMeshes; i++)
	{
		CMesh* pMesh = CMesh::Create(m_pDevice, m_pContext, eType, Desc.pMeshes[i]);
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

CModel* CModel::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODEL eType, const MODEL_DESC& Desc)
{
	CModel* pInstance = new CModel(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype(eType, Desc)))
	{
		MSG_BOX("Failed to Created : CModel");
		Safe_Release(pInstance);
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
