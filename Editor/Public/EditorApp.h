#pragma once

#include "Editor_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END

NS_BEGIN(Editor)

class CEditorApp final : public CBase
{
private:
	CEditorApp();
	virtual ~CEditorApp() = default;

public:
	HRESULT					Initialize(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY);
	void					Update(_float fTimeDelta);
	HRESULT					Render();

private:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	CGameInstance*			m_pGameInstance = { nullptr };

#pragma region IMGUI
private:
	HRESULT					Ready_ImGui(HWND hWnd);
	void					ShutDown_ImGui();

	// ImGui Render ÇÔ¼öµé 
	void					Begin_ImGuiFrame();
	void					Render_DockSpace();
	void					Render_ImGui();
#pragma endregion

public:
	static CEditorApp*		Create(HWND hWnd, _uint iWinSizeX, _uint iWinSizeY);
	virtual void			Free() override;
};

NS_END