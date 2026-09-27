#ifndef GK_PART_H
#define GK_PART_H

#include <stddef.h>
#include "gk/gk_error.h"

#ifdef __cplusplus
extern "C" {
#endif

#define GK_PART_CODE 32
#define GK_PART_NAME 48
#define GK_PART_TEXT 128
#define GK_PART_MAX 128

/* ===================================================================
 * Batch 52: real workpiece management (1371-1385)
 * Prefix: gk_part_
 * =================================================================== */

/* 1371 coding / 1372 traceability */
typedef struct {
    char code[GK_PART_CODE];
    char name[GK_PART_NAME];
    char batch[GK_PART_CODE];
} gk_part_id;

void gk_part_id_init(gk_part_id *p, const char *code, const char *name,
                     const char *batch);
int gk_part_matches(const gk_part_id *p, const char *code);

/* 1373 route card / 1374 inspection sheet / 1375 rework / 1376 scrap */
typedef enum {
    GK_PART_DOC_ROUTE = 0,   /* 1373 */
    GK_PART_DOC_INSPECT,     /* 1374 */
    GK_PART_DOC_REWORK,      /* 1375 */
    GK_PART_DOC_SCRAP        /* 1376 */
} gk_part_doc_kind;

const char *gk_part_doc_name(gk_part_doc_kind k);

typedef struct {
    gk_part_doc_kind kind;
    char code[GK_PART_CODE];
    char content[GK_PART_TEXT];
    int closed;
} gk_part_doc;

void gk_part_doc_init(gk_part_doc *d, gk_part_doc_kind k, const char *code,
                      const char *content);
gk_status gk_part_doc_close(gk_part_doc *d);

/* 1377 in / 1378 out / 1379 stocktaking / 1380 inventory */
typedef struct {
    char code[GK_PART_CODE];
    char name[GK_PART_NAME];
    int quantity;
    double unit_cost;
} gk_part_stock_item;

typedef struct {
    gk_part_stock_item items[GK_PART_MAX];
    int count;
} gk_part_inventory;

void gk_part_inventory_init(gk_part_inventory *inv);
gk_status gk_part_stock_in(gk_part_inventory *inv, const char *code,
                           const char *name, int qty, double unit_cost);
gk_status gk_part_stock_out(gk_part_inventory *inv, const char *code, int qty);
int gk_part_inventory_qty(const gk_part_inventory *inv, const char *code);
double gk_part_inventory_value(const gk_part_inventory *inv);
gk_status gk_part_stocktake(gk_part_inventory *inv, const char *code,
                            int counted_qty);

/* 1381 packaging / 1382 transport / 1383 delivery / 1384 after-sales /
 * 1385 recall */
typedef enum {
    GK_PART_STAGE_PACKAGING = 0, /* 1381 */
    GK_PART_STAGE_TRANSPORT,     /* 1382 */
    GK_PART_STAGE_DELIVERED,     /* 1383 */
    GK_PART_STAGE_AFTERSALES,    /* 1384 */
    GK_PART_STAGE_RECALLED       /* 1385 */
} gk_part_stage;

const char *gk_part_stage_name(gk_part_stage s);

typedef struct {
    char code[GK_PART_CODE];
    gk_part_stage stage;
    char carrier[GK_PART_NAME];
} gk_part_shipment;

void gk_part_shipment_init(gk_part_shipment *sh, const char *code);
gk_status gk_part_shipment_advance(gk_part_shipment *sh, gk_part_stage stage,
                                   const char *carrier);
int gk_part_is_recalled(const gk_part_shipment *sh);

#ifdef __cplusplus
}
#endif

#endif /* GK_PART_H */
