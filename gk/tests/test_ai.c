#include "gk_test.h"

#include "gk/gk_ai.h"
#include "gk/gk_iot.h"
#include "gk/gk_viz.h"

#include <math.h>
#include <string.h>

static int near(double a, double b, double eps)
{
    return fabs(a - b) <= eps;
}

/* ---------------- visualization ---------------- */

static void test_scope_logger(void)
{
    gk_scope s;
    gk_logger l;
    double vals[2] = {1.0, 2.0};
    char buf[2048];

    gk_scope_init(&s);
    GK_CHECK_EQ_INT(gk_scope_add_channel(&s, "X"), 0);
    GK_CHECK_EQ_INT(gk_scope_add_channel(&s, "Y"), 1);
    gk_scope_push(&s, 0, 1.0);
    gk_scope_push(&s, 0, -3.0);
    gk_scope_push(&s, 1, 2.0);
    GK_CHECK(near(gk_scope_value(&s, 0, 0), 1.0, 1e-9));
    GK_CHECK(near(gk_scope_peak(&s, 0), 3.0, 1e-9));
    GK_CHECK(near(gk_scope_mean(&s, 0), -1.0, 1e-9));
    GK_CHECK(near(gk_scope_value(&s, 0, 9), 0.0, 1e-9));
    GK_CHECK(near(gk_scope_peak(&s, 9), 0.0, 1e-9));

    gk_logger_init(&l, 2, 10.0);
    GK_CHECK_EQ_INT(gk_logger_record(&l, 0.0, vals), GK_OK);
    vals[0] = 5.0;
    GK_CHECK_EQ_INT(gk_logger_record(&l, 1.0, vals), GK_OK);
    GK_CHECK_EQ_INT(l.count, 2);
    GK_CHECK(near(gk_logger_span(&l), 1.0, 1e-9));
    GK_CHECK(gk_logger_at(&l, 5) == NULL);
    GK_CHECK(gk_logger_export(&l, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "time,ch0,ch1") != NULL);

    GK_CHECK(gk_viz_overlay(&s, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "OVERLAY") != NULL);
    GK_CHECK(gk_viz_export_chart(&s, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "CHART|channels=2") != NULL);
}

static void test_fields(void)
{
    gk_field f;
    gk_vector_field vf;
    gk_field_init(&f, 4, 4);
    gk_field_set(&f, 0, 0, 1.0);
    gk_field_set(&f, 3, 3, 5.0);
    GK_CHECK(near(gk_field_get(&f, 0, 0), 1.0, 1e-9));
    GK_CHECK(near(gk_field_mean(&f), 6.0 / 16.0, 1e-9));
    gk_field_rescale(&f);
    GK_CHECK(near(f.min, 0.0, 1e-9));
    GK_CHECK(near(f.max, 5.0, 1e-9));
    GK_CHECK(near(gk_field_max_deviation(&f, 3.0), 3.0, 1e-9));
    GK_CHECK_EQ_INT(gk_field_set(&f, 9, 0, 1.0), GK_ERR_OUT_OF_RANGE);

    gk_vfield_init(&vf, 2, 2);
    gk_vfield_set(&vf, 0, 0, 3.0, 4.0);
    GK_CHECK(near(gk_vfield_magnitude(&vf, 0, 0), 5.0, 1e-9));
    GK_CHECK(near(gk_vfield_magnitude(&vf, 9, 9), 0.0, 1e-9));
}

static void test_gantt_timeline_tree(void)
{
    gk_gantt g;
    gk_timeline t;
    gk_process_tree p;
    gk_section sec;
    gk_dual_view dv;

    gk_gantt_init(&g);
    gk_gantt_add(&g, "rough", 0.0, 10.0, 1);
    gk_gantt_add(&g, "finish", 8.0, 15.0, 2);
    GK_CHECK(near(gk_gantt_total(&g), 15.0, 1e-9));
    GK_CHECK_EQ_INT(gk_gantt_add(&g, "bad", 5.0, 1.0, 0), GK_ERR_INVALID_ARG);

    gk_timeline_init(&t);
    gk_timeline_add(&t, 0.0, 10.0, 0);
    gk_timeline_add(&t, 1.0, 90.0, 41);
    gk_timeline_add(&t, 2.0, 30.0, 0);
    GK_CHECK(near(t.min_load, 10.0, 1e-9));
    GK_CHECK(near(t.max_load, 90.0, 1e-9));
    GK_CHECK_EQ_INT(gk_timeline_alarm_count(&t), 1);

    gk_process_tree_init(&p);
    gk_process_add(&p, 1, "setup", -1, 5.0);
    gk_process_add(&p, 2, "rough", 1, 10.0);
    gk_process_add(&p, 3, "finish", 1, 7.0);
    GK_CHECK(near(gk_process_subtree_duration(&p, 1), 22.0, 1e-9));
    GK_CHECK(near(gk_process_subtree_duration(&p, 2), 10.0, 1e-9));

    gk_section_init(&sec);
    gk_section_add(&sec, -1.0);
    gk_section_add(&sec, -5.0);
    gk_section_add(&sec, -2.0);
    GK_CHECK(near(gk_section_min(&sec), -5.0, 1e-9));
    GK_CHECK(near(gk_section_depth_range(&sec), 4.0, 1e-9));

    gk_dual_view_init(&dv, "X");
    gk_dual_view_push(&dv, 10.0, 10.2);
    gk_dual_view_push(&dv, 20.0, 19.5);
    GK_CHECK(near(gk_dual_view_max_error(&dv), 0.5, 1e-9));
}

/* ---------------- IoT ---------------- */

static void test_bus(void)
{
    gk_data_bus b;
    gk_data_bus_init(&b);
    GK_CHECK_EQ_INT(gk_bus_add_double(&b, "x", 1.5), GK_OK);
    GK_CHECK_EQ_INT(gk_bus_add_int(&b, "tool", 7), GK_OK);
    GK_CHECK_EQ_INT(gk_bus_add_bool(&b, "run", 1), GK_OK);
    GK_CHECK_EQ_INT(gk_bus_tag_count(&b), 3);
    GK_CHECK(near(gk_bus_get_double(&b, "x"), 1.5, 1e-9));
    GK_CHECK_EQ_INT(gk_bus_get_int(&b, "tool"), 7);
    GK_CHECK_EQ_INT(gk_bus_set_double(&b, "x", 9.0), GK_OK);
    GK_CHECK(near(gk_bus_get_double(&b, "x"), 9.0, 1e-9));
    GK_CHECK_EQ_INT(gk_bus_set_double(&b, "nope", 1.0), GK_ERR_NOT_FOUND);
    GK_CHECK_EQ_INT(gk_bus_tag_index(&b, "tool"), 1);
    GK_CHECK_EQ_INT(gk_bus_tag_index(&b, "z"), -1);
}

static void test_opcua_mtconnect_mqtt(void)
{
    gk_opcua_server srv;
    gk_opcua_client cli;
    gk_mtconnect mt;
    gk_mqtt mq;
    char buf[256];

    gk_opcua_server_init(&srv);
    GK_CHECK_EQ_INT(srv.port, 4840);
    GK_CHECK_EQ_INT(gk_opcua_server_start(&srv, 4841), GK_OK);
    GK_CHECK(srv.server_running);
    GK_CHECK(strstr(srv.endpoint, "4841") != NULL);
    GK_CHECK_EQ_INT(gk_opcua_server_stop(&srv), GK_OK);
    GK_CHECK(!srv.server_running);

    gk_opcua_client_init(&cli);
    GK_CHECK_EQ_INT(gk_opcua_read(&cli, "ns=2;s=X", NULL), GK_ERR_INVALID_ARG);
    {
        double v = 0.0;
        GK_CHECK_EQ_INT(gk_opcua_read(&cli, "ns=2;s=X", &v), GK_ERR_STATE);
        GK_CHECK_EQ_INT(gk_opcua_connect(&cli, "opc.tcp://host:4840"), GK_OK);
        GK_CHECK_EQ_INT(gk_opcua_read(&cli, "ns=2;s=X", &v), GK_OK);
        GK_CHECK(cli.read_count == 1);
        GK_CHECK_EQ_INT(gk_opcua_write(&cli, "ns=2;s=X", 1.0), GK_OK);
    }

    gk_mtconnect_init(&mt);
    GK_CHECK(gk_mtconnect_probe(&mt, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "GK-CNC") != NULL);

    gk_mqtt_init(&mq);
    GK_CHECK_EQ_INT(gk_mqtt_publish(&mq, "t", "p"), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_mqtt_connect(&mq, "localhost", 1883), GK_OK);
    GK_CHECK_EQ_INT(gk_mqtt_publish(&mq, "cnc/x", "1.5"), GK_OK);
    GK_CHECK_EQ_INT(mq.published, 1);
}

static void test_modbus_fieldbus_api(void)
{
    gk_modbus mb;
    gk_fieldbus fb;
    gk_api_service api;

    gk_modbus_init(&mb, 1);
    GK_CHECK_EQ_INT(gk_modbus_write_register(&mb, 0, 100), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_modbus_connect(&mb), GK_OK);
    GK_CHECK_EQ_INT(gk_modbus_write_register(&mb, 5, 1234), GK_OK);
    GK_CHECK_EQ_INT(gk_modbus_read_register(&mb, 5), 1234);
    GK_CHECK_EQ_INT(gk_modbus_write_register(&mb, 99, 1), GK_ERR_OUT_OF_RANGE);
    GK_CHECK_EQ_INT(gk_modbus_write_coil(&mb, 2, 1), GK_OK);
    GK_CHECK_EQ_INT(gk_modbus_read_coil(&mb, 2), 1);

    GK_CHECK_STR_EQ(gk_fieldbus_name(GK_FIELDBUS_ETHERCAT), "EtherCAT");
    GK_CHECK_EQ_INT(gk_fieldbus_init(&fb, GK_FIELDBUS_PROFINET, 4), GK_OK);
    GK_CHECK_EQ_INT(fb.slaves, 4);
    GK_CHECK(gk_fieldbus_start(&fb) == GK_OK);
    gk_fieldbus_tick(&fb, 1.0);
    GK_CHECK_EQ_INT(gk_fieldbus_init(&fb, GK_FIELDBUS_PROFINET, 0),
                    GK_ERR_INVALID_ARG);

    GK_CHECK_STR_EQ(gk_api_kind_name(GK_API_WEBSOCKET), "WebSocket");
    GK_CHECK_EQ_INT(gk_api_init(&api, GK_API_REST, 8080), GK_OK);
    GK_CHECK_EQ_INT(gk_api_request(&api, "/x", 1), GK_ERR_STATE);
    GK_CHECK_EQ_INT(gk_api_start(&api), GK_OK);
    GK_CHECK_EQ_INT(gk_api_request(&api, "/x", 1), GK_OK);
    GK_CHECK_EQ_INT(gk_api_request(&api, "/bad", 0), GK_ERR_PARSE);
    GK_CHECK_EQ_INT(gk_api_request_count(&api), 2);
    GK_CHECK_EQ_INT(api.errors, 1);
}

static void test_twin_tsdb(void)
{
    gk_digital_twin twin;
    gk_data_bus b;
    gk_tsdb db;
    double out[16];

    gk_data_bus_init(&b);
    gk_bus_add_double(&b, "virt_x", 0.0);
    gk_digital_twin_init(&twin);
    GK_CHECK_EQ_INT(gk_twin_map_add(&twin, "phys_x", "virt_x", 2.0, 1.0),
                    GK_OK);
    GK_CHECK(near(gk_twin_translate(&twin, "phys_x", 3.0), 7.0, 1e-9));
    GK_CHECK(near(gk_twin_translate(&twin, "other", 3.0), 3.0, 1e-9));
    GK_CHECK_EQ_INT(gk_twin_sync(&twin, &b, 3.0), 1);
    GK_CHECK(near(gk_bus_get_double(&b, "virt_x"), 7.0, 1e-9));

    gk_tsdb_init(&db, "cnc");
    gk_tsdb_write(&db, "load", 0.0, 10.0);
    gk_tsdb_write(&db, "load", 1.0, 20.0);
    gk_tsdb_write(&db, "load", 2.0, 30.0);
    gk_tsdb_write(&db, "temp", 1.0, 50.0);
    GK_CHECK_EQ_INT(gk_tsdb_query(&db, "load", 0.0, 1.5, out, 16), 2);
    GK_CHECK(near(gk_tsdb_mean(&db, "load"), 20.0, 1e-9));
    GK_CHECK(near(gk_tsdb_mean(&db, "temp"), 50.0, 1e-9));
}

static void test_acq_playback(void)
{
    gk_data_bus b;
    gk_acquisition a;
    gk_tsdb db;
    gk_playback p;
    gk_ts_point pt;
    char buf[256];

    gk_data_bus_init(&b);
    gk_acquisition_init(&a, &b, 0.5);
    GK_CHECK_EQ_INT(gk_acquisition_start(&a, 0.0), GK_OK);
    GK_CHECK_EQ_INT(gk_acquisition_tick(&a, 0.2), 0);
    GK_CHECK_EQ_INT(gk_acquisition_tick(&a, 0.4), 1);
    GK_CHECK_EQ_INT(a.samples, 1);

    gk_tsdb_init(&db, "play");
    gk_tsdb_write(&db, "x", 0.0, 1.0);
    gk_tsdb_write(&db, "x", 1.0, 2.0);
    gk_playback_init(&p, &db);
    GK_CHECK_EQ_INT(gk_playback_step(&p, 0.1, &pt), 0);
    GK_CHECK_EQ_INT(gk_playback_start(&p, 2.0), GK_OK);
    GK_CHECK_EQ_INT(gk_playback_step(&p, 0.1, &pt), 1);
    GK_CHECK(near(pt.value, 1.0, 1e-9));
    GK_CHECK_EQ_INT(gk_playback_step(&p, 0.1, &pt), 1);
    GK_CHECK(near(pt.value, 2.0, 1e-9));
    GK_CHECK_EQ_INT(gk_playback_step(&p, 0.1, &pt), 0);

    GK_CHECK(gk_tsviz_render(&db, "x", 40, 10, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "tag=x") != NULL);
    GK_CHECK_STR_EQ(gk_iot_protocol_name(1), "opcua");
}

/* ---------------- AI ---------------- */

static void test_ai_basic(void)
{
    gk_error_explanation ex;
    gk_param_reco pr;
    char buf[512];

    GK_CHECK_EQ_INT(gk_ai_explain_error(41, &ex), GK_OK);
    GK_CHECK(strstr(ex.explanation, "limit") != NULL);
    GK_CHECK_EQ_INT(gk_ai_explain_error(999, &ex), GK_OK);

    GK_CHECK_EQ_INT(gk_ai_recommend_params("aluminum", 10.0, 2, &pr), GK_OK);
    GK_CHECK(pr.speed > 0.0);
    GK_CHECK(near(pr.depth, 5.0, 1e-9));
    GK_CHECK_EQ_INT(gk_ai_recommend_params(NULL, 10.0, 2, &pr),
                    GK_ERR_INVALID_ARG);

    GK_CHECK(gk_ai_nl_to_gcode("please rapid to origin", buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "G00") != NULL);
    GK_CHECK(gk_ai_nl_to_gcode("drill a hole", buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "G81") != NULL);
}

static void test_ai_voice_toolset(void)
{
    gk_voice_assistant va;
    gk_auto_toolset ts;

    gk_voice_assistant_init(&va);
    GK_CHECK(gk_voice_listen(&va, "start cycle") == GK_OK);
    GK_CHECK_STR_EQ(va.response, "Cycle start");
    GK_CHECK(va.recognized);
    GK_CHECK(gk_voice_listen(&va, "blah") == GK_OK);
    GK_CHECK(!va.recognized);

    gk_auto_toolset_init(&ts);
    GK_CHECK_EQ_INT(gk_auto_toolset_probe(&ts, 1.0, 2.0, 3.0), GK_OK);
    GK_CHECK(near(ts.measured_offset, 3.0, 1e-9));
    GK_CHECK_EQ_INT(ts.probes, 1);
}

static void test_ai_anomaly(void)
{
    gk_anomaly_detector d;
    double times[3] = {0.0, 1.0, 2.0};
    double wear[3] = {0.0, 0.1, 0.2};
    int i;

    gk_anomaly_init(&d, 2.0);
    for (i = 0; i < 10; ++i) {
        gk_anomaly_observe(&d, 10.0);
    }
    GK_CHECK(near(d.mean, 10.0, 1e-9));
    GK_CHECK(!gk_anomaly_is_anomaly(&d, 10.0));
    GK_CHECK(near(gk_ai_predict_tool_life(times, wear, 3, 1.0), 10.0, 1e-6));
    GK_CHECK(near(gk_ai_predict_tool_life(times, wear, 3, 0.0), 0.0, 1e-6));
}

static void test_ai_profile(void)
{
    gk_learner_profile p;
    char buf[128];
    int ids[8];

    gk_learner_profile_init(&p, "alice");
    gk_learner_add_ability(&p, "gcode", 0.8);
    gk_learner_add_ability(&p, "setup", 0.6);
    GK_CHECK(near(p.overall, 0.7, 1e-9));
    GK_CHECK(gk_learner_radar(&p, buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "80,60") != NULL);

    GK_CHECK_EQ_INT(gk_ai_adaptive_difficulty(0.5, 0.75), 3);

    {
        double ab[3] = {0.5, 0.9, 0.3};
        int n = gk_ai_recommend_courses(ab, 3, ids, 8);
        GK_CHECK_EQ_INT(n, 2);
        GK_CHECK_EQ_INT(ids[0], 0);
        GK_CHECK_EQ_INT(ids[1], 2);
    }
    {
        double sc[4] = {60, 70, 80, 90};
        double pred = gk_ai_predict_score(sc, 4);
        GK_CHECK(pred > 70.0 && pred < 90.0);
    }
}

static void test_ai_error_pattern(void)
{
    gk_error_pattern ep;
    gk_error_pattern_init(&ep);
    gk_error_pattern_add(&ep, 1);
    gk_error_pattern_add(&ep, 2);
    gk_error_pattern_add(&ep, 3);
    gk_error_pattern_add(&ep, 1);
    gk_error_pattern_add(&ep, 2);
    gk_error_pattern_add(&ep, 3);
    GK_CHECK_EQ_INT(gk_error_pattern_mine(&ep), 3);
    GK_CHECK_EQ_INT(ep.pattern_len, 3);
    GK_CHECK_EQ_INT(ep.pattern[0], 1);
}

static void test_ai_clustering_teaching(void)
{
    gk_clustering c;
    double f1[2] = {0.0, 0.0};
    double f2[2] = {10.0, 10.0};
    double pre[3] = {40, 50, 60};
    double post[3] = {60, 70, 80};
    gk_teaching_effect e;

    gk_clustering_init(&c, 2);
    gk_clustering_add(&c, "a", f1, 2);
    gk_clustering_add(&c, "b", f1, 2);
    gk_clustering_add(&c, "c", f2, 2);
    GK_CHECK_EQ_INT(gk_clustering_run(&c), 2);
    GK_CHECK_EQ_INT(c.labels[0], c.labels[1]);
    GK_CHECK(c.labels[2] != c.labels[0]);
    GK_CHECK_EQ_INT(gk_clustering_size(&c, c.labels[2]), 1);
    {
        double sc[3] = {10, 20, 30};
        GK_CHECK(near(gk_ai_group_average(sc, 3), 20.0, 1e-9));
    }

    GK_CHECK_EQ_INT(gk_ai_teaching_effect(pre, post, 3, &e), GK_OK);
    GK_CHECK(near(e.pre_mean, 50.0, 1e-9));
    GK_CHECK(near(e.post_mean, 70.0, 1e-9));
    GK_CHECK(near(gk_ai_effect_size(&e), 20.0, 1e-9));
}

static void test_ai_kg_qa(void)
{
    gk_knowledge_graph g;
    int nb[8];
    gk_qa_base q;

    gk_kg_init(&g);
    gk_kg_add_node(&g, 0, "G00", "gcode");
    gk_kg_add_node(&g, 1, "G01", "gcode");
    gk_kg_add_node(&g, 2, "G02", "gcode");
    gk_kg_add_edge(&g, 0, 1, 1.0);
    gk_kg_add_edge(&g, 1, 2, 1.0);
    GK_CHECK_EQ_INT(gk_kg_neighbors(&g, 0, nb, 8), 1);
    GK_CHECK_EQ_INT(nb[0], 1);
    GK_CHECK_EQ_INT(gk_kg_path_length(&g, 0, 2), 2);
    GK_CHECK_EQ_INT(gk_kg_path_length(&g, 2, 0), -1);
    GK_CHECK_EQ_INT(gk_kg_path_length(&g, 0, 0), 0);

    gk_qa_init(&q);
    gk_qa_add(&q, "what is G00?", "G00 is rapid positioning", "G00 rapid");
    gk_qa_add(&q, "how to drill?", "Use G81", "drill G81");
    GK_CHECK(strstr(gk_qa_ask(&q, "explain G00 rapid move"), "rapid") != NULL);
    GK_CHECK(gk_qa_ask(&q, "unrelated topic") == NULL);
}

static void test_ai_vision_voice(void)
{
    gk_image im;
    char buf[256];
    gk_voice_dialog vd;
    gk_multimodal_input mmi;
    gk_multimodal_result mmr;

    gk_image_init(&im, 8, 8);
    gk_image_set(&im, 0, 0, 255);
    GK_CHECK(near(gk_image_density(&im), 255.0 / (255.0 * 64.0), 1e-9));
    GK_CHECK_EQ_INT(gk_image_classify(&im), 0);
    GK_CHECK_EQ_INT(gk_image_set(&im, 99, 0, 1), GK_ERR_OUT_OF_RANGE);
    GK_CHECK(gk_vqa_answer(&im, "is it bright?", buf, sizeof(buf)) > 0);
    GK_CHECK(strstr(buf, "no") != NULL);

    gk_voice_dialog_init(&vd);
    GK_CHECK(gk_voice_dialog_say(&vd, "hello", buf, sizeof(buf)) == GK_OK);
    GK_CHECK_EQ_INT(vd.turns, 1);

    GK_CHECK_STR_EQ(gk_gesture_name(GK_GESTURE_PINCH), "pinch");
    GK_CHECK_EQ_INT(gk_gesture_classify(0, 0, -50, 0, 0.0),
                    GK_GESTURE_SWIPE_LEFT);
    GK_CHECK_EQ_INT(gk_gesture_classify(0, 0, 1, 1, 0.0), GK_GESTURE_TAP);
    GK_CHECK_EQ_INT(gk_gesture_classify(0, 0, 0, 50, 0.0),
                    GK_GESTURE_ROTATE);
    GK_CHECK_EQ_INT(gk_gesture_classify(0, 0, 0, 0, 0.5),
                    GK_GESTURE_PINCH);

    GK_CHECK_STR_EQ(gk_face_expression_name(GK_FACE_FRUSTRATED), "frustrated");
    GK_CHECK_EQ_INT(gk_face_classify(0.8, 0.0), GK_FACE_HAPPY);
    GK_CHECK_EQ_INT(gk_face_classify(-0.8, 0.8), GK_FACE_FRUSTRATED);
    GK_CHECK_EQ_INT(gk_face_classify(-0.8, 0.0), GK_FACE_CONFUSED);
    GK_CHECK_EQ_INT(gk_face_classify(0.0, 0.0), GK_FACE_NEUTRAL);

    memset(&mmi, 0, sizeof(mmi));
    mmi.error_score = 1;
    mmi.voice_score = 1;
    mmi.face_score = 1;
    GK_CHECK(gk_multimodal_diagnose(&mmi, &mmr) == GK_OK);
    GK_CHECK(near(mmr.confidence, 0.75, 1e-9));
    GK_CHECK(strstr(mmr.diagnosis, "struggling") != NULL);
}

int main(void)
{
    test_scope_logger();
    test_fields();
    test_gantt_timeline_tree();
    test_bus();
    test_opcua_mtconnect_mqtt();
    test_modbus_fieldbus_api();
    test_twin_tsdb();
    test_acq_playback();
    test_ai_basic();
    test_ai_voice_toolset();
    test_ai_anomaly();
    test_ai_profile();
    test_ai_error_pattern();
    test_ai_clustering_teaching();
    test_ai_kg_qa();
    test_ai_vision_voice();

    printf("tests run: %d, failed: %d\n", g_tests_run, g_tests_failed);
    return g_tests_failed == 0 ? 0 : 1;
}
