#ifndef IB_LONGVIEW_H
#define IB_LONGVIEW_H

#include <jni.h>

/* Main-Looper platform attachment query. This is not a browser task or
 * authentication decision; it only prevents the dormant reader replacing
 * the protected page's Android surface. */
int ib_longview_surface_visible(JNIEnv *env, jobject activity);

#endif
