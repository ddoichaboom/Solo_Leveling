#pragma once

#include "Component.h"
#include "NavMesh_Types.h"

NS_BEGIN(Engine)

class CCell;

class ENGINE_DLL CNavMesh final : public CComponent
{
public:
	typedef struct tagNavMeshDesc
	{
		const NAVMESH_SNAPSHOT* pInitialSnapshot = { nullptr };
	}NAVMESH_DESC;

private:
	CNavMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CNavMesh(const CNavMesh& Prototype);
	virtual ~CNavMesh() = default;

public:
	virtual HRESULT Initialize_Prototype() override;
	virtual HRESULT Initialize(void* pArg) override;

public:
	const vector<_float3>&		Get_Vertices() const { return m_Vertices; }
	const vector<NAVMESH_CELL>& Get_CellDescs() const { return m_CellDescs; }

	_uint						Get_NumVertices() const { return static_cast<_uint>(m_Vertices.size()); }
	_uint						Get_NumCells() const { return static_cast<_uint>(m_CellDescs.size()); }

	const CCell*				Get_Cell(_int iCellIndex) const;

public:
	_int						Add_Vertex(const _float3& vPosition);
	_int						Find_OrAddVertex(const _float3& vPosition, _float fSnapRadius = NAVMESH_DEFAULT_SNAP_RADIUS);

	HRESULT						Add_Cell(const NAVMESH_CELL& Cell, _int* pOutCellIndex = nullptr);
	HRESULT						Try_AddCell(_int iVertex0, _int iVertex1, _int iVertex2, _int* pOutCellIndex = nullptr);

	void						Clear();
	NAVMESH_SNAPSHOT			Capture_Snapshot() const;
	HRESULT						Restore_Snapshot(const NAVMESH_SNAPSHOT& Snapshot);

	_int						Find_Cell(const _float3& vPosition) const;
	_bool						Try_Move(_int* pCurrentCellIndex, const _float3& vCandidatePosition, _float3* pOutAdjustedPosition) const;
	_float						Compute_Height(_int iCellIndex, const _float3& vPosition) const;

	HRESULT						Rebuild_Neighbors();
	HRESULT						Rebuild_CellObjects();

private:
	_bool						Is_ValidVertexIndex(_int iVertexIndex) const;
	_bool						Is_ValidCellIndex(_int iCellIndex) const;
	_float						Compute_CellSignedArea_XZ(_int iVertex0, _int iVertex1, _int iVertex2) const;
	void						Release_CellObjects();

private:
	vector<_float3>				m_Vertices;
	vector<NAVMESH_CELL>		m_CellDescs;
	vector<CCell*>				m_Cells;

public:
	static CNavMesh*			Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*			Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END