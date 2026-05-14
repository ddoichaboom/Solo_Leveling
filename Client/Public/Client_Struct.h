#ifndef Client_Struct_h
#define Client_Struct_h

#include "NavMesh_Types.h"

namespace Client
{
	typedef struct tagCharacterAnimBindDesc
	{
		CHARACTER_STATE				eState			= { CHARACTER_STATE::END };
		CHARACTER_ACTION			eAction			= { CHARACTER_ACTION::END };
		WEAPON_TYPE					eWeapon			= { WEAPON_TYPE::END };
		const _char*				pAnimationName	= { nullptr };
		_bool						bRestartOnEnter = { true };
	}CHARACTER_ANIM_BIND_DESC;

	typedef struct tagCharacterActionPolicy
	{
		CHARACTER_ACTION			eAction = { CHARACTER_ACTION::END };
		_uint						iPriority = {};
		_bool						bAutoReturn = { false };
		CHARACTER_ACTION			eReturnAction = { CHARACTER_ACTION::IDLE };
		_float						fEnterBlendTime = { 0.f };
	} CHARACTER_ACTION_POLICY;

	typedef struct tagCharacterAnimTableDesc
	{
		CHARACTER_TYPE						eCharacterType		= { CHARACTER_TYPE::END };
		const _tchar*						pModelPrototypeTag	= { nullptr };
		const _char*						pRootBoneName		= { nullptr };

		const CHARACTER_ANIM_BIND_DESC*		pBinds				= { nullptr };
		_uint								iNumBinds			= {};

		const CHARACTER_ACTION_POLICY*		pPolicies			= { nullptr };
		_uint								iNumPolicies		= {};
	} CHARACTER_ANIM_TABLE_DESC;

	typedef struct tagMonsterAnimBindDesc
	{
		MONSTER_ACTION			eAction = { MONSTER_ACTION::END };
		MONSTER_PHASE			ePhase = { MONSTER_PHASE::COMMON };
		const _char* pAnimationName = { nullptr };
		_bool					bRestartOnEnter = { true };
	} MONSTER_ANIM_BIND_DESC;

	typedef struct tagMonsterActionPolicy
	{
		MONSTER_ACTION			eAction = { MONSTER_ACTION::END };
		_uint					iPriority = {};
		_bool					bAutoReturn = { false };
		MONSTER_ACTION			eReturnAction = { MONSTER_ACTION::IDLE };
	} MONSTER_ACTION_POLICY;

	typedef struct tagMonsterAnimTableDesc
	{
		MONSTER_ANIM_SET				eAnimSet = { MONSTER_ANIM_SET::END };
		const _tchar*					pModelPrototypeTag = { nullptr };
		const _char*					pRootBoneName = { nullptr };

		const MONSTER_ANIM_BIND_DESC*	pClips = { nullptr };
		_uint							iNumClips = {};

		const MONSTER_ACTION_POLICY*	pPolicies = { nullptr };
		_uint							iNumPolicies = {};
	} MONSTER_ANIM_TABLE_DESC;

	typedef struct tagPlayerRawInputFrame
	{
		_bool bMoveForwardHeld = { false };
		_bool bMoveBackwardHeld = { false };
		_bool bMoveLeftHeld = { false };
		_bool bMoveRightHeld = { false };

		_bool bRButtonHeld = { false };
		_bool bLButtonPressed = { false };
		_bool bDashPressed = { false };


		_long lMouseDeltaX = {};
		_long lMouseDeltaY = {};

	}PLAYER_RAW_INPUT_FRAME;

	typedef struct tagPlayerIntentFrame
	{
		_float3 vMoveDirWorld = {};
		_long	lLookDeltaX = {};

		_bool bDashRequested = { false };		// 이번 프레임에 Dash 요청
		_bool bAttackRequested = { false };

		_bool bGuardHeld = { false };

	}PLAYER_INTENT_FRAME;

	typedef struct tagSpawnPoint
	{
		SPAWN_TYPE		eType = { SPAWN_TYPE::END };
		_float3			vPosition = {};
		_float3			vRotationDeg = {};
		_int			iNavCellIndex = { NAVMESH_INVALID_INDEX };
		_tchar			szName[MAX_PATH] = {};
	}SPAWN_POINT;

	typedef struct tagSceneData
	{
		_tchar					szNavDataPath[MAX_PATH] = { };
		vector<SPAWN_POINT>		SpawnPoints;
	}SCENE_DATA;

	typedef struct tagUIElemet
	{
		UI_ELEMENT_TYPE		eType = { UI_ELEMENT_TYPE::IMAGE };
		_tchar              szName[MAX_PATH] = {};
		_tchar              szTexturePath[MAX_PATH] = {};

		_tchar				szTextureProtoTag[MAX_PATH] = {};
		_uint				iTextureProtoLevel = { 0 };

		_float              fCenterX = { 0.f };
		_float              fCenterY = { 0.f };
		_float              fSizeX = { 100.f };
		_float              fSizeY = { 100.f };
		_uint               iZOrder = { 0 };

		_tchar				szFontTag[MAX_PATH] = {};
		_tchar				szText[MAX_PATH] = {};
		_uint				iAtlasCols = { 1 };
		_uint				iAtlasRows = { 1 };
		_float				fFrameDuration = { 0.1f };

		_bool				bLoop = { true };
		_float				fPlaybackSpeed = { 1.f };

		UI_TEXT_HALIGN		eHAlign = UI_TEXT_HALIGN::CENTER;
		UI_TEXT_VALIGN		eVAlign = UI_TEXT_VALIGN::MIDDLE;
		_bool				bAutoFit = false;
		_bool				bVisible = { true };
	}UI_ELEMENT;

	typedef struct tagUISceneData
	{
		_tchar                  szName[MAX_PATH] = {};
		_float                  fAuthoringWidth = { 1280.f };
		_float                  fAuthoringHeight = { 720.f };
		vector<UI_ELEMENT>      Elements;
	}UI_SCENE_DATA;
}

#endif // Client_Struct_h