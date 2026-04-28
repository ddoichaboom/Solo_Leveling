"""Find Translate bone and dump per-anim translation displacement"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from parse_slmd import parse

KEEP = {
    "Idle","Walk","Run","Run_Fast","Dash","BackDash",
    "BaseAttack_01","BaseAttack_02","BaseAttack_03","CoreAttack_01",
    "Skill_02","Skill_02_Start","Skill_02_End","Skill_02_Loop",
    "Stun","Dead_A","Breakfall","Knockdown","Down_Recovery","KD_Recovery","Landing",
    "Damage_A","Damage_B_Left","Damage_B_Right","Damage_C","Damage_UpperOnly",
    "GS_ChainSmash_01_OneHandSword","GS_ChainSmash_01_Dagger","GS_ChainSmash_01_GreatSword",
    "Guard_Dagger_Start","Guard_Dagger_Loop","Guard_Dagger_End",
    "U_Skill_Crank_01_Start","U_Skill_Crank_01_End",
    "U_Skill_Srank_01_Start","U_Skill_Srank_01_Mid","U_Skill_Srank_01_End",
    "Skill01_ChainA_01","Skill01_ChainA_02","Skill01_ChainB_01","Skill01_ChainB_02",
    "Skill01_ChainC_01_Start","Skill01_ChainC_01_Loop","Skill01_ChainC_01_End","Skill01_ChainC_02",
    "Avoid_Dash","Tag_In","Battle_Idle","Formation_Idle",
    "Parrying_Dagger_Strong","Parrying_Dagger_Right","Parrying_Dagger_Left",
    "Parrying_Dagger_Upper_1","Parrying_Dagger_Upper_2","Parrying_Dagger_Upper_3",
    "SungJinWoo_Dagger_GuardUpperOnly","SungJinWoo_ERank_Guard_Fighter_UpperOnly",
    "Float_A","Float_B","Float_End",
    "Run_End","Run_End_Left","Run_End_Right","Run_FastLeft","Run_FastRight",
}

d = parse(sys.argv[1])
# Find Translate
trans_idx = None
for i,(n,p) in enumerate(d['bones']):
    if n == 'Translate':
        trans_idx = i; break
print(f"Translate bone index: {trans_idx}")

# also find Bip001 just in case
bip_idx = None
for i,(n,p) in enumerate(d['bones']):
    if n == 'Bip001':
        bip_idx = i; break
print(f"Bip001 bone index: {bip_idx}")
print()

print(f"  {'name':45} {'dur':>6} {'frames':>6} {'ch':>4} {'TransD':>8} {'BipD':>8}")
for a in d['anims']:
    if a['name'] not in KEEP: continue
    td = bd = 0.0
    for c in a['channels']:
        if c['bone_idx'] == trans_idx:
            td = c['max_disp']
        if c['bone_idx'] == bip_idx:
            bd = c['max_disp']
    print(f"  {a['name']:45} {a['dur']:6.2f} {len(a['channels']):4} {td:8.3f} {bd:8.3f}")
