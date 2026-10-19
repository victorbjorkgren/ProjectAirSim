"""Point the default quadrotor robot configs at the OpenFrame v8 drone.

    python3 apply_to_configs.py <config.jsonc> [...]

Rewrites only the "links", "joints" and "actuators" arrays of each file (sensors, controller, battery and comments
elsewhere stay as they are). Sections that are missing in a file (non-physics configs have no actuators, no inertia)
are left missing. Idempotent. The numbers and their sources are in docs/openframe_v8.md.
"""
import copy
import json
import re
import sys

MESH = "/Drone/OpenFrame_v8/"
HUB_XY, HUB_Z = 0.145, 0.006      # NED body frame: z is down, so the hubs sit 6 mm below the frame origin
PROP_RADIUS, PROP_DIAMETER = 0.0889, 0.1778
CORNERS = {"FL": (1, -1), "FR": (1, 1), "RL": (-1, -1), "RR": (-1, 1)}   # (sign x forward, sign y right)
# The mixer of simple-flight fixes which rotors turn which way (FR and RL counter-clock-wise); the meshes follow it.
DEFAULT_DIRECTION = {"FL": "clock-wise", "FR": "counter-clock-wise", "RL": "counter-clock-wise", "RR": "clock-wise"}

AUW_KG = 0.984
FRAME_INERTIA = {"type": "matrix", "ixx": 0.0020, "iyy": 0.0032, "izz": 0.0026}
FRAME_BOX = "0.280 0.105 0.130"
MOTOR_PROP_KG = 0.051
ROTOR = {"coeff-of-thrust": 0.06, "coeff-of-torque": 0.015, "max-rpm": 16800.0, "propeller-diameter": PROP_DIAMETER}
# Visual-only links carrying the parts the thermal camera classifies separately.
PART_LINKS = [("Battery", "SM_OpenFrame_v8_Battery"), ("Motors", "SM_OpenFrame_v8_Motors"),
              ("ESC", "SM_OpenFrame_v8_ESC")]


def strip_comments(text):
    out, i, n, in_str = [], 0, len(text), False
    while i < n:
        c = text[i]
        if in_str:
            out.append(c)
            if c == "\\":
                out.append(text[i + 1]); i += 1
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True; out.append(c)
        elif text.startswith("//", i):
            while i < n and text[i] != "\n":
                i += 1
            continue
        elif text.startswith("/*", i):
            i = text.index("*/", i) + 2
            continue
        else:
            out.append(c)
        i += 1
    return re.sub(r",(\s*[\]}])", r"\1", "".join(out))


def array_span(text, key):
    """(start, end) of the top-level array value of "key", end exclusive; comments and strings are skipped."""
    m = re.search(r'^( +)"%s"\s*:\s*\[' % re.escape(key), text, re.M)
    if not m:
        return None
    i, depth, in_str = m.end() - 1, 0, False
    while i < len(text):
        c = text[i]
        if in_str:
            if c == "\\":
                i += 1
            elif c == '"':
                in_str = False
        elif c == '"':
            in_str = True
        elif text.startswith("//", i):
            i = text.index("\n", i)
            continue
        elif c == "[":
            depth += 1
        elif c == "]":
            depth -= 1
            if depth == 0:
                return m.end() - 1, i + 1, len(m.group(1))
        i += 1
    raise ValueError(f"unterminated array {key}")


def dump(obj, base):
    """The array as JSON with two-space steps, its lines indented to the key's column."""
    return json.dumps(obj, indent=2).replace("\n", "\n" + " " * base)


def xyz(corner, z=HUB_Z):
    sx, sy = CORNERS[corner]
    return f"{sx * HUB_XY:g} {sy * HUB_XY:g} {z:g}"


def corner_of(name):
    m = re.search(r"Prop_(FL|FR|RL|RR)", name)
    return m.group(1) if m else None


def patch_links(links, directions, physics):
    out = []
    for link in links:
        link = copy.deepcopy(link)
        name = link["name"]
        corner = corner_of(name)
        if name == "Frame":
            if "visual" in link:
                link["visual"]["geometry"]["name"] = MESH + "SM_OpenFrame_v8_Body"
            if physics and "inertial" in link:
                inertial = link["inertial"]
                inertial["mass"] = AUW_KG
                inertial["inertia"] = dict(FRAME_INERTIA)
                aero = inertial.get("aerodynamics")
                if aero:
                    aero["geometry"] = {"box": {"size": FRAME_BOX}}
        elif corner:
            direction = directions.get(corner, DEFAULT_DIRECTION[corner])
            if "visual" in link:
                link["visual"]["geometry"]["name"] = MESH + (
                    "SM_OpenFrame_v8_Prop_CW" if direction == "clock-wise" else "SM_OpenFrame_v8_Prop_CCW")
            for part in ("inertial", "visual"):
                if part in link and "origin" in link[part]:
                    link[part]["origin"]["xyz"] = xyz(corner)
            if "inertial" in link:
                link["inertial"]["mass"] = MOTOR_PROP_KG
                aero = link["inertial"].get("aerodynamics")
                if aero and "cylinder" in aero.get("geometry", {}):
                    aero["geometry"]["cylinder"]["radius"] = PROP_RADIUS
        out.append(link)
    if any(l["name"] == "Frame" and "visual" in l for l in out):
        for name, mesh in PART_LINKS:
            part = {"name": name, "visual": {"geometry": {"type": "unreal_mesh", "name": MESH + mesh}}}
            if physics:
                part["inertial"] = {"mass": 0.0, "inertia": {"type": "point-mass"},
                                    "aerodynamics": {"drag-coefficient": 0.0, "type": "geometry",
                                                     "geometry": {"box": {"size": "0.001 0.001 0.001"}}}}
            else:
                part["collision"] = {"enabled": False}
            out = [l for l in out if l["name"] != name] + [part]
    return out


def patch_joints(joints, links):
    out = []
    for joint in joints:
        joint = copy.deepcopy(joint)
        corner = corner_of(joint.get("child-link", ""))
        if corner and "origin" in joint:
            joint["origin"]["xyz"] = xyz(corner)
        out.append(joint)
    have = {j["child-link"] for j in out}
    for name, _ in PART_LINKS:
        if any(l["name"] == name for l in links) and name not in have:
            out.append({"id": f"Frame_{name}", "type": "fixed", "parent-link": "Frame", "child-link": name,
                        "axis": "0 0 1"})
    return out


def patch_actuators(actuators):
    out = []
    for act in actuators:
        act = copy.deepcopy(act)
        corner = corner_of(act.get("name", ""))
        if corner:
            if "origin" in act:
                act["origin"]["xyz"] = xyz(corner)
            if "rotor-settings" in act:
                act["rotor-settings"].update(ROTOR)
        out.append(act)
    return out


def patch(text):
    data = json.loads(strip_comments(text))
    physics = data.get("physics-type") != "non-physics"
    directions = {}
    for act in data.get("actuators", []):
        corner = corner_of(act.get("name", ""))
        if corner and "rotor-settings" in act:
            directions[corner] = act["rotor-settings"]["turning-direction"]
    new = {"links": patch_links(data["links"], directions, physics)}
    if "joints" in data:
        new["joints"] = patch_joints(data["joints"], new["links"])
    if "actuators" in data:
        new["actuators"] = patch_actuators(data["actuators"])
    # Replace from the back so the earlier spans stay valid.
    spans = sorted(((array_span(text, k), k) for k in new), key=lambda t: -t[0][0])
    for (start, end, base), key in spans:
        text = text[:start] + dump(new[key], base) + text[end:]
    return text


if __name__ == "__main__":
    for path in sys.argv[1:]:
        with open(path) as f:
            old = f.read()
        updated = patch(old)
        if updated != old:
            with open(path, "w") as f:
                f.write(updated)
        print(("patched " if updated != old else "unchanged ") + path)
