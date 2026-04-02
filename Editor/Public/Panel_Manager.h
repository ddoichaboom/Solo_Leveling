#pragma once

#include "Editor_Defines.h"
#include "Base.h"

#include "Panel.h"
#include "Panel_ContentBrowser.h"
#include "Panel_Hierarchy.h"
#include "Panel_Inspector.h"
#include "Panel_Log.h"
#include "Panel_Viewport.h"

NS_BEGIN(Editor)

class CPanel;

class CPanel_Manager final : public CBase
{
	DECLARE_SINGLETON(CPanel_Manager)
private:
	CPanel_Manager();
	virtual ~CPanel_Manager() = default;

public:
	HRESULT		Add_Panel(const _wstring& strPanelTag, CPanel* pPanel);
	CPanel*		Get_Panel(const _wstring& strPanelTag);
	void		Update_Panels(_float fTimeDelta);
	void		Render_Panels();

private:
	map<_wstring, CPanel*> m_Panels;

public:
	virtual void Free() override;
};

NS_END