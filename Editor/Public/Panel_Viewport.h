#pragma once

#include "Editor_Defines.h"
#include "Panel.h"
#include "NavMesh_Types.h"

NS_BEGIN(Engine)
class CGameObject;
class CNavMesh;
NS_END

NS_BEGIN(Editor)

class CPanel_Viewport final : public CPanel
{
private:
    CPanel_Viewport(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual ~CPanel_Viewport() = default;

public:
    virtual HRESULT             Initialize() override;
    virtual void                Update(_float fTimeDelta) override;
    virtual void                Render() override;

#pragma region RENDER_TARGET
public:
    HRESULT                     Begin_RT(); 
    HRESULT                     End_RT();

    // RT 접근자
    ID3D11ShaderResourceView*   Get_SRV() const { return m_pSRV; }
    _uint                       Get_RTWidth() const { return m_iRTWidth; }
	_uint                       Get_RTHeight() const { return m_iRTHeight; }

private:
    HRESULT                     Create_RenderTarget(_uint iWidth, _uint iHeight);
	void                        Release_RenderTarget();

private:
    // Render Target 리소스
	ID3D11Texture2D*            m_pRTTexture = { nullptr };
	ID3D11RenderTargetView*     m_pRTV = { nullptr };
	ID3D11ShaderResourceView*   m_pSRV = { nullptr };

	ID3D11Texture2D*            m_pDSTexture = { nullptr };
    ID3D11DepthStencilView*     m_pDSV = { nullptr };

    D3D11_VIEWPORT              m_Viewport = {};

    _uint                       m_iRTWidth = { 0 };
	_uint                       m_iRTHeight = { 0 };

#pragma endregion

#pragma region PICKING
private:
    typedef struct tagPickResult
    {
        CGameObject*      pObject = { nullptr };
        _float3			        vPosition = {};
        _float			        fDistance = {};
    }PICK_RESULT;

    void                        Pick_Object();

    _float                      m_fPickX = {};
    _float                      m_fPickY = {};
#pragma endregion

#pragma region NAVMESH
private:
    typedef struct tagNavMeshPickPoint
    {
        _float3                 vRawPosition = {};
        _float3                 vPreviewPosition = {};
        _int                    iSnapVertexIndex = { -1 };
        _bool                   bSnapped = { false };
    }NAVMESH_PICK_POINT;

    _bool                       Render_NavMeshEditToolbar(const ImVec2& vImagePos);
    void                        Render_NavMesh_PickPreview(const ImVec2& vImagePos);
    void                        Render_SelectedNavMeshCell(const ImVec2& vImagePos);
    void                        Render_SelectedNavMeshVertex(const ImVec2& vImagePos);

    void                        Render_SpawnPoints(const ImVec2& vImagePos);
    _bool                       Build_SpawnPointFromSelectedCell(SPAWN_TYPE eType, const _tchar* pName, SPAWN_POINT* pOutPoint);
    void                        Push_OrReplacePlayerSpawnPoint(const SPAWN_POINT& Point);

    HRESULT                     Set_PlayerSpawnPoint();
    HRESULT                     Add_MonsterSpawnPoint(SPAWN_TYPE eType);
    HRESULT                     Save_SceneData();
    HRESULT                     Load_SceneData();

    void                        Select_NavMeshVertex();
    HRESULT                     Move_SelectedNavMeshVertex();


    void                        Select_NavMeshCell();
    HRESULT                     Delete_SelectedNavMeshCell();
    HRESULT                     Undo_NavMeshEdit();
    HRESULT                     Redo_NavMeshEdit();

    HRESULT                     Save_NavMeshData();
    HRESULT                     Load_NavMeshData();

    void                        Push_NavMeshUndoSnapshot(const NAVMESH_SNAPSHOT& Snapshot);
    void                        Clear_NavMeshEditState();

    _bool                       World_To_Viewport(const _float3& vWorldPosition, const ImVec2& vImagePos, ImVec2* pOutScreenPosition) const;
    CNavMesh*                   Find_NavMesh() const;

    _bool                       Pick_Surface(PICK_RESULT* pOutResult, _bool bMapOnly = false );
    void                        Pick_NavMeshEditPoint();

    HRESULT                     Try_Create_NavMeshCell();
    void                        Clear_NavMeshPickPoints();

    void                        Log_EditStatus(LOG_LEVEL eLevel, const string& strMessage) const;

    _bool                       m_bHasLastNavMeshPick = { false };
    _float3                     m_vLastNavMeshPick = {};
    vector<NAVMESH_PICK_POINT>  m_NavMeshPickedPoints;

    _bool                       m_bNavMeshToggleKeyHeld = { false };
    _int                        m_iSelectedNavMeshCellIndex = { NAVMESH_INVALID_INDEX };
    _int                        m_iSelectedNavMeshVertexIndex = { NAVMESH_INVALID_INDEX };
    vector<NAVMESH_SNAPSHOT>    m_NavMeshUndoStack;
    vector<NAVMESH_SNAPSHOT>    m_NavMeshRedoStack;
    vector<Client::SPAWN_POINT>         m_SpawnPoints;

#pragma endregion

#pragma region GIZMO

private:
    ImGuizmo::OPERATION         m_eGizmoOperation   = ImGuizmo::TRANSLATE;
    ImGuizmo::MODE              m_eGizmoMode        = ImGuizmo::LOCAL;

    _float                      m_fSnapTranslate    = { 1.0f };
    _float                      m_fSnapRotate       = { 15.0f };
    _float                      m_fSnapScale        = { 0.1f };
#pragma endregion



public:
    static CPanel_Viewport*     Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
    virtual void                Free() override;

};

NS_END