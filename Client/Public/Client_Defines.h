#pragma once
#include <Windows.h>
#include <process.h>

#include "Engine_Defines.h"
#include "Client_Enum.h"
#include "Client_Struct.h"


namespace Client
{
    enum class LEVEL { STATIC, LOADING, LOGO, GAMEPLAY, END };
}

using namespace Client;