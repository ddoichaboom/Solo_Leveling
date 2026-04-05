#pragma once

#include "Editor_Defines.h"
#include "Level.h"

NS_BEGIN(Editor)

class CLevel_Editor final : public CLevel
{
private:
	CLevel_Editor(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual ~CLevel_Editor() = default;

public:
	static CLevel_Editor*	Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext);
	virtual void			Free() override;
};

NS_END