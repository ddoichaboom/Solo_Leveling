#pragma once

#include "Editor_Defines.h"
#include "Panel.h"

NS_BEGIN(Editor)

class CNavMeshEditorTool;

class CPanel_NavMeshEditor final : public CPanel
{
private:
	CPanel_NavMeshEditor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CPanel_NavMeshEditor() = default;

public:
	virtual HRESULT				Initialize() override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Render() override;

	CNavMeshEditorTool*			Get_Tool() const { return m_pTool; }

private:
	CNavMeshEditorTool*			m_pTool = { nullptr };

public:
	static CPanel_NavMeshEditor* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void				Free() override;
};

NS_END
