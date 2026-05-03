#ifndef Editor_Struct_h__
#define Editor_Struct_h__

namespace Editor
{
	typedef struct tagLogDesc
	{
		LOG_LEVEL		eLevel		= { LOG_LEVEL::INFO };
		std::string		strMessage	= {};
	}LOG_DESC;

}

#endif // Editor_Struct_h__
