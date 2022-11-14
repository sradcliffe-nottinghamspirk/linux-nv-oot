// SPDX-License-Identifier: GPL-2.0-only
// Copyright (c) 2022, NVIDIA CORPORATION.  All rights reserved.

#include <linux/of.h>
#include <linux/of_fdt.h>
#include <linux/ioport.h>
#include <linux/io.h>
#include <linux/highmem.h>

static int __init display_tegra_dts_info(void)
{
	int ret_d;
	int ret_t;
	int ret_dtb;
	const char *dtb;
	const char *dtb_bdate;
	const char *dtb_btime;
	struct device_node *root;

	root = of_find_node_by_path("/");
	if (!root)
		pr_info("root node NULL\n");

	ret_dtb = of_property_read_string_index(root,
			"nvidia,dtsfilename", 0, &dtb);
	if (!ret_dtb)
		pr_err("DTS File Name: %s\n", dtb);
	else
		pr_err("DTS File Name: <unknown>\n");

	ret_d = of_property_read_string_index(root,
			"nvidia,dtbbuildtime", 0, &dtb_bdate);
	ret_t = of_property_read_string_index(root,
			"nvidia,dtbbuildtime", 1, &dtb_btime);
	if (!ret_d && !ret_t)
		pr_err("DTB Build time: %s %s\n", dtb_bdate, dtb_btime);
	else
		pr_err("DTB Build time: <unknown>\n");

	if (root)
		of_node_put(root);
	return 0;
}
module_init(display_tegra_dts_info);
MODULE_LICENSE("GPL v2");
