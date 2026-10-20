# The OpenFrame v8 drone (the default quadrotor)

The default quadrotor robot configs (`robot_quadrotor_fastphysics*.jsonc`, `robot_quadrotor_manual_*.jsonc`,
`robot_quadrotor_nonphysics*.jsonc` in `acquisition/sim_config` and `client/python/example_user_scripts/sim_config`)
fly a 7-inch open-frame FPV-style quad, **OpenFrame v8**: carbon plates and arms, a blue shrink-wrapped battery on a
velcro strap, an open electronics stack, an integrated camera nose and a rear antenna mast. The PX4, MATLAB and
ArduPilot configs keep the old `Quadrotor1` meshes and rotor geometry, because those controllers are tuned for them.
`Quadrotor1`, `Propeller*` and the other stock meshes stay in the Drone plugin for any config that names them.

## Content

Source package: `assets/drones/OpenFrame_v8/source/` (GLBs, Blender source, textures, the designer's HANDOFF notes).
`split_parts.py` (Blender) cuts it into the meshes below, `import_openframe_v8.py` (Unreal editor Python) imports them
into `unreal/Blocks/Plugins/Drone/Content/OpenFrame_v8/` and builds the materials from `UNREAL_MATERIAL_SETTINGS.json`;
`apply_to_configs.py` rewrites the robot configs. All three are repeatable.

| Cooked path (`/Drone/OpenFrame_v8/...`) | What it is |
| --- | --- |
| `SM_OpenFrame_v8` | the whole drone with static blades, one mesh (`spawn_object`, distant views, props) |
| `SM_OpenFrame_v8_Body` | frame, electronics, camera, antenna, strap, wiring; simple collision is the body box (0.280 x 0.105 x 0.180 m, centre 0.005, 0, 0.030) plus one sphere of the prop radius (0.0889 m) at each rotor hub (x, y = ±0.145, z up 0.006), so the collision covers the 0.47 x 0.47 m prop sweep, plan-view radius 0.294 m |
| `SM_OpenFrame_v8_Battery` | the battery and its end fold |
| `SM_OpenFrame_v8_Motors` | the four motor bells |
| `SM_OpenFrame_v8_ESC` | the ESC board and its capacitor |
| `SM_OpenFrame_v8_Prop_CW`, `_Prop_CCW` | one rotor's blades each, origin at the hub, spin axis +Z |
| `SM_OpenFrame_v8_BlurDisc` | one translucent beauty disc centred on its hub (media shots only; never in SLAM captures) |

Every mesh except the props has its vertices in the body frame, so the part links need no offset. All but the props
and the disc carry the designer's LOD1 as their LOD1 (19.7k triangles at LOD0, 5.5k at LOD1 for the whole drone).
Splitting the heat-carrying parts into meshes of their own is what lets a thermal camera give the battery, the motors
and the ESC their own classes (one static mesh is one thermal class); a robot config carries them as the visual-only
links `Battery`, `Motors` and `ESC`, fixed to `Frame`.

Materials (`/Drone/OpenFrame_v8/Materials`) are material instances of two small masters, named after the glTF
materials: velcro specular 0, roughness 1; carbon texture tiling 1 (the weave scale is in the UVs), normal strength
0.65; hardware metallic 1, roughness 0.28; motors metallic 0.85, roughness 0.36; battery wrap metallic 0, roughness
0.18, specular 0.5. The optional clearcoat and anisotropy of the glTF are not carried over.

## Axes and the rotor frame

The glTF is metres, +Y up, facing +X; the Unreal importer converts it once (nothing is converted in the configs).
HANDOFF gives the hubs at z = -0.006 m in a **Z-up** body frame, that is 6 mm *below* the frame origin. The robot
configs use the NED body frame, where down is positive, so the hubs are at **z = +0.006**:

| Rotor | NED x | NED y | z | Mesh | `turning-direction` |
| --- | --- | --- | --- | --- | --- |
| Prop_FL | 0.145 | -0.145 | 0.006 | `Prop_CW` | clock-wise |
| Prop_FR | 0.145 | 0.145 | 0.006 | `Prop_CCW` | counter-clock-wise |
| Prop_RL | -0.145 | -0.145 | 0.006 | `Prop_CCW` | counter-clock-wise |
| Prop_RR | -0.145 | 0.145 | 0.006 | `Prop_CW` | clock-wise |

The sim-flight mixer fixes which actuator turns which way (FR and RL counter-clock-wise), so the rotors keep the
spin directions of the old config and each corner carries the blade handedness that turns that way (measured in a
running scene: the prop of a `clock-wise` rotor turns clockwise seen from above). The designer's HANDOFF lists the
opposite assignment (FL counter-clock-wise), which only changes which of the two mirrored meshes sits at a corner. Motor spacing is 0.290 m, prop radius 0.0889 m (7 inch).

The nose camera mount (`Mount_Camera`) is at NED (0.100, 0, 0.024), looking +X.

## Mass, inertia and rotor numbers

`fast-physics` takes the mass of the root link only and uses its inertia as the body's about the origin, plus the
other links through the parallel-axis theorem. The body frame origin is taken as the centre of gravity (the real
one is about 5 mm behind and 2 cm above it; that is not modelled).

Mass (kg), estimated from the part sizes and typical 7-inch components (the battery is the 145 x 58 x 93 mm block of
the model, at about 0.7 g/cm3 for a Li-ion pack of that size):

| Part | kg |
| --- | --- |
| battery with wrap | 0.550 |
| four motors (45 g each, 2806.5 class) | 0.180 |
| carbon plates, arms, standoffs | 0.120 |
| ESC, flight controller, video transmitter, receiver | 0.060 |
| camera, antenna, wiring, hardware | 0.050 |
| four props (6 g each) | 0.024 |
| **all-up weight (root link `Frame`)** | **0.984** |

The `Frame` inertia is the body without motors and props (battery 0.55 kg as a box, plates as a box, arms as rods,
electronics as a box, all about the frame origin): `ixx 0.0020, iyy 0.0032, izz 0.0026 kg m^2`. Each prop link is a
point mass of 0.051 kg (motor plus prop) at its hub, which adds `0.0043, 0.0043, 0.0086` to the total, so the flight
model sees `Ixx 0.0063, Iyy 0.0075, Izz 0.0112 kg m^2`. Drag uses a 0.280 x 0.105 x 0.130 m box (the body as seen by
the wind) with the stock coefficient 0.325, and 0.0889 m radius discs for the props.

Rotors (`rotor-settings`): `propeller-diameter 0.1778`, `max-rpm 16800`, `coeff-of-thrust 0.06`,
`coeff-of-torque 0.015`, `smoothing-tc 0.005` (unchanged). With `T = CT rho n^2 D^4` that is 5.76 N per rotor at full
command (23 N in all, thrust to weight 2.4, hover at 42 % command), and a reaction torque of 0.041 N m per rotor at full
command. The thrust-to-weight is deliberately modest: the simple-flight gains are tuned against the old 1 kg quad
(1.7 thrust-to-weight, 4.2 N per rotor), and a real 7-inch quad with several times that would need retuned gains.
The coefficients are stand-ins chosen for that, not measured propeller data.

## Checks

Flown through the client API with `simple-flight` (hover, velocity steps, a yaw-rate step, a fast forward leg), the new
and the old (`Quadrotor1`, 1.0 kg) default behave alike: hover holds 0.005 m (std) with no tilt or drift; a 4 m/s step
reaches 90 % in 2-3 s without overshoot (pitch 20 degrees at the peak); a yaw-rate command of 90 deg/s is followed;
a 12 m/s command settles at 11.5 m/s (old: 11.7) with the 32.7 degree tilt limit reached on the way and no oscillation
(gyro rms 0.001 rad/s); a 20 m/s command settles at 18.5 m/s at the tilt limit. The packaged NordicProvingGround build
gives the same numbers. The drone's own nose camera (640x480, 90 degrees, at `Mount_Camera`) sees no blade, arm or
housing.

## Regenerating

```
Blender -b --python assets/drones/OpenFrame_v8/split_parts.py -- assets/drones/OpenFrame_v8/source assets/drones/OpenFrame_v8/parts
# editor (Python console or agent job), Drone plugin loaded:
#   sys.path.insert(0, "<fork>/assets/drones/OpenFrame_v8"); import import_openframe_v8 as I; I.build()
python3 assets/drones/OpenFrame_v8/apply_to_configs.py <robot configs...>
```
