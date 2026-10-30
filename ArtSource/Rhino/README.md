# Rhino (walk cycle)

| File | What it is |
| --- | --- |
| `SK_Rhino.fbx` | **Import this into Unreal.** Fixed mesh, skeleton and the `Walk` loop (frames 1-23, 24 fps). Not committed yet (Git LFS upload was blocked from the cloud session); it was shared in the project thread. Drop it in this folder and commit it, or regenerate it with the script below. |
| `Rhino.blend` | Fixed Blender source the FBX was exported from. |
| `Rhino_original.blend` | The file as we received it, kept for reference. |
| `fix_rhino_rig.py` | The script that turns the original into the fixed version. |

## What was wrong with the front legs

1. **The front legs were not attached to the body.** The two shoulder bones (`Bone.011`, `Bone.026`) had no parent, so each front leg was its own separate bone tree.
2. **The rig had 11 root bones** (the spine, both shoulders, and the 8 IK controller/pole bones `*_Cont`, `*_Res`). Unreal only supports one root. It drops Blender's `Armature` node and keeps one tree, so the front legs (and controllers) went missing from the skeleton. That is why one import had frozen front legs and the other had broken ones.
3. The feet were driven by IK constraints pointing at those controller bones, and the controllers were exported as deform bones.
4. The `.blend` was saved in Edit Mode, so the pose was frozen until you leave Edit Mode.
5. Smaller things: one vertex had no weights, leaf bones (`*_end`) were exported, and two bones were misnamed (`BR_LowLeg.002` is the left leg, `FL_Toe.001` is the right toe).

## What the fix does

- Adds a single `root` bone at the origin; the pelvis (`Bone`) hangs off it.
- Parents the shoulders to the chest (`Bone.002`) and renames them `FR_Shoulder` / `FL_Shoulder`.
- Bakes the IK result into plain keyframes on every frame, then removes the IK constraints and the controller/pole bones.
- Renames `BR_LowLeg.002` to `BL_LowLeg` and `FL_Toe.001` to `FR_Toe`; material is now `M_Rhino`.
- Exports without leaf bones, with the scale applied (rhino is about 297 cm long in Unreal).

Checked by re-importing `SK_Rhino.fbx` and comparing the deformed mesh against the original Blender animation on every frame. It matches to within 0.1 mm, except for the one vertex that used to have no weights.

## Importing into Unreal (UE5)

1. Delete the old `rhino_`, `rhino__Skeleton` and `rhino__Anim` assets in `Content/Poaching_Patrol/Assets/Animals/Rhino`. The bone hierarchy changed, so the old skeleton cannot be reused.
2. Drag `SK_Rhino.fbx` into that folder. In the import dialog:
   - Skeletal Mesh: on. Skeleton: leave empty, so a new one is created.
   - Import Animations: on.
   - Import Uniform Scale: `1.0`. Convert Scene: on. Force Front X Axis: off.
   - Use T0 As Reference Pose: off.
3. You get `SK_Rhino`, `SK_Rhino_Skeleton`, `SK_Rhino_PhysicsAsset` and the walk animation. Open the animation and turn on **Loop** in the preview.
4. Assign the rhino material to the `M_Rhino` slot on `SK_Rhino`.
5. Point any Blueprint or AnimBP that used the old rhino mesh at `SK_Rhino` and the new walk animation.

The rhino faces +X, which is Unreal's forward, so it needs no rotation offset on a Character mesh.

## Re-exporting after you edit the rig in Blender

Animate in `Rhino.blend` (it has no IK any more, so key the leg bones directly), then use File > Export > FBX with these settings: Object Types Armature + Mesh, Apply Scalings `FBX All`, Add Leaf Bones off, Bake Animation on, NLA Strips off. If you go back to an IK rig, keep the controller bones parented under `root` and untick their Deform option so there is still only one root.
