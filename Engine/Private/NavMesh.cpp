#include "NavMesh.h"
#include "Cell.h"

CNavMesh::CNavMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CComponent{ pDevice, pContext }
{
}

CNavMesh::CNavMesh(const CNavMesh& Prototype)
	: CComponent{ Prototype }
	, m_Vertices{ Prototype.m_Vertices }
	, m_CellDescs{ Prototype.m_CellDescs }
{
}

HRESULT CNavMesh::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CNavMesh::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	if (nullptr != pArg)
	{
		NAVMESH_DESC* pDesc = static_cast<NAVMESH_DESC*>(pArg);

		if (nullptr != pDesc->pInitialSnapshot)
			return Restore_Snapshot(*pDesc->pInitialSnapshot);
	}

	if (FAILED(Rebuild_CellObjects()))
		return E_FAIL;

	return S_OK;
}

const CCell* CNavMesh::Get_Cell(_int iCellIndex) const
{
	if (iCellIndex < 0)
		return nullptr;

	if (static_cast<size_t>(iCellIndex) >= m_Cells.size())
		return nullptr;

	return m_Cells[iCellIndex];
}

_int CNavMesh::Add_Vertex(const _float3& vPosition)
{
	m_Vertices.push_back(vPosition);

	return static_cast<_int>(m_Vertices.size() - 1);
}

_int CNavMesh::Find_OrAddVertex(const _float3& vPosition, _float fSnapRadius)
{
	const _float fSnapRadiusSq = fSnapRadius * fSnapRadius;

	for (_uint i = 0; i < static_cast<_uint>(m_Vertices.size()); ++i)
	{
		const _float3& vVertex = m_Vertices[i];

		const _float fDistanceSq =
			(vVertex.x - vPosition.x) * (vVertex.x - vPosition.x) +
			(vVertex.y - vPosition.y) * (vVertex.y - vPosition.y) +
			(vVertex.z - vPosition.z) * (vVertex.z - vPosition.z);

		if (fDistanceSq <= fSnapRadiusSq)
			return static_cast<_int>(i);
	}

	return Add_Vertex(vPosition);
}

HRESULT CNavMesh::Add_Cell(const NAVMESH_CELL& Cell, _int* pOutCellIndex)
{
	if (nullptr != pOutCellIndex)
		*pOutCellIndex = NAVMESH_INVALID_INDEX;

	const _int iCellIndex = static_cast<_int>(m_CellDescs.size());

	CCell* pValidationCell = CCell::Create(iCellIndex, &m_Vertices, Cell);
	if (nullptr == pValidationCell)
		return E_FAIL;

	Safe_Release(pValidationCell);

	m_CellDescs.push_back(Cell);

	if (FAILED(Rebuild_Neighbors()))
	{
		m_CellDescs.pop_back();
		Rebuild_Neighbors();
		return E_FAIL;
	}

	if (nullptr != pOutCellIndex)
		*pOutCellIndex = iCellIndex;

	return S_OK;
}

HRESULT CNavMesh::Try_AddCell(_int iVertex0, _int iVertex1, _int iVertex2, _int* pOutCellIndex)
{
	if (nullptr != pOutCellIndex)
		*pOutCellIndex = NAVMESH_INVALID_INDEX;

	if (false == Is_ValidVertexIndex(iVertex0) ||
		false == Is_ValidVertexIndex(iVertex1) ||
		false == Is_ValidVertexIndex(iVertex2))
		return E_FAIL;

	if (iVertex0 == iVertex1 ||
		iVertex1 == iVertex2 ||
		iVertex2 == iVertex0)
		return E_FAIL;

	_float fSignedArea = Compute_CellSignedArea_XZ(iVertex0, iVertex1, iVertex2);
	if (fabsf(fSignedArea) <= NAVMESH_MIN_CELL_AREA)
		return E_FAIL;

	NAVMESH_CELL Cell{};
	Cell.iVertexIndices[0] = iVertex0;
	Cell.iVertexIndices[1] = iVertex1;
	Cell.iVertexIndices[2] = iVertex2;

	// 위에서 봤을 때 CCW 기준으로 통일한다.
	if (fSignedArea < 0.f)
	{
		std::swap(Cell.iVertexIndices[1], Cell.iVertexIndices[2]);
		fSignedArea = -fSignedArea;
	}

	return Add_Cell(Cell, pOutCellIndex);
}

void CNavMesh::Clear()
{
	Release_CellObjects();

	m_CellDescs.clear();
	m_Vertices.clear();
}

NAVMESH_SNAPSHOT CNavMesh::Capture_Snapshot() const
{
	NAVMESH_SNAPSHOT Snapshot{};
	Snapshot.Vertices = m_Vertices;
	Snapshot.Cells = m_CellDescs;

	return Snapshot;
}

HRESULT CNavMesh::Restore_Snapshot(const NAVMESH_SNAPSHOT& Snapshot)
{
	Release_CellObjects();

	m_Vertices = Snapshot.Vertices;
	m_CellDescs = Snapshot.Cells;

	if (FAILED(Rebuild_Neighbors()))
		return E_FAIL;

	return S_OK;
}

_int CNavMesh::Find_Cell(const _float3& vPosition) const
{
	for (_uint i = 0; i < static_cast<_uint>(m_Cells.size()); ++i)
	{
		if (nullptr == m_Cells[i])
			continue;

		if (true == m_Cells[i]->Is_In(vPosition))
			return static_cast<_int>(i);
	}

	return NAVMESH_INVALID_INDEX;
}

_bool CNavMesh::Try_Move(_int* pCurrentCellIndex, const _float3& vCandidatePosition, _float3* pOutAdjustedPosition) const
{
	if (nullptr == pCurrentCellIndex)
		return false;

	if (nullptr != pOutAdjustedPosition)
		*pOutAdjustedPosition = vCandidatePosition;

	_int iCurrentCellIndex = *pCurrentCellIndex;

	if (false == Is_ValidCellIndex(iCurrentCellIndex))
	{
		iCurrentCellIndex = Find_Cell(vCandidatePosition);

		if (false == Is_ValidCellIndex(iCurrentCellIndex))
			return false;

		*pCurrentCellIndex = iCurrentCellIndex;
	}

	_int iNeighborIndex = NAVMESH_INVALID_INDEX;

	if (true == m_Cells[iCurrentCellIndex]->Is_In(vCandidatePosition, &iNeighborIndex))
	{
		if (nullptr != pOutAdjustedPosition)
		{
			*pOutAdjustedPosition = vCandidatePosition;
			pOutAdjustedPosition->y = m_Cells[iCurrentCellIndex]->Compute_Height(vCandidatePosition);
		}

		return true;
	}

	while (true == Is_ValidCellIndex(iNeighborIndex))
	{
		const _int iNextCellIndex = iNeighborIndex;
		iNeighborIndex = NAVMESH_INVALID_INDEX;

		if (true == m_Cells[iNextCellIndex]->Is_In(vCandidatePosition, &iNeighborIndex))
		{
			*pCurrentCellIndex = iNextCellIndex;

			if (nullptr != pOutAdjustedPosition)
			{
				*pOutAdjustedPosition = vCandidatePosition;
				pOutAdjustedPosition->y = m_Cells[iNextCellIndex]->Compute_Height(vCandidatePosition);
			}

			return true;
		}
	}

	return false;
}

_float CNavMesh::Compute_Height(_int iCellIndex, const _float3& vPosition) const
{
	if (false == Is_ValidCellIndex(iCellIndex))
		return vPosition.y;

	return m_Cells[iCellIndex]->Compute_Height(vPosition);
}

HRESULT CNavMesh::Rebuild_Neighbors()
{
	for (auto& CellDesc : m_CellDescs)
	{
		for (_uint iLine = 0; iLine < ETOUI(NAVMESH_LINE::END); ++iLine)
			CellDesc.iNeighborIndices[iLine] = NAVMESH_INVALID_INDEX;
	}

	if (FAILED(Rebuild_CellObjects()))
		return E_FAIL;

	for (_uint i = 0; i < static_cast<_uint>(m_Cells.size()); ++i)
	{
		for (_uint j = i + 1; j < static_cast<_uint>(m_Cells.size()); ++j)
		{
			_uint iLine = ETOUI(NAVMESH_LINE::END);
			_uint iOtherLine = ETOUI(NAVMESH_LINE::END);

			if (false == m_Cells[i]->Shares_Edge(m_Cells[j], &iLine, &iOtherLine))
				continue;

			if (iLine >= ETOUI(NAVMESH_LINE::END) ||
				iOtherLine >= ETOUI(NAVMESH_LINE::END))
				continue;

			m_CellDescs[i].iNeighborIndices[iLine] = static_cast<_int>(j);
			m_CellDescs[j].iNeighborIndices[iOtherLine] = static_cast<_int>(i);
		}
	}

	if (FAILED(Rebuild_CellObjects()))
		return E_FAIL;

	return S_OK;
}

HRESULT CNavMesh::Rebuild_CellObjects()
{
	Release_CellObjects();

	m_Cells.reserve(m_CellDescs.size());

	for (_uint i = 0; i < static_cast<_uint>(m_CellDescs.size()); ++i)
	{
		CCell* pCell = CCell::Create(static_cast<_int>(i), &m_Vertices, m_CellDescs[i]);
		if (nullptr == pCell)
		{
			Release_CellObjects();
			return E_FAIL;
		}

		m_Cells.push_back(pCell);
	}

	return S_OK;
}

_bool CNavMesh::Is_ValidVertexIndex(_int iVertexIndex) const
{
	if (iVertexIndex < 0)
		return false;

	return static_cast<size_t>(iVertexIndex) < m_Vertices.size();
}

_bool CNavMesh::Is_ValidCellIndex(_int iCellIndex) const
{
	if (iCellIndex < 0)
		return false;

	return static_cast<size_t>(iCellIndex) < m_Cells.size() &&
		nullptr != m_Cells[iCellIndex];
}

_float CNavMesh::Compute_CellSignedArea_XZ(_int iVertex0, _int iVertex1, _int iVertex2) const
{
	if (false == Is_ValidVertexIndex(iVertex0) ||
		false == Is_ValidVertexIndex(iVertex1) ||
		false == Is_ValidVertexIndex(iVertex2))
		return 0.f;

	const _float3& vA = m_Vertices[iVertex0];
	const _float3& vB = m_Vertices[iVertex1];
	const _float3& vC = m_Vertices[iVertex2];

	_vector vAB = XMVectorSubtract(XMLoadFloat3(&vB), XMLoadFloat3(&vA));
	_vector vAC = XMVectorSubtract(XMLoadFloat3(&vC), XMLoadFloat3(&vA));
	_vector vCross = XMVector3Cross(vAB, vAC);

	return XMVectorGetY(vCross) * 0.5f;
}

void CNavMesh::Release_CellObjects()
{
	for (auto& pCell : m_Cells)
		Safe_Release(pCell);

	m_Cells.clear();
}

CNavMesh* CNavMesh::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CNavMesh* pInstance = new CNavMesh(pDevice, pContext);

	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CNavMesh");
		Safe_Release(pInstance);
	}

	return pInstance;
}

CComponent* CNavMesh::Clone(void* pArg)
{
	CNavMesh* pInstance = new CNavMesh(*this);

	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CNavMesh");
		Safe_Release(pInstance);
	}

	return pInstance;
}

void CNavMesh::Free()
{
	__super::Free();

	Release_CellObjects();

	m_CellDescs.clear();
	m_Vertices.clear();
}
