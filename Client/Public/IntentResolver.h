#pragma once

#include "Client_Defines.h"
#include "Base.h"

NS_BEGIN(Client)

class CIntentResolver final : public CBase
{
public:
	CIntentResolver() = default;
	virtual ~CIntentResolver() = default;

public:
	void Resolve(const PLAYER_RAW_INPUT_FRAME& Raw, PLAYER_INTENT_FRAME* pOutIntent);

private:
	_float Compute_Axis(_bool bNegativeHeld, _bool bPositiveHeld) const;

public:
	static CIntentResolver* Create();
	virtual void Free() override;
};

NS_END