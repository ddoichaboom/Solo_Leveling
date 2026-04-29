#include "Animation.h"
#include "Bone.h"
#include "Channel.h"

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
{
	strcpy_s(m_szName, Prototype.m_szName);

	for (auto& pChannel : m_Channels)
		Safe_AddRef(pChannel);
}

void CAnimation::Reset_TrackPosition()
{
	m_fCurrentTrackPosition = 0.f;
	m_fPrevTrackPosition = 0.f;

	for (auto& iKeyFrameIndex : m_CurrentKeyFrameIndices)
		iKeyFrameIndex = 0;
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

	return S_OK;
}

_bool CAnimation::Update_TransformationMatrix(const vector<class CBone*>& Bones, _float fTimeDelta, _bool isLoop)
{
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
			Reset_TrackPosition();
		}

	}

	_uint iChannelIndex = {};

	for (auto& pChannel : m_Channels)
	{
		pChannel->Update_TransformationMatrix(Bones, m_fCurrentTrackPosition, &m_CurrentKeyFrameIndices[iChannelIndex++]);
	}


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
