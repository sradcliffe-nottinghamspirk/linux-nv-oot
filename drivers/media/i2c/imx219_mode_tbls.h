// SPDX-License-Identifier: GPL-2.0-only
/*
 * imx219_tables.h - sensor mode tables for imx219 HDR sensor.
 *
 * Copyright (c) 2015-2023, NVIDIA CORPORATION & AFFILIATES. All rights reserved.
 *
 */

#ifndef __IMX219_I2C_TABLES__
#define __IMX219_I2C_TABLES__

#define IMX219_TABLE_WAIT_MS	0
#define IMX219_TABLE_END	1

#define imx219_reg struct reg_8

static imx219_reg imx219_start_stream[] = {
	{0x0100, 0x01},
	{IMX219_TABLE_WAIT_MS, 3},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_stop_stream[] = {
	{0x0100, 0x00},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_mode_common[] = {
	/* software reset */
	{0x0103, 0x01},
	{IMX219_TABLE_WAIT_MS, 10},
	/* sensor config */
	{0x0114, 0x01}, /* D-Phy, 2-lane */
	{0x0128, 0x00},
	{0x012A, 0x18}, /* 24 MHz INCK */
	{0x012B, 0x00},
	/* access code - vendor addr. ranges */
	{0x30EB, 0x05},
	{0x30EB, 0x0C},
	{0x300A, 0xFF},
	{0x300B, 0xFF},
	{0x30EB, 0x05},
	{0x30EB, 0x09},
	/* cis tuning */
	{0x455E, 0x00},
	{0x471E, 0x4B},
	{0x4767, 0x0F},
	{0x4750, 0x14},
	{0x4540, 0x00},
	{0x47B4, 0x14},
	{0x4713, 0x30},
	{0x478B, 0x10},
	{0x478F, 0x10},
	{0x4793, 0x10},
	{0x4797, 0x0E},
	{0x479B, 0x0E},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_mode_3280x2464_21fps[] = {
	/* capture settings */
	{0x0157, 0x00}, /* ANALOG_GAIN_GLOBAL[7:0] */
	{0x015A, 0x09}, /* COARSE_INTEG_TIME[15:8] */
	{0x015B, 0xbd}, /* COARSE_INTEG_TIME[7:0] */
	/* format settings */
	{0x0160, 0x09}, /* FRM_LENGTH[15:8] */
	{0x0161, 0xC1}, /* FRM_LENGTH[7:0] */
	{0x0162, 0x0D}, /* LINE_LENGTH[15:8] */
	{0x0163, 0x78}, /* LINE_LENGTH[7:0] */
	{0x0164, 0x00},
	{0x0165, 0x00},
	{0x0166, 0x0C},
	{0x0167, 0xCF},
	{0x0168, 0x00},
	{0x0169, 0x00},
	{0x016A, 0x09},
	{0x016B, 0x9F},
	{0x016C, 0x0C},
	{0x016D, 0xD0},
	{0x016E, 0x09},
	{0x016F, 0xA0},
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x00},
	{0x0175, 0x00},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	/* clock dividers */
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x39},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x72},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_mode_3280x1848_28fps[] = {
	/* capture settings */
    {0x0157, 0x00}, /* ANALOG_GAIN_GLOBAL[7:0] */
	{0x015A, 0x07}, /* COARSE_INTEG_TIME[15:8] */
	{0x015B, 0x55}, /* COARSE_INTEG_TIME[7:0] */
	/* format settings */
    {0x0160, 0x07}, /* FRM_LENGTH[15:8] */
    {0x0161, 0x59}, /* FRM_LENGTH[7:0] */
    {0x0162, 0x0D}, /* LINE_LENGTH[15:8] */
    {0x0163, 0x78}, /* LINE_LENGTH[7:0] */
    {0x0164, 0x00},
    {0x0165, 0x00},
    {0x0166, 0x0C},
    {0x0167, 0xCF},
    {0x0168, 0x01},
    {0x0169, 0x34},
    {0x016A, 0x08},
    {0x016B, 0x6B},
    {0x016C, 0x0C},
    {0x016D, 0xD0},
    {0x016E, 0x07},
    {0x016F, 0x38},
    {0x0170, 0x01},
    {0x0171, 0x01},
    {0x0174, 0x00},
    {0x0175, 0x00},
    {0x018C, 0x0A},
    {0x018D, 0x0A},
	/* clocks dividers */
    {0x0301, 0x05},
    {0x0303, 0x01},
    {0x0304, 0x03},
    {0x0305, 0x03},
    {0x0306, 0x00},
    {0x0307, 0x39},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030C, 0x00},
    {0x030D, 0x72},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_mode_1920x1080_30fps[] = {
	/* capture settings */
    {0x0157, 0x00}, /* ANALOG_GAIN_GLOBAL[7:0] */
	{0x015A, 0x06}, /* COARSE_INTEG_TIME[15:8] */
	{0x015B, 0xde}, /* COARSE_INTEG_TIME[7:0] */
	/* format settings */
    {0x0160, 0x06}, /* FRM_LENGTH[15:8] */
    {0x0161, 0xE2}, /* FRM_LENGTH[7:0] */
    {0x0162, 0x0D}, /* LINE_LENGTH[15:8] */
    {0x0163, 0x78}, /* LINE_LENGTH[7:0] */
    {0x0164, 0x02},
    {0x0165, 0xA8},
    {0x0166, 0x0A},
    {0x0167, 0x27},
    {0x0168, 0x02},
    {0x0169, 0xB4},
    {0x016A, 0x06},
    {0x016B, 0xEB},
    {0x016C, 0x07},
    {0x016D, 0x80},
    {0x016E, 0x04},
    {0x016F, 0x38},
    {0x0170, 0x01},
    {0x0171, 0x01},
    {0x0174, 0x00},
    {0x0175, 0x00},
    {0x018C, 0x0A},
    {0x018D, 0x0A},
	/* clocks dividers */
    {0x0301, 0x05},
    {0x0303, 0x01},
    {0x0304, 0x03},
    {0x0305, 0x03},
    {0x0306, 0x00},
    {0x0307, 0x39},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030C, 0x00},
    {0x030D, 0x72},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_mode_1640x1232_30fps[] = {
	/* capture settings */
	{0x0157, 0x00}, /* ANALOG_GAIN_GLOBAL[7:0] */
	{0x015A, 0x06}, /* COARSE_INTEG_TIME[15:8] */
	{0x015B, 0xA8}, /* COARSE_INTEG_TIME[7:0] */
	/* format settings */
	{0x0160, 0x06}, /* FRM_LENGTH[15:8] */
	{0x0161, 0xE2}, /* FRM_LENGTH[7:0] */
	{0x0162, 0x0D}, /* LINE_LENGTH[15:8] */
	{0x0163, 0x78}, /* LINE_LENGTH[7:0] */
	{0x0164, 0x00},
	{0x0165, 0x00},
	{0x0166, 0x0C},
	{0x0167, 0xCF},
	{0x0168, 0x00},
	{0x0169, 0x00},
	{0x016A, 0x09},
	{0x016B, 0x9F},
	{0x016C, 0x06},
	{0x016D, 0x68},
	{0x016E, 0x04},
	{0x016F, 0xD0},
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x01},
	{0x0175, 0x01},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	/* clocks dividers */
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x39},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x72},
	{IMX219_TABLE_END, 0x00}
};

static imx219_reg imx219_mode_1280x720_60fps[] = {
	/* capture settings */
    {0x0157, 0x00}, /* ANALOG_GAIN_GLOBAL[7:0] */
	{0x015A, 0x03}, /* COARSE_INTEG_TIME[15:8] */
	{0x015B, 0x6c}, /* COARSE_INTEG_TIME[7:0] */
	/* format settings */
    {0x0160, 0x03}, /* FRM_LENGTH[15:8] */
    {0x0161, 0x70}, /* FRM_LENGTH[7:0] */
    {0x0162, 0x0D}, /* LINE_LENGTH[15:8] */
    {0x0163, 0x78}, /* LINE_LENGTH[7:0] */
    {0x0164, 0x01},
    {0x0165, 0x68},
    {0x0166, 0x0B},
    {0x0167, 0x67},
    {0x0168, 0x02},
    {0x0169, 0x00},
    {0x016A, 0x07},
    {0x016B, 0x9F},
    {0x016C, 0x05},
    {0x016D, 0x00},
    {0x016E, 0x02},
    {0x016F, 0xD0},
    {0x0170, 0x01},
    {0x0171, 0x01},
    {0x0174, 0x01},
    {0x0175, 0x01},
    {0x018C, 0x0A},
    {0x018D, 0x0A},
	/* clocks dividers */
    {0x0301, 0x05},
    {0x0303, 0x01},
    {0x0304, 0x03},
    {0x0305, 0x03},
    {0x0306, 0x00},
    {0x0307, 0x39},
    {0x0309, 0x0A},
    {0x030B, 0x01},
    {0x030C, 0x00},
    {0x030D, 0x72},
	{IMX219_TABLE_END, 0x00}
};

/*
static imx219_reg imx219_mode_1280x720_120fps[] = {
	// capture settings
	{0x0157, 0x00}, // ANALOG_GAIN_GLOBAL[7:0]
	{0x015A, 0x01}, // COARSE_INTEG_TIME[15:8]
	{0x015B, 0x85}, // COARSE_INTEG_TIME[7:0]
	// format settings
	{0x0160, 0x01}, // FRM_LENGTH[15:8]
	{0x0161, 0x89}, // FRM_LENGTH[7:0]
	{0x0162, 0x0D}, // LINE_LENGTH[15:8]
	{0x0163, 0xE8}, // LINE_LENGTH[7:0]
	{0x0164, 0x01},
	{0x0165, 0x68},
	{0x0166, 0x0B},
	{0x0167, 0x67},
	{0x0168, 0x02},
	{0x0169, 0x00},
	{0x016A, 0x07},
	{0x016B, 0x9F},
	{0x016C, 0x05},
	{0x016D, 0x00},
	{0x016E, 0x02},
	{0x016F, 0xD0},
	{0x0170, 0x01},
	{0x0171, 0x01},
	{0x0174, 0x03},
	{0x0175, 0x03},
	{0x018C, 0x0A},
	{0x018D, 0x0A},
	// clocks dividers
	{0x0301, 0x05},
	{0x0303, 0x01},
	{0x0304, 0x03},
	{0x0305, 0x03},
	{0x0306, 0x00},
	{0x0307, 0x35},
	{0x0309, 0x0A},
	{0x030B, 0x01},
	{0x030C, 0x00},
	{0x030D, 0x66},
	{IMX219_TABLE_END, 0x00}
};
*/
enum {
	IMX219_MODE_3280x2464_21FPS,
	IMX219_MODE_3280x1848_28FPS,
	IMX219_MODE_1920x1080_30FPS,
	IMX219_MODE_1640x1232_30FPS,
	IMX219_MODE_1280x720_60FPS,

	IMX219_MODE_COMMON,
	IMX219_START_STREAM,
	IMX219_STOP_STREAM,
};

static imx219_reg *mode_table[] = {
	[IMX219_MODE_3280x2464_21FPS] = imx219_mode_3280x2464_21fps,
	[IMX219_MODE_3280x1848_28FPS] = imx219_mode_3280x1848_28fps,
	[IMX219_MODE_1920x1080_30FPS] = imx219_mode_1920x1080_30fps,
	[IMX219_MODE_1640x1232_30FPS] = imx219_mode_1640x1232_30fps,
	[IMX219_MODE_1280x720_60FPS] = imx219_mode_1280x720_60fps,

	[IMX219_MODE_COMMON]  = imx219_mode_common,
	[IMX219_START_STREAM]  = imx219_start_stream,
	[IMX219_STOP_STREAM]  = imx219_stop_stream,
};

static const int imx219_21fps[] = {
	21,
};

static const int imx219_28fps[] = {
	28,
};

static const int imx219_30fps[] = {
	30,
};

static const int imx219_60fps[] = {
	60,
};

/*
 * WARNING: frmfmt ordering need to match mode definition in
 * device tree!
 */
static const struct camera_common_frmfmt imx219_frmfmt[] = {
	{{3280, 2464},	imx219_21fps, 1, 0, IMX219_MODE_3280x2464_21FPS},
	/* Add modes with no device tree support after below */
	{{3280, 1848},	imx219_28fps, 1, 0, IMX219_MODE_3280x1848_28FPS},
	{{1920, 1080},	imx219_30fps, 1, 0, IMX219_MODE_1920x1080_30FPS},
	{{1640, 1232},	imx219_30fps, 1, 0, IMX219_MODE_1640x1232_30FPS},
	{{1280, 720},	imx219_60fps, 1, 0, IMX219_MODE_1280x720_60FPS},
};

#endif /* __IMX219_I2C_TABLES__ */