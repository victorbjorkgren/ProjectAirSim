"""Builds /ProjectAirSim/Sensors/ThermalMaterial in the Unreal editor (run it as an editor Python job).

The material is a post-process material that replaces the tonemapper and writes, per pixel,
the apparent temperature in kelvin in R, the object class in G (class id / 255) and the sunlit
fraction in B. Its inputs are the lit scene colour, the GBuffer (base colour, normal, roughness,
metallic), scene depth, the custom stencil (object class), and the parameters FThermalScene sets
on the material instance each capture. The physics is in THERMAL_HLSL below.
"""
import unreal

PKG = "/ProjectAirSim/Sensors"
NAME = "ThermalMaterial"

THERMAL_HLSL = r"""
const float sigma = 5.670374e-8;
const float kPi = 3.14159265;
float depthCm = Dep.x;
float3 N = Nrm.xyz;  // world-space normal, -1..1
N = dot(N, N) < 0.01 ? float3(0, 0, 1) : normalize(N);

float4 hh = mul(float4(Parameters.SvPosition.xy, 1.0, 1.0), ResolvedView.SVPositionToTranslatedWorld);
float3 V = normalize(hh.xyz / hh.w - ResolvedView.TranslatedWorldCameraOrigin);

float air4 = AirK * AirK * AirK * AirK;

// Sky apparent temperature: the clear zenith is coldest, the horizon is close to the air.
float zen = saturate(V.z);
float skyG = lerp(0.35, 1.0, sqrt(zen));
float skyT = AirK - SkyDep * skyG;

float lumW = dot(Sc.rgb * ResolvedView.OneOverPreExposure, float3(0.2126, 0.7152, 0.0722));
bool isSky = (depthCm > 4.0e5) || (depthCm <= 0.0);

float cid = 0;
// The class is the custom stencil of the nearest custom-depth surface, valid only where that
// surface is the visible one.
if (CDp.x > 0.0 && CDp.x < depthCm * 1.01 + 2.0) cid = floor(Stn.x + 0.5);
int ci = (int)clamp(cid, 0, 255);

float4 c0 = Tex.Load(int3(ci, 0, 0));
float4 c1 = Tex.Load(int3(ci, 1, 0));
float eps = c0.x, alpha = c0.y, inertia = c0.z, conv = c0.w;
float Tint = c1.x, wInt = c1.y, snowSusc = c1.z;

if (ci == 0) {
  // No class: infer from the GBuffer. Metals reflect, everything else is a generic dielectric.
  eps = lerp(0.95, 0.15, saturate(Met.x));
  snowSusc = 1.0;
}

float rho = max(dot(Base.rgb, float3(0.2126, 0.7152, 0.0722)), 0.04);
float Elux = lumW * kPi / rho;
float fSun = clamp(Elux / (max(SunLux, 1e-3) * 1.15), 0.0, 1.1);

// Snow settles on up-facing surfaces.
float snowW = saturate(SnowCover) * snowSusc * smoothstep(0.35, 0.85, N.z);
eps = lerp(eps, 0.985, snowW);
alpha = lerp(alpha, 0.15, snowW);
inertia = lerp(inertia, 0.25, snowW);

// Surface energy balance, linearised: absorbed sun - long-wave loss to the sky = convection + radiation to the air.
float qInst = fSun * SolNow;
// Only a real sun makes the pixel's lit fraction mean sunlight: at night the light is the drones' own.
float sunUp = saturate(SolNow / 50.0);
float qSlow = lerp(0.6, fSun, 0.5 * sunUp) * SolSlow;
float absorbed = alpha * lerp(qInst, qSlow, inertia);
float fSky = saturate(0.5 * (1.0 + N.z));
fSky *= fSky;
float skyT4 = pow(AirK - SkyDep * 0.8, 4.0);
float deficit = eps * fSky * sigma * (air4 - skyT4) * (1.0 - 0.75 * inertia);
float hc = (5.7 + 3.8 * Wind) * conv;
float hr = 4.0 * eps * sigma * AirK * AirK * AirK;
float hg = 5.0 * inertia;  // conduction into the ground and the body of the object
float dT = clamp((absorbed - deficit) / (hc + hr + hg), -12.0, 35.0);
float Ts = AirK + dT;
Ts = lerp(Ts, min(Ts, 273.65), snowW);
Ts = lerp(Ts, Tint, wInt * (1.0 - snowW) * (Tint > 0.0 ? 1.0 : 0.0));

// Fresnel: smooth surfaces reflect more at grazing angles, rough ones barely.
float gloss = saturate(1.0 - Rgh.x);
float cosT = saturate(dot(-V, N));
float F0 = 1.0 - eps;
float F = F0 + (1.0 - F0) * pow(1.0 - cosT, 5.0);
float epsEff = lerp(eps, 1.0 - F, gloss * gloss);

// What the surface mirrors: sky above the horizon, ground and trees (about air temperature) below it.
float3 R = reflect(V, N);
float wSky = smoothstep(-0.1 - Rgh.x * 0.5, 0.2 + Rgh.x * 0.5, R.z);
float refSkyT = AirK - SkyDep * lerp(0.35, 1.0, sqrt(saturate(R.z)));
float Lenv = lerp(pow(AirK + 1.0, 4.0), pow(refSkyT, 4.0), wSky);

float Lsurf = epsEff * pow(Ts, 4.0) + (1.0 - epsEff) * Lenv;

// Atmosphere between the surface and the camera.
float tau = exp(-ExtinctionPerM * depthCm * 0.01);
float Lpix = tau * Lsurf + (1.0 - tau) * air4;

float Tapp = pow(Lpix, 0.25);
if (isSky) { Tapp = skyT; cid = 0; }

if (DebugMode > 0.5) {
  if (DebugMode < 1.5) return float3(Stn.x, CDp.x * 0.01, depthCm * 0.01);
  if (DebugMode < 2.5) return float3(fSun, Elux, lumW);
  if (DebugMode < 3.5) return float3(N);
  return float3(Ts, epsEff, dT);
}
return float3(Tapp, cid / 255.0, fSun);
"""


def run():
    lib = unreal.MaterialEditingLibrary
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    path = PKG + "/" + NAME
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    mat = tools.create_asset(NAME, PKG, unreal.Material, unreal.MaterialFactoryNew())
    mat.set_editor_property("material_domain", unreal.MaterialDomain.MD_POST_PROCESS)
    mat.set_editor_property("blendable_location", unreal.BlendableLocation.BL_REPLACING_TONEMAPPER)

    def node(cls, x, y):
        return lib.create_material_expression(mat, cls, x, y)

    def scene_tex(tex_id, x, y):
        n = node(unreal.MaterialExpressionSceneTexture, x, y)
        n.set_editor_property("scene_texture_id", tex_id)
        n.set_editor_property("filtered", False)
        return n

    ids = unreal.SceneTextureId
    nodes = [
        ("Sc", scene_tex(ids.PPI_POST_PROCESS_INPUT0, -1400, -600)),
        ("Dep", scene_tex(ids.PPI_SCENE_DEPTH, -1400, -450)),
        ("Base", scene_tex(ids.PPI_BASE_COLOR, -1400, -300)),
        ("Nrm", scene_tex(ids.PPI_WORLD_NORMAL, -1400, -150)),
        ("Rgh", scene_tex(ids.PPI_ROUGHNESS, -1400, 0)),
        ("Met", scene_tex(ids.PPI_METALLIC, -1400, 150)),
        ("Stn", scene_tex(ids.PPI_CUSTOM_STENCIL, -1400, 300)),
        ("CDp", scene_tex(ids.PPI_CUSTOM_DEPTH, -1400, 450)),
    ]
    scalars = ["AirK", "SkyDep", "SolNow", "SolSlow", "SunLux", "Wind", "ExtinctionPerM", "SnowCover", "DebugMode"]
    # The scalar parameter names the C++ side sets.
    param_names = {"SkyDep": "SkyDepressionK", "SolNow": "SolarNow", "SolSlow": "SolarSlow", "Wind": "WindMS"}
    defaults = {"AirK": 285.0, "SkyDep": 25.0, "SunLux": 8.0, "Wind": 2.0, "ExtinctionPerM": 0.0004}

    custom = node(unreal.MaterialExpressionCustom, -400, 0)
    custom.set_editor_property("code", THERMAL_HLSL)
    custom.set_editor_property("description", "Thermal")
    custom.set_editor_property("output_type", unreal.CustomMaterialOutputType.CMOT_FLOAT3)

    def custom_input(name):
        ci = unreal.CustomInput()
        ci.set_editor_property("input_name", name)
        return ci

    inputs = [custom_input(name) for name, _ in nodes] + [custom_input(s) for s in scalars] + [custom_input("Tex")]
    custom.set_editor_property("inputs", inputs)

    for name, n in nodes:
        lib.connect_material_expressions(n, "Color", custom, name)
    for i, s in enumerate(scalars):
        p = node(unreal.MaterialExpressionScalarParameter, -1000, 650 + i * 90)
        p.set_editor_property("parameter_name", param_names.get(s, s))
        p.set_editor_property("default_value", defaults.get(s, 0.0))
        lib.connect_material_expressions(p, "", custom, s)
    tex = node(unreal.MaterialExpressionTextureObjectParameter, -1000, 1500)
    tex.set_editor_property("parameter_name", "ClassTable")
    tex.set_editor_property("texture", unreal.load_asset("/Engine/EngineResources/WhiteSquareTexture"))
    tex.set_editor_property("sampler_type", unreal.MaterialSamplerType.SAMPLERTYPE_LINEAR_COLOR)
    lib.connect_material_expressions(tex, "", custom, "Tex")

    lib.connect_material_property(custom, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    lib.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    print("built", path)


run()
