#pragma once

#include "Editor_Defines.h"
#include "Base.h"

NS_BEGIN(Engine)
class CGameInstance;
NS_END


NS_BEGIN(Editor)

class CPanel abstract : public CBase
{
protected:
	CPanel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CPanel() = default;

public:
	virtual HRESULT			Initialize() PURE;
	virtual void			Update(_float fTimeDelta) PURE;
	virtual void			Render() PURE;

	_bool					Is_Open() const { return m_bOpen; }
	void					Set_Open(_bool bOpen) { m_bOpen = bOpen; }
	const _char*			Get_Name() const { return m_szName; }

protected:
	ID3D11Device*			m_pDevice = { nullptr };
	ID3D11DeviceContext*	m_pContext = { nullptr };
	CGameInstance*	m_pGameInstance = { nullptr };

	_char					m_szName[MAX_PATH] = {};	// ImGui 윈도우 이름
	_bool					m_bOpen = { true };			// 패널 표시 여부

public:
	
	virtual void			Free() override;
};

NS_END