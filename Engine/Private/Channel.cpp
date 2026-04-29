#include "Channel.h"
#include "Bone.h"

CChannel::CChannel()
{

}

HRESULT CChannel::Initialize(const CHANNEL_DESC& Desc)
{
    m_iBoneIndex = Desc.iBoneIndex;
    m_iNumKeyFrames = Desc.iNumKeyFrames;

    if (m_iNumKeyFrames > 0)
    {
        m_KeyFrames.resize(m_iNumKeyFrames);
        memcpy(m_KeyFrames.data(), Desc.pKeyFrames, sizeof(KEYFRAME) * m_iNumKeyFrames);
    }

	return S_OK;
}

void CChannel::Update_TransformationMatrix(const vector<class CBone*>& Bones, _float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex)
{
    if (0 == m_iNumKeyFrames)
        return;

    if (0.f == fCurrentTrackPosition)
        *pCurrentKeyFrameIndex = 0;

    const KEYFRAME& LastKeyFrameDesc = m_KeyFrames.back();

    _vector     vScale = {};
    _vector     vRotation = {};
    _vector     vTranslation = {};

    // 마지막 키 프레임 이후 -> 마지막 값 고정
    if (fCurrentTrackPosition >= LastKeyFrameDesc.fTrackPosition)
    {
        vScale = XMLoadFloat3(&LastKeyFrameDesc.vScale);
        vRotation = XMLoadFloat4(&LastKeyFrameDesc.vRotation);
        vTranslation = XMVectorSetW(XMLoadFloat3(&LastKeyFrameDesc.vTranslation), 1.f);
    }
    // 보간
    else
    {
        while (fCurrentTrackPosition >= m_KeyFrames[*pCurrentKeyFrameIndex + 1].fTrackPosition)
            ++(*pCurrentKeyFrameIndex);

        _uint iCur  = *pCurrentKeyFrameIndex;
        _uint iNext = iCur + 1;

        _float fRatio       = (fCurrentTrackPosition - m_KeyFrames[iCur].fTrackPosition) / (m_KeyFrames[iNext].fTrackPosition - m_KeyFrames[iCur].fTrackPosition);

        // Scale - Lerp
        _vector vSrcScale = XMLoadFloat3(&m_KeyFrames[iCur].vScale);
        _vector vDstScale = XMLoadFloat3(&m_KeyFrames[iNext].vScale);

        // Rotation - Slerp
        _vector vSrcRotation = XMLoadFloat4(&m_KeyFrames[iCur].vRotation);
        _vector vDstRotation = XMLoadFloat4(&m_KeyFrames[iNext].vRotation);

        // Translation - Lerp
        _vector vSrcTranslation = XMVectorSetW(XMLoadFloat3(&m_KeyFrames[iCur].vTranslation), 1.f);
        _vector vDstTranslation = XMVectorSetW(XMLoadFloat3(&m_KeyFrames[iNext].vTranslation), 1.f);

        vScale = XMVectorLerp(vSrcScale, vDstScale, fRatio);
        vRotation = XMQuaternionSlerp(vSrcRotation, vDstRotation, fRatio);
        vTranslation = XMVectorLerp(vSrcTranslation, vDstTranslation, fRatio);
    }

    // 보간 결과 -> 변환 행렬 -> 본에 적용
    _matrix TransformationMatrix = XMMatrixAffineTransformation(
        vScale,
        XMQuaternionIdentity(),
        vRotation,
        vTranslation);

    Bones[m_iBoneIndex]->Set_TransformationMatrix(TransformationMatrix);
}

void CChannel::Evaluate_Pose(_float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex, BONE_POSE* pOutPose)
{
    if (0 == m_iNumKeyFrames || nullptr == pOutPose)
        return;

    if (0.f == fCurrentTrackPosition)
        *pCurrentKeyFrameIndex = 0;

    const KEYFRAME& LastKeyFrameDesc = m_KeyFrames.back();

    _vector vScale, vRotation, vTranslation;

    if (fCurrentTrackPosition >= LastKeyFrameDesc.fTrackPosition)
    {
        vScale          = XMLoadFloat3(&LastKeyFrameDesc.vScale);
        vRotation       = XMLoadFloat4(&LastKeyFrameDesc.vRotation);
        vTranslation    = XMLoadFloat3(&LastKeyFrameDesc.vTranslation);
    }
    else
    {
        while (fCurrentTrackPosition >= m_KeyFrames[*pCurrentKeyFrameIndex + 1].fTrackPosition)
            ++(*pCurrentKeyFrameIndex);

        _uint iCur = *pCurrentKeyFrameIndex;
        _uint iNext = iCur + 1;
        _float fRatio = (fCurrentTrackPosition - m_KeyFrames[iCur].fTrackPosition)
            / (m_KeyFrames[iNext].fTrackPosition - m_KeyFrames[iCur].fTrackPosition);

        _vector vSrcS = XMLoadFloat3(&m_KeyFrames[iCur].vScale);
        _vector vDstS = XMLoadFloat3(&m_KeyFrames[iNext].vScale);
        _vector vSrcR = XMLoadFloat4(&m_KeyFrames[iCur].vRotation);
        _vector vDstR = XMLoadFloat4(&m_KeyFrames[iNext].vRotation);
        _vector vSrcT = XMLoadFloat3(&m_KeyFrames[iCur].vTranslation);
        _vector vDstT = XMLoadFloat3(&m_KeyFrames[iNext].vTranslation);

        vScale = XMVectorLerp(vSrcS, vDstS, fRatio);
        vRotation = XMQuaternionSlerp(vSrcR, vDstR, fRatio);
        vTranslation = XMVectorLerp(vSrcT, vDstT, fRatio);
    }

    XMStoreFloat3(&pOutPose->vScale, vScale);
    XMStoreFloat4(&pOutPose->vRotation, vRotation);
    XMStoreFloat3(&pOutPose->vTranslation, vTranslation);
}

CChannel* CChannel::Create(const CHANNEL_DESC& Desc)
{
    CChannel* pInstance = new CChannel();

    if (FAILED(pInstance->Initialize(Desc)))
    {
        MSG_BOX("Failed to Created : CChannel");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CChannel::Free()
{
	__super::Free();
}
