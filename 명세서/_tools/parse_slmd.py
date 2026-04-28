"""SLMD v2 parser - dumps model info, bones, meshes, animations + per-channel translation magnitude"""
import struct, sys, os, math
from collections import defaultdict

MAX_PATH = 260

def read(fp, fmt):
    size = struct.calcsize(fmt)
    buf = fp.read(size)
    return struct.unpack(fmt, buf)

def read_cstr(fp, n):
    buf = fp.read(n)
    end = buf.find(b'\x00')
    return buf[:end].decode('utf-8', errors='replace') if end >= 0 else buf.decode('utf-8', errors='replace')

def read_wcstr(fp, n):
    buf = fp.read(n * 2)
    s = buf.decode('utf-16-le', errors='replace')
    end = s.find('\x00')
    return s[:end] if end >= 0 else s

def parse(path):
    with open(path, 'rb') as fp:
        magic = fp.read(4)
        assert magic == b'SLMD', magic
        version, model_type, reserved = read(fp, '<III')
        pre_xform = read(fp, '<16f')
        root_bone = read_cstr(fp, MAX_PATH) if version >= 2 else ''

        # Meshes
        (num_meshes,) = read(fp, '<I')
        meshes = []
        for i in range(num_meshes):
            name = read_cstr(fp, MAX_PATH)
            mat_idx, vstride, nverts = read(fp, '<III')
            fp.seek(vstride * nverts, 1)  # skip verts
            (nidx,) = read(fp, '<I')
            fp.seek(nidx * 4, 1)
            num_bones = 0
            bone_indices = []
            if model_type == 1:  # ANIM
                (num_bones,) = read(fp, '<I')
                if num_bones > 0:
                    bone_indices = list(read(fp, f'<{num_bones}I'))
                    fp.seek(num_bones * 64, 1)  # OffsetMatrices
            meshes.append(dict(name=name, mat=mat_idx, stride=vstride, nv=nverts, ni=nidx,
                               nbones=num_bones, bone_indices=bone_indices))

        # Materials
        (num_mats,) = read(fp, '<I')
        materials = []
        for i in range(num_mats):
            (ntex,) = read(fp, '<I')
            texs = []
            for j in range(ntex):
                (etype,) = read(fp, '<I')
                texpath = read_wcstr(fp, MAX_PATH)
                texs.append((etype, texpath))
            materials.append(texs)

        # Bones
        (num_bones,) = read(fp, '<I')
        bones = []
        for i in range(num_bones):
            name = read_cstr(fp, MAX_PATH)
            (parent,) = read(fp, '<i')
            fp.seek(64, 1)  # transform
            bones.append((name, parent))

        # Animations
        (num_anims,) = read(fp, '<I')
        anims = []
        for i in range(num_anims):
            name = read_cstr(fp, MAX_PATH)
            duration, tps = read(fp, '<ff')
            (loop,) = read(fp, '<I')
            use_rm = 0
            if version >= 2:
                (use_rm,) = read(fp, '<I')
            (nch,) = read(fp, '<I')
            channels = []
            for j in range(nch):
                bidx, nkf = read(fp, '<II')
                # KEYFRAME: scale(3f) rot(4f) trans(3f) tpos(1f) = 11f = 44B
                # Get first/last translation to compute root motion-ish delta
                first_t = None; last_t = None; max_disp = 0.0
                for k in range(nkf):
                    s = read(fp, '<3f')
                    r = read(fp, '<4f')
                    t = read(fp, '<3f')
                    tp = read(fp, '<f')
                    if first_t is None:
                        first_t = t
                    last_t = t
                    d = math.sqrt((t[0]-first_t[0])**2 + (t[1]-first_t[1])**2 + (t[2]-first_t[2])**2)
                    if d > max_disp: max_disp = d
                channels.append(dict(bone_idx=bidx, nkf=nkf, first_t=first_t, last_t=last_t, max_disp=max_disp))
            anims.append(dict(name=name, dur=duration, tps=tps, loop=loop, rm=use_rm, channels=channels))

        return dict(version=version, model_type=model_type, root_bone=root_bone,
                    meshes=meshes, materials=materials, bones=bones, anims=anims)


def heuristic_loop_rm(name):
    n = name.lower()
    loop = any(k in n for k in ['idle', 'walk', 'run', 'guard_loop', 'stun_loop', 'breath'])
    rm = any(k in n for k in ['dash', 'attack', 'skill', 'chainsmash', 'chain_smash',
                              'breakfall', 'avoid', 'parrying', 'damage', 'stun', 'dead',
                              'roll', 'jump', 'core'])
    if 'idle' in n or 'loop' in n: rm = False
    return loop, rm


if __name__ == '__main__':
    path = sys.argv[1]
    d = parse(path)
    print(f"=== SLMD v{d['version']} type={'ANIM' if d['model_type']==1 else 'NONANIM'} ===")
    print(f"RootBone: '{d['root_bone']}'")
    print(f"Meshes: {len(d['meshes'])}, Materials: {len(d['materials'])}, Bones: {len(d['bones'])}, Anims: {len(d['anims'])}")
    print()
    print("=== MESHES ===")
    for i,m in enumerate(d['meshes']):
        print(f"  [{i:2}] {m['name']:48} mat={m['mat']} verts={m['nv']:5} idx={m['ni']:5} bones={m['nbones']}")
    print()
    print("=== MATERIALS ===")
    for i,texs in enumerate(d['materials']):
        for et,tp in texs:
            print(f"  [{i:2}] type={et} {tp}")
    print()
    print("=== BONES (top-level only) ===")
    for i,(n,p) in enumerate(d['bones']):
        if p == -1 or (p >= 0 and d['bones'][p][1] == -1):
            print(f"  [{i:3}] parent={p:3}  {n}")
    print(f"  ...total {len(d['bones'])} bones")
    print()
    if d['root_bone']:
        for i,(n,p) in enumerate(d['bones']):
            if n == d['root_bone']:
                print(f"  RootBone '{d['root_bone']}' index={i} parent={p}")
                break
    print()
    print("=== ANIMATIONS ===")
    print(f"  {'#':>3} {'name':40} {'dur':>7} {'tps':>5} {'frames':>6} {'ch':>3} {'rootCh|D|':>10}  loop_hint  rm_hint")
    for i,a in enumerate(d['anims']):
        # find channel for root bone
        root_idx = None
        if d['root_bone']:
            for bi,(bn,_) in enumerate(d['bones']):
                if bn == d['root_bone']: root_idx = bi; break
        root_disp = 0.0
        for c in a['channels']:
            if c['bone_idx'] == root_idx:
                root_disp = c['max_disp']; break
        frames = int(round(a['dur']))
        loop_h, rm_h = heuristic_loop_rm(a['name'])
        print(f"  [{i:3}] {a['name']:40} {a['dur']:7.2f} {a['tps']:5.1f} {frames:6} {len(a['channels']):3} {root_disp:10.3f}  {str(loop_h):5}      {str(rm_h):5}")
