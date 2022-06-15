// SPDX-License-Identifier: GPL-2.0-only
/*
 * Host1x fence UAPI
 *
 * Copyright (c) 2022, NVIDIA Corporation.
 */

#include <linux/anon_inodes.h>
#include <linux/cdev.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/host1x-next.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/sync_file.h>

#include "include/uapi/linux/host1x-fence.h"

static struct host1x_uapi {
	struct class *class;

	struct cdev cdev;
	struct device *dev;
	dev_t dev_num;
} uapi_data;

static int dev_file_open(struct inode *inode, struct file *file)
{
	struct platform_device *host1x_pdev;
	struct device_node *np;

	static const struct of_device_id host1x_match[] = {
		{ .compatible = "nvidia,tegra186-host1x", },
		{ .compatible = "nvidia,tegra194-host1x", },
		{},
	};

	np = of_find_matching_node(NULL, host1x_match);
	if (!np) {
		return -ENODEV;
	}

	host1x_pdev = of_find_device_by_node(np);
	if (!host1x_pdev) {
		return -EAGAIN;
	}

	file->private_data = platform_get_drvdata(host1x_pdev);

	return 0;
}

static int host1x_fence_create_fd(struct host1x_syncpt *sp, u32 threshold)
{
	struct sync_file *file;
	struct dma_fence *f;
	int fd;

	f = host1x_fence_create(sp, threshold);
	if (IS_ERR(f))
		return PTR_ERR(f);

	fd = get_unused_fd_flags(O_CLOEXEC);
	if (fd < 0) {
		dma_fence_put(f);
		return fd;
	}

	file = sync_file_create(f);
	dma_fence_put(f);
	if (!file)
		return -ENOMEM;

	fd_install(fd, file->file);

	return fd;
}

static int dev_file_ioctl_create_fence(struct host1x *host1x, void __user *data)
{
	struct host1x_create_fence args;
	struct host1x_syncpt *syncpt;
	unsigned long copy_err;
	int fd;

	copy_err = copy_from_user(&args, data, sizeof(args));
	if (copy_err)
		return -EFAULT;

	if (args.reserved[0])
		return -EINVAL;

	syncpt = host1x_syncpt_get_by_id_noref(host1x, args.id);
	if (!syncpt)
		return -EINVAL;

	fd = host1x_fence_create_fd(syncpt, args.threshold);
	if (fd < 0)
		return fd;

	args.fence_fd = fd;

	copy_err = copy_to_user(data, &args, sizeof(args));
	if (copy_err)
		return -EFAULT;

	return 0;
}

static int dev_file_ioctl_fence_extract(struct host1x *host1x, void __user *data)
{
	struct host1x_fence_extract_fence __user *fences_user_ptr;
	struct dma_fence *fence, **fences;
	struct host1x_fence_extract args;
	struct dma_fence_array *array;
	unsigned int num_fences, i;
	unsigned long copy_err;
	int err;

	copy_err = copy_from_user(&args, data, sizeof(args));
	if (copy_err)
		return -EFAULT;

	fences_user_ptr = u64_to_user_ptr(args.fences_ptr);

	if (args.reserved[0] || args.reserved[1])
		return -EINVAL;

	fence = sync_file_get_fence(args.fence_fd);
	if (!fence)
		return -EINVAL;

	array = to_dma_fence_array(fence);
	if (array) {
		fences = array->fences;
		num_fences = array->num_fences;
	} else {
		fences = &fence;
		num_fences = 1;
	}

	for (i = 0; i < min(num_fences, args.num_fences); i++) {
		struct host1x_fence_extract_fence f;

		err = host1x_fence_extract(fences[i], &f.id, &f.threshold);
		if (err)
			goto put_fence;

		copy_err = copy_to_user(fences_user_ptr + i, &f, sizeof(f));
		if (copy_err) {
			err = -EFAULT;
			goto put_fence;
		}
	}

	args.num_fences = num_fences;

	copy_err = copy_to_user(data, &args, sizeof(args));
	if (copy_err) {
		err = -EFAULT;
		goto put_fence;
	}

	return 0;

put_fence:
	dma_fence_put(fence);

	return err;
}

static long dev_file_ioctl(struct file *file, unsigned int cmd,
			   unsigned long arg)
{
	void __user *data = (void __user *)arg;
	long err;

	switch (cmd) {
	case HOST1X_IOCTL_CREATE_FENCE:
		err = dev_file_ioctl_create_fence(file->private_data, data);
		break;

	case HOST1X_IOCTL_FENCE_EXTRACT:
		err = dev_file_ioctl_fence_extract(file->private_data, data);
		break;

	default:
		err = -ENOTTY;
	}

	return err;
}

static const struct file_operations dev_file_fops = {
	.owner = THIS_MODULE,
	.open = dev_file_open,
	.unlocked_ioctl = dev_file_ioctl,
	.compat_ioctl = dev_file_ioctl,
};

static char *host1x_fence_devnode(struct device *dev, umode_t *mode)
{
	*mode = 0666;
	return NULL;
}

static int host1x_uapi_init(struct host1x_uapi *uapi)
{
	int err;
	dev_t dev_num;

	err = alloc_chrdev_region(&dev_num, 0, 1, "host1x-fence");
	if (err)
		return err;

	uapi->class = class_create(THIS_MODULE, "host1x-fence");
	if (IS_ERR(uapi->class)) {
		err = PTR_ERR(uapi->class);
		goto unregister_chrdev_region;
	}
	uapi->class->devnode = host1x_fence_devnode;

	cdev_init(&uapi->cdev, &dev_file_fops);
	err = cdev_add(&uapi->cdev, dev_num, 1);
	if (err)
		goto destroy_class;

	uapi->dev = device_create(uapi->class, NULL,
				  dev_num, NULL, "host1x-fence");
	if (IS_ERR(uapi->dev)) {
		err = PTR_ERR(uapi->dev);
		goto del_cdev;
	}

	cdev_add(&uapi->cdev, dev_num, 1);

	uapi->dev_num = dev_num;

	return 0;

del_cdev:
	cdev_del(&uapi->cdev);
destroy_class:
	class_destroy(uapi->class);
unregister_chrdev_region:
	unregister_chrdev_region(dev_num, 1);

	return err;
}

static int __init tegra_host1x_init(void)
{
	return host1x_uapi_init(&uapi_data);
}
module_init(tegra_host1x_init);

static void __exit tegra_host1x_exit(void)
{
}
module_exit(tegra_host1x_exit);

MODULE_DESCRIPTION("Host1x fence UAPI");
MODULE_LICENSE("GPL");
