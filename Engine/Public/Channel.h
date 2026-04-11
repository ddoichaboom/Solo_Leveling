#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CChannel final : public CBase
{
private:
	CChannel();
	virtual ~CChannel() = default;

public:
	_uint					Get_BoneIndex() const { return m_iBoneIndex; }
	_uint					Get_NumKeyFrames() const { return m_iNumKeyFrames; }

public:
	HRESULT					Initialize(const CHANNEL_DESC& Desc);
	void					Update_TransformationMatrix(const vector<class CBone*>& Bones,
														_float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex);
	void					Get_SQT(_float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex,
									_float3& vOutScale, _float4& vOutRotation, _float3& vOutTranslation);

private:
	_uint					m_iBoneIndex = {};
	_uint					m_iNumKeyFrames = {};
	vector<KEYFRAME>		m_KeyFrames;

public:
	static CChannel*		Create(const CHANNEL_DESC& Desc);
	virtual void			Free() override;
};

NS_END