#include <stdio.h>
#include <errno.h>

#include <kernel.h>
#include <init.h>

#define OBSERVER_CLASS_DEFINE
#include "gx_api.h"

#include "guix_input_zephyr.h"


#define FOREACH_ITEM(_iter, _head) \
	for (_iter = (_head); _iter != NULL; _iter = _iter->next)

static struct guix_driver *guix_driver_list;
static struct gxres_device *guix_resource_dev_list;

UINT __weak guix_main(UINT disp_id, struct guix_driver *drv)
{
	printk("Warnning***: Please define"
		   " \"UINT guix_main(UINT disp_id, struct guix_driver *drv)\" "
		   "to initialize GUI!\n");
	return GX_FAILURE;
}

UINT guix_binres_load(struct guix_driver *drv, INT theme_id, struct GX_THEME_STRUCT **theme,
					  struct GX_STRING_STRUCT ***language)
{
	UINT ret = GX_SUCCESS;

	if (!drv)
		return GX_FAILURE;
	drv->map_base = drv->dev->mmap(~0u);
	if (drv->map_base == INVALID_MAP_ADDR)
		return GX_FAILURE;

	/* Load gui resource */
	if (language) {
		ret = gx_binres_language_table_load_ext(drv->map_base, language);
		if (ret != GX_SUCCESS) {
			printk("%s load gui language resource failed\n", __func__);
			goto out;
		}
	}
	if (theme) {
		ret = gx_binres_theme_load(drv->map_base, theme_id, theme);
		if (ret != GX_SUCCESS) {
			printk("%s load gui theme resource failed\n", __func__);
			goto out;
		}
	}
out:
	drv->dev->unmap(drv->map_base);
	return ret;
}

static void *gxres_mmap_null(size_t size)
{
	(void)size;
	return INVALID_MAP_ADDR;
}

static void gxres_unmap_null(void *ptr)
{
	(void)ptr;
}

static struct gxres_device gxres_null_device = {
	.dev_name = "null", .link_id = -1, .mmap = gxres_mmap_null, .unmap = gxres_unmap_null};

static bool guix_attach_devres(struct guix_driver *drv)
{
	struct gxres_device *iter;

	FOREACH_ITEM(iter, guix_resource_dev_list) {
		if (drv->id == iter->link_id) {
			drv->dev = iter;
			return true;
		}
	}
	drv->dev = &gxres_null_device;
	return false;
}

int gxres_device_register(struct gxres_device *dev)
{
	struct gxres_device *iter;

	if (!dev || !dev->mmap || !dev->unmap)
		return -EINVAL;

	FOREACH_ITEM(iter, guix_resource_dev_list) {
		if (iter->link_id == dev->link_id)
			return -EEXIST;
	}
	dev->next = guix_resource_dev_list;
	guix_resource_dev_list = dev;
	return 0;
}

int guix_driver_register(struct guix_driver *drv)
{
	struct guix_driver *iter;

	if (!drv || !drv->setup)
		return -EINVAL;

	FOREACH_ITEM(iter, guix_driver_list) {
		if (iter->id == drv->id)
			return -EEXIST;
	}
	drv->next = guix_driver_list;
	guix_driver_list = drv;
	return 0;
}

struct guix_driver *guix_get_drv_list(void)
{
	return guix_driver_list;
}

static int guix_initialize(const struct device *dev __unused)
{
	struct guix_driver *drv;
	int ret = -EINVAL;

	FOREACH_ITEM(drv, guix_driver_list) {
		if (!guix_attach_devres(drv))
			printk("%s(): No found guix resource device\n", __func__);
		ret = (int)guix_main(drv->id, drv);
		if (ret)
			goto _exit;
	}
	guix_state_notify(GUIX_STATE_UP, NULL);
	guix_zephyr_input_dev_init(guix_driver_list);
_exit:
	return ret;
}

SYS_INIT(guix_initialize, 
		APPLICATION, CONFIG_APPLICATION_INIT_PRIORITY);
		