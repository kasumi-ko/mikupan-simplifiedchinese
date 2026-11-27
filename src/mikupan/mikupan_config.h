#ifndef MIKUPAN_MIKUPAN_CONFIG_H
#define MIKUPAN_MIKUPAN_CONFIG_H

#include "mikupan_basictypes.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

extern MikuPan_Config mikupan_configuration;

enum
{
    MIKUPAN_UI_FONT_MONOSPACE,
    MIKUPAN_UI_FONT_CENTURY,
    MIKUPAN_UI_FONT_CHANCERY,
    MIKUPAN_UI_FONT_WENKAI,
    MIKUPAN_UI_FONT_COUNT
};

void MikuPan_ConfigurationValidate(void);
void MikuPan_LoadConfiguration(const char *filename);
int MikuPan_SaveConfiguration(const char *filename);
int MikuPan_ResolveUserPath(const char *path, char *buffer,
                            size_t buffer_size);

#ifdef __cplusplus
}
#endif

#endif//MIKUPAN_MIKUPAN_CONFIG_H
