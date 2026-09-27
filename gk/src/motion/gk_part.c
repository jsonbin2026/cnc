#include "gk/gk_part.h"

#include <string.h>

static void gk__part_copy(char *dst, size_t cap, const char *src)
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

void gk_part_id_init(gk_part_id *p, const char *code, const char *name,
                     const char *batch)
{
    if (p == NULL) {
        return;
    }
    memset(p, 0, sizeof(*p));
    gk__part_copy(p->code, sizeof(p->code), code);
    gk__part_copy(p->name, sizeof(p->name), name);
    gk__part_copy(p->batch, sizeof(p->batch), batch);
}

int gk_part_matches(const gk_part_id *p, const char *code)
{
    if (p == NULL || code == NULL) {
        return 0;
    }
    return strcmp(p->code, code) == 0;
}

const char *gk_part_doc_name(gk_part_doc_kind k)
{
    switch (k) {
    case GK_PART_DOC_ROUTE: return "route-card";
    case GK_PART_DOC_INSPECT: return "inspection-sheet";
    case GK_PART_DOC_REWORK: return "rework-sheet";
    case GK_PART_DOC_SCRAP: return "scrap-sheet";
    default: return "unknown";
    }
}

void gk_part_doc_init(gk_part_doc *d, gk_part_doc_kind k, const char *code,
                      const char *content)
{
    if (d == NULL) {
        return;
    }
    memset(d, 0, sizeof(*d));
    d->kind = k;
    gk__part_copy(d->code, sizeof(d->code), code);
    gk__part_copy(d->content, sizeof(d->content), content);
}

gk_status gk_part_doc_close(gk_part_doc *d)
{
    if (d == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if (d->content[0] == '\0') {
        return GK_ERR_STATE;
    }
    d->closed = 1;
    return GK_OK;
}

/* ===================================================================
 * Inventory (1377-1380)
 * =================================================================== */

void gk_part_inventory_init(gk_part_inventory *inv)
{
    if (inv == NULL) {
        return;
    }
    memset(inv, 0, sizeof(*inv));
}

static gk_part_stock_item *gk_part_find(gk_part_inventory *inv,
                                        const char *code)
{
    int i;
    if (inv == NULL || code == NULL) {
        return NULL;
    }
    for (i = 0; i < inv->count; i++) {
        if (strcmp(inv->items[i].code, code) == 0) {
            return &inv->items[i];
        }
    }
    return NULL;
}

gk_status gk_part_stock_in(gk_part_inventory *inv, const char *code,
                           const char *name, int qty, double unit_cost)
{
    gk_part_stock_item *it;
    if (inv == NULL || code == NULL || name == NULL || qty < 0 ||
        unit_cost < 0.0) {
        return GK_ERR_INVALID_ARG;
    }
    it = gk_part_find(inv, code);
    if (it != NULL) {
        it->quantity += qty;
        it->unit_cost = unit_cost;
        return GK_OK;
    }
    if (inv->count >= GK_PART_MAX) {
        return GK_ERR_OVERFLOW;
    }
    it = &inv->items[inv->count];
    memset(it, 0, sizeof(*it));
    gk__part_copy(it->code, sizeof(it->code), code);
    gk__part_copy(it->name, sizeof(it->name), name);
    it->quantity = qty;
    it->unit_cost = unit_cost;
    inv->count++;
    return GK_OK;
}

gk_status gk_part_stock_out(gk_part_inventory *inv, const char *code, int qty)
{
    gk_part_stock_item *it = gk_part_find(inv, code);
    if (it == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (qty < 0) {
        return GK_ERR_INVALID_ARG;
    }
    if (it->quantity < qty) {
        return GK_ERR_OUT_OF_RANGE;
    }
    it->quantity -= qty;
    return GK_OK;
}

int gk_part_inventory_qty(const gk_part_inventory *inv, const char *code)
{
    int i;
    if (inv == NULL || code == NULL) {
        return 0;
    }
    for (i = 0; i < inv->count; i++) {
        if (strcmp(inv->items[i].code, code) == 0) {
            return inv->items[i].quantity;
        }
    }
    return 0;
}

double gk_part_inventory_value(const gk_part_inventory *inv)
{
    double v = 0.0;
    int i;
    if (inv == NULL) {
        return 0.0;
    }
    for (i = 0; i < inv->count; i++) {
        v += inv->items[i].unit_cost * (double)inv->items[i].quantity;
    }
    return v;
}

gk_status gk_part_stocktake(gk_part_inventory *inv, const char *code,
                            int counted_qty)
{
    gk_part_stock_item *it = gk_part_find(inv, code);
    if (it == NULL) {
        return GK_ERR_NOT_FOUND;
    }
    if (counted_qty < 0) {
        return GK_ERR_INVALID_ARG;
    }
    it->quantity = counted_qty;
    return GK_OK;
}

/* ===================================================================
 * Shipment (1381-1385)
 * =================================================================== */

const char *gk_part_stage_name(gk_part_stage s)
{
    switch (s) {
    case GK_PART_STAGE_PACKAGING: return "packaging";
    case GK_PART_STAGE_TRANSPORT: return "transport";
    case GK_PART_STAGE_DELIVERED: return "delivered";
    case GK_PART_STAGE_AFTERSALES: return "after-sales";
    case GK_PART_STAGE_RECALLED: return "recalled";
    default: return "unknown";
    }
}

void gk_part_shipment_init(gk_part_shipment *sh, const char *code)
{
    if (sh == NULL) {
        return;
    }
    memset(sh, 0, sizeof(*sh));
    gk__part_copy(sh->code, sizeof(sh->code), code);
    sh->stage = GK_PART_STAGE_PACKAGING;
}

gk_status gk_part_shipment_advance(gk_part_shipment *sh, gk_part_stage stage,
                                   const char *carrier)
{
    if (sh == NULL) {
        return GK_ERR_INVALID_ARG;
    }
    if ((int)stage <= (int)sh->stage) {
        return GK_ERR_STATE;
    }
    sh->stage = stage;
    if (carrier != NULL) {
        gk__part_copy(sh->carrier, sizeof(sh->carrier), carrier);
    }
    return GK_OK;
}

int gk_part_is_recalled(const gk_part_shipment *sh)
{
    if (sh == NULL) {
        return 0;
    }
    return sh->stage == GK_PART_STAGE_RECALLED;
}
