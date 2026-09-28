#!/usr/bin/env python3
"""Generate dark-industrial line icons (vector drawables) for the GK modules.

Each icon is a 24x24 viewport, cyan stroke, transparent fill, sized 24dp
in the app. We keep paths simple and geometric.
"""
import os

OUT = os.path.join(os.path.dirname(__file__), "..", "android", "app", "src",
                   "main", "res", "drawable")
OUT = os.path.abspath(OUT)

HEAD = '''<?xml version="1.0" encoding="utf-8"?>
<vector xmlns:android="http://schemas.android.com/apk/res/android"
    android:width="24dp"
    android:height="24dp"
    android:viewportWidth="24"
    android:viewportHeight="24">
'''
FOOT = '</vector>\n'
STROKE = ('    <path android:strokeColor="#00E5C7" android:strokeWidth="1.7" '
          'android:strokeLineCap="round" android:strokeLineJoin="round" '
          'android:fillColor="#00000000" android:pathData="%s" />\n')
FILL = ('    <path android:fillColor="#00E5C7" android:pathData="%s" />\n')

# name -> list of (kind, pathdata)
ICONS = {
 "ic_mod_gcode":    [("s", "M4,6 L20,6 M4,12 L20,12 M4,18 L14,18")],
 "ic_mod_mcode":    [("s", "M5,5 L5,19 M5,12 L13,5 L13,19 M13,12 L19,7 M13,16 L19,19")],
 "ic_mod_macro":    [("s", "M9,5 L5,9 L9,13 L5,17 M15,5 L19,9 L15,13 L19,17")],
 "ic_mod_motion":   [("s", "M3,18 L9,9 L14,14 L21,4"), ("f", "M21,4 m-1.6,0 a1.6,1.6 0 1,0 3.2,0 a1.6,1.6 0 1,0 -3.2,0")],
 "ic_mod_material": [("s", "M12,3 L21,8 L21,16 L12,21 L3,16 L3,8 Z M3,8 L12,13 L21,8 M12,13 L12,21")],
 "ic_mod_collision":[("s", "M12,3 L20,7 L20,13 C20,18 16,21 12,22 C8,21 4,18 4,13 L4,7 Z"), ("s", "M12,9 L12,14 M12,16.5 L12,17")],
 "ic_mod_machine":  [("s", "M4,20 L20,20 M6,20 L6,9 L18,9 L18,20 M6,9 L18,4 L18,9 M10,12 L14,12 L14,20")],
 "ic_mod_cnc":      [("s", "M5,5 L19,5 L19,19 L5,19 Z M9,9 L15,9 L15,15 L9,15 Z")],
 "ic_mod_action":   [("s", "M7,11 L7,6 A1.6,1.6 0 0 1 10.2,6 L10.2,10 L10.2,5 A1.6,1.6 0 0 1 13.4,5 L13.4,10 L13.4,6 A1.6,1.6 0 0 1 16.6,6 L16.6,14 C16.6,18 14,20 11,20 C8,20 6,18 5,15 L4,12")],
 "ic_mod_measure":  [("s", "M3,9 L21,9 L21,15 L3,15 Z M7,9 L7,12 M11,9 L11,12 M15,9 L15,12 M19,9 L19,12")],
 "ic_mod_teach":    [("s", "M4,5 L11,5 L12,7 L20,7 L20,19 L4,19 Z M4,5 L4,19")],
 "ic_mod_brain":    [("s", "M12,5 C9,3 5,5 5,9 C3,10 3,15 6,16 C6,20 11,20 12,18 C13,20 18,20 18,16 C21,15 21,10 19,9 C19,5 15,3 12,5 Z M12,5 L12,18")],
 "ic_mod_fault":    [("s", "M12,3 L22,20 L2,20 Z M12,9 L12,14 M12,16.5 L12,17")],
 "ic_mod_ui":       [("s", "M3,4 L21,4 L21,17 L3,17 Z M3,8 L21,8 M6,6 L6.01,6"), ("f", "M7,10 h4 v4 h-4 z")],
 "ic_mod_panel":    [("s", "M4,5 L20,5 L20,19 L4,19 Z M4,10 L20,10 M11,10 L11,19 M4,15 L20,15")],
 "ic_mod_editor":   [("s", "M15,4 L19,8 L9,18 L4,19 L5,14 Z M13,6 L17,10")],
 "ic_mod_chat":     [("s", "M4,5 L20,5 L20,16 L12,16 L8,20 L8,16 L4,16 Z")],
 "ic_mod_data":     [("s", "M4,6 C4,4.5 7.6,3.5 12,3.5 C16.4,3.5 20,4.5 20,6 L20,18 C20,19.5 16.4,20.5 12,20.5 C7.6,20.5 4,19.5 4,18 Z M4,6 C4,7.5 7.6,8.5 12,8.5 C16.4,8.5 20,7.5 20,6 M4,12 C4,13.5 7.6,14.5 12,14.5 C16.4,14.5 20,13.5 20,12")],
 "ic_mod_chart":    [("s", "M4,4 L4,20 L20,20 M8,20 L8,12 M12,20 L12,8 M16,20 L16,15")],
 "ic_mod_iot":      [("s", "M12,12 m-2,0 a2,2 0 1,0 4,0 a2,2 0 1,0 -4,0 M12,12 C7,12 5,9 5,5 M12,12 C17,12 19,9 19,5 M12,12 C7,12 5,15 5,19 M12,12 C17,12 19,15 19,19")],
 "ic_mod_ai":       [("s", "M6,6 L18,6 L18,18 L6,18 Z M9,3 L9,6 M15,3 L15,6 M9,18 L9,21 M15,18 L15,21 M3,9 L6,9 M3,15 L6,15 M18,9 L21,9 M18,15 L21,15 M10,10 L10,14 M14,10 L14,14")],
 "ic_mod_people":   [("s", "M8,10 m-3,0 a3,3 0 1,0 6,0 a3,3 0 1,0 -6,0 M3,20 C3,16 5.5,14 8,14 C10.5,14 13,16 13,20 M16,9 m-2.3,0 a2.3,2.3 0 1,0 4.6,0 a2.3,2.3 0 1,0 -4.6,0 M15,14 C17.5,14 21,15.5 21,19")],
 "ic_mod_eco":      [("s", "M12,3 L20,7 L20,15 L12,21 L4,15 L4,7 Z M9,12 L11.5,14.5 L16,9")],
 "ic_mod_physics":  [("s", "M12,12 m-2,0 a2,2 0 1,0 4,0 a2,2 0 1,0 -4,0 M12,4 C16,4 18,12 18,12 C18,12 16,20 12,20 C8,20 6,12 6,12 C6,12 8,4 12,4 Z M4,12 L20,12")],
 "ic_mod_flask":    [("s", "M9,3 L15,3 M10,3 L10,10 L5,19 C4.5,20 5,21 6,21 L18,21 C19,21 19.5,20 19,19 L14,10 L14,3 M8,15 L16,15")],
 "ic_mod_flow":     [("s", "M5,4 L10,4 L10,9 L5,9 Z M14,15 L19,15 L19,20 L14,20 Z M7.5,9 L7.5,17.5 L14,17.5 M16.5,15 L16.5,6.5 L10,6.5")],
 "ic_mod_cost":     [("s", "M12,4 L12,20 M15.5,7 C15.5,5.3 13.9,4 12,4 C10.1,4 8.5,5.3 8.5,7 C8.5,9 10,9.6 12,10.2 C14,10.8 15.5,11.5 15.5,13.5 C15.5,15.5 13.9,16.8 12,16.8 C10.1,16.8 8.5,15.5 8.5,13.8")],
 "ic_mod_wrench":   [("s", "M15,3 C12,3 10,5 10,8 C10,9 10.3,9.8 10.8,10.5 L4,17.3 L6.7,20 L13.5,13.2 C14.2,13.7 15,14 16,14 C19,14 21,12 21,9 C21,8 20.7,7 20.2,6.3 L17,9.5 L14.5,7 L17.7,3.8 C17,3.3 16,3 15,3 Z")],
 "ic_mod_tool":     [("s", "M12,2 L12,10 M9.5,10 L14.5,10 L13.5,16 L10.5,16 Z M10.5,16 L11.2,21 L12.8,21 L13.5,16"), ("s", "M7,5 L17,5 M7,7 L17,7")],
 "ic_mod_shield":   [("s", "M12,3 L20,6 L20,12 C20,17 16.5,20.5 12,21.5 C7.5,20.5 4,17 4,12 L4,6 Z M9,12 L11.3,14.5 L15.5,9.5")],
 "ic_mod_dice":     [("s", "M4,6 L20,6 L20,20 L4,20 Z M8,10 L8.01,10 M12,10 L12.01,10 M16,10 L16.01,10 M8,16 L8.01,16 M12,16 L12.01,16 M16,16 L16.01,16")],
 "ic_mod_eye":      [("s", "M2,12 C6,6 18,6 22,12 C18,18 6,18 2,12 Z M12,12 m-3,0 a3,3 0 1,0 6,0 a3,3 0 1,0 -6,0")],
 "ic_mod_media":    [("s", "M3,6 L21,6 L21,18 L3,18 Z M3,9 L21,9 M3,15 L21,15 M7,6 L7,18 M17,6 L17,18")],
 "ic_mod_history":  [("s", "M12,3 A9,9 0 1 1 3,12 M3,12 L3,7 M3,12 L8,12 M12,7 L12,12 L16,14")],
 "ic_mod_scale":    [("s", "M12,4 L12,20 M6,20 L18,20 M4,8 L12,5 L20,8 M4,8 L2,13 L6,13 Z M20,8 L18,13 L22,13 Z")],
 "ic_mod_waves":    [("s", "M3,9 C6,5 9,5 12,9 C15,13 18,13 21,9 M3,15 C6,11 9,11 12,15 C15,19 18,19 21,15")],
 "ic_mod_cube":     [("s", "M12,3 L21,8 L21,16 L12,21 L3,16 L3,8 Z M3,8 L12,13 L21,8 M12,13 L12,21 M7.5,5.5 L16.5,10.5")],
 "ic_mod_cloud":    [("s", "M7,18 C4,18 2,16 2,13.5 C2,11 4,9 6.5,9 C7,5.5 10,3 13.5,3 C17,3 20,6 20,9.5 C22,9.5 23,11 23,13 C23,15.5 21,18 18,18 Z")],
 "ic_mod_business": [("s", "M3,7 L21,7 L21,20 L3,20 Z M8,7 L8,4 L16,4 L16,7 M3,12 L21,12 M11,12 L13,12 L13,15 L11,15 Z")],
 "ic_mod_a11y":     [("s", "M12,4 m-2,0 a2,2 0 1,0 4,0 a2,2 0 1,0 -4,0 M4,8 L20,8 M12,8 L12,14 M12,14 L8,21 M12,14 L16,21")],
 "ic_mod_sigma":    [("s", "M18,5 L7,5 L14,12 L7,19 L18,19")],
 "ic_mod_hil":      [("s", "M4,8 L12,4 L20,8 L20,16 L12,20 L4,16 Z M9,12 L15,12 M12,9 L12,15")],
 "ic_mod_academic": [("s", "M12,4 L22,9 L12,14 L2,9 Z M6,11 L6,16 C6,17.5 8.7,19 12,19 C15.3,19 18,17.5 18,16 L18,11")],
 "ic_mod_compliance":[("s", "M5,3 L16,3 L19,6 L19,21 L5,21 Z M16,3 L16,6 L19,6 M8,11 L16,11 M8,15 L16,15 M8,7 L11,7")],
 "ic_mod_star":     [("s", "M12,3 L14.6,9.3 L21.5,9.8 L16.3,14.3 L18,21 L12,17.4 L6,21 L7.7,14.3 L2.5,9.8 L9.4,9.3 Z")],
 "ic_mod_spark":    [("s", "M12,3 L13.5,9 L19,7 L15,12 L20,15 L14,15 L15,21 L11,15 L5,18 L9,13 L3,12 L9,10 Z")],
 "ic_mod_spark2":   [("s", "M12,2 L12,8 M12,16 L12,22 M2,12 L8,12 M16,12 L22,12 M5,5 L9,9 M15,15 L19,19 M19,5 L15,9 M9,15 L5,19")],
}


def main():
    os.makedirs(OUT, exist_ok=True)
    count = 0
    for name, parts in ICONS.items():
        body = "".join(STROKE % p if k == "s" else FILL % p for k, p in parts)
        with open(os.path.join(OUT, name + ".xml"), "w", encoding="utf-8") as f:
            f.write(HEAD + body + FOOT)
        count += 1
    print(f"wrote {count} icons to {OUT}")


if __name__ == "__main__":
    main()
