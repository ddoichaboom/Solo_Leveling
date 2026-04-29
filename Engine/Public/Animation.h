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
	_bool					Get_UseRootMotion() const { return m_bUseRootMotion; }
	_float					Get_CurrentTrackPosition() const { return m_fCurrentTrackPosition; }
	void					Reset_TrackPosition();
	void					Set_IsLoop(_bool bLoop) { m_isLoop = bLoop; }
	void					Set_UseRootMotion(_bool bUse) { m_bUseRootMotion = bUse; }
	_float					Get_PrevTrackPosition() const { return m_fPrevTrackPosition; }

public:
	HRESULT					Initialize(const ANIMATION_DESC& Desc);
	_bool					Update_TransformationMatrix(const vector<class CBone*>& Bones,
														_float fTimeDelta, _bool isLoop);
	_bool					Advance_Time(_float fTimeDelta, _bool isLoop);
	void					Evaluate_Pose(BONE_POSE* pOutPoses, _ubyte* pOutHasPose);


private:
	_char					m_szName[MAX_PATH] = {};
	_float					m_fDuration = {};
	_float					m_fTickPerSecond = {};
	_float					m_fCurrentTrackPosition = {};
	_float					m_fPrevTrackPosition = {};
	_bool					m_isLoop = { false };
	_bool					m_bUseRootMotion = { false };

	_uint					m_iNumChannels = {};
	vector<class CChannel*> m_Channels;
	vector<_uint>			m_CurrentKeyFrameIndices;

public:
	static CAnimation*		Create(const ANIMATION_DESC& Desc);
	CAnimation*				Clone();
	virtual void			Free() override;
};

NS_END