"""특정 본 이름들이 .bin 안에 있는지 확인"""
import sys, os
sys.path.insert(0, os.path.dirname(__file__))
from parse_slmd import parse

NEEDLES = ["FX_Point_R_Foot", "FX_Point_L_Foot", "Foot", "Toe", "Ankle"]

d = parse(sys.argv[1])
print(f"총 {len(d['bones'])} bones")
for needle in NEEDLES:
    print(f"\n--- '{needle}' 포함 ---")
    for i,(n,p) in enumerate(d['bones']):
        if needle.lower() in n.lower():
            print(f"  [{i:3}] parent={p:3}  {n}")
