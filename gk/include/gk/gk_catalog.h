#ifndef GK_CATALOG_H
#define GK_CATALOG_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    GK_DOMAIN_GCODE = 0,
    GK_DOMAIN_MCODE,
    GK_DOMAIN_MACRO,
    GK_DOMAIN_MOTION,
    GK_DOMAIN_MATERIAL,
    GK_DOMAIN_COLLISION,
    GK_DOMAIN_MACHINE_TYPE,
    GK_DOMAIN_CNC_SYSTEM,
    GK_DOMAIN_MACHINE_ACTION,
    GK_DOMAIN_MEASURE,
    GK_DOMAIN_TEACHING,
    GK_DOMAIN_COGNITION,
    GK_DOMAIN_FAULT,
    GK_DOMAIN_UI,
    GK_DOMAIN_PANEL,
    GK_DOMAIN_EDITOR,
    GK_DOMAIN_INTERACTION,
    GK_DOMAIN_DATA_FILE,
    GK_DOMAIN_VISUALIZATION,
    GK_DOMAIN_INDUSTRIAL,
    GK_DOMAIN_AI,
    GK_DOMAIN_COLLABORATION,
    GK_DOMAIN_CONTENT_ECOSYSTEM,
    GK_DOMAIN_PHYSICS,
    GK_DOMAIN_MATERIAL_LIB,
    GK_DOMAIN_PROCESS_FLOW,
    GK_DOMAIN_ENERGY_COST,
    GK_DOMAIN_MAINTENANCE,
    GK_DOMAIN_SAFETY,
    GK_DOMAIN_UNCERTAINTY,
    GK_DOMAIN_SENSORY,
    GK_DOMAIN_CROSS_MEDIA,
    GK_DOMAIN_REVERSE_NARRATIVE,
    GK_DOMAIN_CROSS_SCALE,
    GK_DOMAIN_MULTIPHYSICS,
    GK_DOMAIN_CADCAM,
    GK_DOMAIN_PLATFORM,
    GK_DOMAIN_BUSINESS,
    GK_DOMAIN_ACCESSIBILITY,
    GK_DOMAIN_DATA_SCIENCE,
    GK_DOMAIN_HIL,
    GK_DOMAIN_ACADEMIC,
    GK_DOMAIN_COMPLIANCE,
    GK_DOMAIN_ADVANCED,
    GK_DOMAIN_REALISM,
    GK_DOMAIN_REALISM_FINAL,
    GK_DOMAIN_COUNT
} gk_domain;

typedef enum {
    GK_STATUS_NOT_IMPLEMENTED = 0,
    GK_STATUS_STUB,
    GK_STATUS_PARTIAL,
    GK_STATUS_IMPLEMENTED,
    GK_STATUS_VERIFIED
} gk_feature_status;

typedef struct {
    int id;
    const char *name;
    gk_domain domain;
    gk_feature_status status;
    const char *impl_symbol;
} gk_feature;

const char *gk_domain_name(gk_domain domain);
const char *gk_domain_code(gk_domain domain);
const char *gk_feature_status_name(gk_feature_status status);

size_t gk_catalog_count(void);
const gk_feature *gk_catalog_all(void);
const gk_feature *gk_catalog_get(int id);
const gk_feature *gk_catalog_find_by_name(const char *name);

size_t gk_catalog_count_by_domain(gk_domain domain);
size_t gk_catalog_count_by_status(gk_feature_status status);

gk_status gk_catalog_validate(int *out_errors);

#ifdef __cplusplus
}
#endif

#endif
