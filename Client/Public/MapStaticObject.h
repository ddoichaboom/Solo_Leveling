#pragma once

#include "Client_Defines.h"
#include "PartObject.h"

NS_BEGIN(Engine)
class CShader;
class CModel;
NS_END

NS_BEGIN(Client)

class CLIENT_DLL CMapStaticObject final : public CPartObject
{
public:
	typedef struct tagMapStaticObjectDesc : public CPartObject::PARTOBJECT_DESC
	{
		const _tchar* pShaderProtoTag = { nullptr };
		const _tchar* pModelProtoTag = { nullptr };
	}MAPSTATICOBJECT_DESC;

private:
	CMapStaticObject(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	CMapStaticObject(const CMapStaticObject& Prototype);
	virtual ~CMapStaticObject() = default;

public:
	virtual HRESULT				Initialize_Prototype() override;
	virtual HRESULT				Initialize(void* pArg) override;
	virtual void				Priority_Update(_float fTimeDelta) override;
	virtual void				Update(_float fTimeDelta) override;
	virtual void				Late_Update(_float fTimeDelta) override;
	virtual HRESULT				Render() override;

private:
	CShader*					m_pShaderCom	= { nullptr };
	CModel*						m_pModelCom		= { nullptr };

	_wstring					m_strShaderProtoTag;
	_wstring					m_strModelProtoTag;

private:
	HRESULT						Ready_Components();
	HRESULT						Bind_ShaderResources();

public:
	static CMapStaticObject*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual CGameObject*		Clone(void* pArg) override;
	virtual void				Free() override;
};

NS_END