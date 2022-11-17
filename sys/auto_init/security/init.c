#define ENABLE_DEBUG 0
#include "debug.h"


void auto_init_security(void)
{
#if IS_USED(MODULE_CRYPTOAUTHLIB)
    extern void auto_init_atca(void);
    DEBUG("auto_init_security: atca\n");
    auto_init_atca();
#endif
}
