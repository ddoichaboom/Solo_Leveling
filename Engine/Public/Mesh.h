#pragma once

#include "VIBuffer.h"

NS_BEGIN(Engine)

class CMesh final : public CVIBuffer
{
private:
	CMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMesh(const CMesh& Prototype);
	virtual ~CMesh() = default;

public:
	_uint					Get_MaterialIndex() const { return m_iMaterialIndex; }
	const _char*			Get_Name() const {	return m_szName; }

public:
	virtual HRESULT			Initialize_Prototype(MODEL eType, const MESH_DESC& Desc);
	virtual HRESULT			Initialize(void* pArg) override;

public:
	HRESULT					Bind_BoneMatrices(class CShader* pShader, const _char* pConstantName, const vector<class CBone*>& Bones);

private:
	HRESULT					Ready_NonAnimMesh(const MESH_DESC& Desc);
	HRESULT					Ready_AnimMesh(const MESH_DESC& Desc);

private:
	_char					m_szName[MAX_PATH] = {};
	_uint					m_iMaterialIndex = {};

	_uint					m_iNumBones = {};
	vector<_uint>			m_BoneIndices = {};
	vector<_float4x4>		m_OffsetMatrices;
	_float4x4				m_BoneMatrices[g_iNumMeshBones] = {};

public:
	static CMesh*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext, MODEL eType, const MESH_DESC& Desc);
	virtual CComponent*		Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END