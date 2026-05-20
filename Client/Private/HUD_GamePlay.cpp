#include "HUD_GamePlay.h"
#include "GameInstance.h"
#include "Layer.h"
#include "UI_Image.h"
#include "UIObject.h"
#include "Monster.h"
#include "Player.h"
#include "Transform.h"
#include "UI_Text.h"

CHUD_GamePlay* CHUD_GamePlay::s_pInstance = { nullptr };

CHUD_GamePlay::CHUD_GamePlay(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
	: CGameObject{ pDevice, pContext }
{
}

CHUD_GamePlay::CHUD_GamePlay(const CHUD_GamePlay& Prototype)
	: CGameObject{ Prototype }
{
}

void CHUD_GamePlay::Notify_Hit(CMonster* pMonster)
{
	if (nullptr == pMonster)
		return;

	const _bool bTargetChanged = (m_pCurrentTarget != pMonster);

	if (bTargetChanged)
	{
		Safe_Release(m_pCurrentTarget);
		m_pCurrentTarget = pMonster;
		Safe_AddRef(m_pCurrentTarget);

		const _float fHpMax = m_pCurrentTarget->Get_MaxHP();
		const _float fHpCur = m_pCurrentTarget->Get_CurrentHP();
		const _float fHpR = (fHpMax > 0.f) ? (fHpCur / fHpMax) : 0.f;
		m_fMonHpFill = m_fMonHpReduce = fHpR;
		m_fMonHpReduceDelay = 0.f;

		if (m_pCurrentTarget->Has_Break())
		{
			const _float fBrMax = m_pCurrentTarget->Get_MaxBreak();
			const _float fBrCur = m_pCurrentTarget->Get_CurrentBreak();
			const _float fBrR = (fBrMax > 0.f) ? (fBrCur / fBrMax) : 0.f;
			m_fMonBreakFill = m_fMonBreakReduce = fBrR;
		}
		else
		{
			m_fMonBreakFill = m_fMonBreakReduce = 0.f;
		}
		m_fMonBreakReduceDelay = 0.f;
	}

	m_fSinceLastHit = 0.f;
	Set_MonsterBars_Visible(true);
}

void CHUD_GamePlay::Notify_Death(CMonster* pMonster)
{
	if (nullptr == pMonster)
		return;

	if (m_pCurrentTarget != pMonster)
		return;

	//  TODO : 0.5초 Short Fade OUT
	Safe_Release(m_pCurrentTarget);
	m_pCurrentTarget = nullptr;
	m_pLastTextTarget = nullptr;

	m_fSinceLastHit = 0.f;

	m_fMonHpFill = 0.f;
	m_fMonHpReduce = 0.f;
	m_fMonHpReduceDelay = 0.f;

	m_fMonBreakFill = 0.f;
	m_fMonBreakReduce = 0.f;
	m_fMonBreakReduceDelay = 0.f;

	Apply_Bar_Visuals(HUD_SLOT::MONSTER_HP_FILL, HUD_SLOT::MONSTER_HP_REDUCE, 0.f, 0.f);
	Apply_Bar_Visuals(HUD_SLOT::MONSTER_BREAK_FILL, HUD_SLOT::MONSTER_BREAK_REDUCE, 0.f, 0.f);

	Set_MonsterBars_Visible(false);
}

void CHUD_GamePlay::Notify_DashInput()
{
	m_bDashInput = true;
	m_fSinceDashInput = 0.f;
	Set_PlayerDash_Visible(true);
}

void CHUD_GamePlay::Notify_CombatInput()
{
	m_bCombatInput = true;
	m_fSinceCombatInput = 0.f;
	Set_PlayerBars_Visible(true);
}

HRESULT CHUD_GamePlay::Initialize_Prototype()
{
	return S_OK;
}

HRESULT CHUD_GamePlay::Initialize(void* pArg)
{
	if (FAILED(__super::Initialize(pArg)))
		return E_FAIL;

	s_pInstance = this;
	return S_OK;
}

void CHUD_GamePlay::Priority_Update(_float fTimeDelta)
{
}

void CHUD_GamePlay::Update(_float fTimeDelta)
{
	if (false == m_bCached)
	{
		Cache_UIs();
		Resolve_Player();
		Cache_Viewport();
		Cache_Dash_Offsets();
		Set_MonsterBars_Visible(false);
		Set_PlayerBars_Visible(false);
		Set_PlayerDash_Visible(false);
		m_bCached = true;
	}

	if (m_bDashInput)
	{
		m_fSinceDashInput += fTimeDelta;
		if (m_fSinceDashInput >= DASH_VISIBLE_FOR)
			Set_PlayerDash_Visible(false);
	}

	if (m_bCombatInput)
	{
		m_fSinceCombatInput += fTimeDelta;
		if (m_fSinceCombatInput >= BARS_VISIBLE_FOR)
			Set_PlayerBars_Visible(false);
	}

	if (nullptr != m_pCurrentTarget)
		m_fSinceLastHit += fTimeDelta;

	Tick_MonsterBars(fTimeDelta);
	Tick_PlayerBars(fTimeDelta);
	Tick_Dash(fTimeDelta);
	Tick_Sweep(fTimeDelta);
}

void CHUD_GamePlay::Late_Update(_float fTimeDelta)
{
}

HRESULT CHUD_GamePlay::Render()
{
	return S_OK;
}

void CHUD_GamePlay::Cache_UIs()
{
	static const _tchar* Names[static_cast<_uint>(HUD_SLOT::END)] = {
	  TEXT("HUD_MonsterHP_Back"), TEXT("HUD_MonsterHP_Reduce"), TEXT("HUD_MonsterHP_Fill"), TEXT("HUD_MonsterHP_BarLight"),
	  TEXT("HUD_MonsterBreak_Back"), TEXT("HUD_MonsterBreak_Reduce"), TEXT("HUD_MonsterBreak_Fill"),
	  TEXT("HUD_PlayerHP_Back"), TEXT("HUD_PlayerHP_Reduce"), TEXT("HUD_PlayerHP_Fill"), TEXT("HUD_PlayerHP_BarLight"),
	  TEXT("HUD_PlayerMP_Back"), TEXT("HUD_PlayerMP_Reduce"), TEXT("HUD_PlayerMP_Fill"), TEXT("HUD_PlayerMP_BarLight"),
	  TEXT("HUD_Dash_Base"), TEXT("HUD_Dash_Line"),
	  TEXT("HUD_Dash_Step1"), TEXT("HUD_Dash_Step1_Glow"),
	  TEXT("HUD_Dash_Step2"), TEXT("HUD_Dash_Step2_Glow"),
	  TEXT("HUD_Dash_Step3"), TEXT("HUD_Dash_Step3_Glow"),
	};

	for (_uint i = 0; i < static_cast<_uint>(HUD_SLOT::END); ++i)
		m_pUI[i] = Find_UI_ByName(Names[i]);

	const auto* pLayers = m_pGameInstance->Get_Layers(ETOUI(LEVEL::GAMEPLAY));
	if (pLayers)
	{
		auto it = pLayers->find(TEXT("Layer_UI"));
		if (it != pLayers->end() && nullptr != it->second)
		{
			for (CGameObject* pObj : it->second->Get_GameObjects())
			{
				CUI_Text* pText = dynamic_cast<CUI_Text*>(pObj);
				if (nullptr == pText) continue;

				const _wstring& strName = pText->Get_ObjectName();
				if (strName == TEXT("HUD_MonsterLevel"))     m_pUI_MonsterLevel = pText;
				else if (strName == TEXT("HUD_MonsterName")) m_pUI_MonsterName = pText;
			}
		}
	}


}

void CHUD_GamePlay::Resolve_Player()
{
	if (nullptr == m_pGameInstance)
		return;

	const auto* pLayers = m_pGameInstance->Get_Layers(ETOUI(LEVEL::GAMEPLAY));
	if (nullptr == pLayers)
		return;

	auto it = pLayers->find(TEXT("Layer_Player"));
	if (it == pLayers->end() || nullptr == it->second)
		return;

	const auto& Objs = it->second->Get_GameObjects();
	if (Objs.empty())
		return;

	CPlayer* pPlayer = dynamic_cast<CPlayer*>(Objs.front());
	if (nullptr == pPlayer)
		return;

	Safe_AddRef(pPlayer);
	m_pPlayer = pPlayer;
}

void CHUD_GamePlay::Cache_Dash_Offsets()
{
	const HUD_SLOT eDashSlots[] = {
				HUD_SLOT::DASH_BASE, HUD_SLOT::DASH_LINE,
				HUD_SLOT::DASH_STEP1, HUD_SLOT::DASH_STEP1_GLOW,
				HUD_SLOT::DASH_STEP2, HUD_SLOT::DASH_STEP2_GLOW,
				HUD_SLOT::DASH_STEP3, HUD_SLOT::DASH_STEP3_GLOW,
	};

	for (HUD_SLOT eSlot : eDashSlots)
	{
		const _uint iIdx = ETOUI(eSlot);
		CUI_Image* pUI = m_pUI[iIdx];
		if (nullptr == pUI) continue;

		m_fDashOffsetX[iIdx] = pUI->Get_CenterX() - m_fViewW * 0.5f;
		m_fDashOffsetY[iIdx] = pUI->Get_CenterY() - m_fViewH * 0.5f;
	}
	m_bDashBaseCached = true;
}

void CHUD_GamePlay::Cache_Viewport()
{
	_uint           iNumViewport = 1;
	D3D11_VIEWPORT  Viewport = {};
	m_pContext->RSGetViewports(&iNumViewport, &Viewport);
	m_fViewW = Viewport.Width;
	m_fViewH = Viewport.Height;
}

CUI_Image* CHUD_GamePlay::Find_UI_ByName(const _tchar* pName)
{
	if (nullptr == pName || nullptr == m_pGameInstance)
		return nullptr;

	const auto* pLayers = m_pGameInstance->Get_Layers(ETOUI(LEVEL::GAMEPLAY));
	if (nullptr == pLayers)
		return nullptr;

	auto it = pLayers->find(TEXT("Layer_UI"));
	if (it == pLayers->end() || nullptr == it->second)
		return nullptr;

	for (CGameObject* pObj : it->second->Get_GameObjects())
	{
		CUIObject* pUI = dynamic_cast<CUIObject*>(pObj);
		if (nullptr == pUI)
			continue;
		if (pUI->Get_ObjectName() == pName)
			return dynamic_cast<CUI_Image*>(pUI);
	}

	return nullptr;
}

void CHUD_GamePlay::Tick_LazyBar(_float& fFill, _float& fReduce, _float& fDelay, _float fTarget, _float fTimeDelta)
{
	if (fTarget < fFill)
	{
		fFill = fTarget;
		fDelay = REDUCE_HOLD;   
	}
	else if (fTarget > fFill)
	{
		fFill = fReduce = fTarget;
		fDelay = 0.f;
		return;
	}

	if (fReduce > fFill)
	{
		if (fDelay > 0.f)
		{
			fDelay -= fTimeDelta;
		}
		else
		{
			fReduce -= REDUCE_LERP_SPEED * fTimeDelta;
			if (fReduce < fFill) fReduce = fFill;
		}
	}
}

void CHUD_GamePlay::Tick_Sweep(_float fTimeDelta)
{
	// === BarLight Position-sweep ===
	m_fBarSweepTime += fTimeDelta;
	if (m_fBarSweepTime >= BAR_SWEEP_TOTAL)
		m_fBarSweepTime -= BAR_SWEEP_TOTAL;

	_float t = -1.f;   // -1 = 휴식 구간
	if (m_fBarSweepTime < BAR_SWEEP_DURATION)
		t = m_fBarSweepTime / BAR_SWEEP_DURATION;

	struct SWEEP_PAIR
	{
		HUD_SLOT eLight;
		HUD_SLOT eBack;
		_float   fFillRatio;
	};

	const SWEEP_PAIR Pairs[] = {
		{ HUD_SLOT::MONSTER_HP_BARLIGHT, HUD_SLOT::MONSTER_HP_BACK, m_fMonHpFill },
		{ HUD_SLOT::PLAYER_HP_BARLIGHT,  HUD_SLOT::PLAYER_HP_BACK,  m_fPlyHpFill },
		{ HUD_SLOT::PLAYER_MP_BARLIGHT,  HUD_SLOT::PLAYER_MP_BACK,  m_fPlyMpFill },
	};

	for (const SWEEP_PAIR& P : Pairs)
	{
		CUI_Image* pBack = m_pUI[ETOUI(P.eBack)];
		CUI_Image* pLight = m_pUI[ETOUI(P.eLight)];
		if (nullptr == pBack || nullptr == pLight) continue;

		const _float fBackHalf = pBack->Get_SizeX() * 0.5f;
		const _float fLightHalf = pLight->Get_SizeX() * 0.5f;
		const _float fBackLeft = pBack->Get_CenterX() - fBackHalf;
		const _float fFillEndX = fBackLeft + pBack->Get_SizeX() * P.fFillRatio;

		// 광원의 좌측 가장자리가 fill 시작점 = sweep 시작
		// 광원의 우측 가장자리가 fill 끝점   = sweep 종료
		const _float fStartCx = fBackLeft + fLightHalf;
		const _float fEndCx = fFillEndX - fLightHalf;

		// 휴식 구간 OR fill 이 광원 폭보다 작아 sweep 불가능 → 화면 밖
		if (t < 0.f || fEndCx <= fStartCx)
		{
			pLight->Set_Center(fStartCx - 10000.f, pLight->Get_CenterY());
			continue;
		}

		const _float fX = fStartCx + (fEndCx - fStartCx) * t;
		pLight->Set_Center(fX, pLight->Get_CenterY());
	}

	// === Glow UV-sweep ===
	m_fGlowSweepTime += fTimeDelta * GLOW_UV_SPEED;
	if (m_fGlowSweepTime >= 1.f)
		m_fGlowSweepTime -= floorf(m_fGlowSweepTime);

	static const HUD_SLOT GlowSlots[] = {
		HUD_SLOT::DASH_STEP1_GLOW,
		HUD_SLOT::DASH_STEP2_GLOW,
		HUD_SLOT::DASH_STEP3_GLOW,
	};

	for (HUD_SLOT eSlot : GlowSlots)
	{
		if (CUI_Image* pGlow = m_pUI[ETOUI(eSlot)])
			pGlow->Set_UVOffset(0.f, m_fGlowSweepTime);
	}

	// === Glow trigger ===
	if (m_pPlayer)
	{
		const _int iCharge = m_pPlayer->Get_DashCharge();
		m_bGlowTrigger[0] = (iCharge >= 1);
		m_bGlowTrigger[1] = (iCharge >= 2);
		m_bGlowTrigger[2] = (iCharge >= 3);
	}
}

void CHUD_GamePlay::Apply_Bar_Visuals(HUD_SLOT eFill, HUD_SLOT eReduce, _float fFill, _float fReduce)
{
	if (CUI_Image* pFill = m_pUI[ETOUI(eFill)])
		pFill->Set_GaugeRatio(fFill);
	if (CUI_Image* pReduce = m_pUI[ETOUI(eReduce)])
		pReduce->Set_GaugeRatio(fReduce);
}

void CHUD_GamePlay::Tick_MonsterBars(_float fTimeDelta)
{
	if (nullptr == m_pCurrentTarget)
		return;

	// HP
	{
		const _float fMax = m_pCurrentTarget->Get_MaxHP();
		const _float fCur = m_pCurrentTarget->Get_CurrentHP();
		const _float fTarget = (fMax > 0.f) ? (fCur / fMax) : 0.f;
		Tick_LazyBar(m_fMonHpFill, m_fMonHpReduce, m_fMonHpReduceDelay, fTarget, fTimeDelta);
		Apply_Bar_Visuals(HUD_SLOT::MONSTER_HP_FILL, HUD_SLOT::MONSTER_HP_REDUCE,
			m_fMonHpFill, m_fMonHpReduce);
	}

	// Break
	if (m_pCurrentTarget->Has_Break())
	{
		const _float fMax = m_pCurrentTarget->Get_MaxBreak();
		const _float fCur = m_pCurrentTarget->Get_CurrentBreak();
		const _float fTarget = (fMax > 0.f) ? (fCur / fMax) : 0.f;
		Tick_LazyBar(m_fMonBreakFill, m_fMonBreakReduce, m_fMonBreakReduceDelay, fTarget, fTimeDelta);
		Apply_Bar_Visuals(HUD_SLOT::MONSTER_BREAK_FILL, HUD_SLOT::MONSTER_BREAK_REDUCE,
			m_fMonBreakFill, m_fMonBreakReduce);
	}

	if (m_pLastTextTarget != m_pCurrentTarget)
	{
		m_pLastTextTarget = m_pCurrentTarget;

		if (nullptr != m_pCurrentTarget)
		{
			if (m_pUI_MonsterLevel)
			{
				_tchar szLevel[32] = {};
				swprintf_s(szLevel, TEXT("Lv. %d"), m_pCurrentTarget->Get_Level());
				m_pUI_MonsterLevel->Set_Text(szLevel);
			}
			if (m_pUI_MonsterName)
				m_pUI_MonsterName->Set_Text(m_pCurrentTarget->Get_DisplayName().c_str());
		}
	}
}

void CHUD_GamePlay::Tick_PlayerBars(_float fTimeDelta)
{
	if (nullptr == m_pPlayer)
		return;

	// HP
	{
		const _float fMax = m_pPlayer->Get_MaxHP();
		const _float fCur = m_pPlayer->Get_CurrentHP();
		const _float fTarget = (fMax > 0.f) ? (fCur / fMax) : 0.f;
		Tick_LazyBar(m_fPlyHpFill, m_fPlyHpReduce, m_fPlyHpReduceDelay, fTarget, fTimeDelta);
		Apply_Bar_Visuals(HUD_SLOT::PLAYER_HP_FILL, HUD_SLOT::PLAYER_HP_REDUCE,
			m_fPlyHpFill, m_fPlyHpReduce);
	}

	// MP
	{
		const _float fMax = m_pPlayer->Get_MaxMP();
		const _float fCur = m_pPlayer->Get_CurrentMP();
		const _float fTarget = (fMax > 0.f) ? (fCur / fMax) : 0.f;
		Tick_LazyBar(m_fPlyMpFill, m_fPlyMpReduce, m_fPlyMpReduceDelay, fTarget, fTimeDelta);
		Apply_Bar_Visuals(HUD_SLOT::PLAYER_MP_FILL, HUD_SLOT::PLAYER_MP_REDUCE,
			m_fPlyMpFill, m_fPlyMpReduce);
	}
}

void CHUD_GamePlay::Tick_Dash(_float fTimeDelta)
{
	if (false == m_bDashInput)
		return;

	if (nullptr == m_pPlayer || false == m_bDashBaseCached)
		return;

	CTransform* pPlayerTr = m_pPlayer->Get_Transform();
	if (nullptr == pPlayerTr)
		return;

	_float3 vDashWorldPos{};
	_vector vWorld = {};

	if (m_pPlayer->Try_GetDashHUDWorldPosition(&vDashWorldPos))
	{
		vWorld = XMLoadFloat3(&vDashWorldPos);
	}
	else
	{
		vWorld = pPlayerTr->Get_State(STATE::POSITION);
		vWorld = XMVectorSetY(vWorld, XMVectorGetY(vWorld) + 1.0f);
	}

	const _float4x4* pView = m_pGameInstance->Get_Transform(D3DTS::VIEW);
	const _float4x4* pProj = m_pGameInstance->Get_Transform(D3DTS::PROJ);
	if (nullptr == pView || nullptr == pProj) return;

	_matrix mViewProj = XMLoadFloat4x4(pView) * XMLoadFloat4x4(pProj);
	_vector vClip = XMVector3TransformCoord(vWorld, mViewProj);

	const _float fNdcX = XMVectorGetX(vClip);
	const _float fNdcY = XMVectorGetY(vClip);
	const _float fNdcZ = XMVectorGetZ(vClip);

	const _bool bBehind = (fNdcZ < 0.f) || (fNdcZ > 1.f);

	const _float fScreenX = (fNdcX * 0.5f + 0.5f) * m_fViewW;
	const _float fScreenY = (1.f - (fNdcY * 0.5f + 0.5f)) * m_fViewH;

	const _int iCharge = m_pPlayer->Get_DashCharge();
	const _int iChargeMax = m_pPlayer->Get_DashChargeMax();

	const _float fScaleX = m_fViewW / 1280.f;
	const _float fScaleY = m_fViewH / 720.f;

	const _float fDashCenterX = fScreenX + (-105.f * fScaleX);
	const _float fDashCenterY = fScreenY + (65.f * fScaleY);

	auto MoveSlot = [&](HUD_SLOT eSlot)
		{
			CUI_Image* pUI = m_pUI[ETOUI(eSlot)];
			if (nullptr == pUI)
				return;

			pUI->Set_Center(fDashCenterX, fDashCenterY);
		};

	auto SetVis = [&](HUD_SLOT eSlot, _bool bVis)
		{
			if (CUI_Image* pUI = m_pUI[ETOUI(eSlot)])
				pUI->Set_Visible(!bBehind && bVis);
		};

	MoveSlot(HUD_SLOT::DASH_BASE);
	MoveSlot(HUD_SLOT::DASH_LINE);
	MoveSlot(HUD_SLOT::DASH_STEP1);
	MoveSlot(HUD_SLOT::DASH_STEP1_GLOW);
	MoveSlot(HUD_SLOT::DASH_STEP2); 
	MoveSlot(HUD_SLOT::DASH_STEP2_GLOW);
	MoveSlot(HUD_SLOT::DASH_STEP3); 
	MoveSlot(HUD_SLOT::DASH_STEP3_GLOW);

	SetVis(HUD_SLOT::DASH_BASE, true);
	SetVis(HUD_SLOT::DASH_LINE, true);

	SetVis(HUD_SLOT::DASH_STEP1, iCharge >= 1);
	SetVis(HUD_SLOT::DASH_STEP2, iCharge >= 2);
	SetVis(HUD_SLOT::DASH_STEP3, iCharge >= 3);

	SetVis(HUD_SLOT::DASH_STEP1_GLOW, (iCharge >= 1) && m_bGlowTrigger[0]);
	SetVis(HUD_SLOT::DASH_STEP2_GLOW, (iCharge >= 2) && m_bGlowTrigger[1]);
	SetVis(HUD_SLOT::DASH_STEP3_GLOW, (iCharge >= 3) && m_bGlowTrigger[2]);

	(void)iChargeMax;
}

void CHUD_GamePlay::Set_MonsterBars_Visible(_bool bVisible)
{
	const HUD_SLOT eSlots[] = {
				HUD_SLOT::MONSTER_HP_BACK,   HUD_SLOT::MONSTER_HP_REDUCE,    HUD_SLOT::MONSTER_HP_FILL,
				HUD_SLOT::MONSTER_HP_BARLIGHT,
				HUD_SLOT::MONSTER_BREAK_BACK,HUD_SLOT::MONSTER_BREAK_REDUCE, HUD_SLOT::MONSTER_BREAK_FILL,
	};

	for (HUD_SLOT eSlot : eSlots)
	{
		if (CUI_Image* pUI = m_pUI[ETOUI(eSlot)])
			pUI->Set_Visible(bVisible);
	}

	if (m_pUI_MonsterLevel) 
		m_pUI_MonsterLevel->Set_Visible(bVisible);

	if (m_pUI_MonsterName)  
		m_pUI_MonsterName->Set_Visible(bVisible);
}

void CHUD_GamePlay::Set_PlayerBars_Visible(_bool bVisible)
{
	if (!bVisible)
	{
		m_bCombatInput = false;
		m_fSinceCombatInput = 0.f;
	}

	const HUD_SLOT eSlots[] = {
				HUD_SLOT::PLAYER_HP_BACK, HUD_SLOT::PLAYER_HP_REDUCE, HUD_SLOT::PLAYER_HP_FILL,
				HUD_SLOT::PLAYER_HP_BARLIGHT,
				HUD_SLOT::PLAYER_MP_BACK, HUD_SLOT::PLAYER_MP_REDUCE, HUD_SLOT::PLAYER_MP_FILL,
				HUD_SLOT::PLAYER_MP_BARLIGHT,
	};

	for (HUD_SLOT eSlot : eSlots)
	{
		if (CUI_Image* pUI = m_pUI[ETOUI(eSlot)])
			pUI->Set_Visible(bVisible);
	}
}

void CHUD_GamePlay::Set_PlayerDash_Visible(_bool bVisible)
{
	if (!bVisible)
	{
		m_bDashInput = false;
		m_fSinceDashInput = 0.f;

		for (_int i = 0; i < 3; ++i)
		{
			m_bGlowTrigger[i] = false;
		}
	}

	const HUD_SLOT eSlots[] = {
			HUD_SLOT::DASH_BASE, HUD_SLOT::DASH_LINE,
			HUD_SLOT::DASH_STEP1, HUD_SLOT::DASH_STEP1_GLOW,
			HUD_SLOT::DASH_STEP2, HUD_SLOT::DASH_STEP2_GLOW,
			HUD_SLOT::DASH_STEP3, HUD_SLOT::DASH_STEP3_GLOW,
	};

	for (HUD_SLOT eSlot : eSlots)
	{
		if (CUI_Image* pUI = m_pUI[ETOUI(eSlot)])
			pUI->Set_Visible(bVisible);
	}
}

CHUD_GamePlay* CHUD_GamePlay::Create(ID3D11Device* pDevice, ID3D11DeviceContext* pContext)
{
	CHUD_GamePlay* pInstance = new CHUD_GamePlay(pDevice, pContext);
	if (FAILED(pInstance->Initialize_Prototype()))
	{
		MSG_BOX("Failed to Created : CHUD_GamePlay");
		Safe_Release(pInstance);
	}
	return pInstance;
}

CGameObject* CHUD_GamePlay::Clone(void* pArg)
{
	CHUD_GamePlay* pInstance = new CHUD_GamePlay(*this);
	if (FAILED(pInstance->Initialize(pArg)))
	{
		MSG_BOX("Failed to Cloned : CHUD_GamePlay");
		Safe_Release(pInstance);
	}
	return pInstance;
}

void CHUD_GamePlay::Free()
{
	__super::Free();

	if (s_pInstance == this)
		s_pInstance = nullptr;

	Safe_Release(m_pCurrentTarget);
	Safe_Release(m_pPlayer);
}
