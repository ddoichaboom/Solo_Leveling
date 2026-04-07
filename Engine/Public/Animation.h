#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CAnimation final : public CBase
{
private:
	CAnimation();
	CAnimation(const CAnimation& Prototype);
	virtual ~CAnimation() = default;

public:
	const _char*			Get_Name() const { return m_szName; }
	_float					Get_Duration() const { return m_fDuration; }
	_float					Get_TickPerSecond() const { return m_fTickPerSecond; }
	_bool					Get_IsLoop() const { return m_isLoop; }
	_float					Get_CurrentTrackPosition() const { return m_fCurrentTrackPosition; }
	void					Reset_TrackPosition() { m_fCurrentTrackPosition = 0.f; }

public:
	HRESULT					Initialize(const ANIMATION_DESC& Desc);
	_bool					Update_TransformationMatrix(const vector<class CBone*>& Bones,
														_float fTimeDelta, _bool isLoop);

private:
	_char					m_szName[MAX_PATH] = {};
	_float					m_fDuration = {};
	_float					m_fTickPerSecond = {};
	_float					m_fCurrentTrackPosition = {};
	_bool					m_isLoop = { false };

	_uint					m_iNumChannels = {};
	vector<class CChannel*> m_Channels;
	vector<_uint>			m_CurrentKeyFrameIndices;

public:
	static CAnimation*		Create(const ANIMATION_DESC& Desc);
	CAnimation*				Clone();
	virtual void			Free() override;
};

NS_END