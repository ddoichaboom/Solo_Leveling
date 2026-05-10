#pragma once

#include "VIBuffer.h"
#include "NavMesh_Types.h"

NS_BEGIN(Engine)

class ENGINE_DLL CVIBuffer_NavMesh final : public CVIBuffer
{
private:
	CVIBuffer_NavMesh(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CVIBuffer_NavMesh(const CVIBuffer_NavMesh& Prototype);
	virtual ~CVIBuffer_NavMesh() = default;

public:
	virtual HRESULT					Initialize_Prototype() override;
	virtual HRESULT					Initialize(void* pArg) override;

public:
	HRESULT							Build(const vector<_float3>& Vertices, const vector<NAVMESH_CELL>& Cells, _float fYOffset = 0.05f);
	virtual HRESULT					Render() override;

private:
	void							Release_Buffers();


public:
	static CVIBuffer_NavMesh*		Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CComponent*				Clone(void* pArg) override;
	virtual void					Free() override;
};

NS_END