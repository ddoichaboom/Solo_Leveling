#include "Bone.h"
#include "Animation.h"

CBone::CBone()
{
}

HRESULT CBone::Initialize(const BONE_DESC& Desc)
{
    strcpy_s(m_szName, Desc.szName);
    m_iParentIndex = Desc.iParentIndex;

    m_TransformationMatrix = Desc.TransformationMatrix;
    XMStoreFloat4x4(&m_CombinedTransformationMatrix, XMMatrixIdentity());

    return S_OK;
}

void CBone::Update_CombinedTransformationMatrix(const vector<CBone*>& Bones, _fmatrix PreTransformMatrix)
{
    if (-1 == m_iParentIndex)
        XMStoreFloat4x4(&m_CombinedTransformationMatrix,
                        PreTransformMatrix * XMLoadFloat4x4(&m_TransformationMatrix));
    else
        XMStoreFloat4x4(&m_CombinedTransformationMatrix,
                        XMLoadFloat4x4(&m_TransformationMatrix) * XMLoadFloat4x4(Bones[m_iParentIndex]->Get_CombinedTransformMatrixPtr()));
}

CBone* CBone::Create(const BONE_DESC& Desc)
{
    CBone* pInstance = new CBone();

    if (FAILED(pInstance->Initialize(Desc)))
    {
        MSG_BOX("Failed to Created : CBone");
        Safe_Release(pInstance);
    }

    return pInstance;
}

CBone* CBone::Clone()
{
    return new CBone(*this);
}


void CBone::Free()
{
    __super::Free();
}
