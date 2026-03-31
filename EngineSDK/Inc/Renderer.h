#pragma once

#include "Base.h"

// 1. 화면에 그려져야할 객체들을 그리는 순서대로 모아놓는다. (그룹으로 구분)
// 2. 보관된 순서대로 객체들의 드로우콜을 해준다.

NS_BEGIN(Engine)

class CRenderer final : public CBase
{
private:
	CRenderer(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CRenderer() = default;

public:
	HRESULT Initialize();
	void Add_RenderGroup(RENDERID eGroupID, class CGameObject* pGameObject);
	HRESULT Draw();

private:
	ID3D11Device*						m_pDevice = { nullptr };
	ID3D11DeviceContext*				m_pContext = { nullptr };

private:
	list<class CGameObject*> m_RenderObjects[ETOUI(RENDERID::END)];				// Render 그룹의 크기는 정해져있으므로 할당

private:
	HRESULT Render_Priority();
	HRESULT Render_NonBlend();
	HRESULT Render_Blend();
	HRESULT Render_UI();

public:
	static CRenderer* Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void Free() override;
};

NS_END