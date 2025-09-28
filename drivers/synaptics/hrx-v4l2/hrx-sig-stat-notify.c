#include "hrx-drv.h"

static struct kobject *hrx_kobj;
static char curr_status_buf[32];

static ssize_t signal_status_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
	return sprintf(buf, "%s\n", curr_status_buf);
}

static struct kobj_attribute hrx_sig_stat_attribute = __ATTR_RO(signal_status);

void hrx_sig_stat_set_attr (struct hdmi_rx_input_change_event *in_event)
{
	if (hrx_kobj != NULL) {
		sprintf(curr_status_buf, "%d:%dx%d@%d/%d\n", in_event->hrx_stable_state,
			in_event->width, in_event->height, in_event->fi_den, in_event->fi_num);
		sysfs_notify(hrx_kobj, NULL, ATTRIBUTE_NAME);
	} else
		HRX_LOG(HRX_DRV_ERROR, "%s is not available\n", ATTRIBUTE_NAME);
}

int hrx_sig_stat_create(void)
{
	int ret = 0;

	hrx_kobj = kobject_create_and_add(SYSFS_DIR_NAME, kernel_kobj);
	if (!hrx_kobj) {
		HRX_LOG(HRX_DRV_ERROR, "failed to create hrx_kobj\n");
		return -ENOMEM;
	}

	ret = sysfs_create_file(hrx_kobj, &hrx_sig_stat_attribute.attr);
	if (ret) {
		HRX_LOG(HRX_DRV_ERROR, "failed to create sysfs attribute '%s'.\n", ATTRIBUTE_NAME);
		kobject_put(hrx_kobj);
		return ret;
	}

	return 0;
}

void hrx_sig_stat_remove(void)
{
	sysfs_notify(hrx_kobj, NULL, ATTRIBUTE_NAME);
	sysfs_remove_file(hrx_kobj, &hrx_sig_stat_attribute.attr);
	kobject_put(hrx_kobj);
}