"""권장값 vs 저장값 diff 출력"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from parse_slmd import parse

# (name) -> (loop_recommend, rm_recommend)  None = "신경 안 씀 (게임 미사용)"
RECO = {
    # 5.1 이동/기본
    "Idle":          (True,  False),
    "Walk":          (True,  False),
    "Run":           (True,  False),
    "Battle_Idle":   (True,  False),
    "Formation_Idle":(True,  False),
    "Run_Fast":      (True,  False),
    "Run_Fast_2":    (True,  False),
    "Run_FastLeft":  (True,  False),
    "Run_FastRight": (True,  False),
    "Run_End":       (False, False),
    "Run_End_Left":  (False, True),
    "Run_End_Right": (False, True),
    # 5.2 회피
    "Dash":          (False, True),
    "BackDash":      (False, True),
    "Avoid_Dash":    (False, True),
    "Breakfall":     (False, True),
    # 5.3 기본 공격
    "BaseAttack_01": (False, True),
    "BaseAttack_02": (False, True),
    "BaseAttack_03": (False, True),
    "CoreAttack_01": (False, True),
    # 5.4 사슬
    "Skill01_ChainA_01": (False, True),
    "Skill01_ChainA_02": (False, True),
    "Skill01_ChainB_01": (False, True),
    "Skill01_ChainB_02": (False, False),
    "Skill01_ChainC_01_Start": (False, True),
    "Skill01_ChainC_01_Loop":  (True,  False),
    "Skill01_ChainC_01_End":   (False, True),
    "Skill01_ChainC_02":       (False, False),
    "GS_ChainSmash_01_OneHandSword": (False, True),
    "GS_ChainSmash_01_Dagger":       (False, False),
    "GS_ChainSmash_01_GreatSword":   (False, True),
    # 5.5 가드/패링
    "Guard_Dagger_Start": (False, False),
    "Guard_Dagger_Loop":  (True,  False),
    "Guard_Dagger_End":   (False, False),
    "Parrying_Dagger_Strong":  (False, True),
    "Parrying_Dagger_Right":   (False, True),
    "Parrying_Dagger_Left":    (False, True),
    "Parrying_Dagger_Upper_1": (False, True),
    "Parrying_Dagger_Upper_2": (False, True),
    "Parrying_Dagger_Upper_3": (False, True),
    # 5.6 U_SKILL
    "U_Skill_Crank_01_Start": (False, False),
    "U_Skill_Crank_01_End":   (False, False),
    "U_Skill_Srank_01_Start": (False, False),
    "U_Skill_Srank_01_Mid":   (False, False),
    "U_Skill_Srank_01_End":   (False, False),
    # 5.7 피격/다운
    "Damage_A":        (False, False),
    "Damage_B_Left":   (False, False),
    "Damage_B_Right":  (False, False),
    "Damage_C":        (False, False),
    "Damage_UpperOnly":(False, False),
    "Stun":            (False, False),
    "Knockdown":       (False, False),
    "Down_Recovery":   (False, False),
    "KD_Recovery":     (False, False),
    "Dead_A":          (False, False),
    "Landing":         (False, False),
    "Float_A":         (False, False),
    "Float_B":         (False, False),
    "Float_End":       (False, False),
    "Tag_In":          (False, False),
}

d = parse(sys.argv[1])
print(f"RootBone: '{d['root_bone']}'  (권장: 'Translate')")
print()
mismatches = []
covered = []
for a in d['anims']:
    if a['name'] in RECO:
        rl, rr = RECO[a['name']]
        al, ar = bool(a['loop']), bool(a['rm'])
        ok = (al == rl) and (ar == rr)
        covered.append(a['name'])
        if not ok:
            mismatches.append((a['name'], (rl,rr), (al,ar)))

print(f"=== 검토 대상 {len(covered)}/{len(RECO)} 클립 ===")
print()
if not mismatches:
    print("  [OK] 권장값 전부 일치")
else:
    print(f"  [MISMATCH] {len(mismatches)}건:")
    print(f"  {'name':35} {'권장(L,R)':>14}  {'저장(L,R)':>14}")
    for n,(rl,rr),(al,ar) in mismatches:
        print(f"  {n:35} ({str(rl):5},{str(rr):5})  ({str(al):5},{str(ar):5})")

# Also: scan all loop=True anims to check sanity (이름에 loop/idle 류 단어 있는지)
print()
print("=== 전체 .bin 에서 Loop=True 인 클립 ===")
for a in d['anims']:
    if a['loop']:
        marker = " " if any(k in a['name'].lower() for k in ['idle','loop','walk','run']) else "?"
        print(f"  {marker} {a['name']:45} (RM={bool(a['rm'])})")

print()
print("=== KEEP 외 클립 중 RM=True 인 것 (게임 미사용 클립의 RM 잘못 켜짐 의심) ===")
for a in d['anims']:
    if a['rm'] and a['name'] not in RECO:
        print(f"    {a['name']:45} loop={bool(a['loop'])}")
