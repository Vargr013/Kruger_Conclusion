"""Fix the rhino rig for Unreal: single root, front legs attached to the spine,
IK baked to FK, controller bones removed, clean FBX export.

Usage: blender -b --python fix_rhino_rig.py -- Rhino_original.blend Rhino.blend SK_Rhino.fbx
"""
import bpy, sys

src, out_blend, out_fbx = sys.argv[sys.argv.index('--') + 1:][:3]
bpy.ops.wm.open_mainfile(filepath=src, load_ui=False)
scene = bpy.context.scene
arm = bpy.data.objects['Armature']
mesh = bpy.data.objects['Plane']

# File was saved in Edit Mode, which freezes pose evaluation.
bpy.context.view_layer.objects.active = arm
if arm.mode != 'OBJECT':
    bpy.ops.object.mode_set(mode='OBJECT')

FIRST, LAST = 1, 23  # walk cycle: frame 24 == frame 1
CONTROL = [b.name for b in arm.data.bones if b.name.endswith(('_Cont', '_Res'))]
KEEP = [b.name for b in arm.data.bones if b.name not in CONTROL]
RENAME = {'BR_LowLeg.002': 'BL_LowLeg', 'FL_Toe.001': 'FR_Toe',
          'Bone.011': 'FR_Shoulder', 'Bone.026': 'FL_Shoulder'}

# 1. Record the evaluated (IK-solved) armature-space pose of every kept bone.
baked = {}
for f in range(FIRST, LAST + 1):
    scene.frame_set(f)
    baked[f] = {n: arm.pose.bones[n].matrix.copy() for n in KEEP}

# 2. Rebuild hierarchy: add root at origin, hang the front shoulders off the chest,
#    delete IK controller/pole bones.
for pb in arm.pose.bones:
    for c in list(pb.constraints):
        pb.constraints.remove(c)
bpy.ops.object.mode_set(mode='EDIT')
eb = arm.data.edit_bones
root = eb.new('root')
root.head, root.tail = (0, 0, 0), (0, -0.3, 0)
root.use_deform = False  # still exported: it is the parent of deform bones
eb['Bone'].parent = root
for shoulder in ('Bone.011', 'Bone.026'):
    eb[shoulder].use_connect = False
    eb[shoulder].parent = eb['Bone.002']
for n in CONTROL:
    eb.remove(eb[n])
bpy.ops.object.mode_set(mode='OBJECT')

# 3. Clean vertex groups: drop controller groups, fix the one unweighted vertex.
for n in CONTROL:
    vg = mesh.vertex_groups.get(n)
    if vg:
        mesh.vertex_groups.remove(vg)
verts = mesh.data.vertices
weighted = [v for v in verts if any(g.weight > 0 for g in v.groups)]
for v in verts:
    if not any(g.weight > 0 for g in v.groups):
        near = min(weighted, key=lambda w: (w.co - v.co).length)
        for g in near.groups:
            mesh.vertex_groups[g.group].add([v.index], g.weight, 'REPLACE')

# 4. Re-key the recorded pose as plain FK on the new hierarchy.
old = arm.animation_data.action
act = bpy.data.actions.new('Walk')
arm.animation_data.action = act
bpy.data.actions.remove(old)
order = [b.name for b in arm.data.bones]  # parents always precede children
for f in range(FIRST, LAST + 1):
    pose = baked[f]
    pose['root'] = arm.data.bones['root'].matrix_local.copy()
    for n in order:
        bone, pb = arm.data.bones[n], arm.pose.bones[n]
        if bone.parent:
            rest = bone.parent.matrix_local.inverted() @ bone.matrix_local
            basis = rest.inverted() @ pose[bone.parent.name].inverted() @ pose[n]
        else:
            basis = bone.matrix_local.inverted() @ pose[n]
        pb.matrix_basis = basis
        for path in ('location', 'rotation_quaternion', 'scale'):
            pb.keyframe_insert(path, frame=f, group=n)

for oldn, newn in RENAME.items():
    arm.data.bones[oldn].name = newn  # vertex groups follow automatically

scene.frame_start, scene.frame_end = FIRST, LAST
mesh.data.materials[0].name = 'M_Rhino'
mesh.name = 'SK_Rhino'
arm.name = 'Armature'  # Unreal drops a top node called "Armature", leaving "root" as the single root
bpy.ops.wm.save_as_mainfile(filepath=out_blend, compress=True, copy=True)

bpy.ops.object.select_all(action='DESELECT')
arm.select_set(True); mesh.select_set(True)
bpy.ops.export_scene.fbx(
    filepath=out_fbx, use_selection=True, object_types={'ARMATURE', 'MESH'},
    apply_scale_options='FBX_SCALE_ALL', axis_forward='-Z', axis_up='Y',
    use_mesh_modifiers=True, mesh_smooth_type='FACE',
    add_leaf_bones=False, use_armature_deform_only=False,
    primary_bone_axis='Y', secondary_bone_axis='X',
    bake_anim=True, bake_anim_use_all_bones=True, bake_anim_use_nla_strips=False,
    bake_anim_use_all_actions=True, bake_anim_force_startend_keying=True,
    bake_anim_step=1.0, bake_anim_simplify_factor=0.0)
print('DONE', out_blend, out_fbx)
