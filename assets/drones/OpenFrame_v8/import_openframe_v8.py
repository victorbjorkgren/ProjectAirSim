"""Editor job: import the OpenFrame v8 drone into the Drone plugin content at /Drone/OpenFrame_v8.

Run it in an editor that has the Drone plugin (Python console or an agent job):

    import sys; sys.path.insert(0, "<fork>/assets/drones/OpenFrame_v8")
    import import_openframe_v8 as I; I.build()

Inputs: parts/*.glb (made by split_parts.py), source/Prop_CW.glb, source/Prop_CCW.glb, source/Textures,
source/UNREAL_MATERIAL_SETTINGS.json. Then copy <project>/Plugins/Drone/Content/OpenFrame_v8 into the fork's
unreal/Blocks/Plugins/Drone/Content/.

Static meshes (cooked paths /Drone/OpenFrame_v8/<name>):
    SM_OpenFrame_v8            the whole drone, static blades, one mesh (spawn_object, distant views)
    SM_OpenFrame_v8_Body       frame, electronics, camera, antenna, strap and wiring; the one box of simple collision
    SM_OpenFrame_v8_Battery    Battery + its end fold            (thermal class battery)
    SM_OpenFrame_v8_Motors     the four motor bells              (thermal class motor)
    SM_OpenFrame_v8_ESC        ESC board and capacitor           (thermal class esc)
    SM_OpenFrame_v8_Prop_CW    one rotor blade pair, origin at the hub, spin axis +Z after import
    SM_OpenFrame_v8_Prop_CCW   its mirror image
    SM_OpenFrame_v8_BlurDisc   one translucent beauty disc centred on its hub, never in SLAM captures
Every part except the props has its vertices in the body frame, so all of them sit at the link origin. Each
non-prop mesh has the glTF LOD1 as its LOD1.
"""
import json
import os

import unreal

HERE = os.path.dirname(os.path.abspath(__file__))
DIR = "/Drone/OpenFrame_v8"
STAGE = DIR + "/_stage"
MAT_DIR = DIR + "/Materials"
TEX_DIR = DIR + "/Textures"

assets = unreal.AssetToolsHelpers.get_asset_tools()
eal = unreal.EditorAssetLibrary
mel = unreal.MaterialEditingLibrary
sme = unreal.get_editor_subsystem(unreal.StaticMeshEditorSubsystem)

# (mesh name, LOD0 file, LOD1 file or None)
MESHES = [
    ("SM_OpenFrame_v8", "parts/Assembled_LOD0.glb", "parts/Assembled_LOD1.glb"),
    ("SM_OpenFrame_v8_Body", "parts/Body_LOD0.glb", "parts/Body_LOD1.glb"),
    ("SM_OpenFrame_v8_Battery", "parts/Battery_LOD0.glb", "parts/Battery_LOD1.glb"),
    ("SM_OpenFrame_v8_Motors", "parts/Motors_LOD0.glb", "parts/Motors_LOD1.glb"),
    ("SM_OpenFrame_v8_ESC", "parts/ESC_LOD0.glb", "parts/ESC_LOD1.glb"),
    ("SM_OpenFrame_v8_Prop_CW", "source/Prop_CW.glb", None),
    ("SM_OpenFrame_v8_Prop_CCW", "source/Prop_CCW.glb", None),
    ("SM_OpenFrame_v8_BlurDisc", "parts/BlurDisc.glb", None),
]
# Simple collision of the body in Unreal centimetres: Collision_Body.glb's box (HANDOFF: centre (0.005, 0, 0.030) m,
# size (0.280, 0.105, 0.180) m in the +X forward, +Z up frame; +Y flips to right and the centre has y = 0).
COLLISION_BOX = {"center": (0.5, 0.0, 3.0), "size": (28.0, 10.5, 18.0)}

# Materials that carry texture maps, with the target base names in source/Textures.
TEXTURED = {"Carbon_Fibre_2x2_PBR": "Carbon", "Velcro_LoopFabric_PBR": "Velcro"}
BLUR_ALPHA = 0.13


def _p(rel):
    return os.path.join(HERE, rel)


def _expr(m, cls, x, y, **props):
    e = mel.create_material_expression(m, cls, x, y)
    for k, v in props.items():
        e.set_editor_property(k, v)
    return e


def _fresh_material(name):
    full = f"{MAT_DIR}/{name}"
    if eal.does_asset_exist(full):
        eal.delete_asset(full)
    return assets.create_asset(name, MAT_DIR, unreal.Material, unreal.MaterialFactoryNew())


def _master_flat():
    """Default Lit, parameters BaseColor (linear), Metallic, Roughness, Specular."""
    m = _fresh_material("M_OpenFrame")
    bc = _expr(m, unreal.MaterialExpressionVectorParameter, -600, -200, parameter_name="BaseColor",
               default_value=unreal.LinearColor(0.5, 0.5, 0.5, 1))
    mel.connect_material_property(bc, "", unreal.MaterialProperty.MP_BASE_COLOR)
    for i, (n, v, prop) in enumerate((("Metallic", 0.0, unreal.MaterialProperty.MP_METALLIC),
                                      ("Roughness", 0.5, unreal.MaterialProperty.MP_ROUGHNESS),
                                      ("Specular", 0.5, unreal.MaterialProperty.MP_SPECULAR))):
        s = _expr(m, unreal.MaterialExpressionScalarParameter, -600, 0 + 150 * i, parameter_name=n, default_value=v)
        mel.connect_material_property(s, "", prop)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def _master_textured():
    """Default Lit from a base colour, a roughness and a DirectX normal map; parameters Specular, NormalFlatness."""
    m = _fresh_material("M_OpenFrame_Textured")
    S = unreal.MaterialExpressionTextureSampleParameter2D
    bc = _expr(m, S, -800, -300, parameter_name="BaseColorMap")
    mel.connect_material_property(bc, "RGB", unreal.MaterialProperty.MP_BASE_COLOR)
    rg = _expr(m, S, -800, 0, parameter_name="RoughnessMap", sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_MASKS)
    mel.connect_material_property(rg, "R", unreal.MaterialProperty.MP_ROUGHNESS)
    nm = _expr(m, S, -800, 300, parameter_name="NormalMap", sampler_type=unreal.MaterialSamplerType.SAMPLERTYPE_NORMAL)
    # Normal strength s: blend the decoded normal toward the flat normal (0, 0, 1) by 1 - s.
    up = _expr(m, unreal.MaterialExpressionConstant3Vector, -700, 440, constant=unreal.LinearColor(0.0, 0.0, 1.0, 1.0))
    fl = _expr(m, unreal.MaterialExpressionScalarParameter, -700, 560, parameter_name="NormalFlatness", default_value=0.4)
    lerp = _expr(m, unreal.MaterialExpressionLinearInterpolate, -450, 300)
    mel.connect_material_expressions(nm, "RGB", lerp, "A")
    mel.connect_material_expressions(up, "", lerp, "B")
    mel.connect_material_expressions(fl, "", lerp, "Alpha")
    norm = _expr(m, unreal.MaterialExpressionNormalize, -250, 300)
    mel.connect_material_expressions(lerp, "", norm, "")
    mel.connect_material_property(norm, "", unreal.MaterialProperty.MP_NORMAL)
    sp = _expr(m, unreal.MaterialExpressionScalarParameter, -600, 150, parameter_name="Specular", default_value=0.16)
    mel.connect_material_property(sp, "", unreal.MaterialProperty.MP_SPECULAR)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def _master_blur():
    m = _fresh_material("M_OpenFrame_Blur")
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("two_sided", True)
    c = _expr(m, unreal.MaterialExpressionConstant3Vector, -500, 0, constant=unreal.LinearColor(0.05, 0.055, 0.06, 1))
    a = _expr(m, unreal.MaterialExpressionConstant, -500, 200, r=BLUR_ALPHA)
    mel.connect_material_property(c, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(a, "", unreal.MaterialProperty.MP_OPACITY)
    mel.recompile_material(m)
    eal.save_loaded_asset(m)
    return m


def _import_texture(base, kind):
    """kind: color | normal | mask. Returns the texture asset."""
    fn = {"color": f"{base}_BaseColor.png", "normal": f"{base}_NormalDX.png", "mask": f"{base}_Roughness.png"}[kind]
    name = "T_OpenFrame_" + os.path.splitext(fn)[0]
    t = unreal.AssetImportTask()
    t.filename, t.destination_path, t.destination_name = _p("source/Textures/" + fn), TEX_DIR, name
    t.automated, t.replace_existing, t.save = True, True, False
    assets.import_asset_tasks([t])
    tex = unreal.load_asset(f"{TEX_DIR}/{name}")
    if kind == "color":
        tex.set_editor_property("srgb", True)
    else:
        tex.set_editor_property("srgb", False)
        tex.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_NORMALMAP if kind == "normal"
                                else unreal.TextureCompressionSettings.TC_MASKS)
    eal.save_loaded_asset(tex)
    return tex


def build_materials():
    """One MaterialInstanceConstant per glTF material, named after it, from UNREAL_MATERIAL_SETTINGS.json."""
    for d in (DIR, MAT_DIR, TEX_DIR):
        if not eal.does_directory_exist(d):
            eal.make_directory(d)
    with open(_p("source/UNREAL_MATERIAL_SETTINGS.json")) as f:
        spec = json.load(f)["materials"]
    flat, textured, blur = _master_flat(), _master_textured(), _master_blur()
    out = {}
    for name, s in spec.items():
        full = f"{MAT_DIR}/{name}"
        if eal.does_asset_exist(full):
            eal.delete_asset(full)
        mi = assets.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
        mi.set_editor_property("parent", textured if name in TEXTURED else flat)
        spec_val = float(s["unreal_specular"])
        mel.set_material_instance_scalar_parameter_value(mi, "Specular", spec_val)
        if name in TEXTURED:
            base = TEXTURED[name]
            mel.set_material_instance_texture_parameter_value(mi, "BaseColorMap", _import_texture(base, "color"))
            mel.set_material_instance_texture_parameter_value(mi, "RoughnessMap", _import_texture(base, "mask"))
            mel.set_material_instance_texture_parameter_value(mi, "NormalMap", _import_texture(base, "normal"))
            mel.set_material_instance_scalar_parameter_value(mi, "NormalFlatness", 1.0 - float(s["normal_strength"]))
        else:
            r, g, b = s["base_color_linear"]
            mel.set_material_instance_vector_parameter_value(mi, "BaseColor", unreal.LinearColor(r, g, b, 1.0))
            mel.set_material_instance_scalar_parameter_value(mi, "Metallic", float(s["metallic"]))
            mel.set_material_instance_scalar_parameter_value(mi, "Roughness", float(s["roughness"]))
        eal.save_loaded_asset(mi)
        out[name] = mi
    mi = assets.create_asset("MI_OpenFrame_Blur", MAT_DIR, unreal.MaterialInstanceConstant,
                             unreal.MaterialInstanceConstantFactoryNew()) \
        if not eal.does_asset_exist(f"{MAT_DIR}/MI_OpenFrame_Blur") else unreal.load_asset(f"{MAT_DIR}/MI_OpenFrame_Blur")
    mi.set_editor_property("parent", blur)
    eal.save_loaded_asset(mi)
    out["__blur__"] = mi
    return out


def _import_stage(path):
    t = unreal.AssetImportTask()
    t.filename, t.destination_path = path, STAGE
    t.automated, t.replace_existing, t.save = True, True, False
    assets.import_asset_tasks([t])
    for p in t.imported_object_paths:
        a = unreal.load_asset(p)
        if isinstance(a, unreal.StaticMesh):
            return a
    raise RuntimeError(f"import of {path} produced no static mesh: {list(t.imported_object_paths)}")


def _move(sm, name):
    target = f"{DIR}/{name}"
    if eal.does_asset_exist(target):
        eal.delete_asset(target)
    if not eal.rename_asset(sm.get_path_name().split(".")[0], target):
        raise RuntimeError(f"cannot move {sm.get_path_name()} to {target}")
    return unreal.load_asset(target)


def _assign_materials(sm, mats, name):
    slots = [str(s.material_slot_name) for s in sm.static_materials]
    for i, slot in enumerate(slots):
        key = "__blur__" if name.endswith("BlurDisc") else slot
        if key not in mats:
            raise RuntimeError(f"{name}: slot {i} '{slot}' has no material")
        sm.set_material(i, mats[key])
    return slots


def _collision(sm):
    unreal.EditorStaticMeshLibrary.remove_collisions(sm)
    box = unreal.KBoxElem()
    box.set_editor_property("center", unreal.Vector(*COLLISION_BOX["center"]))
    box.set_editor_property("x", COLLISION_BOX["size"][0])
    box.set_editor_property("y", COLLISION_BOX["size"][1])
    box.set_editor_property("z", COLLISION_BOX["size"][2])
    body = sm.get_editor_property("body_setup")
    agg = body.get_editor_property("agg_geom")
    agg.set_editor_property("box_elems", [box])
    body.set_editor_property("agg_geom", agg)
    body.set_editor_property("collision_trace_flag", unreal.CollisionTraceFlag.CTF_USE_SIMPLE_AND_COMPLEX)
    for step in ("invalidate_physics_data", "create_physics_meshes"):
        try:
            getattr(body, step)()
        except Exception as e:
            print("collision rebuild step skipped:", step, e)


def build_meshes(mats):
    if not eal.does_directory_exist(STAGE):
        eal.make_directory(STAGE)
    for name, lod0, lod1 in MESHES:
        sm = _move(_import_stage(_p(lod0)), name)
        nan = sm.get_editor_property("nanite_settings")
        nan.set_editor_property("enabled", False)
        sm.set_editor_property("nanite_settings", nan)
        if lod1:
            if not sme.import_lod(sm, 1, _p(lod1)) and sme.get_lod_count(sm) < 2:
                raise RuntimeError(f"{name}: LOD1 import of {lod1} failed")
        if name.endswith("_Body"):
            _collision(sm)
        else:
            unreal.EditorStaticMeshLibrary.remove_collisions(sm)
        slots = _assign_materials(sm, mats, name)
        eal.save_loaded_asset(sm)
        print("mesh", name, "lods", sme.get_lod_count(sm), "slots", slots)
    for p in eal.list_assets(DIR, recursive=True):
        if p.startswith(STAGE):
            eal.delete_asset(p)
    if eal.does_directory_exist(STAGE):
        eal.delete_directory(STAGE)


def describe():
    for name, _, _ in MESHES:
        sm = unreal.load_asset(f"{DIR}/{name}")
        b = sm.get_bounds()
        lods = sme.get_lod_count(sm)
        tris = [unreal.EditorStaticMeshLibrary.get_number_triangles(sm, i) for i in range(lods)] \
            if hasattr(unreal.EditorStaticMeshLibrary, "get_number_triangles") else "?"
        mn = b.origin - b.box_extent
        mx = b.origin + b.box_extent
        print("describe", name, "min cm", mn, "max cm", mx, "lods", lods, "tris", tris,
              "simple collisions", unreal.EditorStaticMeshLibrary.get_simple_collision_count(sm))


def build():
    mats = build_materials()
    build_meshes(mats)
    describe()
