#ifndef __CUSTOM_BINRES_THEME_LOAD_H__
#define __CUSTOM_BINRES_THEME_LOAD_H__

#include "gx_api.h"
void custom_binres_theme_unload(void **buff_addr);

UINT custom_binres_theme_load(GX_UBYTE *root_address, INT theme_id,
							  GX_THEME **returned_theme, void **buff_addr);

#endif