#pragma once

#include "Component.h"

NS_BEGIN(Engine)

class ENGINE_DLL CModel final : public CComponent
{
private:
	CModel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CModel(const CModel& Prototype);
	virtual ~CModel() = default;

public:
	MODEL							Get_ModelType() const { return m_eModelType; }
	_uint							Get_NumMeshes() const { return m_iNumMeshes; }
	_uint							Get_NumMaterials() const { return m_iNumMaterials; }
	_uint							Get_NumBones() const { return m_iNumBones; }
	_uint							Get_NumAnimations() const { return m_iNumAnimations; }

	_int							Get_BoneIndex(const _char* pBoneName) const;
	const _float4x4*				Get_BoneCombinedMatrixPtr(_uint iBoneIndex) const;

	_int							Get_AnimationIndex(const _char* pAnimationName) const;

public:
	virtual HRESULT					Initialize_Prototype(MODEL eType, const MODEL_DESC& Desc);
	virtual HRESULT					Initialize(void* pArg) override;

public:
	HRESULT							Bind_Material(class CShader* pShader, const _char* pConstantName,
												_uint iMeshIndex, TEXTURE_TYPE eType, _uint iTexIndex = 0);
	HRESULT							Bind_BoneMatrices(class CShader* pShader,
													const _char* pConstantName, _uint iMeshIndex);

	HRESULT							Render(_uint iMeshIndex);

	_bool							Play_Animation(_float fTimeDelta);

	void							Set_AnimationIndex(_uint iIndex);
	HRESULT							Set_Animation(const _char* pAnimationName);

private:
	HRESULT							Ready_Meshes(MODEL eType, const MODEL_DESC& Desc);
	HRESULT							Ready_Materials(const MODEL_DESC& Desc);
	HRESULT							Ready_Bones(const MODEL_DESC& Desc);
	HRESULT							Ready_Animations(const MODEL_DESC& Desc);

private:
	MODEL							m_eModelType = { MODEL::END };

	_float4x4						m_PreTransformMatrix = {};

	_uint							m_iNumMeshes = {};
	vector<class CMesh*>			m_Meshes;

	_uint							m_iNumMaterials = {};
	vector<class CMaterial*>		m_Materials;

	_uint							m_iNumBones = {};
	vector<class CBone*>			m_Bones;

	_uint							m_iNumAnimations = {};
	vector<class CAnimation*>		m_Animations;
	unordered_map<string, _uint>	m_AnimationIndices;

	_uint							m_iCurrentAnimationIndex = {};
	_bool							m_isAnimLoop = { false };

public:
	static CModel*					Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODEL eType, const MODEL_DESC& Desc);
	virtual CComponent*				Clone(void* pArg) override;
	virtual void					Free() override;
};

NS_END