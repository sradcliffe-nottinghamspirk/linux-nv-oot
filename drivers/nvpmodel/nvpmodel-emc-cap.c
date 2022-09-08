// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2022, NVIDIA CORPORATION & AFFILIATES. All rights reserved.

#include <linux/of.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/module.h>

#include <linux/platform/tegra/mc_utils.h>
#include <linux/interconnect.h>
#include <dt-bindings/interconnect/tegra_icc_id.h>

/* TBD: Use the implementation from mc_utils */
#define BYTES_PER_CLK_PER_CH    4
#define CH_16                   16
#define CH_16_BYTES_PER_CLK     (BYTES_PER_CLK_PER_CH * CH_16)
unsigned long emc_freq_to_bw(unsigned long freq)
{
	return freq * CH_16_BYTES_PER_CLK;
}

struct nvpmodel_emc_cap {
	struct device *dev;
	struct kobject *kobj;
	struct icc_path *icc_path_handle;
	struct kobj_attribute attr;
	unsigned long emc_iso_cap;
};

static ssize_t emc_iso_cap_show(struct kobject *kobj,
				struct kobj_attribute *attr, char *buf)
{
	struct nvpmodel_emc_cap *data;

	data = container_of(attr, struct nvpmodel_emc_cap, attr);
	return sprintf(buf, "%lu\n", data->emc_iso_cap);
}

static ssize_t emc_iso_cap_store(struct kobject *kobj,
				struct kobj_attribute *attr, const char *buf,
				size_t count)
{
	struct nvpmodel_emc_cap *data;
	int err = 0;

	data = container_of(attr, struct nvpmodel_emc_cap, attr);
	if (sscanf(buf, "%lu", &data->emc_iso_cap) != 1)
		return -EINVAL;

	if (data->icc_path_handle != NULL) {
		err = icc_set_bw(data->icc_path_handle, 0,
				(u32) emc_freq_to_bw(data->emc_iso_cap/1000));
		if (err) {
			dev_err(data->dev,
				"%s: Failed to set emc cap with icc, err=%d\n",
				__func__, err);
			return err;
		}
	}

	return count;
}

static int nvpmodel_emc_cap_probe(struct platform_device *pdev)
{
	struct kobject *kobj;
	struct kobj_attribute *attr;
	struct nvpmodel_emc_cap *data;
	struct icc_path *icc_path_handle;
	int err;

	data = devm_kzalloc(&pdev->dev, sizeof(*data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto out;
	}

	kobj = kobject_create_and_add("nvpmodel_emc_cap", kernel_kobj);
	if (!kobj) {
		dev_err(&pdev->dev,
			"%s: Failed to create nvpmodel_emc_cap kobject\n",
			__func__);
		err = -ENOMEM;
		goto out;
	}

	icc_path_handle = icc_get(NULL,
			TEGRA_ICC_NVPMODEL,
			TEGRA_ICC_PRIMARY);
	if (IS_ERR_OR_NULL(icc_path_handle)) {
		err = IS_ERR(icc_path_handle) ?
			PTR_ERR(icc_path_handle) :
			-ENODEV;
		dev_err(&pdev->dev,
			"%s: Failed to get icc path for nvpmodel client\n",
			__func__);
		goto icc_err;
	}

	data->dev = &pdev->dev;
	data->kobj = kobj;
	data->icc_path_handle = icc_path_handle;

	attr = &data->attr;
	attr->attr.name = "emc_iso_cap";
	attr->attr.mode = 0644;
	attr->show = emc_iso_cap_show;
	attr->store = emc_iso_cap_store;
	sysfs_attr_init(&attr->attr);
	if (sysfs_create_file(data->kobj, &attr->attr)) {
		dev_err(&pdev->dev,
			"%s: Failed to create emc_iso_cap sysfs node\n",
			__func__);
		err = -ENOMEM;
		goto sys_err;
	}

	return 0;

sys_err:
	if (data && data->icc_path_handle) {
		icc_put(data->icc_path_handle);
		data->icc_path_handle = NULL;
	}
icc_err:
	if (data && data->kobj) {
		kobject_put(data->kobj);
		data->kobj = NULL;
	}
out:
	return err;
}

static int nvpmodel_emc_cap_remove(struct platform_device *pdev)
{
	struct nvpmodel_emc_cap *data = platform_get_drvdata(pdev);

	if (data && data->icc_path_handle) {
		icc_put(data->icc_path_handle);
		data->icc_path_handle = NULL;
	}

	if (data && data->kobj) {
		sysfs_remove_file(data->kobj, &data->attr.attr);
		kobject_put(data->kobj);
		data->kobj = NULL;
	}

	return 0;
}

static const struct of_device_id nvpmodel_emc_cap_of_match[] = {
	{ .compatible = "nvidia,nvpmodel-emc-cap", },
	{ },
};

MODULE_DEVICE_TABLE(of, nvpmodel_emc_cap_of_match);

static struct platform_driver nvpmodel_emc_cap_driver = {
	.driver = {
		.name = "nvpmodel-emc-cap",
		.owner = THIS_MODULE,
		.of_match_table = nvpmodel_emc_cap_of_match,
	},
	.probe = nvpmodel_emc_cap_probe,
	.remove = nvpmodel_emc_cap_remove,
};

module_platform_driver(nvpmodel_emc_cap_driver);

MODULE_AUTHOR("Johnny Liu <johnliu@nvidia.com>");
MODULE_DESCRIPTION("NVIDIA nvpmodel driver for emc clock cap");
MODULE_LICENSE("GPL v2");
