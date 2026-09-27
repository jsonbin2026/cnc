#include "gk/gk_mspec.h"

#include <math.h>
#include <string.h>

static void gk__mspec_copy(char *dst, size_t cap, const char *src)
{
    size_t n;
    if (dst == NULL || cap == 0) {
        return;
    }
    if (src == NULL) {
        dst[0] = '\0';
        return;
    }
    n = strlen(src);
    if (n >= cap) {
        n = cap - 1;
    }
    memcpy(dst, src, n);
    dst[n] = '\0';
}

void gk_mspec_init(gk_mspec *m)
{
    if (m == NULL) {
        return;
    }
    memset(m, 0, sizeof(*m));
    gk__mspec_copy(m->model, sizeof(m->model), "GK-VMC850");
    gk__mspec_copy(m->serial, sizeof(m->serial), "SN-000000");
    gk__mspec_copy(m->manufacture_date, sizeof(m->manufacture_date),
                   "2026-01-01");
    gk__mspec_copy(m->spindle_taper, sizeof(m->spindle_taper), "BT40");
    gk__mspec_copy(m->ip_rating, sizeof(m->ip_rating), "IP54");
    gk__mspec_copy(m->acceptance_standard, sizeof(m->acceptance_standard),
                   "GB/T 17421");
    m->weight_kg = 5500.0;
    m->power_kw = 25.0;
    m->length_mm = 2600.0;
    m->width_mm = 2400.0;
    m->height_mm = 2800.0;
    m->travel_x_mm = 800.0;
    m->travel_y_mm = 500.0;
    m->travel_z_mm = 500.0;
    m->accuracy_mm = 0.008;
    m->repeatability_mm = 0.005;
    m->spindle_rpm_min = 100.0;
    m->spindle_rpm_max = 12000.0;
    m->rapid_mm_min = 30000.0;
    m->feed_min_mm_min = 1.0;
    m->feed_max_mm_min = 10000.0;
    m->atc_capacity = 24;
    m->max_tool_diameter_mm = 80.0;
    m->max_tool_length_mm = 300.0;
    m->max_tool_weight_kg = 8.0;
    m->table_length_mm = 1000.0;
    m->table_width_mm = 500.0;
    m->table_load_kg = 500.0;
    m->tslot_width_mm = 18.0;
    m->tslot_pitch_mm = 125.0;
    m->spindle_power_kw = 11.0;
    m->spindle_torque_nm = 70.0;
    m->coolant_tank_l = 200.0;
    m->air_pressure_mpa = 0.6;
    m->voltage_v = 380.0;
    m->frequency_hz = 50.0;
    m->ambient_temp_min_c = 5.0;
    m->ambient_temp_max_c = 40.0;
    m->ambient_humidity_max = 80.0;
    m->noise_db = 78.0;
    m->vibration_mm_s = 2.8;
    m->thermal_balance_min = 30.0;
    m->has_warmup_program = 1;
    m->has_warmup_cycle = 1;
}

/* ===================================================================
 * Field access by feature id
 * =================================================================== */

#define GK_MSPEC_STR_CASE(id, field) \
    case id: \
        if (value != NULL) { \
            gk__mspec_copy(m->field, sizeof(m->field), value); \
            return GK_OK; \
        } \
        return GK_ERR_INVALID_ARG

gk_status gk_mspec_set_str(gk_mspec *m, int fid, const char *value)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    switch (fid) {
    GK_MSPEC_STR_CASE(1476, model);
    GK_MSPEC_STR_CASE(1477, serial);
    GK_MSPEC_STR_CASE(1478, manufacture_date);
    GK_MSPEC_STR_CASE(1495, spindle_taper);
    GK_MSPEC_STR_CASE(1504, ip_rating);
    GK_MSPEC_STR_CASE(1511, acceptance_standard);
    default:
        return GK_ERR_NOT_FOUND;
    }
}

const char *gk_mspec_get_str(const gk_mspec *m, int fid)
{
    if (m == NULL) {
        return NULL;
    }
    switch (fid) {
    case 1476: return m->model;
    case 1477: return m->serial;
    case 1478: return m->manufacture_date;
    case 1495: return m->spindle_taper;
    case 1504: return m->ip_rating;
    case 1511: return m->acceptance_standard;
    default: return NULL;
    }
}

#define GK_MSPEC_NUM_CASE(id, field) \
    case id: \
        m->field = value; \
        return GK_OK

gk_status gk_mspec_set_num(gk_mspec *m, int fid, double value)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    switch (fid) {
    GK_MSPEC_NUM_CASE(1479, weight_kg);
    GK_MSPEC_NUM_CASE(1480, power_kw);
    GK_MSPEC_NUM_CASE(1481, length_mm);
    GK_MSPEC_NUM_CASE(1482, travel_x_mm);
    GK_MSPEC_NUM_CASE(1483, accuracy_mm);
    GK_MSPEC_NUM_CASE(1484, repeatability_mm);
    GK_MSPEC_NUM_CASE(1485, spindle_rpm_max);
    GK_MSPEC_NUM_CASE(1486, rapid_mm_min);
    GK_MSPEC_NUM_CASE(1487, feed_max_mm_min);
    GK_MSPEC_NUM_CASE(1488, atc_capacity);
    GK_MSPEC_NUM_CASE(1489, max_tool_diameter_mm);
    GK_MSPEC_NUM_CASE(1490, max_tool_length_mm);
    GK_MSPEC_NUM_CASE(1491, max_tool_weight_kg);
    GK_MSPEC_NUM_CASE(1492, table_length_mm);
    GK_MSPEC_NUM_CASE(1493, table_load_kg);
    GK_MSPEC_NUM_CASE(1494, tslot_width_mm);
    GK_MSPEC_NUM_CASE(1496, spindle_power_kw);
    GK_MSPEC_NUM_CASE(1497, spindle_torque_nm);
    GK_MSPEC_NUM_CASE(1498, coolant_tank_l);
    GK_MSPEC_NUM_CASE(1499, air_pressure_mpa);
    GK_MSPEC_NUM_CASE(1500, voltage_v);
    GK_MSPEC_NUM_CASE(1501, frequency_hz);
    GK_MSPEC_NUM_CASE(1502, ambient_temp_max_c);
    GK_MSPEC_NUM_CASE(1503, ambient_humidity_max);
    GK_MSPEC_NUM_CASE(1505, noise_db);
    GK_MSPEC_NUM_CASE(1506, vibration_mm_s);
    GK_MSPEC_NUM_CASE(1507, thermal_balance_min);
    default:
        return GK_ERR_NOT_FOUND;
    }
}

double gk_mspec_get_num(const gk_mspec *m, int fid)
{
    if (m == NULL) {
        return 0.0;
    }
    switch (fid) {
    case 1479: return m->weight_kg;
    case 1480: return m->power_kw;
    case 1481: return m->length_mm;
    case 1482: return m->travel_x_mm;
    case 1483: return m->accuracy_mm;
    case 1484: return m->repeatability_mm;
    case 1485: return m->spindle_rpm_max;
    case 1486: return m->rapid_mm_min;
    case 1487: return m->feed_max_mm_min;
    case 1488: return (double)m->atc_capacity;
    case 1489: return m->max_tool_diameter_mm;
    case 1490: return m->max_tool_length_mm;
    case 1491: return m->max_tool_weight_kg;
    case 1492: return m->table_length_mm;
    case 1493: return m->table_load_kg;
    case 1494: return m->tslot_width_mm;
    case 1496: return m->spindle_power_kw;
    case 1497: return m->spindle_torque_nm;
    case 1498: return m->coolant_tank_l;
    case 1499: return m->air_pressure_mpa;
    case 1500: return m->voltage_v;
    case 1501: return m->frequency_hz;
    case 1502: return m->ambient_temp_max_c;
    case 1503: return m->ambient_humidity_max;
    case 1505: return m->noise_db;
    case 1506: return m->vibration_mm_s;
    case 1507: return m->thermal_balance_min;
    default: return 0.0;
    }
}

#define GK_MSPEC_FLAG_CASE(id, field) \
    case id: \
        m->field = value ? 1 : 0; \
        return GK_OK

gk_status gk_mspec_set_flag(gk_mspec *m, int fid, int value)
{
    if (m == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    switch (fid) {
    GK_MSPEC_FLAG_CASE(1508, has_warmup_program);
    GK_MSPEC_FLAG_CASE(1509, has_warmup_cycle);
    GK_MSPEC_FLAG_CASE(1510, accuracy_inspected);
    GK_MSPEC_FLAG_CASE(1512, installed);
    GK_MSPEC_FLAG_CASE(1513, leveled);
    GK_MSPEC_FLAG_CASE(1514, anchored);
    GK_MSPEC_FLAG_CASE(1515, damped);
    GK_MSPEC_FLAG_CASE(1516, grounded);
    GK_MSPEC_FLAG_CASE(1517, power_connected);
    GK_MSPEC_FLAG_CASE(1518, air_connected);
    GK_MSPEC_FLAG_CASE(1519, water_connected);
    GK_MSPEC_FLAG_CASE(1520, chip_conveyor_connected);
    GK_MSPEC_FLAG_CASE(1521, coolant_drain_connected);
    GK_MSPEC_FLAG_CASE(1522, smoke_exhaust_connected);
    GK_MSPEC_FLAG_CASE(1523, lighting_connected);
    GK_MSPEC_FLAG_CASE(1524, network_connected);
    GK_MSPEC_FLAG_CASE(1525, signal_tower_connected);
    default:
        return GK_ERR_NOT_FOUND;
    }
}

int gk_mspec_get_flag(const gk_mspec *m, int fid)
{
    if (m == NULL) {
        return 0;
    }
    switch (fid) {
    case 1508: return m->has_warmup_program;
    case 1509: return m->has_warmup_cycle;
    case 1510: return m->accuracy_inspected;
    case 1512: return m->installed;
    case 1513: return m->leveled;
    case 1514: return m->anchored;
    case 1515: return m->damped;
    case 1516: return m->grounded;
    case 1517: return m->power_connected;
    case 1518: return m->air_connected;
    case 1519: return m->water_connected;
    case 1520: return m->chip_conveyor_connected;
    case 1521: return m->coolant_drain_connected;
    case 1522: return m->smoke_exhaust_connected;
    case 1523: return m->lighting_connected;
    case 1524: return m->network_connected;
    case 1525: return m->signal_tower_connected;
    default: return 0;
    }
}

const char *gk_mspec_field_name(int fid)
{
    switch (fid) {
    case 1476: return "nameplate";
    case 1477: return "serial-number";
    case 1478: return "manufacture-date";
    case 1479: return "weight";
    case 1480: return "power";
    case 1481: return "dimensions";
    case 1482: return "travel";
    case 1483: return "accuracy";
    case 1484: return "repeatability";
    case 1485: return "spindle-speed-range";
    case 1486: return "rapid-speed";
    case 1487: return "feed-range";
    case 1488: return "atc-capacity";
    case 1489: return "max-tool-diameter";
    case 1490: return "max-tool-length";
    case 1491: return "max-tool-weight";
    case 1492: return "table-size";
    case 1493: return "table-load";
    case 1494: return "tslot-size";
    case 1495: return "spindle-taper";
    case 1496: return "spindle-power";
    case 1497: return "spindle-torque";
    case 1498: return "coolant-tank";
    case 1499: return "air-pressure";
    case 1500: return "voltage";
    case 1501: return "frequency";
    case 1502: return "ambient-temperature";
    case 1503: return "ambient-humidity";
    case 1504: return "ip-rating";
    case 1505: return "noise";
    case 1506: return "vibration";
    case 1507: return "thermal-balance-time";
    case 1508: return "warmup-program";
    case 1509: return "warmup-cycle";
    case 1510: return "accuracy-inspection";
    case 1511: return "acceptance-standard";
    case 1512: return "installation";
    case 1513: return "leveling";
    case 1514: return "anchoring";
    case 1515: return "damping";
    case 1516: return "grounding";
    case 1517: return "power-hookup";
    case 1518: return "air-hookup";
    case 1519: return "water-hookup";
    case 1520: return "chip-conveyor-hookup";
    case 1521: return "coolant-drain-hookup";
    case 1522: return "smoke-exhaust-hookup";
    case 1523: return "lighting-hookup";
    case 1524: return "network-hookup";
    case 1525: return "signal-tower-hookup";
    default: return "unknown";
    }
}

/* ===================================================================
 * Derived checks
 * =================================================================== */

int gk_mspec_spindle_range_ok(const gk_mspec *m)
{
    if (m == NULL) {
        return 0;
    }
    return m->spindle_rpm_max > m->spindle_rpm_min && m->spindle_rpm_min > 0.0;
}

int gk_mspec_tool_fits(const gk_mspec *m, double dia_mm, double len_mm,
                       double weight_kg)
{
    if (m == NULL) {
        return 0;
    }
    return dia_mm <= m->max_tool_diameter_mm &&
           len_mm <= m->max_tool_length_mm &&
           weight_kg <= m->max_tool_weight_kg;
}

int gk_mspec_fully_hooked_up(const gk_mspec *m)
{
    if (m == NULL) {
        return 0;
    }
    return m->grounded && m->power_connected && m->air_connected &&
           m->water_connected && m->chip_conveyor_connected &&
           m->coolant_drain_connected && m->lighting_connected;
}

int gk_mspec_ready_for_use(const gk_mspec *m)
{
    if (m == NULL) {
        return 0;
    }
    return m->installed && m->leveled && m->anchored &&
           m->accuracy_inspected && gk_mspec_fully_hooked_up(m);
}
