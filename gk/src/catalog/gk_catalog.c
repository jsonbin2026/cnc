#include "gk/gk_catalog.h"

#include <string.h>

typedef struct {
    const char *name;
    const char *code;
} domain_meta;

static const domain_meta k_domain_meta[GK_DOMAIN_COUNT] = {
    { "G 代码", "GCODE" },
    { "M 代码", "MCODE" },
    { "宏程序", "MACRO" },
    { "运动控制", "MOTION" },
    { "材料去除与工艺", "MATERIAL" },
    { "碰撞检测", "COLLISION" },
    { "机床类型", "MACHINE_TYPE" },
    { "CNC 系统", "CNC_SYSTEM" },
    { "机床动作", "MACHINE_ACTION" },
    { "测量与质检", "MEASURE" },
    { "教学系统", "TEACHING" },
    { "认知科学", "COGNITION" },
    { "故障与异常", "FAULT" },
    { "操作界面", "UI" },
    { "操作面板", "PANEL" },
    { "程序编辑", "EDITOR" },
    { "交互", "INTERACTION" },
    { "数据与文件", "DATA_FILE" },
    { "可视化分析", "VISUALIZATION" },
    { "数字化与工业接口", "INDUSTRIAL" },
    { "AI 与数据科学", "AI" },
    { "协作与课堂", "COLLABORATION" },
    { "内容生态", "CONTENT_ECOSYSTEM" },
    { "物理仿真深水区", "PHYSICS" },
    { "材料与工艺库", "MATERIAL_LIB" },
    { "制造全流程", "PROCESS_FLOW" },
    { "能源与成本", "ENERGY_COST" },
    { "维护与保养", "MAINTENANCE" },
    { "安全与规范", "SAFETY" },
    { "不确定性与极端场景", "UNCERTAINTY" },
    { "感官扩展", "SENSORY" },
    { "跨媒介表达", "CROSS_MEDIA" },
    { "逆向与叙事", "REVERSE_NARRATIVE" },
    { "跨尺度与跨时间", "CROSS_SCALE" },
    { "多物理场", "MULTIPHYSICS" },
    { "CAD/CAM 集成", "CADCAM" },
    { "平台与工程化", "PLATFORM" },
    { "商业模式", "BUSINESS" },
    { "无障碍与特殊需求", "ACCESSIBILITY" },
    { "数据科学层", "DATA_SCIENCE" },
    { "硬件在环 HIL", "HIL" },
    { "学术研究接口", "ACADEMIC" },
    { "安全合规", "COMPLIANCE" },
    { "其他高级功能", "ADVANCED" },
    { "真实机床特性", "REALISM" },
    { "最终新增", "REALISM_FINAL" },
};

static const char *const k_status_names[GK_STATUS_VERIFIED + 1] = {
    "NOT_IMPLEMENTED", "STUB", "PARTIAL", "IMPLEMENTED", "VERIFIED",
};

extern const gk_feature *g_gk_catalog_ptr;
extern const size_t g_gk_catalog_count;

const char *gk_domain_name(gk_domain domain)
{
    if ((int)domain < 0 || (int)domain >= (int)GK_DOMAIN_COUNT) {
        return "未知";
    }
    return k_domain_meta[(int)domain].name;
}

const char *gk_domain_code(gk_domain domain)
{
    if ((int)domain < 0 || (int)domain >= (int)GK_DOMAIN_COUNT) {
        return "UNKNOWN";
    }
    return k_domain_meta[(int)domain].code;
}

const char *gk_feature_status_name(gk_feature_status status)
{
    if ((int)status < 0 || (int)status > (int)GK_STATUS_VERIFIED) {
        return "UNKNOWN";
    }
    return k_status_names[(int)status];
}

size_t gk_catalog_count(void)
{
    return g_gk_catalog_count;
}

const gk_feature *gk_catalog_all(void)
{
    return g_gk_catalog_ptr;
}

const gk_feature *gk_catalog_get(int id)
{
    if (id < 1 || (size_t)id > g_gk_catalog_count) {
        return NULL;
    }
    return &g_gk_catalog_ptr[id - 1];
}

const gk_feature *gk_catalog_find_by_name(const char *name)
{
    size_t i;
    if (name == NULL) {
        return NULL;
    }
    for (i = 0; i < g_gk_catalog_count; ++i) {
        if (strcmp(g_gk_catalog_ptr[i].name, name) == 0) {
            return &g_gk_catalog_ptr[i];
        }
    }
    return NULL;
}

size_t gk_catalog_count_by_domain(gk_domain domain)
{
    size_t i;
    size_t n = 0;
    for (i = 0; i < g_gk_catalog_count; ++i) {
        if (g_gk_catalog_ptr[i].domain == domain) {
            n += 1;
        }
    }
    return n;
}

size_t gk_catalog_count_by_status(gk_feature_status status)
{
    size_t i;
    size_t n = 0;
    for (i = 0; i < g_gk_catalog_count; ++i) {
        if (g_gk_catalog_ptr[i].status == status) {
            n += 1;
        }
    }
    return n;
}

gk_status gk_catalog_validate(int *out_errors)
{
    size_t i;
    int errors = 0;
    if (g_gk_catalog_ptr == NULL) {
        if (out_errors != NULL) {
            *out_errors = 1;
        }
        return GK_ERR_STATE;
    }
    for (i = 0; i < g_gk_catalog_count; ++i) {
        const gk_feature *f = &g_gk_catalog_ptr[i];
        if (f->id != (int)(i + 1)) {
            errors += 1;
        }
        if (f->name == NULL || f->name[0] == '\0') {
            errors += 1;
        }
        if ((int)f->domain < 0 || (int)f->domain >= (int)GK_DOMAIN_COUNT) {
            errors += 1;
        }
    }
    if (out_errors != NULL) {
        *out_errors = errors;
    }
    return errors == 0 ? GK_OK : GK_ERR_OUT_OF_RANGE;
}
