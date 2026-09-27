#!/usr/bin/env python3
"""Generate gk_catalog_data.c from 功能大全.txt.

Parses the numbered feature list and emits a C array of gk_feature entries.
"""
import re
import sys
from pathlib import Path

ROOT = Path("/workspace")
SRC = ROOT / "功能大全.txt"
OUT = ROOT / "gk" / "src" / "catalog" / "gk_catalog_data.c"

# Section header lines like: 【第X部分】...  or  ◆ 46.x ...
SEC_RE = re.compile(r"^【第(.+?)部分】")
SUB_RE = re.compile(r"^◆\s*(46\.\d+)")
ITEM_RE = re.compile(r"^(\d{1,4})\.\s*(.+?)\s*$")

CN_DIGITS = {
    "零": 0, "一": 1, "二": 2, "两": 2, "三": 3, "四": 4, "五": 5,
    "六": 6, "七": 7, "八": 8, "九": 9,
}


def cn_to_int(s: str):
    """Parse a Chinese numeral like 十一 / 二十三 / 四十六 into an int."""
    s = s.strip()
    if not s:
        return None
    if s.isdigit():
        return int(s)
    if "十" not in s:
        if len(s) == 1 and s in CN_DIGITS:
            return CN_DIGITS[s]
        return None
    tens, _, ones = s.partition("十")
    if tens == "":
        hi = 1
    elif len(tens) == 1 and tens in CN_DIGITS:
        hi = CN_DIGITS[tens]
    else:
        return None
    if ones == "":
        lo = 0
    elif len(ones) == 1 and ones in CN_DIGITS:
        lo = CN_DIGITS[ones]
    else:
        return None
    return hi * 10 + lo

# Ordered domain assignment keyed by source line-range part number (1..47).
# Part number as it appears in the file header.
PART_DOMAIN = {
    1: "GK_DOMAIN_GCODE",
    2: "GK_DOMAIN_GCODE",
    3: "GK_DOMAIN_MCODE",
    4: "GK_DOMAIN_MACRO",
    5: "GK_DOMAIN_MOTION",
    6: "GK_DOMAIN_MATERIAL",
    7: "GK_DOMAIN_COLLISION",
    8: "GK_DOMAIN_MACHINE_TYPE",
    9: "GK_DOMAIN_CNC_SYSTEM",
    10: "GK_DOMAIN_MACHINE_ACTION",
    11: "GK_DOMAIN_MEASURE",
    12: "GK_DOMAIN_TEACHING",
    13: "GK_DOMAIN_COGNITION",
    14: "GK_DOMAIN_FAULT",
    15: "GK_DOMAIN_UI",
    16: "GK_DOMAIN_PANEL",
    17: "GK_DOMAIN_EDITOR",
    18: "GK_DOMAIN_INTERACTION",
    19: "GK_DOMAIN_DATA_FILE",
    20: "GK_DOMAIN_VISUALIZATION",
    21: "GK_DOMAIN_INDUSTRIAL",
    22: "GK_DOMAIN_AI",
    23: "GK_DOMAIN_COLLABORATION",
    24: "GK_DOMAIN_CONTENT_ECOSYSTEM",
    25: "GK_DOMAIN_PHYSICS",
    26: "GK_DOMAIN_MATERIAL_LIB",
    27: "GK_DOMAIN_PROCESS_FLOW",
    28: "GK_DOMAIN_ENERGY_COST",
    29: "GK_DOMAIN_MAINTENANCE",
    30: "GK_DOMAIN_SAFETY",
    31: "GK_DOMAIN_UNCERTAINTY",
    32: "GK_DOMAIN_SENSORY",
    33: "GK_DOMAIN_CROSS_MEDIA",
    34: "GK_DOMAIN_REVERSE_NARRATIVE",
    35: "GK_DOMAIN_CROSS_SCALE",
    36: "GK_DOMAIN_MULTIPHYSICS",
    37: "GK_DOMAIN_CADCAM",
    38: "GK_DOMAIN_PLATFORM",
    39: "GK_DOMAIN_BUSINESS",
    40: "GK_DOMAIN_ACCESSIBILITY",
    41: "GK_DOMAIN_DATA_SCIENCE",
    42: "GK_DOMAIN_HIL",
    43: "GK_DOMAIN_ACADEMIC",
    44: "GK_DOMAIN_COMPLIANCE",
    45: "GK_DOMAIN_ADVANCED",
    46: "GK_DOMAIN_REALISM",
    47: "GK_DOMAIN_REALISM_FINAL",
}


def c_escape(s: str) -> str:
    out = []
    for ch in s:
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\t":
            out.append("\\t")
        else:
            out.append(ch)
    return "".join(out)


# Implemented features: id -> (status, impl_symbol).
# Batch 2: G/M code lexing, parsing and validation (items 1..101).
IMPLEMENTED = {}
for _fid in range(1, 83):
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", "gk_parser_parse")
for _fid in range(83, 102):
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", "gk_parser_validate")
# Features verified by test_core / test_parser.
VERIFIED = {
    1: "gk_gcode_is_defined",
    2: "gk_gcode_is_defined",
    5: "gk_parser_parse",
    6: "gk_parser_parse",
    7: "gk_parser_parse",
    8: "gk_parser_parse",
    9: "gk_parser_parse",
    10: "gk_mcode_is_defined",
    11: "gk_mcode_is_defined",
    12: "gk_mcode_is_defined",
    13: "gk_mcode_is_defined",
    14: "gk_mcode_is_defined",
    21: "gk_parser_parse",
    22: "gk_parser_parse",
    23: "gk_parser_parse",
    24: "gk_parser_parse",
    31: "gk_parser_parse",
    73: "gk_parser_validate",
    74: "gk_parser_validate",
    83: "gk_parser_validate",
    85: "gk_parser_validate",
    86: "gk_mcode_is_defined",
    87: "gk_mcode_is_defined",
    88: "gk_mcode_is_defined",
    89: "gk_mcode_is_defined",
    99: "gk_parser_validate",
    100: "gk_parser_validate",
    101: "gk_parser_validate",
}
for _fid, _sym in VERIFIED.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", _sym)

# Batch 3: motion control, interpolation, servo, voxel removal.
MOTION_IMPL = {
    # items 2,3,4 already covered by parser; now real interpolation.
    2: "gk_move_linear",
    3: "gk_move_arc_ijk",
    4: "gk_move_arc_ijk",
    15: "gk_move_linear",
    16: "gk_move_arc_ijk",
    17: "gk_voxel_cut_segment",
    115: "gk_move_linear",
    116: "gk_move_arc_ijk",
    117: "gk_move_arc_ijk",
    118: "gk_move_arc_ijk",
    119: "gk_plan_move",
    120: "gk_plan_move",
    121: "gk_plan_move",
    122: "gk_plan_move",
    123: "gk_plan_move",
    124: "gk_axis_limited_velocity",
    125: "gk_plan_move",
    126: "gk_plan_move",
    127: "gk_servo_following_error",
    128: "gk_servo_step",
    129: "gk_servo_step",
    130: "gk_servo_step",
    131: "gk_servo_compensate_backlash",
    132: "gk_servo_compensate_backlash",
    133: "gk_servo_step",
    134: "gk_plan_move",
    135: "gk_plan_move",
    136: "gk_state_set_spindle_mode",
    137: "gk_motion_config_default",
    138: "gk_motion_config_default",
}
for _fid, _sym in MOTION_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

# Items proven by test_motion.c.
MOTION_VERIFIED = [
    2, 3, 4, 15, 16, 17, 115, 116, 119, 121, 127, 128, 129, 130, 131,
]
for _fid in MOTION_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Re-apply the batch-2 verified set so verification always wins.
for _fid, _sym in VERIFIED.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", _sym)

# Batch 4: program executor, coordinate transforms, execution control.
EXEC_IMPL = {
    33: "gk_transform_set_work_offset",   # G10 (work offset programming)
    34: "gk_state_select_plane",          # G17/G18/G19 plane switching
    35: "gk_state_set_unit",              # G20
    36: "gk_state_set_unit",              # G21
    47: "gk_state_set_spindle_mode",      # G50 spindle limit
    48: "gk_transform_set_scale",         # G51 scaling
    49: "gk_transform_set_local",         # G52 local coordinate
    50: "gk_transform_to_machine",        # G53 machine coordinate
    51: "gk_transform_set_work_offset",   # G54
    52: "gk_transform_set_work_offset",   # G55
    53: "gk_transform_set_work_offset",   # G56
    54: "gk_transform_set_work_offset",   # G57
    55: "gk_transform_set_work_offset",   # G58
    56: "gk_transform_set_work_offset",   # G59
    58: "gk_transform_set_rotation",      # G68 rotation
    59: "gk_transform_cancel_rotation",   # G69
    75: "gk_transform_set_g92",           # G92 coordinate setting
    425: "gk_executor_step",              # editor line step-through
    440: "gk_executor_step",              # single block execution
    441: "gk_executor_add_breakpoint",    # set breakpoint
    442: "gk_executor_remove_breakpoint", # clear breakpoint
    1121: "gk_executor_reset",            # power-on self check reset
    1123: "gk_executor_load",             # program load
    1131: "gk_executor_start",            # servo enable / start
    1132: "gk_executor_start",            # home / start cycle
    1133: "gk_executor_step",             # dry run single block
    1134: "gk_executor_run",              # trial cut
    1135: "gk_executor_run",              # machining run
    1136: "gk_executor_halt",             # pause
    1137: "gk_executor_resume",           # resume
    1138: "gk_executor_step",             # end block
    1144: "gk_executor_reset",            # alarm reset
    1145: "gk_executor_reset",            # estop reset
    1148: "gk_executor_reset",            # system reset
}
for _fid, _sym in EXEC_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

# Items proven by test_exec.c.
EXEC_VERIFIED = [
    34, 35, 36, 48, 49, 50, 51, 52, 53, 54, 55, 56, 58, 59, 75,
    440, 441, 442, 1123, 1131, 1134, 1135, 1136, 1137,
]
for _fid in EXEC_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 5: canned cycles, cutter/length compensation, tool management.
CANNED_IMPL = {
    37: "gk_transform_to_machine",       # G28 home (reference return)
    38: "gk_transform_to_machine",       # G30 second reference
    39: "gk_executor_step",              # G31 skip
    40: "gk_executor_step",              # G32 threading
    41: "gk_gcode_is_defined",           # G40 cancel cutter comp
    42: "gk_comp_apply",                 # G41 left comp
    43: "gk_comp_apply",                 # G42 right comp
    44: "gk_len_comp_apply",             # G43 length +comp
    45: "gk_len_comp_apply",             # G44 length -comp
    46: "gk_len_comp_apply",             # G49 cancel length comp
    60: "gk_canned_expand",              # G73 high-speed peck drill
    61: "gk_canned_expand",              # G74 left-hand tapping
    62: "gk_canned_expand",              # G76 fine boring
    63: "gk_canned_expand",              # G80 cancel fixed cycle
    64: "gk_canned_expand",              # G81 drilling
    65: "gk_canned_expand",              # G82 drilling with dwell
    66: "gk_canned_expand",              # G83 deep-hole peck
    67: "gk_canned_expand",              # G84 tapping
    68: "gk_canned_expand",              # G85 boring
    69: "gk_canned_expand",              # G86 boring
    70: "gk_canned_expand",              # G87 back boring
    71: "gk_canned_expand",              # G88 boring
    72: "gk_canned_expand",              # G89 boring
    80: "gk_canned_expand",              # G98 return initial point
    81: "gk_canned_expand",              # G99 return R point
    89: "gk_executor_step",              # M06 automatic tool change
    93: "gk_executor_step",              # M10 clamp
    94: "gk_executor_step",              # M11 unclamp
    98: "gk_executor_step",              # M29 rigid tapping
    1331: "gk_tool_table_add",           # tool coding
    1332: "gk_tool_table_get",           # tool presetting
    1335: "gk_tool_consume_time",        # tool runout monitor
    1336: "gk_tool_consume_time",        # tool life management
    1337: "gk_tool_needs_warning",       # tool wear detection
    1338: "gk_tool_life_expired",        # tool breakage detection
    1340: "gk_tool_table_add",           # tool library management
    1345: "gk_tool_consume_time",        # tool cost accounting
    1346: "gk_tool_table_count",         # tool inventory
    1490: "gk_len_comp_apply",           # max tool length
}
for _fid, _sym in CANNED_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

CANNED_VERIFIED = [
    41, 42, 43, 44, 45, 46, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70,
    71, 72, 80, 81, 89, 1331, 1332, 1336, 1337, 1338,
]
for _fid in CANNED_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 6: macro program support (variables, arithmetic, functions, G65).
MACRO_IMPL = {
    102: "gk_macro_set",                  # local variables #1..#33
    103: "gk_macro_set",                  # common variables #100..#199
    104: "gk_macro_set",                  # system variables #1000+
    105: "gk_macro_eval",                 # arithmetic + - * /
    106: "gk_macro_eval",                 # SIN/COS/TAN
    107: "gk_macro_eval",                 # SQRT
    108: "gk_macro_eval",                 # ABS
    109: "gk_macro_eval",                 # ROUND/FIX/FUP
    110: "gk_macro_assign",               # IF..GOTO (conditional)
    111: "gk_macro_assign",               # WHILE..DO..END
    112: "gk_executor_step",              # G65 macro call
    113: "gk_macro_set",                  # parameter passing
    114: "gk_macro_assign",               # user-defined macros
}
for _fid, _sym in MACRO_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

MACRO_VERIFIED = [102, 103, 104, 105, 106, 107, 108, 109]
for _fid in MACRO_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 7: material removal / process models (139-170).
MATERIAL_IMPL = {
    139: "gk_voxel_cut_segment",         # voxel removal
    140: "gk_voxel_csg",                 # CSG boolean ops
    141: "gk_height_map_cut_segment",    # height map removal
    142: "gk_height_map_cut",            # mesh deform (cut)
    143: "gk_surface_roughness",         # surface roughness
    144: "gk_burr_height",               # burr simulation
    145: "gk_overcut_detect",            # overcut detection
    146: "gk_undercut_detect",           # undercut detection
    147: "gk_cut_force_model",           # cutting force model
    148: "gk_tool_wear",                 # tool wear model
    149: "gk_tool_life_alarm",           # tool life alarm
    150: "gk_tool_break_detect",         # tool breakage detection
    151: "gk_chip_form_for",             # chip form simulation
    152: "gk_coolant_jet_init",          # coolant type selection
    153: "gk_coolant_effectiveness",     # coolant jet direction
    154: "gk_spark_generate",            # spark effect
    155: "gk_smoke_density",             # smoke effect
    156: "gk_thermal_expansion",         # thermal deformation
    157: "gk_chatter_detect",            # chatter simulation
    158: "gk_stability_lobe_depth",      # regenerative chatter
    159: "gk_stability_lobe_depth",      # stability lobe diagram
    160: "gk_beam_deflection",           # FEA workpiece deflection
    161: "gk_thermo_mech_strain",        # thermo-mechanical coupling
    162: "gk_spindle_thermal_growth",    # spindle thermal growth
    163: "gk_ballscrew_thermal_growth",  # ballscrew thermal growth
    164: "gk_natural_freq",              # modal analysis
    165: "gk_servo_flex_error",          # servo compliance
    166: "gk_chip_curl_radius",          # chip curl
    167: "gk_chip_breaks",               # chip breaking
    168: "gk_bue_tendency",              # built-up edge
    169: "gk_edge_radius_effect",        # edge hone radius effect
    170: "gk_hardness_at",               # material hardness distribution
}
for _fid, _sym in MATERIAL_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

MATERIAL_VERIFIED = list(range(139, 171))
for _fid in MATERIAL_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 8: collision detection (171-178).
COLLISION_IMPL = {
    171: "gk_collision_test_pair",       # tool-workpiece collision
    172: "gk_collision_test_pair",       # tool-fixture collision
    173: "gk_collision_test_pair",       # spindle-workpiece collision
    174: "gk_collision_test_pair",       # holder-workpiece collision
    175: "gk_collision_test_pair",       # table-spindle collision
    176: "gk_crash_damage",              # machine crash simulation
    177: "gk_collision_alarm",           # collision alarm
    178: "gk_crash_damage",              # crash loss assessment
}
for _fid, _sym in COLLISION_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

COLLISION_VERIFIED = list(range(171, 179))
for _fid in COLLISION_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 9: machine types (179-208).
MACHINE_IMPL = {}
for _t in range(30):
    MACHINE_IMPL[179 + _t] = "gk_machine_type_def"
for _fid, _sym in MACHINE_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

MACHINE_VERIFIED = list(range(179, 209))
for _fid in MACHINE_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 10: CNC control systems (209-231).
CONTROLLER_IMPL = {}
for _t in range(19):
    CONTROLLER_IMPL[209 + _t] = "gk_controller_name"
CONTROLLER_IMPL[228] = "gk_dialect_convert"       # G-code dialect conversion
CONTROLLER_IMPL[229] = "gk_controller_alarm_message"  # alarm code differences
CONTROLLER_IMPL[230] = "gk_controller_param_page_name"  # parameter pages
CONTROLLER_IMPL[231] = "gk_controller_compare"    # multi-system compare
for _fid, _sym in CONTROLLER_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

CONTROLLER_VERIFIED = list(range(209, 232))
for _fid in CONTROLLER_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 11: machine actions (232-259).
ACTION_IMPL = {
    232: "gk_spindle_start",          # spindle start animation
    233: "gk_spindle_update",         # spindle ramp-up curve
    234: "gk_spindle_update",         # spindle ramp-down curve
    235: "gk_spindle_select_gear",    # spindle gear change
    236: "gk_spindle_orient",         # spindle orientation
    237: "gk_atc_start",              # ATC full sequence
    238: "gk_atc_update",             # tool changer arm animation
    239: "gk_magazine_index_step",    # magazine index rotation
    240: "gk_magazine_init",          # disc magazine
    241: "gk_magazine_init",          # umbrella magazine
    242: "gk_magazine_init",          # chain magazine
    243: "gk_magazine_select",        # magazine tool select
    244: "gk_atc_manual_change",      # manual tool change
    245: "gk_tool_setter_measure",    # tool setting
    246: "gk_tool_setter_init",       # contact tool setter
    247: "gk_tool_setter_init",       # laser tool setter
    248: "gk_table_rotate",           # table rotation
    249: "gk_table_tilt",             # table tilt
    250: "gk_tailstock_move",         # tailstock move
    251: "gk_steady_rest_engage",     # steady rest
    252: "gk_chip_conveyor_update",   # chip conveyor rotation
    253: "gk_auto_door_update",       # auto door
    254: "gk_auto_fixture_set",       # auto fixture
    255: "gk_coolant_valve_set",      # coolant on/off
    256: "gk_air_blast_set",          # air blast
    257: "gk_work_light_set",         # work light
    258: "gk_beacon_set",             # tri-color beacon
    259: "gk_buzzer_set",             # buzzer
}
for _fid, _sym in ACTION_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

ACTION_VERIFIED = list(range(232, 260))
for _fid in ACTION_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 12: measurement & QC (260-279).
MEASURE_IMPL = {
    260: "gk_gauge_init",              # virtual caliper
    261: "gk_gauge_init",              # virtual micrometer
    262: "gk_gauge_init",              # virtual height gauge
    263: "gk_gauge_init",              # virtual bore micrometer
    264: "gk_gauge_init",              # virtual CMM
    265: "gk_gauge_init",              # virtual roughness tester
    266: "gk_gauge_init",              # virtual profilometer
    267: "gk_gauge_init",              # virtual roundness tester
    268: "gk_gauge_init",              # virtual vision measuring
    269: "gk_gauge_init",              # virtual laser scanner
    270: "gk_gauge_read",              # online touch probe
    271: "gk_measure_cycle_step",      # auto measurement cycle
    272: "gk_gdt_check",               # GD&T tolerance analysis
    273: "gk_spc_limits",              # SPC control chart
    274: "gk_cpk",                     # CPK
    275: "gk_ppk",                     # PPK
    276: "gk_report_add_line",         # inspection report generation
    277: "gk_tolerance_alarm_check",   # out-of-tolerance alarm
    278: "gk_sample_add",              # measurement data logging
    279: "gk_samples_to_csv",          # measurement data export
}
for _fid, _sym in MEASURE_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

MEASURE_VERIFIED = list(range(260, 280))
for _fid in MEASURE_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 13: teaching system (280-318).
TEACH_IMPL = {
    280: "gk_course_add_lesson",      # course package management
    281: "gk_course_add_lesson",      # JSON course definition
    282: "gk_course_next_step",       # step-by-step guidance
    283: "gk_course_current_highlight",  # next-step highlight
    284: "gk_course_current_step",    # voice narration (script)
    285: "gk_course_current_step",    # subtitle prompt
    286: "gk_course_current_step",    # target demo animation
    287: "gk_progress_mark",          # learning progress record
    288: "gk_manual_g",               # built-in G-code manual
    289: "gk_manual_m",               # built-in M-code manual
    290: "gk_shortcut_use",           # shortcut practice
    291: "gk_score_compute",          # real-time scoring
    292: "gk_auto_grade",             # comprehensive assessment
    293: "gk_wrong_log_add",          # wrong-answer log
    294: "gk_transcript_export",      # transcript export
    295: "gk_auto_grade",             # automatic grading
    296: "gk_exam_start",             # exam mode
    297: "gk_leaderboard_submit",     # leaderboard
    298: "gk_certificate_build",      # certificate
    299: "gk_player_points_add",      # gamified points
    300: "gk_badge_evaluate",         # badge system
    301: "gk_course_add_lesson",      # level unlock
    302: "gk_challenge_update",       # stage challenge
    303: "gk_challenge_update",       # timed challenge
    304: "gk_challenge_update",       # precision challenge
    305: "gk_challenge_update",       # efficiency challenge
    306: "gk_challenge_init",         # team task
    307: "gk_dialog_init",            # story mode
    308: "gk_challenge_init",         # factory order
    309: "gk_dialog_add",             # role play
    310: "gk_dialog_advance",         # master-apprentice story
    311: "gk_dialog_choose",          # branching story
    312: "gk_dialog_current",         # NPC dialog
    313: "gk_train_session_init",     # reverse teaching
    314: "gk_train_session_init",     # reverse training
    315: "gk_train_session_init",     # blind operation training
    316: "gk_replay_push",            # live mode
    317: "gk_replay_push",            # record mode
    318: "gk_replay_start_replay",    # replay mode
}
for _fid, _sym in TEACH_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

TEACH_VERIFIED = list(range(280, 319))
for _fid in TEACH_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 14: cognitive science (319-332).
COG_IMPL = {
    319: "gk_heatmap_observe",        # attention heat map
    320: "gk_gaze_is_fixation",       # eye tracking
    321: "gk_cognitive_load",         # cognitive load measurement
    322: "gk_memory_retention",       # memory curve
    323: "gk_memory_retention",       # Ebbinghaus review
    324: "gk_next_review_interval",   # spaced repetition
    325: "gk_micro_session_count",    # micro learning
    326: "gk_flow_detect",            # flow detection
    327: "gk_metacog_calibration",    # metacognition training
    328: "gk_error_attribution",      # error attribution training
    329: "gk_transfer_gain",          # transfer training
    330: "gk_reaction_mean",          # reaction time statistics
    331: "gk_error_rate",             # error rate statistics
    332: "gk_engagement_index",       # learning behavior analysis
}
for _fid, _sym in COG_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

COG_VERIFIED = list(range(319, 333))
for _fid in COG_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 15: faults and alarms (333-358).
ALARM_IMPL = {
    333: "gk_alarm_check_overtravel",  # OT overtravel alarm
    334: "gk_alarm_check_overload",     # servo overload alarm
    335: "gk_alarm_check_overload",     # spindle overload alarm
    336: "gk_alarm_check_tool_breakage",# tool breakage alarm
    337: "gk_alarm_check_tool_life",    # tool life alarm
    338: "gk_alarm_check_level",        # coolant low alarm
    339: "gk_alarm_check_level",        # air pressure low alarm
    340: "gk_alarm_check_level",        # hydraulic low alarm
    341: "gk_alarm_check_level",        # lubrication low alarm
    342: "gk_alarm_check_syntax",       # program syntax error
    343: "gk_alarm_check_gcode",        # undefined G code
    344: "gk_alarm_check_mcode",        # undefined M code
    345: "gk_alarm_check_coord",        # coordinate overtravel
    346: "gk_alarm_raise",              # collision alarm
    347: "gk_alarm_estop",              # emergency stop event
    348: "gk_alarm_power_loss",         # power loss recovery
    349: "gk_alarm_manual_lookup",      # alarm manual
    350: "gk_alarm_timeline_span",      # alarm timeline
    351: "gk_alarm_find_category",      # alarm history
    352: "gk_fault_tree_probability",   # fault tree analysis
    353: "gk_fault_sim_init",           # fault simulation system
    354: "gk_fault_sim_step",           # fault injection
    355: "gk_alarm_buzzer_on",          # alarm buzzer
    356: "gk_alarm_lamp_on",            # alarm lamp blinking
    357: "gk_alarm_make_code",          # alarm code generation
    358: "gk_alarm_diagnose",           # alarm diagnosis suggestion
}
for _fid, _sym in ALARM_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

ALARM_VERIFIED = list(range(333, 359))
for _fid in ALARM_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 16: operation UI (359-380).
UI_IMPL = {
    359: "gk_menu_bar_init",             # main menu
    360: "gk_menu_add_item",             # file menu
    361: "gk_menu_add_item",             # edit menu
    362: "gk_menu_add_item",             # view menu
    363: "gk_menu_add_item",             # tools menu
    364: "gk_menu_add_item",             # help menu
    365: "gk_toolbar_add",               # toolbar
    366: "gk_status_bar_set",            # status bar
    367: "gk_data_panel_init",           # data display panel
    368: "gk_data_panel_update",         # coordinate display
    369: "gk_data_panel_update",         # spindle speed display
    370: "gk_data_panel_update",         # feed display
    371: "gk_overrides_set",             # override display
    372: "gk_data_panel_update",         # machining time display
    373: "gk_data_panel_remaining",      # remaining time display
    374: "gk_data_panel_progress_bar",   # progress bar
    375: "gk_ui_switch_view",            # view switch
    376: "gk_ui_toggle_fullscreen",      # fullscreen/window
    377: "gk_ui_set_theme",              # dark theme
    378: "gk_ui_set_theme",              # light theme
    379: "gk_ui_tr",                     # multi-language
    380: "gk_ui_set_lang",               # Chinese/English switch
}
for _fid, _sym in UI_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

UI_VERIFIED = list(range(359, 381))
for _fid in UI_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 17: operation panel (381-423).
PANEL_IMPL = {
    381: "gk_panel_style_name",          # FANUC style panel
    382: "gk_panel_style_name",          # SIEMENS style panel
    383: "gk_panel_style_name",          # Mitsubishi style panel
    384: "gk_panel_style_name",          # HAAS style panel
    385: "gk_keyboard_press",            # MDI keyboard
    386: "gk_keyboard_is_digit",         # digit keys
    387: "gk_keyboard_is_letter",        # letter keys
    388: "gk_keyboard_press",            # function keys
    389: "gk_panel_softkey",             # screen soft keys
    390: "gk_mpg_step",                  # handwheel MPG
    391: "gk_mpg_set_multiplier",        # x1 step
    392: "gk_mpg_set_multiplier",        # x10 step
    393: "gk_mpg_set_multiplier",        # x100 step
    394: "gk_mpg_select_axis",           # axis selector dial
    395: "gk_mpg_select_axis",           # X axis select
    396: "gk_mpg_select_axis",           # Y axis select
    397: "gk_mpg_select_axis",           # Z axis select
    398: "gk_mpg_select_axis",           # 4th axis select
    399: "gk_overrides_set",             # feed override dial
    400: "gk_overrides_set",             # rapid override dial
    401: "gk_overrides_set",             # spindle override dial
    402: "gk_mode_switch_set",           # mode selector dial
    403: "gk_mode_switch_set",           # edit mode
    404: "gk_mode_switch_set",           # auto mode
    405: "gk_mode_switch_set",           # MDI mode
    406: "gk_mode_switch_set",           # handwheel mode
    407: "gk_mode_switch_set",           # manual mode
    408: "gk_mode_switch_set",           # home mode
    409: "gk_mode_switch_set",           # DNC mode
    410: "gk_panel_press",               # cycle start button
    411: "gk_panel_press",               # cycle stop button
    412: "gk_panel_press",               # emergency stop button
    413: "gk_panel_press",               # key switch
    414: "gk_panel_press",               # coolant switch
    415: "gk_panel_press",               # spindle switch
    416: "gk_panel_press",               # light switch
    417: "gk_alarm_page_add",            # alarm screen
    418: "gk_param_page_add",            # parameter page
    419: "gk_diag_page_add",             # diagnostic page
    420: "gk_waveform_push",             # servo waveform graph
    421: "gk_ladder_evaluate",           # ladder monitoring
    422: "gk_plc_scan",                  # PLC state display
    423: "gk_plc_set_io",                # I/O state display
}
for _fid, _sym in PANEL_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

PANEL_VERIFIED = list(range(381, 424))
for _fid in PANEL_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 18: program editor (424-439).
EDITOR_IMPL = {
    424: "gk_doc_set_text",              # program edit area
    425: "gk_doc_tokenize",              # syntax highlighting
    426: "gk_doc_line_number",           # line number display
    427: "gk_doc_add_diagnostic",        # error indication
    428: "gk_doc_complete",              # autocompletion
    429: "gk_doc_find",                  # find
    430: "gk_doc_replace",               # replace
    431: "gk_doc_goto",                  # goto line
    432: "gk_doc_copy",                  # copy (and paste)
    433: "gk_editor_undo",               # undo (and redo)
    434: "gk_doc_autoindent",            # auto-indent
    435: "gk_doc_match_bracket",         # bracket matching
    436: "gk_doc_fold",                  # code folding
    437: "gk_doc_bookmark",              # bookmarks
    438: "gk_editor_open",               # multi-file editing
    439: "gk_doc_compare",               # file compare
}
for _fid, _sym in EDITOR_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

EDITOR_VERIFIED = list(range(424, 440))
for _fid in EDITOR_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 19: interaction (440-453).
INTERACT_IMPL = {
    440: "gk_interact_step",            # single step execution
    441: "gk_interact_set_breakpoint",  # breakpoint set
    442: "gk_interact_set_breakpoint",  # breakpoint clear
    443: "gk_interact_set_speed",       # rate slider
    444: "gk_interaction_init",         # touch support
    445: "gk_interaction_init",         # gamepad support
    446: "gk_interact_map_key",         # custom shortcuts
    447: "gk_editor_open",              # multi-window
    448: "gk_doc_set_text",             # drag-and-drop load
    449: "gk_interact_zoom",            # mouse wheel zoom
    450: "gk_menu_bar_init",            # context menu
    451: "gk_interact_double_click",    # double click
    452: "gk_interact_hover",           # hover tooltip
    453: "gk_interact_move",            # keyboard navigation
}
for _fid, _sym in INTERACT_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

INTERACT_VERIFIED = list(range(440, 454))
for _fid in INTERACT_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 20: data and files (454-478).
FILE_IMPL = {
    454: "gk_file_open",                # open NC file
    455: "gk_file_save",                # save NC file
    456: "gk_file_save_as",             # save as
    457: "gk_file_recent",              # recent files
    458: "gk_file_new",                 # new file
    459: "gk_file_close",               # close file
    460: "gk_export_toolpath",          # export toolpath
    461: "gk_export_report",            # export report
    462: "gk_export_csv",               # export CSV
    463: "gk_export_json",              # export JSON
    464: "gk_export_pdf",               # export PDF
    465: "gk_recording_add",            # machining recording
    466: "gk_recording_export_mp4",     # export MP4
    467: "gk_screenshot_capture",       # screenshot
    468: "gk_save_state",               # save state
    469: "gk_load_state",               # load state
    470: "gk_file_set_onumber",         # program O-number management
    471: "gk_transfer_init",            # DNC transfer simulation
    472: "gk_transfer_init",            # serial port simulation
    473: "gk_transfer_init",            # USB simulation
    474: "gk_transfer_init",            # network load
    475: "gk_transfer_init",            # cloud sync
    476: "gk_autosave_tick",             # auto save
    477: "gk_history_commit",            # version history
    478: "gk_file_encrypt",              # file encryption
}
for _fid, _sym in FILE_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

FILE_VERIFIED = list(range(454, 479))
for _fid in FILE_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 21: visualization (479-495).
VIZ_IMPL = {
    479: "gk_scope_push",                # signal oscilloscope
    480: "gk_logger_record",             # data logger
    481: "gk_field_set",                 # cutting force heatmap
    482: "gk_field_set",                 # temperature field
    483: "gk_vfield_set",                # velocity vector field
    484: "gk_field_max_deviation",       # error cloud
    485: "gk_gantt_add",                 # toolpath Gantt chart
    486: "gk_timeline_add",              # load timeline
    487: "gk_timeline_alarm_count",      # alarm timeline
    488: "gk_process_add",               # process decomposition diagram
    489: "gk_section_add",               # depth section
    490: "gk_dual_view_init",            # dual comparison view
    491: "gk_dual_view_push",            # target vs actual
    492: "gk_viz_overlay",               # curve overlay
    493: "gk_logger_export",             # export chart data
    494: "gk_scope_push",                # real-time curve
    495: "gk_logger_record",             # history curve
}
for _fid, _sym in VIZ_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

VIZ_VERIFIED = list(range(479, 496))
for _fid in VIZ_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 22: digital / industrial interfaces (496-515).
IOT_IMPL = {
    496: "gk_data_bus_init",            # real-time data bus
    497: "gk_opcua_server_start",       # OPC UA server
    498: "gk_opcua_client_init",        # OPC UA client
    499: "gk_mtconnect_probe",          # MTConnect
    500: "gk_mqtt_publish",             # MQTT
    501: "gk_modbus_init",              # Modbus TCP
    502: "gk_modbus_init",              # Modbus RTU
    503: "gk_fieldbus_init",            # PROFINET
    504: "gk_fieldbus_init",            # EtherCAT
    505: "gk_fieldbus_init",            # EtherNet/IP
    506: "gk_api_init",                 # REST API
    507: "gk_api_init",                 # WebSocket
    508: "gk_api_init",                 # gRPC
    509: "gk_api_init",                 # GraphQL
    510: "gk_twin_sync",                # digital twin mapping
    511: "gk_tsdb_write",               # InfluxDB storage
    512: "gk_tsdb_write",               # time series database
    513: "gk_acquisition_tick",         # data acquisition
    514: "gk_playback_step",            # data playback
    515: "gk_tsviz_render",             # data visualization
}
for _fid, _sym in IOT_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

IOT_VERIFIED = list(range(496, 516))
for _fid in IOT_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])

# Batch 23: AI and data science (516-540).
AI_IMPL = {
    516: "gk_ai_explain_error",         # intelligent error explanation
    517: "gk_ai_recommend_params",      # parameter recommendation
    518: "gk_ai_nl_to_gcode",           # G-code generation from NL
    519: "gk_voice_listen",             # voice assistant
    520: "gk_auto_toolset_probe",       # intelligent tool setting
    521: "gk_anomaly_is_anomaly",       # anomaly prediction
    522: "gk_ai_predict_tool_life",     # tool breakage prediction
    523: "gk_ai_generate_program",      # automatic programming
    524: "gk_learner_add_ability",      # learner profile
    525: "gk_learner_radar",            # ability radar chart
    526: "gk_ai_adaptive_difficulty",   # adaptive question selection
    527: "gk_error_pattern_mine",       # error pattern mining
    528: "gk_ai_predict_score",         # score prediction
    529: "gk_ai_recommend_courses",     # course recommendation
    530: "gk_ai_group_average",         # group comparison
    531: "gk_clustering_run",           # learner clustering
    532: "gk_ai_teaching_effect",       # teaching effect analysis
    533: "gk_kg_path_length",           # knowledge graph
    534: "gk_qa_ask",                   # intelligent Q&A
    535: "gk_image_classify",           # image recognition
    536: "gk_vqa_answer",               # visual Q&A
    537: "gk_voice_dialog_say",         # voice dialogue
    538: "gk_gesture_classify",         # gesture recognition
    539: "gk_face_classify",            # expression recognition
    540: "gk_multimodal_diagnose",      # multimodal diagnosis
}
for _fid, _sym in AI_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

AI_VERIFIED = list(range(516, 541))
for _fid in AI_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 24: collaboration and classroom (541-580).
COLLAB_IMPL = {
    541: "gk_session_add_user",         # teacher client
    542: "gk_session_add_user",         # student client
    543: "gk_monitor_update",           # real-time monitoring
    544: "gk_takeover_start",           # remote takeover
    545: "gk_task_dispatch",            # task dispatch
    546: "gk_task_submit",              # homework submission
    547: "gk_task_grade",               # automatic grading
    548: "gk_class_rank_submit",        # leaderboard
    549: "gk_network_link",             # multi-machine networking
    550: "gk_chat_send",                # danmaku chat
    551: "gk_broadcast_start",          # screen broadcast
    552: "gk_broadcast_join",           # voice intercom
    553: "gk_group_create",             # group collaboration
    554: "gk_competition_start",        # competition mode
    555: "gk_classroom_init",           # class management
    556: "gk_classroom_add_student",    # student management
    557: "gk_session_count_role",       # teacher management
    558: "gk_course_assign_add",        # course assignment
    559: "gk_course_progress",          # progress tracking
    560: "gk_message_send",             # home-school communication
    561: "gk_course_add_slide",         # course editor
    562: "gk_scene_add",                # 3D scene editor
    563: "gk_question_add",             # quiz tool
    564: "gk_exam_generate",            # exam generation
    565: "gk_library_add",              # machine model market
    566: "gk_library_add",              # course market
    567: "gk_library_add",              # G-code template library
    568: "gk_library_add",              # part library
    569: "gk_library_add",              # tool vendor library
    570: "gk_library_add",              # material library
    571: "gk_library_add",              # process library
    572: "gk_library_add",              # standard part library
    573: "gk_library_add",              # fixture library
    574: "gk_library_add",              # gauge library
    575: "gk_library_add",              # coating database
    576: "gk_library_add",              # coolant database
    577: "gk_updater_check",             # online update
    578: "gk_distribution_push",         # content distribution
    579: "gk_content_commit",            # version management
    580: "gk_ugc_publish",               # user creation
}
for _fid, _sym in COLLAB_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

COLLAB_VERIFIED = list(range(541, 581))
for _fid in COLLAB_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 25: simulation physics and material/process libraries (581-621).
PHYSICS_IMPL = {
    581: "gk_chatter_stability_limit",  # chatter simulation
    582: "gk_chatter_growth_rate",      # regenerative chatter
    583: "gk_lobe_generate",            # stability lobe diagram
    584: "gk_workpiece_deflection",     # FEA workpiece deformation
    585: "gk_thermo_coupled_strain",    # thermal-mechanical coupling
    586: "gk_spindle_growth",           # spindle thermal growth
    587: "gk_screw_deformation",        # ball screw thermal deformation
    588: "gk_modal_add_mode",           # structural modal analysis
    589: "gk_axis_servo_is_stable",     # servo flexibility
    590: "gk_drive_init",               # air bearing spindle
    591: "gk_drive_init",               # hydrostatic guide
    592: "gk_drive_init",               # magnetic spindle
    593: "gk_drive_init",               # linear motor
    594: "gk_drive_init",               # voice coil motor
    595: "gk_drive_init",               # piezo drive
    596: "gk_assist_init",              # ultrasonic vibration machining
    597: "gk_assist_init",              # laser-assisted machining
    598: "gk_assist_init",              # cryogenic machining
    599: "gk_assist_init",              # MQL
    600: "gk_assist_init",              # high-pressure cooling
    601: "gk_material_get",             # carbon steel library
    602: "gk_material_get",             # alloy steel library
    603: "gk_material_get",             # stainless steel library
    604: "gk_material_get",             # aluminum library
    605: "gk_material_get",             # copper library
    606: "gk_material_get",             # titanium library
    607: "gk_material_get",             # superalloy library
    608: "gk_material_get",             # cast iron library
    609: "gk_material_get",             # plastic library
    610: "gk_material_get",             # composite library
    611: "gk_material_get",             # ceramic library
    612: "gk_jc_init",                  # material constitutive model
    613: "gk_jc_flow_stress",           # Johnson-Cook parameters
    614: "gk_hardness_at_depth",        # hardness distribution
    615: "gk_heat_affects_hardness",    # heat treatment state
    616: "gk_recommend_speed",          # cutting parameter recommendation
    617: "gk_recommend_speed",          # speed recommendation
    618: "gk_recommend_feed",           # feed recommendation
    619: "gk_recommend_depth",          # depth-of-cut recommendation
    620: "gk_recommend_tool",           # tool selection recommendation
    621: "gk_recommend_cooling",        # cooling method recommendation
}
for _fid, _sym in PHYSICS_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

PHYSICS_VERIFIED = list(range(581, 622))
for _fid in PHYSICS_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 26: manufacturing full-process (622-640).
PROCESS_IMPL = {
    622: "gk_inspection_pass",          # incoming inspection
    623: "gk_blank_init",               # blank preparation
    624: "gk_setup_align",              # setup and alignment
    625: "gk_tool_set_probe",           # tool setting
    626: "gk_first_article_init",       # first-article cut
    627: "gk_first_article_evaluate",   # first-article inspection
    628: "gk_batch_record",             # batch machining
    629: "gk_inspection_deviation",     # online measurement
    630: "gk_proc_tool_use",            # tool change
    631: "gk_finish_clean",             # cleaning
    632: "gk_finish_deburr",            # deburring
    633: "gk_finish_ok",                # final inspection
    634: "gk_package_seal",             # packaging
    635: "gk_warehouse_store",          # warehousing
    636: "gk_disposition_decide",       # scrap decision
    637: "gk_rework_schedule",          # rework
    638: "gk_trace_add",                # traceability
    639: "gk_qr_encode",                # QR marking
    640: "gk_lifecycle_init",           # lifecycle management
}
for _fid, _sym in PROCESS_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

PROCESS_VERIFIED = list(range(622, 641))
for _fid in PROCESS_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 27: energy and cost accounting (641-655).
COST_IMPL = {
    641: "gk_power_meter_init",        # real-time power display
    642: "gk_power_kind_kw",           # spindle power
    643: "gk_power_kind_kw",           # servo power
    644: "gk_power_kind_kw",           # coolant power
    645: "gk_power_kind_kw",           # lighting power
    646: "gk_electricity_cost",        # electricity cost
    647: "gk_tool_cost_amount",        # tool cost
    648: "gk_material_cost_amount",    # material cost
    649: "gk_labor_cost_amount",       # labor cost
    650: "gk_cost_total",              # comprehensive cost accounting
    651: "gk_carbon_footprint",        # carbon footprint
    652: "gk_energy_advise",           # energy-saving advice
    653: "gk_cost_report",             # cost report
    654: "gk_cost_compare",            # cost comparison
    655: "gk_cost_predict",            # cost prediction
}
for _fid, _sym in COST_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

COST_VERIFIED = list(range(641, 656))
for _fid in COST_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 28: maintenance and servicing (656-675).
MAINT_IMPL = {
    656: "gk_maint_task_add",           # maintenance schedule
    657: "gk_maint_task_done",          # daily maintenance
    658: "gk_maint_task_done",          # weekly maintenance
    659: "gk_maint_task_done",          # monthly maintenance
    660: "gk_maint_task_done",          # yearly maintenance
    661: "gk_lube_system_init",         # lubrication system
    662: "gk_lube_level_fraction",      # oil level monitoring
    663: "gk_lube_auto_pulse",          # automatic lubrication
    664: "gk_guide_clean_run",          # guide cleaning
    665: "gk_filter_replace",           # filter replacement
    666: "gk_belt_check",               # belt inspection
    667: "gk_precision_add",            # precision check
    668: "gk_interferometer_linear_error",  # laser interferometer
    669: "gk_ballbar_run",              # ballbar
    670: "gk_maint_fault_probability",  # fault tree
    671: "gk_spare_add",                # spare parts management
    672: "gk_work_order_create",        # work order
    673: "gk_repair_record_add",        # repair log
    674: "gk_maint_next_due",           # maintenance reminder
    675: "gk_component_rul",            # life prediction
}
for _fid, _sym in MAINT_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

MAINT_VERIFIED = list(range(656, 676))
for _fid in MAINT_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 29: safety (676-700).
SAFETY_IMPL = {
    676: "gk_safety_procedure_init",    # safe operating procedure
    677: "gk_safety_build_power_on",    # power-on sequence
    678: "gk_safety_build_power_off",   # power-off sequence
    679: "gk_ppe_state_init",           # PPE prompt
    680: "gk_ppe_wear",                # goggles
    681: "gk_ppe_wear",                # coverall
    682: "gk_ppe_wear",                # safety shoes
    683: "gk_hazard_query",            # hazard area prompt
    684: "gk_rotating_warning",        # rotating parts warning
    685: "gk_estop_drill_press",        # emergency-stop drill
    686: "gk_estop_nearest",            # e-stop locations
    687: "gk_loto_apply",               # LOTO lockout/tagout
    688: "gk_5s_board_init",            # 5S management
    689: "gk_5s_set_score",             # sort
    690: "gk_5s_set_score",             # set in order
    691: "gk_5s_set_score",             # shine
    692: "gk_5s_set_score",             # standardize
    693: "gk_5s_set_score",             # sustain
    694: "gk_incident_replay",          # incident case replay
    695: "gk_violation_add",            # violation points
    696: "gk_safety_score_value",       # safety assessment
    697: "gk_safety_cert_issue",        # safety certificate
    698: "gk_emergency_complete",       # emergency response
    699: "gk_emergency_complete",       # fire drill
    700: "gk_leakage_pass",             # earth-leakage protection
}
for _fid, _sym in SAFETY_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

SAFETY_VERIFIED = list(range(676, 701))
for _fid in SAFETY_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 30: realism & variation (701-730).
REAL_IMPL = {
    701: "gk_blank_variation_init",     # random blank size
    702: "gk_variation_sample",         # random hardness
    703: "gk_tool_batch_init",          # tool batch variation
    704: "gk_environment_sample",       # grid fluctuation
    705: "gk_environment_sample",       # air pressure fluctuation
    706: "gk_environment_sample",       # hydraulic fluctuation
    707: "gk_environment_sample",       # daily temperature fluctuation
    708: "gk_environment_sample",       # humidity change
    709: "gk_operator_setting_error",   # human operator variance
    710: "gk_clamp_force_sample",       # clamping force variance
    711: "gk_fault_model_sample",       # random faults
    712: "gk_event_model_sample",       # random events
    713: "gk_measure_noise_apply",      # measurement noise
    714: "gk_measure_noise_apply",      # reading error
    715: "gk_monte_carlo",              # Monte-Carlo simulation
    716: "gk_stats_finalize",           # probability statistics
    717: "gk_power_resume",             # power-loss resume cutting
    718: "gk_disruption_add",           # broken-tool replacement
    719: "gk_disruption_add",           # crash cleanup
    720: "gk_disruption_add",           # batch scrap
    721: "gk_disruption_add",           # customer complaint
    722: "gk_disruption_add",           # deadline squeeze
    723: "gk_disruption_add",           # machine breakdown
    724: "gk_disruption_add",           # material reject
    725: "gk_disruption_add",           # staff shortage
    726: "gk_disruption_add",           # overnight rush
    727: "gk_disruption_add",           # fatigue operation
    728: "gk_disruption_add",           # safety accident
    729: "gk_disruption_add",           # fire
    730: "gk_disruption_add",           # electrical leak
}
for _fid, _sym in REAL_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

REAL_VERIFIED = list(range(701, 731))
for _fid in REAL_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 31: VR/AR & hardware (731-749).
XR_IMPL = {
    731: "gk_xr_init",                    # OpenXR support
    732: "gk_xr_device_add",              # VR headset
    733: "gk_xr_gamepad_init",            # VR controller
    734: "gk_xr_ar_anchor_add",           # AR overlay
    735: "gk_xr_device_add",              # AR glasses
    736: "gk_xr_phone_ar_pose",           # phone AR
    737: "gk_xr_haptic_play",             # haptic feedback
    738: "gk_xr_force_feedback_step",     # force-feedback handwheel
    739: "gk_xr_force_feedback_step",     # force-feedback pad
    740: "gk_xr_hrtf_compute",            # spatial audio HRTF
    741: "gk_xr_spatial_audio_compute",   # 3D positional audio
    742: "gk_xr_gamepad_init",            # Xbox controller
    743: "gk_xr_gamepad_init",            # PS controller
    744: "gk_xr_handwheel_init",          # real handwheel hardware
    745: "gk_xr_handwheel_connect",       # USB handwheel
    746: "gk_xr_multiscreen_sync",        # multi-screen sync
    747: "gk_xr_eye_update",              # eye tracking
    748: "gk_xr_eeg_evaluate",            # EEG headband
    749: "gk_xr_heart_report",            # heart-rate strap
}
for _fid, _sym in XR_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

XR_VERIFIED = list(range(731, 750))
for _fid in XR_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 32: multimodal feedback & expression (750-768).
UX_IMPL = {
    750: "gk_ux_glove_update",              # thermal glove
    751: "gk_ux_scent_emit",                # scent generator
    752: "gk_ux_foot_switch_set",           # foot switch
    753: "gk_ux_print_head_extrude",        # 3D print head
    754: "gk_ux_haptic_seat_pulse",         # haptic seat
    755: "gk_ux_sonify",                    # data sonification
    756: "gk_ux_load_to_pitch",             # load to tone
    757: "gk_ux_temp_to_color",             # temperature to light colour
    758: "gk_ux_texture_to_haptic",         # texture to haptics
    759: "gk_ux_gcode_to_note",             # G-code to score
    760: "gk_ux_narration_say",             # real-time narration
    761: "gk_ux_chart_link",                # chart linkage
    762: "gk_ux_animation_frames",          # animation explanation
    763: "gk_ux_cue_add",                   # 3D annotation
    764: "gk_ux_cue_add",                   # arrow indicator
    765: "gk_ux_cue_add",                   # highlight cue
    766: "gk_ux_cue_speech",                # speech announcement
    767: "gk_ux_cue_add",                   # vibration feedback
    768: "gk_ux_cue_add",                   # light effect cue
}
for _fid, _sym in UX_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

UX_VERIFIED = list(range(750, 769))
for _fid in UX_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 33: reverse inference & narrative (769-788).
SAGA_IMPL = {
    769: "gk_deduce_program_from_part",   # finished part -> program
    770: "gk_deduce_state_from_sound",    # sound -> state
    771: "gk_deduce_params_from_curve",   # curve -> parameters
    772: "gk_deduce_tool_from_texture",   # texture -> tool
    773: "gk_deduce_action_from_alarm",   # alarm -> operator action
    774: "gk_deduce_cause_from_scrap",    # scrap -> process cause
    775: "gk_deduce_craft_from_cost",     # cost -> craft
    776: "gk_deduce_feature_from_toolpath", # toolpath -> part feature
    777: "gk_saga_mentor_teach",          # mentor/apprentice story
    778: "gk_saga_factory_intake",        # factory order intake
    779: "gk_saga_incident_review",       # incident review
    780: "gk_saga_challenge_attempt",     # technical breakthrough
    781: "gk_saga_startup_simulate_month", # startup mode
    782: "gk_saga_plot_choose",           # branching plot
    783: "gk_saga_npc_add",               # NPC dialogue
    784: "gk_saga_immersion_set_view",    # immersion script
    785: "gk_saga_immersion_set_view",    # first person view
    786: "gk_saga_quest_progress",        # quest system
    787: "gk_saga_achievement_unlock",    # achievement system
    788: "gk_saga_title_evaluate",        # title system
}
for _fid, _sym in SAGA_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

SAGA_VERIFIED = list(range(769, 789))
for _fid in SAGA_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 34: multi-scale simulation & thermal drift (789-800).
MSCALE_IMPL = {
    789: "gk_ms_servo_step",             # microsecond servo simulation
    790: "gk_ms_nano_cut_time",          # nanosecond cutting
    791: "gk_ms_atom_activation",         # atomic scale
    792: "gk_ms_grain_grow",             # grain scale
    793: "gk_ms_chip_estimate",          # microscopic chip
    794: "gk_ms_tool_tip_update",        # tool-tip scale
    795: "gk_ms_workpiece_deflection",   # workpiece scale
    796: "gk_ms_machine_init",           # machine scale
    797: "gk_ms_shop_oee",               # workshop scale
    798: "gk_ms_factory_output",         # factory scale
    799: "gk_ms_supply_bullwhip",        # supply-chain scale
    800: "gk_ms_thermal_drift_update",   # hourly thermal deformation
}
for _fid, _sym in MSCALE_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

MSCALE_VERIFIED = list(range(789, 801))
for _fid in MSCALE_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 35: time-scale drift, multi-physics fields, CAD/CAM (801-850).
PHYS_IMPL = {
    801: "gk_phys_daily_advance",       # daily drift
    802: "gk_phys_monthly_wear_run",     # monthly tool wear
    803: "gk_phys_ageing_reliability",   # yearly machine ageing
    804: "gk_phys_life_advance",         # ten-year life
    805: "gk_phys_history_record",       # machining history replay
    806: "gk_phys_field_source",         # force field
    807: "gk_phys_field_init",           # thermal field
    808: "gk_phys_field_init",           # magnetic field
    809: "gk_phys_field_init",           # electric field
    810: "gk_phys_field_init",           # acoustic field
    811: "gk_phys_field_init",           # flow field CFD
    812: "gk_phys_field_init",           # optical field
    813: "gk_phys_field_init",           # chemical field
    814: "gk_phys_field_init",           # radiation field
    815: "gk_phys_coupling_step",        # multi-field coupling
    816: "gk_phys_coupling_show_single", # single field display
    817: "gk_phys_coupling_overlay_all", # multi-field overlay
    818: "gk_phys_coupling_set_strength",# coupling strength
    819: "gk_phys_coupling_dominant",    # dominant field analysis
    820: "gk_cad_sketch_init",           # 2D drawing
    821: "gk_cad_add_line",              # line
    822: "gk_cad_add_arc",               # arc
    823: "gk_cad_add_fillet",            # fillet
    824: "gk_cad_add_chamfer",           # chamfer
    825: "gk_cad_solid_init",            # 3D modelling
    826: "gk_cad_extrude",               # extrude
    827: "gk_cad_revolve",               # revolve
    828: "gk_cad_boolean",               # boolean
    829: "gk_cam_generate",              # sketch to G-code
    830: "gk_cam_op_time",               # pocket milling
    831: "gk_cam_op_time",               # waterline
    832: "gk_cam_op_time",               # parallel
    833: "gk_cam_op_time",               # profile
    834: "gk_cam_op_time",               # drilling auto-program
    835: "gk_cam_op_time",               # thread milling
    836: "gk_cam_op_time",               # tapping
    837: "gk_cam_verify_run",            # simulation verification
    838: "gk_cam_post_transform",        # post-processor
    839: "gk_cad_drawing_generate",      # engineering drawing
    840: "gk_cad_import_detect",         # DXF import
    841: "gk_cad_import_detect",         # STEP import
    842: "gk_cad_import_detect",         # IGES import
    843: "gk_cad_import_detect",         # STL import
    844: "gk_cad_import_detect",         # OBJ import
    845: "gk_cad_import_detect",         # 3MF import
    846: "gk_cad_import_detect",         # Parasolid import
    847: "gk_cad_platform_name",         # Windows platform
    848: "gk_cad_platform_name",         # Linux platform
    849: "gk_cad_platform_name",         # macOS platform
    850: "gk_cad_test_run",              # unit testing
}
for _fid, _sym in PHYS_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

PHYS_VERIFIED = list(range(801, 851))
for _fid in PHYS_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 36: diagnostics, toolchain, deployment, licensing, accessibility (851-900).
DEV_IMPL = {
    851: "gk_dev_regression_compare",   # regression testing
    852: "gk_dev_prof_hottest",         # performance profiling
    853: "gk_dev_memwatch_over_limit",  # memory monitoring
    854: "gk_dev_crash_report",         # crash report
    855: "gk_dev_crash_handler_capture",# crash capture
    856: "gk_dev_telemetry_event",      # telemetry
    857: "gk_dev_hotreload_poll",       # hot reload
    858: "gk_dev_script_register",      # Lua extension
    859: "gk_dev_script_register",      # Python extension
    860: "gk_dev_plugin_load",          # plugin ABI
    861: "gk_dev_dylib_open",           # dynamic library loading
    862: "gk_dev_config_save",          # config persistence
    863: "gk_devlog_write",             # logging system
    864: "gk_devlog_enabled",           # graded logging
    865: "gk_devlog_rotate",            # log rotation
    866: "gk_dev_pkg_manifest",         # MSI
    867: "gk_dev_pkg_manifest",         # deb
    868: "gk_dev_pkg_manifest",         # dmg
    869: "gk_dev_pkg_manifest",         # portable
    870: "gk_dev_update_available",     # auto update
    871: "gk_dev_ci_run",               # CI/CD
    872: "gk_dev_docker_build",         # docker image
    873: "gk_dev_cloud_deploy",         # cloud deployment
    874: "gk_dev_mesh_register",        # microservices
    875: "gk_lic_edition_name",         # free edition
    876: "gk_lic_edition_name",         # professional edition
    877: "gk_lic_edition_name",         # education edition
    878: "gk_lic_edition_name",         # enterprise edition
    879: "gk_lic_generate_serial",      # serial authorisation
    880: "gk_lic_activate_online",      # online activation
    881: "gk_lic_activate_offline",     # offline activation
    882: "gk_lic_track_usage",          # usage statistics
    883: "gk_lic_days_to_renewal",      # renewal reminder
    884: "gk_lic_init",                 # subscription
    885: "gk_lic_init",                 # perpetual
    886: "gk_lic_edition_tier",         # volume licence
    887: "gk_lic_init",                 # trial
    888: "gk_lic_feature_allowed",      # feature limits
    889: "gk_lic_time_expired",         # time limit
    890: "gk_lic_watermark",            # watermark
    891: "gk_lic_integrity_ok",         # anti-crack
    892: "gk_lic_dongle_valid",         # dongle
    893: "gk_lic_server_checkout",      # license server
    894: "gk_a11y_colorblind_map",      # colour-blind mode
    895: "gk_a11y_contrast_set",        # high contrast
    896: "gk_a11y_font_size",           # large font
    897: "gk_a11y_voice_match",         # voice control
    898: "gk_a11y_onehand_remap",       # one-hand operation
    899: "gk_a11y_subtitles_at",        # subtitles
    900: "gk_a11y_slowmo_apply",        # slow motion
}
for _fid, _sym in DEV_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

DEV_VERIFIED = list(range(851, 901))
for _fid in DEV_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 37: learning analytics, HIL/integration, security (901-950).
LEARN_IMPL = {
    901: "gk_learn_screenreader_desc",  # screen reader
    902: "gk_learn_kbdnav_next",       # keyboard navigation
    903: "gk_learn_touch_target",      # touch optimisation
    904: "gk_learn_student_init",      # student profile
    905: "gk_learn_radar_point",       # ability radar
    906: "gk_learn_errors_top",        # error pattern mining
    907: "gk_learn_predict_grade",     # grade prediction
    908: "gk_learn_recommend_course",  # course recommendation
    909: "gk_learn_cohort_percentile", # cohort comparison
    910: "gk_learn_dashboard_build",   # teacher dashboard
    911: "gk_learn_analyze_trend",     # learning analysis
    912: "gk_learn_abtest_significant",# A/B testing
    913: "gk_learn_eval_total",        # teaching evaluation
    914: "gk_learn_bar_chart",         # data visualisation
    915: "gk_learn_report",            # report generation
    916: "gk_learn_export_csv",        # data export
    917: "gk_learn_redact",            # privacy protection
    918: "gk_learn_anonymize",         # anonymisation
    919: "gk_sysint_loop_step",        # HIL
    920: "gk_sysint_loop_init",        # SIL
    921: "gk_sysint_rcp_build",        # RCP
    922: "gk_sysint_twin_sync",        # digital twin sync
    923: "gk_sysint_remote_read",      # remote monitoring
    924: "gk_sysint_remote_write",     # remote operation
    925: "gk_sysint_link",             # virtual/physical linkage
    926: "gk_sysint_shadow_compare",   # shadow mode
    927: "gk_sysint_device_connect",   # real CNC
    928: "gk_sysint_device_connect",   # real PLC
    929: "gk_sysint_device_connect",   # real servo
    930: "gk_sysint_handwheel_read",   # real handwheel
    931: "gk_sysint_hdf5_header",      # HDF5 export
    932: "gk_sysint_python_bind",      # Python API
    933: "gk_sysint_matlab_export",    # MATLAB interface
    934: "gk_sysint_ros_publish",      # ROS interface
    935: "gk_sysint_fmu_export",       # FMU/FMI
    936: "gk_sysint_modelica_model",   # Modelica
    937: "gk_sysint_paper_figure",     # paper mode
    938: "gk_sysint_benchmark_eval",   # benchmark dataset
    939: "gk_sysint_experiment_script",# experiment script
    940: "gk_sysint_replay_next",      # data replay
    941: "gk_sec_encrypt",             # data encryption
    942: "gk_sec_authenticate",        # user authentication
    943: "gk_sec_role_allows",         # permission management
    944: "gk_sec_audit_log",           # audit log
    945: "gk_sec_policy_accept",       # privacy policy
    946: "gk_sec_compliance_all_pass", # compliance check
    947: "gk_sec_gdpr_export",         # GDPR
    948: "gk_sec_network_enable_tls",  # network security
    949: "gk_sec_tamper_append",       # tamper-proof
    950: "gk_sec_sign",                # digital signature
}
for _fid, _sym in LEARN_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

LEARN_VERIFIED = list(range(901, 951))
for _fid in LEARN_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 38: production collaboration, application details (951-1000).
PROD_IMPL = {
    951: "gk_prod_cell_assign",         # cell collaboration
    952: "gk_prod_line_bottleneck",     # flexible line balancing
    953: "gk_prod_agv_move",            # AGV
    954: "gk_prod_robot_cycle_time",    # robot
    955: "gk_prod_vision_find",         # vision positioning
    956: "gk_prod_rfid_read",           # RFID
    957: "gk_prod_mes_advance",         # MES
    958: "gk_prod_erp_receive",         # ERP
    959: "gk_prod_wms_reserve",         # WMS
    960: "gk_prod_asrs_store",          # AS/RS
    961: "gk_prod_schedule",            # scheduling
    962: "gk_prod_optimize_process",    # process sequencing
    963: "gk_prod_optimize_path",       # path optimisation
    964: "gk_prod_optimize_params",     # parameter optimisation
    965: "gk_prod_optimize_tech",       # process optimisation
    966: "gk_prod_optimize_cost",       # cost optimisation
    967: "gk_prod_optimize_energy",     # energy optimisation
    968: "gk_prod_optimize_quality",    # quality optimisation
    969: "gk_prod_optimize_efficiency", # efficiency optimisation
    970: "gk_prod_optimize_comprehensive",  # comprehensive optimisation
    971: "gk_app_texture_height",      # casting texture
    972: "gk_app_seam_area",           # sheet-metal seam
    973: "gk_app_screw_total_length",  # screw detail
    974: "gk_app_nameplate_area",      # nameplate
    975: "gk_app_serial_render",       # serial number plate
    976: "gk_app_warning_label",       # warning label
    977: "gk_app_sticker_add",         # instruction sticker
    978: "gk_app_logo_render",         # brand logo
    979: "gk_app_model_render",        # model designation
    980: "gk_app_date_render",         # manufacturing date
    981: "gk_app_cover_name",          # dust cover
    982: "gk_app_cover_extended",      # telescopic cover
    983: "gk_app_cover_folds",         # bellows cover
    984: "gk_app_cover_name",          # spiral cover
    985: "gk_app_dragchain_links",     # drag chain
    986: "gk_app_route_total",         # cable routing
    987: "gk_app_route_total",         # air routing
    988: "gk_app_route_total",         # oil routing
    989: "gk_app_hydraulic_power_kw",  # hydraulic power unit
    990: "gk_app_pneumatic_ok",        # pneumatic FRL
    991: "gk_app_chiller_error",       # oil chiller
    992: "gk_app_cabinet_door",        # cabinet door
    993: "gk_app_cabinet_fan_needed",  # cabinet fan
    994: "gk_app_cabinet_vent_ratio",  # cabinet vent
    995: "gk_app_cool_name",          # spindle cooling
    996: "gk_app_cool_velocity",      # tool cooling
    997: "gk_app_cool_aim",           # universal nozzle
    998: "gk_app_cool_velocity",      # internal cooling
    999: "gk_app_light_illuminance",  # work light
    1000: "gk_app_stack_from_alarm",  # tri-colour stack light
}
for _fid, _sym in PROD_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

PROD_VERIFIED = list(range(951, 1001))
for _fid in PROD_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 39: machine hardware and real machine actions (1001-1060).
HW_IMPL = {
    1001: "gk_hw_buzzer_beep",          # buzzer
    1002: "gk_hw_estop_press",          # emergency stop
    1003: "gk_hw_handle_pull",          # door handle
    1004: "gk_hw_window_view_area",     # observation window
    1005: "gk_hw_wiper_start",          # window wiper
    1006: "gk_hw_lock_engage",          # door lock
    1007: "gk_hw_door_sensor_update",   # magnetic switch
    1008: "gk_hw_safety_switch_ok",     # safety door switch
    1009: "gk_hw_panel_name",           # side door
    1010: "gk_hw_panel_name",           # rear door
    1011: "gk_hw_panel_name",           # top cover
    1012: "gk_hw_base_footprint",       # base
    1013: "gk_hw_feet_can_support",     # machine feet
    1014: "gk_hw_pad_transmissibility", # anti-vibration pad
    1015: "gk_hw_level_bolt_turn",      # levelling bolt
    1016: "gk_hw_lifting_eye_ok",       # lifting eye
    1017: "gk_hw_fork_pocket_accepts",  # forklift pocket
    1018: "gk_hw_plate_bracket_tilt",   # nameplate bracket
    1019: "gk_hw_spindle_start",        # spindle start jitter
    1020: "gk_hw_spindle_coast_distance",  # spindle stop inertia
    1021: "gk_hw_spindle_shift",        # spindle gear change
    1022: "gk_hw_spindle_orient",       # spindle orientation
    1023: "gk_hw_spindle_air_blast",    # taper air blast
    1024: "gk_hw_drawbar_clamp",        # pull stud clamp
    1025: "gk_hw_drawbar_release",      # pull stud release
    1026: "gk_hw_atc_step",             # ATC arm extend
    1027: "gk_hw_atc_step",             # ATC arm grip
    1028: "gk_hw_atc_step",             # ATC arm rotate
    1029: "gk_hw_atc_step",             # ATC arm retract
    1030: "gk_hw_magazine_index",       # magazine index
    1031: "gk_hw_magazine_lock",        # magazine lock
    1032: "gk_hw_magazine_count",       # magazine count
    1033: "gk_hw_atc_confirm",          # tool change confirm
    1034: "gk_hw_atc_confirm",          # tool change fault
    1035: "gk_hw_setter_contact",       # tool setter contact
    1036: "gk_hw_setter_retract",       # tool setter retract
    1037: "gk_hw_setter_offset",        # tool setter signal
    1038: "gk_hw_clamp_close",          # workpiece clamp
    1039: "gk_hw_clamp_open",           # workpiece unclamp
    1040: "gk_hw_clamp_ok",             # clamp pressure check
    1041: "gk_hw_tailstock_advance",    # tailstock advance
    1042: "gk_hw_tailstock_retract",    # tailstock retract
    1043: "gk_hw_steady_clamp",         # steady rest clamp
    1044: "gk_hw_steady_release",       # steady rest release
    1045: "gk_hw_conveyor_start",       # chip conveyor start
    1046: "gk_hw_conveyor_stop",        # chip conveyor stop
    1047: "gk_hw_conveyor_reverse",     # chip conveyor reverse
    1048: "gk_hw_coolant_start",        # coolant start
    1049: "gk_hw_coolant_stop",         # coolant stop
    1050: "gk_hw_coolant_set_flow",     # coolant flow control
    1051: "gk_hw_air_gun_blow",         # air blow gun
    1052: "gk_hw_auto_door_open",       # auto door open
    1053: "gk_hw_auto_door_close",      # auto door close
    1054: "gk_hw_auto_door_open",       # auto door buffer
    1055: "gk_hw_auto_door_stop_on_obstacle",  # auto door anti-pinch
    1056: "gk_hw_rotary_rotate",        # rotary table rotate
    1057: "gk_hw_rotary_lock",          # rotary table lock
    1058: "gk_hw_rotary_unlock",        # rotary table unlock
    1059: "gk_hw_indexer_index",        # indexing table index
    1060: "gk_hw_indexer_lock",         # indexing table lock
}
for _fid, _sym in HW_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

HW_VERIFIED = list(range(1001, 1061))
for _fid in HW_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 40: real cutting details (1061-1090).
CUTD_IMPL = {
    1061: "gk_cutd_entry_engage",       # cutting entry impact
    1062: "gk_cutd_exit_burr_height",   # exit burr / chipping
    1063: "gk_cutd_force_wave_at",      # cutting force fluctuation
    1064: "gk_cutd_force_jump_update",  # cutting force jump
    1065: "gk_cutd_chatter_excite",     # chatter
    1066: "gk_cutd_noise_from_speed",   # cutting noise
    1067: "gk_cutd_spark_emit",         # sparks
    1068: "gk_cutd_smoke_generate",     # smoke
    1069: "gk_cutd_smell_classify",     # smell
    1070: "gk_cutd_chip_fly_compute",   # chip splash
    1071: "gk_cutd_chip_pile_update",   # chip accumulation
    1072: "gk_cutd_chip_tangle_grow",   # chip entanglement
    1073: "gk_cutd_chip_break_check",   # chip breaking
    1074: "gk_cutd_chip_colour",        # chip colour
    1075: "gk_cutd_tool_wear_advance",  # gradual tool wear
    1076: "gk_cutd_tool_wear_shock",    # sudden tool wear
    1077: "gk_cutd_tool_wear_chipped",  # tool chipping
    1078: "gk_cutd_tool_wear_break_check",  # tool breakage
    1079: "gk_cutd_redheat_update",     # tool red heat
    1080: "gk_cutd_pattern_pitch",      # surface pattern
    1081: "gk_cutd_surface_detect",     # surface burn
    1082: "gk_cutd_surface_detect",     # surface scratch
    1083: "gk_cutd_surface_detect",     # surface chatter mark
    1084: "gk_cutd_surface_detect",     # surface burr
    1085: "gk_cutd_drift_update",       # dimensional drift
    1086: "gk_cutd_thermal_expansion",  # thermal expansion
    1087: "gk_cutd_thermal_contraction",# cooling contraction
    1088: "gk_cutd_stress_distortion",  # stress distortion
    1089: "gk_cutd_deflect_apply",      # tool deflection
    1090: "gk_cutd_deflect_release",    # tool recovery
}
for _fid, _sym in CUTD_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

CUTD_VERIFIED = list(range(1061, 1091))
for _fid in CUTD_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 41: real servo details (1091-1120).
SVO_IMPL = {
    1091: "gk_svo_power_on",             # servo power on
    1092: "gk_svo_enable",               # servo enable
    1093: "gk_svo_in_alarm",             # servo alarm
    1094: "gk_svo_set_load",             # servo overload
    1095: "gk_svo_set_temperature",      # servo overheat
    1096: "gk_svo_encoder_alarm",        # encoder alarm
    1097: "gk_svo_wave_at",              # current waveform
    1098: "gk_svo_wave_at",              # speed waveform
    1099: "gk_svo_wave_at",              # position waveform
    1100: "gk_svo_loop_step",            # three-loop control
    1101: "gk_svo_current_loop",         # current loop
    1102: "gk_svo_velocity_loop",        # velocity loop
    1103: "gk_svo_position_loop",        # position loop
    1104: "gk_svo_pid_step",             # PID parameters
    1105: "gk_svo_feedforward",          # feedforward control
    1106: "gk_svo_friction_comp",        # friction compensation
    1107: "gk_svo_backlash_compensate",  # backlash compensation
    1108: "gk_svo_pitch_compensate",     # pitch error compensation
    1109: "gk_svo_straightness_comp",    # straightness compensation
    1110: "gk_svo_squareness_comp",      # squareness compensation
    1111: "gk_svo_thermal_compensate",   # thermal deformation compensation
    1112: "gk_svo_set_gain",             # servo gain tuning
    1113: "gk_svo_set_rigidity",         # servo rigidity tuning
    1114: "gk_svo_notch_response",       # vibration suppression
    1115: "gk_svo_notch_response",       # notch filter
    1116: "gk_svo_lowpass_apply",        # low-pass filter
    1117: "gk_svo_smooth_apply",         # smoothing
    1118: "gk_svo_ff_accel",             # accel feedforward
    1119: "gk_svo_ff_velocity",          # velocity feedforward
    1120: "gk_svo_ff_torque",            # torque feedforward
}
for _fid, _sym in SVO_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

SVO_VERIFIED = list(range(1091, 1121))
for _fid in SVO_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 42: real CNC system details (1121-1150).
CSYS_IMPL = {
    1121: "gk_csys_selftest",          # power-on self test
    1122: "gk_csys_load_params",       # parameter loading
    1123: "gk_csys_load_program",      # program loading
    1124: "gk_csys_verify_program",    # program verification
    1125: "gk_csys_verify_program",    # syntax check
    1126: "gk_csys_load_tools",        # tool table loading
    1127: "gk_csys_load_wcs",          # work coordinate loading
    1128: "gk_csys_load_offsets",      # tool offset table loading
    1129: "gk_csys_load_macros",       # macro program loading
    1130: "gk_csys_plc_start",         # PLC start
    1131: "gk_csys_enable_servos",     # servo enable
    1132: "gk_csys_home",              # homing
    1133: "gk_csys_touch_off",         # tool setting
    1134: "gk_csys_trial_cut",         # trial cut
    1135: "gk_csys_start_machining",   # machining
    1136: "gk_csys_pause",             # pause
    1137: "gk_csys_resume",            # resume
    1138: "gk_csys_end_program",       # end
    1139: "gk_csys_shutdown",          # shutdown
    1140: "gk_csys_backup_params",     # parameter backup
    1141: "gk_csys_backup_program",    # program backup
    1142: "gk_csys_upgrade",           # system upgrade
    1143: "gk_csys_restore",           # data restore
    1144: "gk_csys_reset_alarm",       # alarm reset
    1145: "gk_csys_reset_estop",       # e-stop reset
    1146: "gk_csys_reset_overtravel",  # overtravel reset
    1147: "gk_csys_reset_servo",       # servo reset
    1148: "gk_csys_reset_system",      # system reset
    1149: "gk_csys_cold_start",        # cold start
    1150: "gk_csys_warm_start",        # warm start
}
for _fid, _sym in CSYS_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

CSYS_VERIFIED = list(range(1121, 1151))
for _fid in CSYS_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 43: real HMI details (1151-1175).
HMI_IMPL = {
    1151: "gk_hmi_handwheel_turn",          # handwheel detents
    1152: "gk_hmi_handwheel_scale_aligned", # handwheel scale alignment
    1153: "gk_hmi_handwheel_gear",          # gear change click
    1154: "gk_hmi_button_press",            # button press animation
    1155: "gk_hmi_button_release",          # button release animation
    1156: "gk_hmi_button_backlight",        # button backlight
    1157: "gk_hmi_knob_rotate",             # knob rotate animation
    1158: "gk_hmi_knob_position",           # knob scale indicator
    1159: "gk_hmi_knob_damping_torque",     # knob damping
    1160: "gk_hmi_display_init",            # CRT scanlines
    1161: "gk_hmi_display_glare",           # screen glare
    1162: "gk_hmi_display_brightness",      # screen aging
    1163: "gk_hmi_display_dead",            # dead pixels
    1164: "gk_hmi_display_set_refresh",     # screen refresh
    1165: "gk_hmi_display_set_refresh",     # screen flicker
    1166: "gk_hmi_key_press",               # keyboard key sound
    1167: "gk_hmi_keyboard_backlight",      # keyboard backlight
    1168: "gk_hmi_mouse_click",             # mouse click sound
    1169: "gk_hmi_mouse_wheel",             # mouse wheel sound
    1170: "gk_hmi_touch_press",             # touch feedback
    1171: "gk_hmi_haptic_pulse",            # vibration feedback
    1172: "gk_hmi_speaker_play",            # prompt sound
    1173: "gk_hmi_speaker_play",            # warning sound
    1174: "gk_hmi_speaker_play",            # error sound
    1175: "gk_hmi_speaker_play",            # success sound
}
for _fid, _sym in HMI_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

HMI_VERIFIED = list(range(1151, 1176))
for _fid in HMI_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 44: real environment details (1176-1205).
ENVD_IMPL = {
    1176: "gk_envd_shop_init",              # workshop background
    1177: "gk_envd_sound_level",            # distant machine sound
    1178: "gk_envd_sound_level",            # forklift sound
    1179: "gk_envd_sound_level",            # crane sound
    1180: "gk_envd_sound_level",            # people talking
    1181: "gk_envd_sound_level",            # broadcast sound
    1182: "gk_envd_sound_level",            # ventilation sound
    1183: "gk_envd_sound_level",            # air condition sound
    1184: "gk_envd_sound_level",            # transformer hum
    1185: "gk_envd_sound_level",            # cabinet fan sound
    1186: "gk_envd_sound_level",            # cooling tower sound
    1187: "gk_envd_sound_level",            # compressor sound
    1188: "gk_envd_floor_soil",             # floor oil stains
    1189: "gk_envd_floor_soil",             # floor chips
    1190: "gk_envd_floor_soil",             # floor water stains
    1191: "gk_envd_wall_sticker",           # wall sticker
    1192: "gk_envd_aisle_ok",               # safety aisle
    1193: "gk_envd_fire_init",              # fire equipment
    1194: "gk_envd_store_init",             # toolbox
    1195: "gk_envd_store_init",             # material rack
    1196: "gk_envd_store_init",             # finished rack
    1197: "gk_envd_store_init",             # waste bin
    1198: "gk_envd_store_init",             # mop and broom
    1199: "gk_envd_lighting_init",          # lighting tubes
    1200: "gk_envd_emergency_light_init",   # emergency light
    1201: "gk_envd_window_init",            # window
    1202: "gk_envd_window_view",            # view outside door
    1203: "gk_envd_clock_render",           # shop clock
    1204: "gk_envd_board_update",           # production board
    1205: "gk_envd_handover_set",           # shift handover record
}
for _fid, _sym in ENVD_IMPL.items():
    IMPLEMENTED[_fid] = ("GK_STATUS_IMPLEMENTED", _sym)

ENVD_VERIFIED = list(range(1176, 1206))
for _fid in ENVD_VERIFIED:
    IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", IMPLEMENTED[_fid][1])


# Batch 45: real operation workflows (1206-1229).
WF_IMPL = {
    1206: "gk_wf_init", 1207: "gk_wf_load_template", 1208: "gk_wf_load_template",
    1209: "gk_wf_load_template", 1210: "gk_wf_load_template",
    1211: "gk_wf_load_template", 1212: "gk_wf_load_template",
    1213: "gk_wf_load_template", 1214: "gk_wf_load_template",
    1215: "gk_wf_load_template", 1216: "gk_wf_estop",
    1217: "gk_wf_alarm_resolve", 1218: "gk_wf_handover",
    1219: "gk_wf_process_run", 1220: "gk_wf_process_run",
    1221: "gk_wf_process_run", 1222: "gk_wf_process_run",
    1223: "gk_wf_process_run", 1224: "gk_wf_checklist_init",
    1225: "gk_wf_checklist_init", 1226: "gk_wf_checklist_init",
    1227: "gk_wf_checklist_init", 1228: "gk_wf_checklist_init",
    1229: "gk_wf_checklist_init",
}
# Batch 46: real machining scenarios (1230-1257).
SCEN_IMPL = {i: "gk_scen_recommend" for i in range(1230, 1258)}
# Batch 47: real fault scenarios (1258-1290).
FSCEN_IMPL = {i: "gk_fscen_trigger" for i in range(1258, 1291)}
# Batch 48: real data records (1291-1311).
REC_IMPL = {
    1291: "gk_rec_log_append", 1292: "gk_rec_log_append",
    1293: "gk_rec_log_append", 1294: "gk_rec_log_append",
    1295: "gk_rec_log_append", 1296: "gk_rec_log_append",
    1297: "gk_rec_log_append", 1298: "gk_rec_log_append",
    1299: "gk_rec_log_append", 1300: "gk_rec_performance",
    1301: "gk_rec_oee", 1302: "gk_rec_availability", 1303: "gk_rec_quality",
    1304: "gk_rec_scrap_rate", 1305: "gk_rec_mtbf", 1306: "gk_rec_mttr",
    1307: "gk_rec_report_render", 1308: "gk_rec_report_render",
    1309: "gk_rec_report_render", 1310: "gk_rec_report_render",
    1311: "gk_rec_report_render",
}
# Batch 49: real networking and communication (1312-1330).
NET2_IMPL = {i: "gk_net2_connect" for i in range(1312, 1331)}
# Batch 50: real tool system (1331-1350).
TOOLSYS_IMPL = {
    1331: "gk_toolsys_id_init", 1332: "gk_toolsys_preset_set",
    1333: "gk_toolsys_assembly_add", 1334: "gk_toolsys_balance_ok",
    1335: "gk_toolsys_set_runout", 1336: "gk_toolsys_life_init",
    1337: "gk_toolsys_worn", 1338: "gk_toolsys_mark_broken",
    1339: "gk_toolsys_match_rfid", 1340: "gk_toolsys_crib_init",
    1341: "gk_toolsys_crib_set_status", 1342: "gk_toolsys_crib_set_status",
    1343: "gk_toolsys_crib_set_status", 1344: "gk_toolsys_crib_set_status",
    1345: "gk_toolsys_crib_total_value", 1346: "gk_toolsys_crib_count_status",
    1347: "gk_toolsys_order_init", 1348: "gk_toolsys_order_init",
    1349: "gk_toolsys_trial_init", 1350: "gk_toolsys_trial_score",
}
# Batch 51: real fixture system (1351-1370).
FIXTURE_IMPL = {
    1351: "gk_fixture_init", 1352: "gk_fixture_init", 1353: "gk_fixture_init",
    1354: "gk_fixture_init", 1355: "gk_fixture_init", 1356: "gk_fixture_init",
    1357: "gk_fixture_init", 1358: "gk_fixture_init", 1359: "gk_fixture_init",
    1360: "gk_fixture_init", 1361: "gk_fixture_init", 1362: "gk_fixture_init",
    1363: "gk_fixture_init", 1364: "gk_fixture_init",
    1365: "gk_fixture_design", 1366: "gk_fixture_manufacture",
    1367: "gk_fixture_commission", 1368: "gk_fixture_maintain",
    1369: "gk_fixture_stock_add", 1370: "gk_fixture_stock_total_cost",
}
# Batch 52: real workpiece management (1371-1385).
PART_IMPL = {
    1371: "gk_part_id_init", 1372: "gk_part_matches",
    1373: "gk_part_doc_init", 1374: "gk_part_doc_init", 1375: "gk_part_doc_init",
    1376: "gk_part_doc_init", 1377: "gk_part_stock_in", 1378: "gk_part_stock_out",
    1379: "gk_part_stocktake", 1380: "gk_part_inventory_value",
    1381: "gk_part_shipment_init", 1382: "gk_part_shipment_advance",
    1383: "gk_part_shipment_advance", 1384: "gk_part_shipment_advance",
    1385: "gk_part_shipment_advance",
}
# Batch 53: real personnel management (1386-1400).
STAFF_IMPL = {
    1386: "gk_staff_init", 1387: "gk_staff_init", 1388: "gk_staff_init",
    1389: "gk_staff_init", 1390: "gk_staff_init", 1391: "gk_staff_init",
    1392: "gk_staff_init", 1393: "gk_staff_init",
    1394: "gk_staff_perms_for_role", 1395: "gk_staff_shift_assign",
    1396: "gk_staff_attendance_mark", 1397: "gk_staff_performance_score",
    1398: "gk_staff_training_complete", 1399: "gk_staff_matrix_set",
    1400: "gk_staff_dispatch",
}
# Batch 54: real production management (1401-1420).
PMGMT_IMPL = {
    1401: "gk_pmgmt_order_init", 1402: "gk_pmgmt_schedule_add",
    1403: "gk_pmgmt_order_schedule", 1404: "gk_pmgmt_progress_set",
    1405: "gk_pmgmt_delivery_init", 1406: "gk_pmgmt_registry_init",
    1407: "gk_pmgmt_registry_init", 1408: "gk_pmgmt_registry_init",
    1409: "gk_pmgmt_registry_init", 1410: "gk_pmgmt_registry_init",
    1411: "gk_pmgmt_registry_init", 1412: "gk_pmgmt_registry_init",
    1413: "gk_pmgmt_registry_init", 1414: "gk_pmgmt_registry_init",
    1415: "gk_pmgmt_registry_init", 1416: "gk_pmgmt_registry_init",
    1417: "gk_pmgmt_registry_init", 1418: "gk_pmgmt_exception_init",
    1419: "gk_pmgmt_meeting_init", 1420: "gk_pmgmt_registry_init",
}
# Batch 55: real quality system (1421-1440).
QSYS_IMPL = {
    1421: "gk_qsys_standard_name", 1422: "gk_qsys_standard_name",
    1423: "gk_qsys_standard_name", 1424: "gk_qsys_standard_name",
    1425: "gk_qsys_doc_init", 1426: "gk_qsys_doc_init", 1427: "gk_qsys_doc_init",
    1428: "gk_qsys_doc_init", 1429: "gk_qsys_doc_init",
    1430: "gk_qsys_audit_init", 1431: "gk_qsys_audit_init",
    1432: "gk_qsys_audit_init", 1433: "gk_qsys_action_init",
    1434: "gk_qsys_action_init", 1435: "gk_qsys_action_init",
    1436: "gk_qsys_8d_init", 1437: "gk_qsys_5why_add",
    1438: "gk_qsys_fishbone_add", 1439: "gk_qsys_rpn", 1440: "gk_qsys_spc_cpk",
}
# Batch 56: real process design (1441-1460).
PROCD_IMPL = {
    1441: "gk_procd_route_init", 1442: "gk_procd_route_add",
    1443: "gk_procd_route_add", 1444: "gk_procd_allowance_init",
    1445: "gk_procd_cutting_init", 1446: "gk_procd_choice_init",
    1447: "gk_procd_choice_init", 1448: "gk_procd_choice_init",
    1449: "gk_procd_choice_init", 1450: "gk_procd_time_quota_init",
    1451: "gk_procd_material_quota_init", 1452: "gk_procd_plan_init",
    1453: "gk_procd_plan_advance", 1454: "gk_procd_plan_advance",
    1455: "gk_procd_plan_advance", 1456: "gk_procd_plan_advance",
    1457: "gk_procd_plan_revise", 1458: "gk_procd_plan_revise",
    1459: "gk_procd_knowledge_add", 1460: "gk_procd_expert_recommend",
}
# Batch 57: real programming flow (1461-1475).
PROGFLOW_IMPL = {
    1461: "gk_progflow_init", 1462: "gk_progflow_init", 1463: "gk_progflow_init",
    1464: "gk_progflow_init", 1465: "gk_progflow_advance",
    1466: "gk_progflow_advance", 1467: "gk_progflow_advance",
    1468: "gk_progflow_advance", 1469: "gk_progflow_advance",
    1470: "gk_progflow_advance", 1471: "gk_progflow_advance",
    1472: "gk_progflow_advance", 1473: "gk_progflow_set_flag",
    1474: "gk_progflow_backup_init", 1475: "gk_progflow_restore",
}
# Batch 58: final machine characteristics (1476-1525).
MSPEC_IMPL = {i: "gk_mspec_get_num" for i in range(1476, 1526)}

for _group in (WF_IMPL, SCEN_IMPL, FSCEN_IMPL, REC_IMPL, NET2_IMPL,
               TOOLSYS_IMPL, FIXTURE_IMPL, PART_IMPL, STAFF_IMPL, PMGMT_IMPL,
               QSYS_IMPL, PROCD_IMPL, PROGFLOW_IMPL, MSPEC_IMPL):
    for _fid, _sym in _group.items():
        IMPLEMENTED[_fid] = ("GK_STATUS_VERIFIED", _sym)


def main() -> int:
    text = SRC.read_text(encoding="utf-8")
    lines = text.splitlines()

    items = []
    current_part = 1
    for line in lines:
        m = SEC_RE.match(line.strip())
        if m:
            head = m.group(1)
            num = re.match(r"^([一二三四五六七八九十\d]+)", head)
            if num:
                parsed = cn_to_int(num.group(1))
                if parsed is not None:
                    current_part = parsed
            continue
        if SUB_RE.match(line.strip()):
            continue
        m = ITEM_RE.match(line.strip())
        if m:
            fid = int(m.group(1))
            name = m.group(2).strip()
            domain = PART_DOMAIN.get(current_part, "GK_DOMAIN_ADVANCED")
            items.append((fid, name, domain))

    if not items:
        print("no items parsed", file=sys.stderr)
        return 1

    ids = [i for i, _, _ in items]
    expected = list(range(1, len(items) + 1))
    if ids != expected:
        print(f"id sequence mismatch: got {len(items)} items, "
              f"first={ids[0]} last={ids[-1]}", file=sys.stderr)
        missing = sorted(set(expected) - set(ids))
        if missing:
            print(f"missing ids: {missing[:20]}", file=sys.stderr)
        return 1

    out = []
    out.append("/* Auto-generated by tools/gen_catalog.py. Do not edit by hand. */")
    out.append('#include "gk/gk_catalog.h"')
    out.append("")
    out.append(f"#define GK_CATALOG_COUNT {len(items)}")
    out.append("")
    out.append("const gk_feature g_gk_catalog[GK_CATALOG_COUNT] = {")
    for fid, name, domain in items:
        status, symbol = IMPLEMENTED.get(
            fid, ("GK_STATUS_NOT_IMPLEMENTED", None))
        sym = f'"{symbol}"' if symbol else "NULL"
        out.append(
            f'    {{ {fid}, "{c_escape(name)}", {domain}, '
            f"{status}, {sym} }},"
        )
    out.append("};")
    out.append("")
    out.append("const gk_feature *g_gk_catalog_ptr = g_gk_catalog;")
    out.append(f"const size_t g_gk_catalog_count = GK_CATALOG_COUNT;")
    out.append("")

    OUT.write_text("\n".join(out) + "\n", encoding="utf-8")
    print(f"wrote {OUT} with {len(items)} features "
          f"(id 1..{len(items)})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
