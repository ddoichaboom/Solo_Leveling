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
	_uint                   Get_NumNotifies() const { return static_cast<_uint>(m_Notifies.size()); }
	const ANIM_NOTIFY&		Get_Notify(_uint i) const { return m_Notifies[i]; }
	void                    Set_NotifyTick(_uint i, _float fTick);
	void                    Set_NotifyType(_uint i, ANIM_NOTIFY_TYPE eType);
	void                    Add_Notify(const ANIM_NOTIFY& Notify);
	void                    Remove_Notify(_uint i);
	void                    Sort_Notifies();

public:
	HRESULT					Initialize(const ANIMATION_DESC& Desc);
	_bool					Update_TransformationMatrix(const vector<class CBone*>& Bones,
														_float fTimeDelta, _bool isLoop,
														class INotifyListener* pListener = nullptr );
	_bool					Advance_Time(_float fTimeDelta, _bool isLoop);
	void					Evaluate_Pose(BONE_POSE* pOutPoses, _ubyte* pOutHasPose);

	void					Tick_Notifies(_float fPrevTick, _float fCurTick, _bool bWrapped, class INotifyListener* pListener = nullptr);


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

	vector<ANIM_NOTIFY>		m_Notifies;
	_float					m_fLastNotifyTick = { 0.f };

public:
	static CAnimation*		Create(const ANIMATION_DESC& Desc);
	CAnimation*				Clone();
	virtual void			Free() override;
};

NS_END