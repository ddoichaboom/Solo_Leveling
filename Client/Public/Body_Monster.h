#pragma once

#include "Client_Defines.h"
#include "PartObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CBody_Monster final : public CPartObject
{
public:
	typedef struct tagBodyMonsterDesc : public CPartObject::PARTOBJECT_DESC
	{
		const _tchar*		pModelPrototypeTag = { nullptr };
		MONSTER_ANIM_SET	eAnimSet = { MONSTER_ANIM_SET::NONE };
	}BODY_MONSTER_DESC;

private:
	CBody_Monster(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CBody_Monster(const CBody_Monster& Prototype);
	virtual ~CBody_Monster() = default;

public:
	virtual HRESULT			Initialize_Prototype() override;
	virtual HRESULT			Initialize(void* pArg) override;
	virtual void			Priority_Update(_float fTimeDelta) override;
	virtual void			Update(_float fTimeDelta) override;
	virtual void			Late_Update(_float fTimeDelta) override;
	virtual HRESULT			Render() override;

private:
	HRESULT					Ready_Components(const BODY_MONSTER_DESC& Desc);
	HRESULT					Bind_ShaderResources();

private:
	CShader*				m_pShaderCom = { nullptr };
	CModel*					m_pModelCom = { nullptr };

	_wstring				m_strModelPrototypeTag;
	MONSTER_ANIM_SET		m_eAnimSet = { MONSTER_ANIM_SET::NONE };

public:
	static CBody_Monster*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*		Clone(void* pArg) override;
	virtual void			Free() override;
};

NS_END

