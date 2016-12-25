/*
 * MacBook Air 6,1 and 6,2 (mid 2013) backlight driver
 *
 * Copyright (C) 2014 Patrik Jakobsson (patrik.r.jakobsson@gmail.com)
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/version.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/backlight.h>
#include <linux/acpi.h>
#include <acpi/acpi.h>
#include <acpi/video.h>

#define LP8550_SMBUS_ADDR	(0x58 >> 1)
#define LP8550_REG_BRIGHTNESS	0
#define LP8550_REG_DEV_CTL	1
#define LP8550_REG_FAULT	2
#define LP8550_REG_IDENT	3
#define LP8550_REG_DIRECT_CTL	4
#define LP8550_REG_TEMP_MSB	5 /* Must be read before TEMP_LSB */
#define LP8550_REG_TEMP_LSB	6

#define INIT_BRIGHTNESS		150
#define MAX_RETRIES		20

static struct platform_device *platform_device;
static struct backlight_device *backlight_device;


static struct {
	struct delayed_work work;
	struct mutex mutex;
	struct {
		u8 brightness;	/* Brightness control */
		u8 dev_ctl;	/* Device control */
		u8 fault;	/* Fault indication */
		u8 ident;	/* Identification */
		u8 direct_ctl;	/* Direct control */
		u8 temp_msb;	/* Temperature MSB  */
		u8 temp_lsb;	/* Temperature LSB */
	} lp8550_regs;
} dev_priv;

static int lp8550_reg_read(u8 reg, u8 *val)
{
	acpi_status status;
	acpi_handle handle;
	struct acpi_object_list arg_list;
	struct acpi_buffer buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	union acpi_object args[2];
	union acpi_object *result;
	int ret = 0;

	mutex_lock(&dev_priv.mutex);

	status = acpi_get_handle(NULL, "\\_SB.PCI0.SBUS.SRDB", &handle);
	if (ACPI_FAILURE(status)) {
		pr_err("mba6x_bl: Failed to get acpi handle\n");
		ret = -ENODEV;
		goto out;
	}

	args[0].type = ACPI_TYPE_INTEGER;
	args[0].integer.value = (LP8550_SMBUS_ADDR << 1) | 1;

	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = reg;

	arg_list.count = 2;
	arg_list.pointer = args;

	status = acpi_evaluate_object(handle, NULL, &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		pr_err("mba6x_bl: Failed to read reg: 0x%x\n", reg);
		ret = -ENODEV;
		goto out;
	}

	result = buffer.pointer;

	if (result->type != ACPI_TYPE_INTEGER) {
		pr_debug("mba6x_bl: Invalid response in reg: 0x%x (len: %Ld)\n",
			 reg, buffer.length);
		ret = -EINVAL;
		goto out;
	}

	*val = (u8)result->integer.value;
out:
	mutex_unlock(&dev_priv.mutex);
	kfree(buffer.pointer);
	return ret;
}

static int lp8550_reg_write(u8 reg, u8 val)
{
	acpi_status status;
	acpi_handle handle;
	struct acpi_object_list arg_list;
	struct acpi_buffer buffer = {ACPI_ALLOCATE_BUFFER, NULL};
	union acpi_object args[3];
	union acpi_object *result;
	int ret = 0;
	u8 read_val;

	mutex_lock(&dev_priv.mutex);

	status = acpi_get_handle(NULL, "\\_SB.PCI0.SBUS.SWRB", &handle);
	if (ACPI_FAILURE(status)) {
		pr_err("mba6x_bl: Failed to get acpi handle\n");
		ret = -ENODEV;
		goto out;
	}

	args[0].type = ACPI_TYPE_INTEGER;
	args[0].integer.value = (LP8550_SMBUS_ADDR << 1) & ~1;

	args[1].type = ACPI_TYPE_INTEGER;
	args[1].integer.value = reg;

	args[2].type = ACPI_TYPE_INTEGER;
	args[2].integer.value = val;

	arg_list.count = 3;
	arg_list.pointer = args;

	status = acpi_evaluate_object(handle, NULL, &arg_list, &buffer);
	if (ACPI_FAILURE(status)) {
		pr_err("mba6x_bl: Failed to write reg: 0x%x\n", reg);
		ret = -ENODEV;
		goto out;
	}

	result = buffer.pointer;

	if (result->type != ACPI_TYPE_INTEGER || result->integer.value != 1) {
		pr_debug("mba6x_bl: Invalid response at reg: 0x%x (len: %Ld)\n",
			 reg, buffer.length);
		ret = -EINVAL;
		goto out;
	}

out:
	mutex_unlock(&dev_priv.mutex);

	/* Read back the register to see if it stuck */
	lp8550_reg_read(reg, &read_val);
	if (!ret && read_val != val) {
		pr_err("mba6x_bl: Read-back failed at reg: 0x%x, val: 0x%x\n",
			reg, val);
		ret = -EINVAL;
	}

	kfree(buffer.pointer);
	return ret;
}

static inline int map_brightness(int b)
{
	return ((b * b + 254) / 255);
}

static int set_brightness(int brightness)
{
	int ret;

	if (brightness < 0 || brightness > 255)
		return -EINVAL;

	ret = lp8550_reg_write(LP8550_REG_DEV_CTL, 0x05);
	if (ret)
		return -ENODEV;

	brightness = map_brightness(brightness);

	ret = lp8550_reg_write(LP8550_REG_BRIGHTNESS, (u8)brightness);
	if (ret)
		return -ENODEV;

	return ret;
}

static int get_brightness(struct backlight_device *bdev)
{
	return bdev->props.brightness;
}

static int update_status(struct backlight_device *bdev)
{
	/* We lock when writing to the SMBUS so work must be scheduled */
	cancel_delayed_work_sync(&dev_priv.work);
	schedule_delayed_work(&dev_priv.work, 0);

	return 0;
}

static int lp8550_probe(void)
{
	u8 val;
	int ret;

	ret = lp8550_reg_read(LP8550_REG_IDENT, &val);
	if (ret)
		return ret;

	if (val != 0xfc)
		return -ENODEV;

	pr_info("mba6x_bl: Found LP8550 backlight driver\n");
	return 0;
}

static int lp8550_save(void)
{
	int ret;

	ret = lp8550_reg_read(LP8550_REG_DEV_CTL,
			      &dev_priv.lp8550_regs.dev_ctl);
	if (ret)
		return ret;

	ret = lp8550_reg_read(LP8550_REG_BRIGHTNESS,
			      &dev_priv.lp8550_regs.brightness);
	return ret;
}


static int lp8550_restore(void)
{
	int ret;

	ret = lp8550_reg_write(LP8550_REG_BRIGHTNESS,
			       dev_priv.lp8550_regs.brightness);
	if (ret)
		return ret;

	ret = lp8550_reg_write(LP8550_REG_DEV_CTL,
			       dev_priv.lp8550_regs.dev_ctl);
	return ret;
}

static struct backlight_ops backlight_ops = {
	.update_status = update_status,
	.get_brightness = get_brightness,
};

static struct platform_driver drv;

static void brightness_work(struct work_struct *work)
{
	int ret;

	/* Don't set backlight if it's already powered off */
	if (backlight_device->props.power == 4 &&
	    backlight_device->props.brightness == 0) {
		pr_info("mba6x_bl: skipping to set backlight since it's already off\n");
		return;
	}

	ret = set_brightness(backlight_device->props.brightness);

	/* Warn but reschedule even if we failed */
	if (ret)
		pr_debug("mba6x_bl: failed to set brightness\n");

	if (backlight_device->props.brightness == 0)
		backlight_device->props.power = 4;
	else
		backlight_device->props.power = 0;

	/* Reschedule the next update */
	schedule_delayed_work(&dev_priv.work, 3 * HZ);
}

static int platform_probe(struct platform_device *dev)
{
	struct backlight_properties props;
	int ret;

	mutex_init(&dev_priv.mutex);
	INIT_DELAYED_WORK(&dev_priv.work, brightness_work);

	ret = lp8550_probe();
	if (ret)
		return ret;

	ret = lp8550_save();
	if (ret)
		return ret;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.max_brightness = 255;
	props.type = BACKLIGHT_FIRMWARE;
	props.power = 0; /* Power is on */

	backlight_device = backlight_device_register("mba6x_backlight",
						     NULL, NULL,
						     &backlight_ops, &props);
	if (IS_ERR(backlight_device)) {
		pr_err("mba6x_bl: Failed to register backlight device\n");
		return PTR_ERR(backlight_device);
	}

	backlight_device->props.brightness = INIT_BRIGHTNESS;
	backlight_update_status(backlight_device);

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 2, 0)
	acpi_video_set_dmi_backlight_type(acpi_backlight_vendor);
#else
	acpi_video_dmi_promote_vendor();
#endif
	acpi_video_unregister();

	return 0;
}

static int platform_remove(struct platform_device *dev)
{
	cancel_delayed_work_sync(&dev_priv.work);
	backlight_device_unregister(backlight_device);
	lp8550_restore();
	pr_info("mba6x_bl: Restored old configuration\n");

	return 0;
}

static int platform_resume(struct platform_device *dev)
{
	/* Reschedule the next update */
	schedule_delayed_work(&dev_priv.work, 3 * HZ);

	return 0;
}

static void platform_shutdown(struct platform_device *dev)
{
	/* We must restore or screen might go black on reboot */
	cancel_delayed_work_sync(&dev_priv.work);
	lp8550_restore();
}

static int platform_suspend(struct platform_device *dev, pm_message_t state)
{
	int b;

	/* Takes backlight gradually to zero*/
	for(b = backlight_device->props.brightness; b >= 0 ; b--) {
		set_brightness(b);
	}

	return 0;
}

static struct platform_driver drv = {
	.probe		= platform_probe,
	.remove		= platform_remove,
	.resume		= platform_resume,
	.suspend 	= platform_suspend,
	.shutdown	= platform_shutdown,
	.driver		= {
		.name 	= "mba6x_bl",
		.owner 	= THIS_MODULE,
	},
};

static int __init mba6x_bl_init(void)
{
	int ret;

	ret = platform_driver_register(&drv);
	if (ret) {
		pr_err("Failed to register platform driver\n");
		return ret;
	}

	platform_device = platform_device_register_simple("mba6x_bl", -1, NULL,
							  0);
	return 0;
}

static void __exit mba6x_bl_exit(void)
{
	platform_device_unregister(platform_device);
	platform_driver_unregister(&drv);
}

module_init(mba6x_bl_init);
module_exit(mba6x_bl_exit);

MODULE_AUTHOR("Patrik Jakobsson <patrik.r.jakobsson@gmail.com>");
MODULE_DESCRIPTION("MacBook Air 6,1 and 6,2 backlight driver");
MODULE_LICENSE("GPL");

MODULE_ALIAS("dmi:*:pnMacBookAir6*");
MODULE_ALIAS("dmi:*:pnMacBookAir7*");
