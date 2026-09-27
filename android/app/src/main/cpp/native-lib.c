#include <jni.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gk/gk_catalog.h"

/* Build a single string "id|name|domain|status" per feature, joined by '\n',
   so the Java side can render the whole catalog without many JNI calls. */
JNIEXPORT jstring JNICALL
Java_com_example_gk_MainActivity_nativeCatalogDump(JNIEnv *env, jobject thiz)
{
    size_t n = gk_catalog_count();
    const gk_feature *all = gk_catalog_all();
    size_t cap = n * 128 + 1;
    char *buf = (char *)malloc(cap);
    size_t used = 0;
    size_t i;

    (void)thiz;

    if (buf == NULL) {
        return (*env)->NewStringUTF(env, "");
    }
    buf[0] = '\0';

    for (i = 0; i < n; i++) {
        const gk_feature *f = &all[i];
        int written = snprintf(buf + used, cap - used, "%d|%s|%s|%s\n",
                               f->id,
                               f->name ? f->name : "",
                               gk_domain_name(f->domain),
                               gk_feature_status_name(f->status));
        if (written < 0 || (size_t)written >= cap - used) {
            break;
        }
        used += (size_t)written;
    }

    {
        jstring result = (*env)->NewStringUTF(env, buf);
        free(buf);
        return result;
    }
}

JNIEXPORT jint JNICALL
Java_com_example_gk_MainActivity_nativeFeatureCount(JNIEnv *env, jobject thiz)
{
    (void)env;
    (void)thiz;
    return (jint)gk_catalog_count();
}

JNIEXPORT jint JNICALL
Java_com_example_gk_MainActivity_nativeCountByStatus(JNIEnv *env, jobject thiz,
                                                     jint status)
{
    (void)env;
    (void)thiz;
    if (status < 0 || status > (jint)GK_STATUS_VERIFIED) {
        return 0;
    }
    return (jint)gk_catalog_count_by_status((gk_feature_status)status);
}

JNIEXPORT jint JNICALL
Java_com_example_gk_MainActivity_nativeValidate(JNIEnv *env, jobject thiz)
{
    int errors = 0;
    (void)env;
    (void)thiz;
    gk_catalog_validate(&errors);
    return (jint)errors;
}
