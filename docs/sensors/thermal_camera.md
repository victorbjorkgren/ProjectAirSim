# Thermal (long-wave infrared) camera

Image types `7` (Infrared, an 8-bit display image) and `8` (Temperature, radiometric `16UC1`) model an uncooled
microbolometer camera (8-14 um) such as the ones on inspection and search drones. Both come from **one render**:
enable either or both, they share `width`, `height`, `fov-degrees` and the sensor model (the `thermal` object of
the type-7 entry; a type-8 entry alone also works). The plain pinhole lens only (no `source-sampling` above 1).

```jsonc
"capture-settings": [
  {"image-type": 7, "width": 640, "height": 512, "fov-degrees": 56, "capture-enabled": true,
   "pixels-as-float": false, "compress": false,
   "thermal": {"palette": "white-hot", "agc": "percentile", "seed": 1234, "classes-file": "/path/thermal_classes.json"}},
  {"image-type": 8, "width": 640, "height": 512, "fov-degrees": 56, "capture-enabled": true,
   "pixels-as-float": false, "compress": false}
]
```

Client: `ImageType.INFRARED` (BGR or PNG, like the scene image) and `ImageType.TEMPERATURE`
(`projectairsim.utils.unpack_temperature_kelvin(msg)` gives a float32 array in kelvin). Topics `infrared_camera`
and `temperature_camera`.

## What is rendered

A scene capture with a post-process material (`/ProjectAirSim/Sensors/ThermalMaterial`, built by
`tools/thermal/build_thermal_material.py`) that replaces the tonemapper and writes the **apparent temperature per
pixel** into a 32-bit float target. Fog, bloom, vignette, grain, motion blur, eye adaptation and fringing are off in
this capture. Per pixel the material solves a steady-state surface energy balance and then adds reflection and
atmosphere:

* **Object class** (custom stencil, one byte): emissivity, solar absorptivity, thermal inertia, convective coupling,
  an optional internal temperature (people, motors, water) and how much snow settles on it.
  Surfaces without a class are inferred from the GBuffer: metallic surfaces get a low emissivity, everything else a
  generic 0.95.
* **Sunlight from the visible render**: the lit scene colour divided by the base colour is the irradiance on that
  pixel, so cast shadows, canopy shade and the angle to the sun come from the lighting that is already there.
  It is normalised by the sun light's intensity and scaled by the solar irradiance in W/m2 on a surface facing the sun,
  which comes from the sun's elevation and the cloud cover (`FThermalEnvironment`). The sun is the directional light
  used as the atmosphere's sun light, so the clock, the date and the place are whatever the scene already uses.
* **Thermal lag**: classes with high inertia (rock, asphalt, soil, water) follow a 2 h running mean of the irradiance,
  foliage follows the instant value. At dusk the ground is still warm, and at night rock stays warmer than leaves.
  The mean starts at 0.8 of the instant value at the first capture; an environment that knows the history sets it.
* **Cold sky**: up-facing surfaces lose heat to the sky (`sky-depression` below the air, deepest at the zenith,
  shallower near the horizon, much smaller under cloud), so clear nights give ground a few kelvin under the air.
* **Reflections**: `(1 - emissivity)` of the radiance is the mirrored surroundings along the reflected view ray:
  cold sky above the horizon, ground and trees below. Smooth surfaces (GBuffer roughness) follow Fresnel, so water,
  glass and car paint turn into mirrors at grazing angles and bare metal shows the sky at any angle.
  Hot objects are not mirrored in other surfaces.
* **Atmosphere**: transmission `exp(-k d)` with `k` from the environment (haze, rain, fog) and air-temperature path radiance.
* Snow cover settles on up-facing surfaces (susceptibility per class), stays at or below 0 C, reflects sunlight and
  has an emissivity of 0.985.

## Sensor model

Runs on the packing thread after the readback, so it costs the render thread nothing: optics blur (`psf-sigma-px`),
vignette, optional edge halo, fixed-pattern noise (per-pixel gain and offset, column stripes), temporal noise
(`netd-kelvin`), 14-bit style quantisation (type 8 is 0.01 K), automatic gain control and palette (type 7).

**Display steadiness**: `percentile` AGC keeps the mapping nearly constant frame to frame (spruce lane on NordicProvingGround, TAA off: still-camera corner repeatability 0.94, a 17 deg/s turn re-finds 43% of corners, like FXAA on the colour camera). `histogram-equalization` looks closer to a drone's processed video but its mapping follows the view: 0.89 and 30% on the same tests. Pick `percentile` for SLAM and `histogram-equalization` for pictures.

**Noise is deterministic**: it derives from `seed` and the camera's frame counter only, so a run with the same seed and
the same capture count reproduces. `"noise-enabled": false` gives a clean image. Type 8 carries the same noise and
blur as type 7 (it is what a radiometric camera outputs); set `noise-enabled` false, `psf-sigma-px` 0 and
`vignette` 0 for ground truth.

| Key (`thermal` object) | Default | Meaning |
|---|---|---|
| `psf-sigma-px` | 0.8 | Gaussian blur of the optics, pixels |
| `vignette` | 0.12 | signal falloff to the corners (fraction at the corner) |
| `halo` | 0 | edge-sharpening overshoot of the camera's processing |
| `noise-enabled` | true | fixed-pattern and temporal noise |
| `seed` | 1234 | seed of both noises |
| `netd-kelvin` | 0.05 | temporal noise RMS, K |
| `fpn-gain-sigma`, `fpn-offset-kelvin`, `fpn-column-kelvin` | 0.004, 0.08, 0.05 | fixed-pattern noise after non-uniformity correction |
| `agc` | `percentile` | `percentile`, `histogram-equalization` (plateau, half blended with linear; richer contrast, less steady) or `fixed` |
| `agc-low-percent`, `agc-high-percent` | 1, 99.5 | percentiles mapped to black and white |
| `agc-min-span-kelvin` | 6 | the display never stretches less than this span (keeps a flat scene from becoming noise) |
| `agc-smoothing` | 0.9 | weight of the previous frame's range: a steady display for SLAM |
| `agc-target-median` | 0.42 | `percentile` only: display level (0..1) the scene median is moved to by a smoothed gamma, so a few cold (sky) or hot pixels at the ends of the range do not push the bulk of the picture to white or black, as in a real camera's mid-grey presentation; 0 is the plain linear stretch |
| `fixed-min-celsius`, `fixed-max-celsius` | -20, 60 | span of `agc: fixed` |
| `palette` | `white-hot` | `white-hot`, `black-hot`, `ironbow` |
| `classes-file` | none | JSON of classes, rules and environment, see below |

**Debug read-out**: with the console variable `projectairsim.ThermalDebugCycle N` (or the environment variable
`PROJECTAIRSIM_THERMAL_DEBUG_CYCLE=N` when the variable is 0) the temperature image cycles through the material's
intermediates for modes 1..N, half a second of sim time each: class id, sunlit fraction x100, albedo x100, surface
temperature, effective emissivity x100, surface minus air + 100, roughness x100, metallic x100, normal z x100 + 100,
luminance x100, and as mode 11 the normal temperature. Pixel (0,0) of a frame is 500 + its mode. Use a config with the
thermal noise and blur off, and do not use the display image while it runs. `Simulate/NordicProvingGround/sim/thermal_validate.py`
reads it. Rebuild the material with `tools/thermal/build_thermal_material.py` after changing the HLSL.

## Adopting thermal in another environment

What you get **for free** from the plugin: the render, the sun and clock from the scene's atmosphere sun light, cast
shadows and canopy shade, the cold sky, reflections from smooth surfaces, atmosphere, the sensor model, both image
types, the built-in classes and name patterns (asphalt, concrete, masonry, roof, glass, bare metal, tyre, water, snow, rock,
soil, foliage, wood, vehicle body, skin ...) and a GBuffer fallback for everything else.

What an **environment supplies**, none of it code:

1. **A mapping JSON** (`classes-file`, loaded when the camera is created). It is checked before the built-in patterns:
   ```jsonc
   {
     "environment": {"air-temp-c": 12, "sky-depression-k": 25, "cloud-cover": 0.2, "wind-speed": 2,
                     "extinction-per-m": 0.0008, "snow-cover": 0, "water-temp-c": 8,
                     "solar-slow-irradiance": 200},          // omit a field to keep the default
     "classes": {"city_brick": {"emissivity": 0.93, "solar-absorptivity": 0.7, "inertia": 0.85}},
     "rules": [
       {"class": "city_brick", "material": "*Brick*"},        // any of mesh, material, physical-material, actor (wildcards)
       {"class": "asphalt", "mesh": "SM_Road*"}
     ]
   }
   ```
   Class fields: `emissivity`, `solar-absorptivity`, `inertia` (0 follows the sun, 1 the day's mean), `convection`,
   `internal-temp-c` with `internal-weight` (a warm body: the surface is blended toward that temperature),
   `snow-susceptibility`. A rule with several fields needs all of them to match. Rules are evaluated in order, per
   primitive component, against its static mesh name, its material names (and their parents), its physical material
   names and its actor label.
2. **Tags on special actors or components**: `thermal:<class>` (for example `thermal:human`, `thermal:engine`,
   `thermal:hot_object`) overrides every rule. This is the hook for people, vehicles' engines or anything that
   generates heat; place the tag in the editor or add it at spawn (`Actor->Tags.Add("thermal:human")`).
   Components created after the scan are classified when their actor spawns.
3. **Weather**: an environment's weather code can call `FThermalScene::Get().SetEnvironment(...)` (header
   `ThermalScene.h`, exported from the ProjectAirSim module) when precipitation, snow or cloud changes. Fields left at
   their "auto" value (`SolarIrradiance`, `SolarSlowIrradiance`) come from the sun's elevation and `CloudCover`.
4. Nothing else. The project needs `r.CustomDepth` 3, which the camera sets itself when a thermal capture is enabled.

Cost: one extra scene render (about the cost of the RGB camera) plus the custom-depth pass of the classified
primitives. The scan of the world is spread over frames (2 ms budget per capture), so a city is classified over a few
seconds after the first thermal capture.

## Limits

The class is carried in the custom stencil, which the segmentation camera and the lidar's segmentation ids use too. A
primitive that the thermal scan classifies has its segmentation id overwritten (and the reverse, when ids are set
after the scan), so do not run segmentation or lidar segmentation on the same scene as a thermal camera unless the
thermal classes are limited to meshes that carry no segmentation id.

Reflections are analytic (sky versus ground by the reflection direction), not traced: a hot engine is not mirrored in a
car door. Thermal lag is a per-environment running mean, not a temperature memory per pixel, so the shadow of an object
that moved an hour ago does not linger. A class is per primitive component: a mesh that mixes materials of different
thermal behaviour gets one class (rules can match on material names, so split meshes by material where it matters). The
band is approximated as a blackbody T^4, not the 8-14 um integral.
