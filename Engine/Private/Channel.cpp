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

void CChannel::Get_SQT(_float fCurrentTrackPosition, _uint* pCurrentKeyFrameIndex, _float3& vOutScale, _float4& vOutRotation, _float3& vOutTranslation)
{
    if (0 == m_iNumKeyFrames)
        return;

    if (0.f == fCurrentTrackPosition)
        *pCurrentKeyFrameIndex = 0;

    const KEYFRAME& LastKeyFrameDesc = m_KeyFrames.back();

    _vector vScale, vRotation, vTranslation;

    if (fCurrentTrackPosition >= LastKeyFrameDesc.fTrackPosition)
    {
        vScale = XMLoadFloat3(&LastKeyFrameDesc.vScale);
        vRotation = XMLoadFloat4(&LastKeyFrameDesc.vRotation);
        vTranslation = XMLoadFloat3(&LastKeyFrameDesc.vTranslation);
    }
    else
    {
        while (fCurrentTrackPosition >= m_KeyFrames[*pCurrentKeyFrameIndex + 1].fTrackPosition)
            ++(*pCurrentKeyFrameIndex);

        _uint iCur = *pCurrentKeyFrameIndex;
        _uint iNext = iCur + 1;

        _float fRatio = (fCurrentTrackPosition - m_KeyFrames[iCur].fTrackPosition) /
            (m_KeyFrames[iNext].fTrackPosition - m_KeyFrames[iCur].fTrackPosition);

        vScale = XMVectorLerp(XMLoadFloat3(&m_KeyFrames[iCur].vScale),
            XMLoadFloat3(&m_KeyFrames[iNext].vScale), fRatio);
        vRotation = XMQuaternionSlerp(XMLoadFloat4(&m_KeyFrames[iCur].vRotation),
            XMLoadFloat4(&m_KeyFrames[iNext].vRotation), fRatio);
        vTranslation = XMVectorLerp(XMLoadFloat3(&m_KeyFrames[iCur].vTranslation),
            XMLoadFloat3(&m_KeyFrames[iNext].vTranslation), fRatio);
    }

    XMStoreFloat3(&vOutScale, vScale);
    XMStoreFloat4(&vOutRotation, vRotation);
    XMStoreFloat3(&vOutTranslation, vTranslation);
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
