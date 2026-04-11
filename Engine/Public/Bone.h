#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CBone final : public CBase
{
private:
	CBone();
	virtual ~CBone() = default;

public:
	const _char*		Get_Name() const { return m_szName; }
	_int				Get_ParentIndex() const { return m_iParentIndex; }

	const _float4x4*	Get_TransformationMatrixPtr() const { return &m_TransformationMatrix; }
	const _float4x4*	Get_CombinedTransformMatrixPtr() const { return &m_CombinedTransformationMatrix; }
	void				Set_TransformationMatrix(_fmatrix TransformationMatrix)
	{
		XMStoreFloat4x4(&m_TransformationMatrix, TransformationMatrix);
	}

	_bool				Compare_Name(const _char* pBoneName) const { return !strcmp(m_szName, pBoneName); }

public:
	HRESULT				Initialize(const BONE_DESC& Desc);
	void				Update_CombinedTransformationMatrix(const vector<CBone*>& Bones, _fmatrix PreTransformMatrix);
		
private:
	_char				m_szName[MAX_PATH] = {};
	_int				m_iParentIndex = { -1 };

	_float4x4			m_TransformationMatrix = {};				// 로컬 ( 애니메이션이 갱신 )
	_float4x4			m_CombinedTransformationMatrix = {};		// 월드 ( Local x Parent.Combined )

public:
	static CBone*		Create(const BONE_DESC& Desc);
	CBone*				Clone();
	virtual void		Free() override;
};

NS_END