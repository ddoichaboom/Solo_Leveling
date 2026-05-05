#include "Animation.h"
#include "Bone.h"
#include "Channel.h"
#include "NotifyListener.h"
#include <set>

CAnimation::CAnimation()
{
}

CAnimation::CAnimation(const CAnimation& Prototype)
	: m_fDuration { Prototype.m_fDuration }
	, m_fTickPerSecond { Prototype.m_fTickPerSecond }
	, m_fCurrentTrackPosition { Prototype.m_fCurrentTrackPosition }
	, m_fPrevTrackPosition{ Prototype.m_fPrevTrackPosition }
	, m_iNumChannels { Prototype.m_iNumChannels }
	, m_Channels { Prototype.m_Channels }
	, m_CurrentKeyFrameIndices { Prototype.m_CurrentKeyFrameIndices }
	, m_isLoop { Prototype.m_isLoop }
	, m_bUseRootMotion{ Prototype.m_bUseRootMotion }
	, m_Notifies{ Prototype.m_Notifies }
	, m_fLastNotifyTick{ Prototype.m_fLastNotifyTick }
{
	strcpy_s(m_szName, Prototype.m_szName);

	for (auto& pChannel : m_Channels)
		Safe_AddRef(pChannel);
}

void CAnimation::Reset_TrackPosition()
{
	m_fCurrentTrackPosition = 0.f;
	m_fPrevTrackPosition = 0.f;
	m_fLastNotifyTick = 0.f;

	for (auto& iKeyFrameIndex : m_CurrentKeyFrameIndices)
		iKeyFrameIndex = 0;
}

void CAnimation::Set_NotifyTick(_uint i, _float fTick)
{
	if (i >= m_Notifies.size())
		return;

	if (fTick < 0.f)
		fTick = 0.f;
	if (fTick > m_fDuration)
		fTick = m_fDuration;

	m_Notifies[i].fTick = fTick;
}

void CAnimation::Set_NotifyType(_uint i, ANIM_NOTIFY_TYPE eType)
{
	if (i >= m_Notifies.size())
		return;

	m_Notifies[i].eType = eType;
}

void CAnimation::Add_Notify(const ANIM_NOTIFY& Notify)
{
	m_Notifies.push_back(Notify);
	Sort_Notifies();
}

void CAnimation::Remove_Notify(_uint i)
{
	if (i >= m_Notifies.size())
		return;

	m_Notifies.erase(m_Notifies.begin() + i);
}

void CAnimation::Sort_Notifies()
{
	sort(m_Notifies.begin(), m_Notifies.end(),
		[](const ANIM_NOTIFY& a, const ANIM_NOTIFY& b)
		{
			return a.fTick < b.fTick;
		});
}

HRESULT CAnimation::Initialize(const ANIMATION_DESC& Desc)
{
	strcpy_s(m_szName, Desc.szName);
	m_fDuration			= Desc.fDuration;
	m_fTickPerSecond	= Desc.fTickPerSecond;
	m_iNumChannels		= Desc.iNumChannels;
	m_isLoop			= Desc.bIsLoop;
	m_bUseRootMotion	= Desc.bUseRootMotion;

	m_Channels.reserve(m_iNumChannels);

	for (size_t i = 0; i < m_iNumChannels; i++)
	{
		CChannel* pChannel = CChannel::Create(Desc.pChannels[i]);
		if (nullptr == pChannel)
			return E_FAIL;

		m_Channels.push_back(pChannel);
	}

	m_CurrentKeyFrameIndices.resize(m_iNumChannels);

	if (Desc.iNumNotifies > 0 && nullptr != Desc.pNotifies)
	{
		m_Notifies.assign(Desc.pNotifies, Desc.pNotifies + Desc.iNumNotifies);
	}

	return S_OK;
}

_bool CAnimation::Update_TransformationMatrix(const vector<class CBone*>& Bones, _float fTimeDelta, _bool isLoop, INotifyListener* pListener)
{
	const _float fPrevTick = m_fCurrentTrackPosition;
	m_fCurrentTrackPosition += m_fTickPerSecond * fTimeDelta;

	_bool bFinished = false;
	_bool bWrapped = false;

	if (m_fCurrentTrackPosition >= m_fDuration)
	{
		if (false == isLoop)
		{
			m_fCurrentTrackPosition = m_fDuration;
			bFinished = true;
		}
		else
		{
			Reset_TrackPosition();
			bWrapped = true;
		}

	}

	_uint iChannelIndex = {};

	for (auto& pChannel : m_Channels)
	{
		pChannel->Update_TransformationMatrix(Bones, m_fCurrentTrackPosition, &m_CurrentKeyFrameIndices[iChannelIndex++]);
	}

	Tick_Notifies(fPrevTick, m_fCurrentTrackPosition, bWrapped, pListener);

	return bFinished;
}

_bool CAnimation::Advance_Time(_float fTimeDelta, _bool isLoop)
{
	m_fPrevTrackPosition = m_fCurrentTrackPosition;
	m_fCurrentTrackPosition += m_fTickPerSecond * fTimeDelta;

	_bool bFinished = false;

	if (m_fCurrentTrackPosition >= m_fDuration)
	{
		if (false == isLoop)
		{
			m_fCurrentTrackPosition = m_fDuration;
			bFinished = true;
		}
		else
		{
			m_fCurrentTrackPosition -= m_fDuration;
			for (auto& iKeyFrameIndex : m_CurrentKeyFrameIndices)
				iKeyFrameIndex = 0;
		}
	}

	return bFinished;
}

void CAnimation::Evaluate_Pose(BONE_POSE* pOutPoses, _ubyte* pOutHasPose)
{
	if (nullptr == pOutPoses || nullptr == pOutHasPose)
		return;

	_uint iChannelIndex = 0;
	for (auto& pChannel : m_Channels)
	{
		const _uint iBoneIndex = pChannel->Get_BoneIndex();
		pChannel->Evaluate_Pose(m_fCurrentTrackPosition,
			&m_CurrentKeyFrameIndices[iChannelIndex++],
			&pOutPoses[iBoneIndex]);
		pOutHasPose[iBoneIndex] = 1;
	}
}

void CAnimation::Tick_Notifies(_float fPrevTick, _float fCurTick, _bool bWrapped, INotifyListener* pListener)
{
	if (nullptr == pListener || m_Notifies.empty())
		return;

	auto fire_in_range = [&](_float fLowExclusive, _float fHighInclusive)
		{
			for (const ANIM_NOTIFY& N : m_Notifies)
			{
				if (N.fTick > fLowExclusive && N.fTick <= fHighInclusive)
				{
					NOTIFY_EVENT Ev{};
					Ev.eType = NOTIFY_TYPE::ANIM_EVENT;
					Ev.iPayload = ETOUI(N.eType);
					Ev.pData = nullptr;
					pListener->OnNotify(Ev);
				}
			}
		};

	if (bWrapped)
	{
		fire_in_range(fPrevTick, m_fDuration);
		fire_in_range(-1.f, fCurTick);
	}
	else
	{
		fire_in_range(fPrevTick, fCurTick);
	}
}

CAnimation* CAnimation::Create(const ANIMATION_DESC& Desc)
{
	CAnimation* pInstance = new CAnimation();

	if (FAILED(pInstance->Initialize(Desc)))
	{
		MSG_BOX("Failed to Created : CAnimation");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CAnimation* CAnimation::Clone()
{
	return new CAnimation(*this);
}

void CAnimation::Free()
{
	__super::Free();

	for (auto& pChannel : m_Channels)
		Safe_Release(pChannel);

	m_Channels.clear();
}
