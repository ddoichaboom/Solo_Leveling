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
	, m_iNumChannels { Prototype.m_iNumChannels }
	, m_Channels { Prototype.m_Channels }
	, m_CurrentKeyFrameIndices { Prototype.m_CurrentKeyFrameIndices }
	, m_isLoop { Prototype.m_isLoop }
{
	strcpy_s(m_szName, Prototype.m_szName);

	for (auto& pChannel : m_Channels)
		Safe_AddRef(pChannel);
}

HRESULT CAnimation::Initialize(const ANIMATION_DESC& Desc)
{
	strcpy_s(m_szName, Desc.szName);
	m_fDuration			= Desc.fDuration;
	m_fTickPerSecond	= Desc.fTickPerSecond;
	m_iNumChannels		= Desc.iNumChannels;
	m_isLoop			= Desc.bIsLoop;

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

	if (m_fCurrentTrackPosition >= m_fDuration)
	{
		if (false == isLoop)
			return true;

		Reset_TrackPosition();		// 0으로 이동
	}

	_uint iChannelIndex = {};

	for (auto& pChannel : m_Channels)
	{
		pChannel->Update_TransformationMatrix(Bones, m_fCurrentTrackPosition, &m_CurrentKeyFrameIndices[iChannelIndex++]);
	}


	return false;
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
