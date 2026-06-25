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

typedef struct _jg_systeminfo_t {
    const char *name;
    const char *fname;
    const char *ext;
} jg_systeminfo_t;

typedef struct _jg_biosinfo_t {
    const char *fname;
    const char *desc;
    const char *md5;
    unsigned required;
    unsigned group;
} jg_biosinfo_t;

typedef struct _jg_mediainfo_t {
    const char *name;
    unsigned count;
    unsigned index;
    unsigned inserted;
} jg_mediainfo_t;

jg_inputinfo_t* jg_get_inputlist(size_t*);
jg_setting_t* jg_get_dips(size_t*);
jg_biosinfo_t* jg_get_bioslist(size_t*);
void jg_media_set(unsigned);
void jg_media_mount(unsigned);
jg_mediainfo_t* jg_get_mediainfo(void);
unsigned jg_api_version(void);
jg_systeminfo_t* jg_get_systemlist(size_t*);

#ifdef __cplusplus
}
#endif

#endif
