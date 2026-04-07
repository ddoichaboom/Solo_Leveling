#pragma once

#include "Base.h"

NS_BEGIN(Engine)

class CMaterial final : public CBase
{
private:
	CMaterial(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CMaterial() = default;

public:
	HRESULT								Initialize();
	HRESULT								Add_Texture(const MATERIAL_TEXTURE_DESC& Desc);
	HRESULT								Bind_ShaderResource(class CShader* pShader, const _char* pConstantName, TEXTURE_TYPE eType, _uint iIndex = 0);

private:
	ID3D11Device*						m_pDevice = { nullptr };
	ID3D11DeviceContext*				m_pContext = { nullptr };
	vector<ID3D11ShaderResourceView*>	m_Textures[ETOUI(TEXTURE_TYPE::END)];
public:
	static CMaterial*					Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void						Free() override;
};

NS_END