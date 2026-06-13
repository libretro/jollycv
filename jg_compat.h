/*
Compatibility header for cores building against JG API 1.0.0.
Include this header conditionally:

#if JG_VERSION_NUMBER < 10100
#include "jg_compat.h"
#endif
*/

#ifndef JG_COMPAT_H
#define JG_COMPAT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _jg_biosinfo_t {
    const char *fname;
    const char *desc;
    const char *md5;
    int required;
    int group;
} jg_biosinfo_t;

jg_inputinfo_t* jg_get_inputlist(size_t*);
jg_setting_t* jg_get_dips(size_t*);
jg_biosinfo_t* jg_get_bioslist(size_t*);

#ifdef __cplusplus
}
#endif

#endif
