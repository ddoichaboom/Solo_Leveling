#pragma once

#include "Engine_Defines.h"

NS_BEGIN(Engine)

class INotifyListener
{
public:
	virtual ~INotifyListener() = default;

	virtual void OnNotify(const NOTIFY_EVENT& Event) PURE;
};

NS_END