#pragma once

/* 클라이언트 개발자가 앞으로 제작할 모든 레벨 클래스들의 부모가 되는 클래스 */

#include "Base.h"

NS_BEGIN(Engine)

class ENGINE_DLL CLevel abstract : public CBase
{
protected:
	CLevel(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLevel() = default;

public:
	virtual HRESULT				Initialize();
	virtual void				Update(_float fTimeDelta);
	virtual HRESULT				Render();

protected:
	ID3D11Device*				m_pDevice = { nullptr };
	ID3D11DeviceContext*		m_pContext = { nullptr };
	class CGameInstance*		m_pGameInstance = { nullptr };

public:
	virtual void Free() override;
};

NS_END