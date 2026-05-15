#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CCollider;

class ENGINE_DLL CCollision_Manager final : public CBase
{
private:
    CCollision_Manager(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CCollision_Manager() = default;

public:
    HRESULT                                 Initialize();

    // Renderer 패턴: 매 프레임 Collider 가 Late_Update 에서 등록 → AddRef
    void                                    Add_Collider(COLLISION_GROUP eGroup, CCollider* pCollider);

    // 페어 검사 + Enter/Stay/Exit 콜백 발화 + 리스트 비우기
    void                                    Update();

    // 디버그 드로우 (D3DTS 가 BackBuffer 인 상태에서 호출)
    HRESULT                                 Render();
    void                                    End_Frame();

    void                                    Set_CollisionMatrix(COLLISION_GROUP eA, COLLISION_GROUP eB, _bool bValue);
    void                                    Set_DebugDraw(_bool bValue) { m_bDebugDraw = bValue; }
    _bool                                   Is_DebugDraw() const { return m_bDebugDraw; }

private:
    void                                    Check_PairsAndFire();

private:
    ID3D11Device*                           m_pDevice = { nullptr };
    ID3D11DeviceContext*                    m_pContext = { nullptr };

    list<CCollider*>                        m_Colliders[ETOUI(COLLISION_GROUP::END)];
    _bool                                   m_CollisionMatrix[ETOUI(COLLISION_GROUP::END)][ETOUI(COLLISION_GROUP::END)] = {};

    using PairKey = pair<CCollider*, CCollider*>;
    set<PairKey>                            m_PrevPairs;   // 모두 AddRef 상태로 보유 (다음 프레임 Exit 까지 생존 보장)
    set<PairKey>                            m_CurrPairs;

#ifdef _DEBUG
    PrimitiveBatch<VertexPositionColor>*    m_pBatch = { nullptr };
    BasicEffect*                            m_pEffect = { nullptr };
    ID3D11InputLayout*                      m_pInputLayout = { nullptr };
    CommonStates*                           m_pStates = { nullptr };
    _bool                                   m_bDebugDraw = { true };
#endif

public:
    static CCollision_Manager*              Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                            Free() override;
};

NS_END
