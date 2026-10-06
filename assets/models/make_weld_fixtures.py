"""Generates the signed-zero weld fixtures next to this script.

FaceList::LoadSTL welds coincident vertices through an unordered_map keyed on
exact position. -0.0f and 0.0f compare equal but have different bit patterns, so
a hash over the raw bits puts them in different buckets and the corner welds into
two vertices instead of one.

The bug only shows up once the map has enough buckets for the two hashes to stop
colliding -- a two-triangle file welds correctly by luck and proves nothing, which
is why the fan below is 200 triangles.

  weld_signed_zero.stl          apex spelled +0.0 on even facets, -0.0 on odd
  weld_signed_zero_control.stl  identical geometry, apex always +0.0

Both must weld to 201 vertices (200 rim points + 1 shared apex). Before the fix
the mixed file welds to 202.
"""

import math
import os
import struct

TRIANGLE_COUNT = 200


def write_binary_stl(path, triangles):
    out = b"\0" * 80 + struct.pack("<I", len(triangles))
    for tri in triangles:
        out += struct.pack("<3f", 0.0, 0.0, 1.0)  # normal, recomputed on export
        for vertex in tri:
            out += struct.pack("<3f", *vertex)
        out += struct.pack("<H", 0)
    open(path, "wb").write(out)


def build_fan(mixed_signs):
    """Triangle fan around a shared apex at the origin, all rim points distinct."""
    rim = [
        (math.cos(2 * math.pi * i / TRIANGLE_COUNT) + 2.0,
         math.sin(2 * math.pi * i / TRIANGLE_COUNT),
         0.0)
        for i in range(TRIANGLE_COUNT)
    ]

    triangles = []
    for i in range(TRIANGLE_COUNT):
        apex_x = -0.0 if (mixed_signs and i % 2) else 0.0
        triangles.append([(apex_x, 0.0, 0.0), rim[i], rim[(i + 1) % TRIANGLE_COUNT]])
    return triangles


if __name__ == "__main__":
    here = os.path.dirname(os.path.abspath(__file__))
    write_binary_stl(os.path.join(here, "weld_signed_zero.stl"), build_fan(True))
    write_binary_stl(os.path.join(here, "weld_signed_zero_control.stl"), build_fan(False))
    print(f"wrote both fans, {TRIANGLE_COUNT} triangles each, correct weld = {TRIANGLE_COUNT + 1} vertices")
