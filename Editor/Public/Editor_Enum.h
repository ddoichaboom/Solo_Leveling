#ifndef Editor_Enum_h__
#define Editor_Enum_h__

namespace Editor
{
	enum class MENUTYPE  { PANEL, TOOL, END };
	enum class LOG_LEVEL { INFO, WARNING, ERROR_, END };	// ERROR는 Windows 매크로와 충돌
}
#endif // Editor_Enum.h
