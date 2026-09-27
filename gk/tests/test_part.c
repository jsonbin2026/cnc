#include "gk_test.h"
#include "gk/gk_part.h"

#include <math.h>

static void test_id_doc(void)
{
    gk_part_id p;
    gk_part_doc d;
    gk_part_id_init(&p, "P-100", "shaft", "B2024");
    GK_CHECK_STR_EQ(p.code, "P-100");
    GK_CHECK_STR_EQ(p.batch, "B2024");
    GK_CHECK_EQ_INT(gk_part_matches(&p, "P-100"), 1);
    GK_CHECK_EQ_INT(gk_part_matches(&p, "P-101"), 0);
    GK_CHECK_STR_EQ(gk_part_doc_name(GK_PART_DOC_SCRAP), "scrap-sheet");
    gk_part_doc_init(&d, GK_PART_DOC_ROUTE, "P-100", "turn-drill-mill");
    GK_CHECK_EQ_INT(d.closed, 0);
    GK_CHECK(gk_part_doc_close(&d) == GK_OK);
    GK_CHECK_EQ_INT(d.closed, 1);
    gk_part_doc_init(&d, GK_PART_DOC_ROUTE, "P-100", "");
    GK_CHECK(gk_part_doc_close(&d) == GK_ERR_STATE);
}

static void test_inventory(void)
{
    gk_part_inventory inv;
    gk_part_inventory_init(&inv);
    GK_CHECK(gk_part_stock_in(&inv, "P-100", "shaft", 10, 5.0) == GK_OK);
    GK_CHECK(gk_part_stock_in(&inv, "P-200", "plate", 4, 20.0) == GK_OK);
    GK_CHECK(gk_part_stock_in(&inv, "P-100", "shaft", 5, 6.0) == GK_OK);
    GK_CHECK_EQ_INT(inv.count, 2);
    GK_CHECK_EQ_INT(gk_part_inventory_qty(&inv, "P-100"), 15);
    GK_CHECK(fabs(gk_part_inventory_value(&inv) - (15.0 * 6.0 + 80.0)) < 1e-9);
    GK_CHECK(gk_part_stock_out(&inv, "P-100", 5) == GK_OK);
    GK_CHECK_EQ_INT(gk_part_inventory_qty(&inv, "P-100"), 10);
    GK_CHECK(gk_part_stock_out(&inv, "P-100", 99) == GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_part_stock_out(&inv, "P-999", 1) == GK_ERR_NOT_FOUND);
    GK_CHECK(gk_part_stocktake(&inv, "P-200", 7) == GK_OK);
    GK_CHECK_EQ_INT(gk_part_inventory_qty(&inv, "P-200"), 7);
    GK_CHECK(gk_part_stocktake(&inv, "P-999", 1) == GK_ERR_NOT_FOUND);
}

static void test_shipment(void)
{
    gk_part_shipment sh;
    GK_CHECK_STR_EQ(gk_part_stage_name(GK_PART_STAGE_DELIVERED), "delivered");
    gk_part_shipment_init(&sh, "P-100");
    GK_CHECK_EQ_INT(sh.stage, GK_PART_STAGE_PACKAGING);
    GK_CHECK_EQ_INT(gk_part_is_recalled(&sh), 0);
    GK_CHECK(gk_part_shipment_advance(&sh, GK_PART_STAGE_TRANSPORT, "truck-1") ==
             GK_OK);
    GK_CHECK_STR_EQ(sh.carrier, "truck-1");
    GK_CHECK(gk_part_shipment_advance(&sh, GK_PART_STAGE_PACKAGING, "") ==
             GK_ERR_STATE);
    GK_CHECK(gk_part_shipment_advance(&sh, GK_PART_STAGE_DELIVERED, "") ==
             GK_OK);
    GK_CHECK(gk_part_shipment_advance(&sh, GK_PART_STAGE_RECALLED, "") ==
             GK_OK);
    GK_CHECK_EQ_INT(gk_part_is_recalled(&sh), 1);
}

int main(void)
{
    test_id_doc();
    test_inventory();
    test_shipment();
    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
