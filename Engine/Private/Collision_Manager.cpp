#include "Collision_Manager.h"
#include "Collider.h"
#include "Bounding.h"
#include "GameInstance.h"

CCollision_Manager::CCollision_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
    : m_pDevice{ pDevice }
    , m_pContext{ pContext }
{
    Safe_AddRef(m_pDevice);
    Safe_AddRef(m_pContext);
}

HRESULT CCollision_Manager::Initialize()
{
    // 매트릭스는 전부 false 로 시작. 사용처에서 Set_CollisionMatrix 로 활성화
#ifdef _DEBUG
    m_pStates = new CommonStates(m_pDevice);

    m_pEffect = new BasicEffect(m_pDevice);
    m_pEffect->SetVertexColorEnabled(true);

    const void* pShaderByteCode = nullptr;
    size_t      iByteCodeLength = 0;
    m_pEffect->GetVertexShaderBytecode(&pShaderByteCode, &iByteCodeLength);

    if (FAILED(m_pDevice->CreateInputLayout(
        VertexPositionColor::InputElements,
        VertexPositionColor::InputElementCount,
        pShaderByteCode, iByteCodeLength, &m_pInputLayout)))
        return E_FAIL;

    m_pBatch = new PrimitiveBatch<VertexPositionColor>(m_pContext);
#endif
    return S_OK;
}

void CCollision_Manager::Add_Collider(COLLISION_GROUP eGroup, CCollider* pCollider)
{
    if (nullptr == pCollider || COLLISION_GROUP::END <= eGroup)
        return;

    m_Colliders[ETOUI(eGroup)].push_back(pCollider);
    Safe_AddRef(pCollider);
}

void CCollision_Manager::Set_CollisionMatrix(COLLISION_GROUP eA, COLLISION_GROUP eB, _bool bValue)
{
    if (COLLISION_GROUP::END <= eA || COLLISION_GROUP::END <= eB)
        return;

    m_CollisionMatrix[ETOUI(eA)][ETOUI(eB)] = bValue;
    m_CollisionMatrix[ETOUI(eB)][ETOUI(eA)] = bValue;
}

void CCollision_Manager::Update()
{
    Check_PairsAndFire();
}

void CCollision_Manager::Check_PairsAndFire()
{
    const _uint iNumGroups = ETOUI(COLLISION_GROUP::END);

    for (_uint i = 0; i < iNumGroups; ++i)
    {
        for (_uint j = i; j < iNumGroups; ++j)
        {
            if (!m_CollisionMatrix[i][j])
                continue;

            for (auto pA : m_Colliders[i])
            {
                for (auto pB : m_Colliders[j])
                {
                    if (pA == pB)
                        continue;
                    if (!pA->Intersect(pB))
                        continue;

                    // 양쪽 모두 빨강으로 표시
                    if (pB->Get_Bounding())
                        pB->Get_Bounding()->Set_Coll(true);

                    // 정렬된 키로 페어 중복 방지
                    PairKey Key = (pA < pB) ? make_pair(pA, pB) : make_pair(pB, pA);
                    if (!m_CurrPairs.insert(Key).second)
                        continue; // 이미 이 프레임에 처리한 페어

                    if (m_PrevPairs.find(Key) == m_PrevPairs.end())
                    {
                        pA->Notify_HitEnter(pB);
                        pB->Notify_HitEnter(pA);
                    }
                    else
                    {
                        pA->Notify_HitStay(pB);
                        pB->Notify_HitStay(pA);
                    }
                }
            }
        }
    }

    // 이전엔 있었는데 이번엔 없는 페어 → Exit
    for (auto& Prev : m_PrevPairs)
    {
        if (m_CurrPairs.find(Prev) == m_CurrPairs.end())
        {
            Prev.first->Notify_HitExit(Prev.second);
            Prev.second->Notify_HitExit(Prev.first);
        }
    }
}

HRESULT CCollision_Manager::Render()
{
#ifdef _DEBUG
    if (!m_bDebugDraw)
        return S_OK;

    if (nullptr == m_pBatch || nullptr == m_pEffect)
        return S_OK;

    // 매 프레임 View/Proj 동기화
    const _float4x4* pView = CGameInstance::GetInstance()->Get_Transform(D3DTS::VIEW);
    const _float4x4* pProj = CGameInstance::GetInstance()->Get_Transform(D3DTS::PROJ);
    m_pEffect->SetView(XMLoadFloat4x4(pView));
    m_pEffect->SetProjection(XMLoadFloat4x4(pProj));
    m_pEffect->SetWorld(XMMatrixIdentity());

    m_pContext->OMSetBlendState(m_pStates->Opaque(), nullptr, 0xFFFFFFFF);
    m_pContext->OMSetDepthStencilState(m_pStates->DepthNone(), 0);
    m_pContext->RSSetState(m_pStates->CullNone());

    m_pEffect->Apply(m_pContext);
    m_pContext->IASetInputLayout(m_pInputLayout);

    m_pBatch->Begin();
    for (auto& Group : m_Colliders)
    {
        for (auto& pCollider : Group)
            pCollider->Render(m_pBatch);
    }
    m_pBatch->End();
#endif
    return S_OK;
}

void CCollision_Manager::End_Frame()
{
    // 이전 프레임 페어 AddRef 해제
    for (auto& Prev : m_PrevPairs)
    {
        CCollider* pA = Prev.first;
        CCollider* pB = Prev.second;
        Safe_Release(pA);
        Safe_Release(pB);
    }
    m_PrevPairs.clear();

    // 이번 프레임 페어를 다음 프레임 Exit 검출용으로 보관 — AddRef
    for (auto& Curr : m_CurrPairs)
    {
        Safe_AddRef(Curr.first);
        Safe_AddRef(Curr.second);
    }
    m_PrevPairs.swap(m_CurrPairs);
    m_CurrPairs.clear();

    // Add_Collider AddRef 해제 + 리스트 비우기
    for (auto& Group : m_Colliders)
    {
        for (auto& pCollider : Group)
            Safe_Release(pCollider);
        Group.clear();
    }
}

CCollision_Manager* CCollision_Manager::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
    CCollision_Manager* pInstance = new CCollision_Manager(pDevice, pContext);

    if (FAILED(pInstance->Initialize()))
    {
        MSG_BOX("Failed to Created : CCollision_Manager");
        Safe_Release(pInstance);
    }

    return pInstance;
}

void CCollision_Manager::Free()
{
    __super::Free();

    for (auto& Prev : m_PrevPairs)
    {
        CCollider* pA = Prev.first;
        CCollider* pB = Prev.second;
        Safe_Release(pA);
        Safe_Release(pB);
    }
    m_PrevPairs.clear();

    for (auto& Group : m_Colliders)
    {
        for (auto& pCollider : Group)
            Safe_Release(pCollider);
        Group.clear();
    }

#ifdef _DEBUG

    if (m_pBatch) 
    {   
        delete m_pBatch;  
        m_pBatch = nullptr; 
    }

    if (m_pEffect) 
    { 
        delete m_pEffect; 
        m_pEffect = nullptr;
    }

    if (m_pStates) 
    { 
        delete m_pStates; 
        m_pStates = nullptr;
    }

    Safe_Release(m_pInputLayout);

#endif

    Safe_Release(m_pContext);
    Safe_Release(m_pDevice);
}