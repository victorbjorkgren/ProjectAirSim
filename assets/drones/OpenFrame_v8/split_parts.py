"""Split the OpenFrame v8 source scene into the static meshes the Unreal import job needs.

    Blender -b --python split_parts.py -- <source dir> <output dir>

Writes <output dir>/<Part>_LOD0.glb and <Part>_LOD1.glb for
    Assembled   every visual part including the static blades: one mesh, for spawn_object and distance use
    Body        the frame, electronics, camera, antenna, strap and wiring (no hot parts, no blades)
    Battery     Battery + Battery_WrapFold (thermal class battery)
    Motors      the four Motor_<corner> bells (thermal class motor)
    ESC         ESC + ESC_Capacitor + Capacitor_End (thermal class esc)
and <output dir>/BlurDisc.glb (one translucent beauty disc centred on its hub, for the prop links of media
configs; never in SLAM captures).

Every part is one mesh whose vertices are in the body frame (the origin of every node is applied), so each part
sits at the body origin and links can reference them without an offset. Textures are not carried: the Unreal
import job builds the materials from UNREAL_MATERIAL_SETTINGS.json and keeps only the material names and UVs.
"""
import os
import re
import sys

import bpy
from mathutils import Matrix, Vector

SRC, OUT = sys.argv[sys.argv.index("--") + 1:][:2]
os.makedirs(OUT, exist_ok=True)

HOT = {
    "Battery": re.compile(r"^(Battery|Battery_WrapFold)$"),
    "Motors": re.compile(r"^Motor_(FL|FR|RL|RR)$"),
    "ESC": re.compile(r"^(ESC|ESC_Capacitor|Capacitor_End)$"),
}
PROP = re.compile(r"^Prop_")
SKIP = re.compile(r"^(Mount_|Collision)")


def clear():
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete()
    for block in (bpy.data.meshes, bpy.data.materials, bpy.data.images):
        for item in list(block):
            block.remove(item)


def strip_materials():
    """Keep the material names, drop node trees and textures (the glTF exporter would embed them)."""
    for mat in bpy.data.materials:
        mat.use_nodes = False


def group_of(name):
    base = re.sub(r"_LOD1$", "", name)
    for key, rx in HOT.items():
        if rx.match(base):
            return key
    if PROP.match(base):
        return "Props"
    return "Body"


def export(objs, path):
    bpy.ops.object.select_all(action="DESELECT")
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    if len(objs) > 1:
        bpy.ops.object.join()
    bpy.ops.export_scene.gltf(filepath=path, export_format="GLB", use_selection=True, export_yup=True,
                              export_apply=True, export_cameras=False, export_lights=False,
                              export_image_format="NONE")


for lod in ("LOD0", "LOD1"):
    clear()
    bpy.ops.import_scene.gltf(filepath=os.path.join(SRC, f"Drone_OpenFrame_v8_{lod}.glb"))
    strip_materials()
    meshes = [o for o in bpy.data.objects if o.type == "MESH" and not SKIP.match(o.name)]
    groups = {"Assembled": list(meshes), "Body": [], "Battery": [], "Motors": [], "ESC": []}
    for o in meshes:
        g = group_of(o.name)
        if g != "Props":
            groups[g].append(o)
    originals = set(meshes)
    for name, objs in groups.items():
        assert objs, f"{lod} {name}: no objects"
        # The assembled mesh and the part meshes overlap, so export each from fresh copies of the scene objects.
        copies = []
        for o in objs:
            c = o.copy()
            c.data = o.data.copy()
            bpy.context.scene.collection.objects.link(c)
            copies.append(c)
        export(copies, os.path.join(OUT, f"{name}_{lod}.glb"))
        for c in [x for x in bpy.data.objects if x not in originals]:
            bpy.data.objects.remove(c, do_unlink=True)
    print(lod, {k: len(v) for k, v in groups.items()})

clear()
bpy.ops.import_scene.gltf(filepath=os.path.join(SRC, "Beauty_Blur_Discs.glb"))
strip_materials()
# One disc, recentred on its hub: the four discs are identical, so each prop link carries a copy.
disc = next(o for o in bpy.data.objects if o.type == "MESH" and o.name.startswith("Beauty_Blur_FL"))
for o in [o for o in bpy.data.objects if o is not disc]:
    bpy.data.objects.remove(o, do_unlink=True)
bpy.context.view_layer.objects.active = disc
disc.select_set(True)
bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
corners = [v.co for v in disc.data.vertices]
centre = sum(corners, Vector((0, 0, 0))) / len(corners)
disc.data.transform(Matrix.Translation(-centre))
export([disc], os.path.join(OUT, "BlurDisc.glb"))
