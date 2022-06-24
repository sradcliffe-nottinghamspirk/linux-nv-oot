/* SPDX-License-Identifier: GPL-2.0-only */
/* Copyright (c) 2015-2022, NVIDIA CORPORATION.  All rights reserved.
 */

#include <linux/time.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/version.h>
#include <soc/tegra/fuse.h>
#include <linux/reset.h>
#include <soc/tegra/pmc.h>
#include <linux/gpio/consumer.h>
#include <linux/gpio.h>
#include <linux/jiffies.h>

#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>

#ifdef CONFIG_DEBUG_FS
#include <linux/debugfs.h>
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 17, 0)
#include <drivers-private/scsi/ufs/k515/ufshcd-pltfrm.h>
#include <drivers-private/scsi/ufs/k515/ufshcd.h>
#include <drivers-private/scsi/ufs/k515/unipro.h>
#include <drivers-private/scsi/ufs/k515/ufshci.h>
#else
#include <drivers-private/scsi/ufs/k517/ufshcd-pltfrm.h>
#include <drivers-private/scsi/ufs/k517/ufshcd.h>
#include <drivers-private/scsi/ufs/k517/unipro.h>
#include <drivers-private/scsi/ufs/k517/ufshci.h>
#endif

#include "ufs-tegra.h"

static void ufs_tegra_set_clk_div(struct ufs_hba *hba, u32 divider_val)
{
	ufshcd_writel(hba, divider_val, REG_UFS_VNDR_HCLKDIV);
}

static void ufs_tegra_ufs_mmio_axi(struct ufs_hba *hba)
{
	u32 mask = GENMASK(15, 13);

	ufshcd_rmwl(hba, mask, VS_BURSTMBLCONFIG, VS_BURSTMBLREGISTER);

}

static int ufs_tegra_host_clk_get(struct device *dev,
		const char *name, struct clk **clk_out)
{
	struct clk *clk;
	int err = 0;

	clk = devm_clk_get(dev, name);
	if (IS_ERR(clk)) {
		err = PTR_ERR(clk);
		if (err != -EPROBE_DEFER) {
			dev_err(dev, "%s: failed to get %s err %d",
				__func__, name, err);
		}
	} else {
		*clk_out = clk;
	}

	return err;
}

static int ufs_tegra_host_clk_enable(struct device *dev,
		const char *name, struct clk *clk)
{
	int err = 0;

	err = clk_prepare_enable(clk);
	if (err)
		dev_err(dev, "%s: %s enable failed %d\n", __func__, name, err);

	return err;
}

static int __ufs_tegra_mphy_receiver_calibration(
		struct ufs_tegra_host *ufs_tegra,
		void __iomem *mphy_base)
{
	struct device *dev = ufs_tegra->hba->dev;
	u32 mphy_rx_vendor;
	int timeout = 100;

	mphy_update(mphy_base, MPHY_GO_BIT, MPHY_RX_APB_VENDOR2_0);

	while (timeout--) {
		udelay(1);
		mphy_rx_vendor = mphy_readl(mphy_base, MPHY_RX_APB_VENDOR2_0);
		if (mphy_rx_vendor & MPHY_RX_APB_VENDOR2_0_RX_CAL_DONE) {
			dev_info(dev, "MPhy Receiver Calibration passed\n");
			return 0;
		}
	}

	dev_err(dev, "MPhy Receiver Calibration failed\n");
	return -ETIMEDOUT;

}

/**
 * ufs_tegra_mphy_receiver_calibration
 * @ufs_tegra: ufs_tegra_host controller instance
 *
 * Implements MPhy Receiver Calibration Sequence
 *
 * Returns -ETIMEDOUT if receiver calibration fails
 * and returns zero on success.
 */
static int ufs_tegra_mphy_receiver_calibration(struct ufs_tegra_host *ufs_tegra,
					       void __iomem *mphy_base)
{
	int err = 0;

	if (!ufs_tegra->enable_mphy_rx_calib)
		return 0;

	mphy_update(mphy_base, MPHY_RX_APB_VENDOR2_0_RX_CAL_EN,
		    MPHY_RX_APB_VENDOR2_0);
	err = __ufs_tegra_mphy_receiver_calibration(ufs_tegra, mphy_base);
	if (err)
		return err;

	mphy_clear_bits(mphy_base, MPHY_RX_APB_VENDOR2_0_RX_CAL_EN,
			MPHY_RX_APB_VENDOR2_0);
	err = __ufs_tegra_mphy_receiver_calibration(ufs_tegra, mphy_base);

	return err;
}

static void ufs_tegra_disable_mphylane_clks(struct ufs_tegra_host *host)
{
	if (!host->is_lane_clks_enabled)
		return;

	clk_disable_unprepare(host->mphy_core_pll_fixed);
	clk_disable_unprepare(host->mphy_l0_tx_symb);
	clk_disable_unprepare(host->mphy_tx_1mhz_ref);
	clk_disable_unprepare(host->mphy_l0_rx_ana);
	clk_disable_unprepare(host->mphy_l0_rx_symb);
	clk_disable_unprepare(host->mphy_l0_tx_ls_3xbit);
	clk_disable_unprepare(host->mphy_l0_rx_ls_bit);

	if (host->x2config)
		clk_disable_unprepare(host->mphy_l1_rx_ana);

	host->is_lane_clks_enabled = false;
}

static int ufs_tegra_enable_t234_mphy_clocks(struct ufs_tegra_host *host)
{
	int err;
	struct device *dev = host->hba->dev;

	err = clk_set_rate(host->mphy_tx_hs_symb_div, MPHY_TX_HS_BIT_DIV_CLK);
	if (err) {
		if (err != -EPROBE_DEFER)
			dev_err(dev,
				"%s: mphy_tx_hs_symb_div set rate failed %d\n",
				__func__, err);
		goto out;
	}

	err = clk_set_parent(host->mphy_tx_hs_mux_symb_div, host->mphy_tx_hs_symb_div);
	if (err) {
		if (err != -EPROBE_DEFER)
			dev_err(dev,
			 "%s mphy_tx_hs_mux_symb_div set parent failed %d\n",
			 __func__, err);
		goto out;
	}

	err = ufs_tegra_host_clk_enable(dev, "mphy_tx_hs_symb_div",
			host->mphy_tx_hs_symb_div);
	if (err) {
		if (err != -EPROBE_DEFER)
			dev_err(dev,
			 "%s mphy_tx_hs_symb_div clock enable failed %d\n",
			 __func__, err);
		goto out;
	}

	err = clk_set_rate(host->mphy_rx_hs_symb_div, MPHY_RX_HS_BIT_DIV_CLK);
	if (err) {
		if (err != -EPROBE_DEFER)
			dev_err(dev,
				"%s: mphy_rx_hs_symb_div set rate failed %d\n",
				__func__, err);
		goto disable_mphy_tx_hs_symb_div;
	}

	err = clk_set_parent(host->mphy_rx_hs_mux_symb_div, host->mphy_rx_hs_symb_div);
	if (err) {
		 dev_err(dev,
			"%s: mphy_rx_hs_symb_div set parent failed %d\n",
			__func__, err);
		goto disable_mphy_tx_hs_symb_div;
	}

	err = ufs_tegra_host_clk_enable(dev, "mphy_rx_hs_symb_div", host->mphy_rx_hs_symb_div);
	if (err) {
		if (err != -EPROBE_DEFER)
			dev_err(dev,
				"%s: mphy_rx_hs_symb_div clock enable failed %d\n",
				__func__, err);
		goto disable_mphy_tx_hs_symb_div;
	}

	err = ufs_tegra_host_clk_enable(dev, "mphy_l0_tx_2x_symb", host->mphy_l0_tx_2x_symb);
	if (err) {
		if (err != -EPROBE_DEFER)
			dev_err(dev,
				"%s: mphy_l0_tx_2x_symb clock enable failed %d\n",
				__func__, err);
		goto disable_mphy_rx_hs_symb_div;
	} else {
		goto out;
	}

disable_mphy_rx_hs_symb_div:
	clk_disable_unprepare(host->mphy_rx_hs_symb_div);
disable_mphy_tx_hs_symb_div:
	clk_disable_unprepare(host->mphy_tx_hs_symb_div);
out:
	return err;
}

static int ufs_tegra_enable_mphylane_clks(struct ufs_tegra_host *host)
{
	int err = 0;
	struct device *dev = host->hba->dev;

	if (host->is_lane_clks_enabled)
		return 0;

	err = clk_prepare_enable(host->pllrefe_clk);
	if (err < 0)
		goto out;

	err = ufs_tegra_host_clk_enable(dev, "mphy_core_pll_fixed",
		host->mphy_core_pll_fixed);
	if (err)
		goto disable_mphy_core_pll_fixed;

	err = ufs_tegra_host_clk_enable(dev, "mphy_l0_tx_symb",
		host->mphy_l0_tx_symb);
	if (err)
		goto disable_l0_tx_symb;

	err = ufs_tegra_host_clk_enable(dev, "mphy_tx_1mhz_ref",
		host->mphy_tx_1mhz_ref);
	if (err)
		goto disable_tx_1mhz_ref;

	err = ufs_tegra_host_clk_enable(dev, "mphy_l0_rx_ana",
		host->mphy_l0_rx_ana);
	if (err)
		goto disable_l0_rx_ana;

	err = ufs_tegra_host_clk_enable(dev, "mphy_l0_rx_symb",
		host->mphy_l0_rx_symb);
	if (err)
		goto disable_l0_rx_symb;

	err = ufs_tegra_host_clk_enable(dev, "mphy_l0_tx_ls_3xbit",
		host->mphy_l0_tx_ls_3xbit);
	if (err)
		goto disable_l0_tx_ls_3xbit;

	err = ufs_tegra_host_clk_enable(dev, "mphy_l0_rx_ls_bit",
		host->mphy_l0_rx_ls_bit);
	if (err)
		goto disable_l0_rx_ls_bit;

	if (host->x2config) {
		err = ufs_tegra_host_clk_enable(dev, "mphy_l1_rx_ana",
			host->mphy_l1_rx_ana);
		if (err)
			goto disable_l1_rx_ana;
	}

	if (host->soc->chip_id == TEGRA234) {
		err = ufs_tegra_enable_t234_mphy_clocks(host);
		if (err)
			goto disable_t234_clocks;
	}

	host->is_lane_clks_enabled = true;
	goto out;

disable_t234_clocks:
	if (host->x2config)
		clk_disable_unprepare(host->mphy_l1_rx_ana);
disable_l1_rx_ana:
	clk_disable_unprepare(host->mphy_l0_rx_ls_bit);
disable_l0_rx_ls_bit:
	clk_disable_unprepare(host->mphy_l0_tx_ls_3xbit);
disable_l0_tx_ls_3xbit:
	clk_disable_unprepare(host->mphy_l0_rx_symb);
disable_l0_rx_symb:
	clk_disable_unprepare(host->mphy_l0_rx_ana);
disable_l0_rx_ana:
	clk_disable_unprepare(host->mphy_tx_1mhz_ref);
disable_tx_1mhz_ref:
	clk_disable_unprepare(host->mphy_l0_tx_symb);
disable_l0_tx_symb:
	clk_disable_unprepare(host->mphy_core_pll_fixed);
disable_mphy_core_pll_fixed:
	clk_disable_unprepare(host->pllrefe_clk);
out:
	return err;
}

static int ufs_tegra_init_mphy_lane_clks(struct ufs_tegra_host *host)
{
	int err = 0;
	struct device *dev = host->hba->dev;

	err = ufs_tegra_host_clk_get(dev,
			"pllrefe_vcoout", &host->pllrefe_clk);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev,
			"mphy_core_pll_fixed", &host->mphy_core_pll_fixed);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev,
			"mphy_l0_tx_symb", &host->mphy_l0_tx_symb);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_tx_1mhz_ref",
		&host->mphy_tx_1mhz_ref);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_rx_ana",
		&host->mphy_l0_rx_ana);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_rx_symb",
		&host->mphy_l0_rx_symb);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_tx_ls_3xbit",
		&host->mphy_l0_tx_ls_3xbit);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_rx_ls_bit",
		&host->mphy_l0_rx_ls_bit);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_force_ls_mode",
		&host->mphy_force_ls_mode);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_tx_hs_symb_div",
		&host->mphy_tx_hs_symb_div);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_tx_mux_symb_div",
		&host->mphy_tx_hs_mux_symb_div);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_rx_hs_symb_div",
		&host->mphy_rx_hs_symb_div);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_rx_mux_symb_div",
		&host->mphy_rx_hs_mux_symb_div);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev, "mphy_l0_tx_2x_symb",
		&host->mphy_l0_tx_2x_symb);
	if (err)
		goto out;

	if (host->x2config) {
		err = ufs_tegra_host_clk_get(dev, "mphy_l1_rx_ana",
			&host->mphy_l1_rx_ana);
		if (err)
			goto out;
	}

out:
	return err;
}

static int ufs_tegra_enable_ufs_uphy_pll3(struct ufs_tegra_host *ufs_tegra,
			bool is_rate_b)
{
	int err = 0;
	struct device *dev = ufs_tegra->hba->dev;

	if (!ufs_tegra->configure_uphy_pll3)
		return 0;

	err = ufs_tegra_host_clk_enable(dev, "uphy_pll3",
		ufs_tegra->ufs_uphy_pll3);
	if (err)
		return err;

	if (is_rate_b) {
		if (ufs_tegra->ufs_uphy_pll3)
			err = clk_set_rate(ufs_tegra->ufs_uphy_pll3,
				UFS_CLK_UPHY_PLL3_RATEB);
	} else {
		if (ufs_tegra->ufs_uphy_pll3)
			err = clk_set_rate(ufs_tegra->ufs_uphy_pll3,
					UFS_CLK_UPHY_PLL3_RATEA);
	}
	if (err)
		dev_err(dev, "%s: failed to set ufs_uphy_pll3 freq err %d",
				__func__, err);
	return err;
}

static int ufs_tegra_init_uphy_pll3(struct ufs_tegra_host *ufs_tegra)
{
	int err = 0;
	struct device *dev = ufs_tegra->hba->dev;

	if (!ufs_tegra->configure_uphy_pll3)
		return 0;

	err = ufs_tegra_host_clk_get(dev,
		"uphy_pll3", &ufs_tegra->ufs_uphy_pll3);
	return err;
}

static int ufs_tegra_init_ufs_clks(struct ufs_tegra_host *ufs_tegra)
{
	int err = 0;
	struct device *dev = ufs_tegra->hba->dev;

	err = ufs_tegra_host_clk_get(dev,
		"pll_p", &ufs_tegra->ufshc_parent);
	if (err)
		goto out;
	err = ufs_tegra_host_clk_get(dev,
		"ufshc", &ufs_tegra->ufshc_clk);
	if (err)
		goto out;
	err = ufs_tegra_host_clk_get(dev,
		"clk_m", &ufs_tegra->ufsdev_parent);
	if (err)
		goto out;
	err = ufs_tegra_host_clk_get(dev,
		"ufsdev_ref", &ufs_tegra->ufsdev_ref_clk);
	if (err)
		goto out;

	err = ufs_tegra_host_clk_get(dev,
		"osc", &ufs_tegra->ufsdev_osc);

out:
	return err;
}

static int ufs_tegra_enable_ufs_clks(struct ufs_tegra_host *ufs_tegra)
{
	struct device *dev = ufs_tegra->hba->dev;
	int err = 0;

	err = ufs_tegra_host_clk_enable(dev, "ufshc",
		ufs_tegra->ufshc_clk);
	if (err)
		goto out;
	err = clk_set_parent(ufs_tegra->ufshc_clk,
				ufs_tegra->ufshc_parent);
	if (err) {
		pr_err("Function clk_set_parent failed\n");
		goto out;
	}
	err = clk_set_rate(ufs_tegra->ufshc_clk, UFSHC_CLK_FREQ);
	if (err) {
		pr_err("Function clk_set_rate failed\n");
		goto out;
	}

	/* clk_m is the parent for ufsdev_ref
	 * Frequency is 19.2 MHz.
	 */
	err = ufs_tegra_host_clk_enable(dev, "ufsdev_ref",
		ufs_tegra->ufsdev_ref_clk);
	if (err)
		goto disable_ufshc;

	if ((ufs_tegra->soc->chip_id == TEGRA234) &&
			(ufs_tegra->enable_38mhz_clk)) {
		err = clk_set_parent(ufs_tegra->ufsdev_ref_clk,
				ufs_tegra->ufsdev_osc);

		if (err) {
			pr_err("Function clk_set_parent failed\n");
			goto out;
		}
	}

	ufs_tegra->hba->clk_gating.state = CLKS_ON;

	return err;

disable_ufshc:
	clk_disable_unprepare(ufs_tegra->ufshc_clk);
out:
	return err;
}

static void ufs_tegra_disable_ufs_clks(struct ufs_tegra_host *ufs_tegra)
{
	if (ufs_tegra->hba->clk_gating.state == CLKS_OFF)
		return;

	clk_disable_unprepare(ufs_tegra->ufshc_clk);
	clk_disable_unprepare(ufs_tegra->ufsdev_ref_clk);

	ufs_tegra->hba->clk_gating.state = CLKS_OFF;
}

static int ufs_tegra_ufs_reset_init(struct ufs_tegra_host *ufs_tegra)
{
	struct device *dev = ufs_tegra->hba->dev;
	int ret = 0;

	ufs_tegra->ufs_rst = devm_reset_control_get(dev, "ufs-rst");
	if (IS_ERR(ufs_tegra->ufs_rst)) {
		ret = PTR_ERR(ufs_tegra->ufs_rst);
		dev_err(dev,
			"Reset control for ufs-rst not found: %d\n", ret);
	}
	ufs_tegra->ufs_axi_m_rst = devm_reset_control_get(dev, "ufs-axi-m-rst");
	if (IS_ERR(ufs_tegra->ufs_axi_m_rst)) {
		ret = PTR_ERR(ufs_tegra->ufs_axi_m_rst);
		dev_err(dev,
			"Reset control for ufs-axi-m-rst not found: %d\n", ret);
	}
	ufs_tegra->ufshc_lp_rst = devm_reset_control_get(dev, "ufshc-lp-rst");
	if (IS_ERR(ufs_tegra->ufshc_lp_rst)) {
		ret = PTR_ERR(ufs_tegra->ufshc_lp_rst);
		dev_err(dev,
			"Reset control for ufshc-lp-rst not found: %d\n", ret);
	}
	return ret;
}

static void ufs_tegra_ufs_deassert_reset(struct ufs_tegra_host *ufs_tegra)
{
	reset_control_deassert(ufs_tegra->ufs_rst);
	reset_control_deassert(ufs_tegra->ufs_axi_m_rst);
	reset_control_deassert(ufs_tegra->ufshc_lp_rst);
}

static int ufs_tegra_mphy_reset_init(struct ufs_tegra_host *ufs_tegra)
{
	struct device *dev = ufs_tegra->hba->dev;
	int ret = 0;

	ufs_tegra->mphy_l0_rx_rst =
				devm_reset_control_get(dev, "mphy-l0-rx-rst");
	if (IS_ERR(ufs_tegra->mphy_l0_rx_rst)) {
		ret = PTR_ERR(ufs_tegra->mphy_l0_rx_rst);
		dev_err(dev,
			"Reset control for mphy-l0-rx-rst not found: %d\n",
									ret);
	}

	ufs_tegra->mphy_l0_tx_rst =
				devm_reset_control_get(dev, "mphy-l0-tx-rst");
	if (IS_ERR(ufs_tegra->mphy_l0_tx_rst)) {
		ret = PTR_ERR(ufs_tegra->mphy_l0_tx_rst);
		dev_err(dev,
			"Reset control for mphy-l0-tx-rst not found: %d\n",
									ret);
	}

	ufs_tegra->mphy_clk_ctl_rst =
				devm_reset_control_get(dev, "mphy-clk-ctl-rst");
	if (IS_ERR(ufs_tegra->mphy_clk_ctl_rst)) {
		ret = PTR_ERR(ufs_tegra->mphy_clk_ctl_rst);
		dev_err(dev,
			"Reset control for mphy-clk-ctl-rst not found: %d\n",
									ret);
	}

	if (ufs_tegra->x2config) {
		ufs_tegra->mphy_l1_rx_rst =
				devm_reset_control_get(dev, "mphy-l1-rx-rst");
		if (IS_ERR(ufs_tegra->mphy_l1_rx_rst)) {
			ret = PTR_ERR(ufs_tegra->mphy_l1_rx_rst);
			dev_err(dev,
			"Reset control for mphy-l1-rx-rst not found: %d\n",
									ret);
		}

		ufs_tegra->mphy_l1_tx_rst =
				devm_reset_control_get(dev, "mphy-l1-tx-rst");
		if (IS_ERR(ufs_tegra->mphy_l1_tx_rst)) {
			ret = PTR_ERR(ufs_tegra->mphy_l1_tx_rst);
			dev_err(dev,
			"Reset control for mphy_l1_tx_rst not found: %d\n",
									ret);
		}
	}

	return ret;
}

static void ufs_tegra_mphy_assert_reset(struct ufs_tegra_host *ufs_tegra)
{
	reset_control_assert(ufs_tegra->mphy_l0_rx_rst);
	reset_control_assert(ufs_tegra->mphy_l0_tx_rst);
	reset_control_assert(ufs_tegra->mphy_clk_ctl_rst);
	if (ufs_tegra->x2config) {
		reset_control_assert(ufs_tegra->mphy_l1_rx_rst);
		reset_control_assert(ufs_tegra->mphy_l1_tx_rst);
	}
}

static void ufs_tegra_mphy_deassert_reset(struct ufs_tegra_host *ufs_tegra)
{
	reset_control_deassert(ufs_tegra->mphy_l0_rx_rst);
	reset_control_deassert(ufs_tegra->mphy_l0_tx_rst);
	reset_control_deassert(ufs_tegra->mphy_clk_ctl_rst);
	if (ufs_tegra->x2config) {
		reset_control_deassert(ufs_tegra->mphy_l1_rx_rst);
		reset_control_deassert(ufs_tegra->mphy_l1_tx_rst);
	}
}

static void ufs_tegra_pwr_change_clk_boost(struct ufs_tegra_host *ufs_tegra)
{
	u32 reg_vendor_0;

	if (ufs_tegra->soc->chip_id == TEGRA234)
		reg_vendor_0 = MPHY_RX_APB_VENDOR2_0_T234;
	else
		reg_vendor_0 = MPHY_RX_APB_VENDOR2_0;

	mphy_writel(ufs_tegra->mphy_l0_base, MPHY_PWR_CHANGE_CLK_BOOST,
			MPHY_RX_APB_VENDOR49_0_T234);
	mphy_update(ufs_tegra->mphy_l0_base, MPHY_GO_BIT, reg_vendor_0);

	if (ufs_tegra->x2config) {
		mphy_writel(ufs_tegra->mphy_l1_base, MPHY_PWR_CHANGE_CLK_BOOST,
				MPHY_RX_APB_VENDOR49_0_T234);
		mphy_update(ufs_tegra->mphy_l1_base, MPHY_GO_BIT, reg_vendor_0);
	}
	udelay(20);
}

void ufs_tegra_disable_mphy_slcg(struct ufs_tegra_host *ufs_tegra)
{
	u32 val = 0, reg_cg_over, reg_vendor_0;

	if (ufs_tegra->soc->chip_id == TEGRA234) {
		reg_cg_over = MPHY_TX_APB_TX_CG_OVR0_0_T234;
		reg_vendor_0 = MPHY_TX_APB_TX_VENDOR0_0_T234;
	} else {
		reg_cg_over = MPHY_TX_APB_TX_CG_OVR0_0;
		reg_vendor_0 = MPHY_TX_APB_TX_VENDOR0_0;
	}

	val = (MPHY_TX_CLK_EN_SYMB | MPHY_TX_CLK_EN_SLOW |
			MPHY_TX_CLK_EN_FIXED | MPHY_TX_CLK_EN_3X);
	mphy_writel(ufs_tegra->mphy_l0_base, val, reg_cg_over);
	mphy_writel(ufs_tegra->mphy_l0_base, MPHY_GO_BIT, reg_vendor_0);

	if (ufs_tegra->x2config) {
		mphy_writel(ufs_tegra->mphy_l1_base, val, reg_cg_over);
		mphy_writel(ufs_tegra->mphy_l1_base, MPHY_GO_BIT, reg_vendor_0);
	}

}


static void ufs_tegra_mphy_rx_sync_capability(struct ufs_tegra_host *ufs_tegra)
{
	u32 val_88_8b = 0;
	u32 val_94_97 = 0;
	u32 val_8c_8f = 0;
	u32 val_98_9b = 0;
	u32 vendor2_reg;

	if (ufs_tegra->soc->chip_id == TEGRA234)
		vendor2_reg = MPHY_RX_APB_VENDOR2_0_T234;
	else
		vendor2_reg = MPHY_RX_APB_VENDOR2_0;

	/* MPHY RX sync lengths capability changes */

	/*Update HS_G1 Sync Length MPHY_RX_APB_CAPABILITY_88_8B_0*/
	val_88_8b = mphy_readl(ufs_tegra->mphy_l0_base,
			MPHY_RX_APB_CAPABILITY_88_8B_0);
	val_88_8b &= ~RX_HS_G1_SYNC_LENGTH_CAPABILITY(~0);
	val_88_8b |= RX_HS_G1_SYNC_LENGTH_CAPABILITY(RX_HS_SYNC_LENGTH);

	/*Update HS_G2&G3 Sync Length MPHY_RX_APB_CAPABILITY_94_97_0*/
	val_94_97 = mphy_readl(ufs_tegra->mphy_l0_base,
			MPHY_RX_APB_CAPABILITY_94_97_0);
	val_94_97 &= ~RX_HS_G2_SYNC_LENGTH_CAPABILITY(~0);
	val_94_97 |= RX_HS_G2_SYNC_LENGTH_CAPABILITY(RX_HS_SYNC_LENGTH);
	val_94_97 &= ~RX_HS_G3_SYNC_LENGTH_CAPABILITY(~0);
	val_94_97 |= RX_HS_G3_SYNC_LENGTH_CAPABILITY(RX_HS_SYNC_LENGTH);

	/* MPHY RX TActivate_capability changes */

	/* Update MPHY_RX_APB_CAPABILITY_8C_8F_0 */
	val_8c_8f = mphy_readl(ufs_tegra->mphy_l0_base,
			MPHY_RX_APB_CAPABILITY_8C_8F_0);
	val_8c_8f &= ~RX_MIN_ACTIVATETIME_CAP_ARG(~0);
	val_8c_8f |= RX_MIN_ACTIVATETIME_CAP_ARG(RX_MIN_ACTIVATETIME);

	/* Update MPHY_RX_APB_CAPABILITY_98_9B_0 */
	val_98_9b = mphy_readl(ufs_tegra->mphy_l0_base,
			MPHY_RX_APB_CAPABILITY_98_9B_0);
	val_98_9b &= ~RX_ADVANCED_FINE_GRANULARITY(~0);
	val_98_9b &= ~RX_ADVANCED_GRANULARITY(~0);
	val_98_9b &= ~RX_ADVANCED_MIN_ACTIVATETIME(~0);
	val_98_9b |= RX_ADVANCED_MIN_ACTIVATETIME(RX_ADVANCED_MIN_AT);

	mphy_writel(ufs_tegra->mphy_l0_base, val_88_8b,
			MPHY_RX_APB_CAPABILITY_88_8B_0);
	mphy_writel(ufs_tegra->mphy_l0_base, val_94_97,
			MPHY_RX_APB_CAPABILITY_94_97_0);
	mphy_writel(ufs_tegra->mphy_l0_base, val_8c_8f,
			MPHY_RX_APB_CAPABILITY_8C_8F_0);
	mphy_writel(ufs_tegra->mphy_l0_base, val_98_9b,
			MPHY_RX_APB_CAPABILITY_98_9B_0);
	mphy_update(ufs_tegra->mphy_l0_base,
				MPHY_GO_BIT, vendor2_reg);

	if (ufs_tegra->x2config) {
		mphy_writel(ufs_tegra->mphy_l1_base, val_88_8b,
			MPHY_RX_APB_CAPABILITY_88_8B_0);
		mphy_writel(ufs_tegra->mphy_l1_base, val_94_97,
			MPHY_RX_APB_CAPABILITY_94_97_0);
		mphy_writel(ufs_tegra->mphy_l1_base, val_8c_8f,
			MPHY_RX_APB_CAPABILITY_8C_8F_0);
		mphy_writel(ufs_tegra->mphy_l1_base, val_98_9b,
			MPHY_RX_APB_CAPABILITY_98_9B_0);
		/* set gobit */
		mphy_update(ufs_tegra->mphy_l1_base,
				MPHY_GO_BIT, vendor2_reg);
	}
}

void ufs_tegra_mphy_tx_advgran(struct ufs_tegra_host *ufs_tegra)
{
	u32 val = 0, reg_vendor_0;

	if (ufs_tegra->soc->chip_id == TEGRA234)
		reg_vendor_0 = MPHY_TX_APB_TX_VENDOR0_0_T234;
	else
		reg_vendor_0 = MPHY_TX_APB_TX_VENDOR0_0;

	val = (TX_ADVANCED_GRANULARITY | TX_ADVANCED_GRANULARITY_SETTINGS);
	mphy_update(ufs_tegra->mphy_l0_base, val,
					MPHY_TX_APB_TX_ATTRIBUTE_34_37_0);
	mphy_writel(ufs_tegra->mphy_l0_base, MPHY_GO_BIT,
						reg_vendor_0);

	if (ufs_tegra->x2config) {
		mphy_update(ufs_tegra->mphy_l1_base, val,
					MPHY_TX_APB_TX_ATTRIBUTE_34_37_0);
		mphy_writel(ufs_tegra->mphy_l1_base, MPHY_GO_BIT,
						reg_vendor_0);
	}
}


void ufs_tegra_mphy_rx_advgran(struct ufs_tegra_host *ufs_tegra)
{
	u32 val = 0, reg_vendor_2;

	if (ufs_tegra->soc->chip_id == TEGRA234)
		reg_vendor_2 = MPHY_RX_APB_VENDOR2_0_T234;
	else
		reg_vendor_2 = MPHY_RX_APB_VENDOR2_0;

	val = mphy_readl(ufs_tegra->mphy_l0_base, MPHY_RX_APB_CAPABILITY_98_9B_0);
	val &= ~RX_ADVANCED_GRANULARITY(~0);
	val |= RX_ADVANCED_GRANULARITY(0x1);

	val &= ~RX_ADVANCED_MIN_ACTIVATETIME(~0);
	val |= RX_ADVANCED_MIN_ACTIVATETIME(0x8);

	mphy_writel(ufs_tegra->mphy_l0_base, val,
					MPHY_RX_APB_CAPABILITY_98_9B_0);
	mphy_update(ufs_tegra->mphy_l0_base, MPHY_GO_BIT,
			reg_vendor_2);

	if (ufs_tegra->x2config) {
		val = mphy_readl(ufs_tegra->mphy_l1_base,
				MPHY_RX_APB_CAPABILITY_98_9B_0);
		val &= ~RX_ADVANCED_GRANULARITY(~0);
		val |= RX_ADVANCED_GRANULARITY(0x1);

		val &= ~RX_ADVANCED_MIN_ACTIVATETIME(~0);
		val |= RX_ADVANCED_MIN_ACTIVATETIME(0x8);

		mphy_writel(ufs_tegra->mphy_l1_base, val,
					MPHY_RX_APB_CAPABILITY_98_9B_0);
		mphy_update(ufs_tegra->mphy_l1_base, MPHY_GO_BIT,
			reg_vendor_2);
	}
}

void ufs_tegra_ufs_aux_ref_clk_enable(struct ufs_tegra_host *ufs_tegra)
{
	ufs_aux_update(ufs_tegra->ufs_aux_base, UFSHC_DEV_CLK_EN,
						UFSHC_AUX_UFSHC_DEV_CTRL_0);
}

void ufs_tegra_ufs_aux_ref_clk_disable(struct ufs_tegra_host *ufs_tegra)
{
	ufs_aux_clear_bits(ufs_tegra->ufs_aux_base, UFSHC_DEV_CLK_EN,
						UFSHC_AUX_UFSHC_DEV_CTRL_0);
}

void ufs_tegra_aux_reset_enable(struct ufs_tegra_host *ufs_tegra)
{
	ufs_aux_clear_bits(ufs_tegra->ufs_aux_base,
					UFSHC_DEV_RESET,
					UFSHC_AUX_UFSHC_DEV_CTRL_0);
}

void ufs_tegra_ufs_aux_prog(struct ufs_tegra_host *ufs_tegra)
{

	/*
	 * Release the reset to UFS device on pin ufs_rst_n
	 */

	if (ufs_tegra->ufshc_state != UFSHC_SUSPEND)
		ufs_aux_update(ufs_tegra->ufs_aux_base, UFSHC_DEV_RESET,
						UFSHC_AUX_UFSHC_DEV_CTRL_0);


	if (ufs_tegra->ufshc_state == UFSHC_SUSPEND) {
		/*
		 * Disable reference clock to Device
		 */
		ufs_tegra_ufs_aux_ref_clk_disable(ufs_tegra);

	} else {
		/*
		 * Enable reference clock to Device
		 */
		ufs_tegra_ufs_aux_ref_clk_enable(ufs_tegra);
	}
}

static void ufs_tegra_context_save(struct ufs_tegra_host *ufs_tegra)
{
	u32 reg_len = 0;
	u32 len = 0;
	u32 *mphy_context_save = ufs_tegra->mphy_context;

	reg_len = ARRAY_SIZE(mphy_rx_apb);
	/*
	 * Save mphy_rx_apb lane0 and lane1 context
	 */
	ufs_save_regs(ufs_tegra->mphy_l0_base, mphy_context_save,
							mphy_rx_apb, reg_len);
	len += reg_len;

	if (ufs_tegra->x2config) {
		ufs_save_regs(ufs_tegra->mphy_l1_base, mphy_context_save + len,
							mphy_rx_apb, reg_len);
		len += reg_len;
	}

	reg_len = ARRAY_SIZE(mphy_tx_apb);
	/*
	 * Save mphy_tx_apb lane0 and lane1 context
	 */
	ufs_save_regs(ufs_tegra->mphy_l0_base, mphy_context_save + len,
							mphy_tx_apb, reg_len);
	len += reg_len;
	if (ufs_tegra->x2config)
		ufs_save_regs(ufs_tegra->mphy_l1_base,
			mphy_context_save + len, mphy_tx_apb, reg_len);
}

static void ufs_tegra_context_restore(struct ufs_tegra_host *ufs_tegra)
{
	u32 reg_len = 0;
	u32 len = 0;
	u32 *mphy_context_restore = ufs_tegra->mphy_context;
	u32 reg_vendor_0, reg_vendor_2;

	if (ufs_tegra->soc->chip_id == TEGRA234) {
		reg_vendor_0 = MPHY_TX_APB_TX_VENDOR0_0_T234;
		reg_vendor_2 = MPHY_RX_APB_VENDOR2_0_T234;
	} else {
		reg_vendor_0 = MPHY_TX_APB_TX_VENDOR0_0;
		reg_vendor_2 = MPHY_RX_APB_VENDOR2_0;
	}

	reg_len = ARRAY_SIZE(mphy_rx_apb);
	/*
	 * Restore mphy_rx_apb lane0 and lane1 context
	 */
	ufs_restore_regs(ufs_tegra->mphy_l0_base, mphy_context_restore,
							mphy_rx_apb, reg_len);
	mphy_update(ufs_tegra->mphy_l0_base, MPHY_GO_BIT, reg_vendor_2);

	len += reg_len;
	if (ufs_tegra->x2config) {
		ufs_restore_regs(ufs_tegra->mphy_l1_base,
			mphy_context_restore + len, mphy_rx_apb, reg_len);
		mphy_update(ufs_tegra->mphy_l1_base, MPHY_GO_BIT,
				reg_vendor_2);
		len += reg_len;
	}

	reg_len = ARRAY_SIZE(mphy_tx_apb);
	/*
	 * Restore mphy_tx_apb lane0 and lane1 context
	 */
	ufs_restore_regs(ufs_tegra->mphy_l0_base, mphy_context_restore + len,
							mphy_tx_apb, reg_len);
	mphy_writel(ufs_tegra->mphy_l0_base, MPHY_GO_BIT, reg_vendor_0);

	len += reg_len;
	if (ufs_tegra->x2config) {
		ufs_restore_regs(ufs_tegra->mphy_l1_base,
			mphy_context_restore + len, mphy_tx_apb, reg_len);
		mphy_writel(ufs_tegra->mphy_l1_base, MPHY_GO_BIT, reg_vendor_0);
	}
}

#if LINUX_VERSION_CODE < KERNEL_VERSION(5, 16, 0)
static int ufs_tegra_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op)
#else
static int ufs_tegra_suspend(struct ufs_hba *hba, enum ufs_pm_op pm_op,
		enum ufs_notify_change_status status)
#endif
{
	struct ufs_tegra_host *ufs_tegra = hba->priv;
	struct device *dev = hba->dev;
	u32 val;
	int ret = 0;
	int timeout = 500;
	bool is_ufs_lp_pwr_gated = false;

	if (pm_op != UFS_SYSTEM_PM)
		return 0;

	ufs_tegra->ufshc_state = UFSHC_SUSPEND;

	if (ufs_tegra->soc->chip_id != TEGRA234) {
		/*
		 * Enable DPD for UFS
		 */
		if (ufs_tegra->ufs_pinctrl &&
				!IS_ERR_OR_NULL(ufs_tegra->dpd_enable)) {
			ret = pinctrl_select_state(ufs_tegra->ufs_pinctrl,
						   ufs_tegra->dpd_enable);
			if (ret)
				dev_err(dev, "pinctrl power down fail %d\n",
						ret);
		}

		do {
			udelay(100);
			val = ufs_aux_readl(ufs_tegra->ufs_aux_base,
					UFSHC_AUX_UFSHC_STATUS_0);
			if (val & UFSHC_HIBERNATE_STATUS) {
				is_ufs_lp_pwr_gated = true;
				break;
			}
			timeout--;
		} while (timeout > 0);

		if (timeout <= 0) {
			dev_err(dev, "UFSHC_AUX_UFSHC_STATUS_0 = %x\n", val);
			return -ETIMEDOUT;
		}

		if (is_ufs_lp_pwr_gated) {
			/*
			 * Save all armphy_rx_apb and armphy_tx_apb registers
			 * T234 does not require context save
			 */
			ufs_tegra_context_save(ufs_tegra);
			reset_control_assert(ufs_tegra->ufshc_lp_rst);
		}
	} else {
		/*
		 * For T234, during sc7 entry, the link is set to off state
		 * so that during sc7 exit link startup happens (According to IAS)
		 */
		ufshcd_set_link_off(hba);
	}

	/*
	 * Disable ufs, mphy tx/rx lane clocks if they are on
	 * and assert the reset
	 */

	ufs_tegra_disable_mphylane_clks(ufs_tegra);
	ufs_tegra_mphy_assert_reset(ufs_tegra);
	ufs_tegra_disable_ufs_clks(ufs_tegra);
	reset_control_assert(ufs_tegra->ufs_axi_m_rst);

	return ret;
}


static int ufs_tegra_resume(struct ufs_hba *hba, enum ufs_pm_op pm_op)
{
	struct ufs_tegra_host *ufs_tegra = hba->priv;
	struct device *dev = hba->dev;
	int ret = 0;

	if (pm_op != UFS_SYSTEM_PM)
		return 0;

	ufs_tegra->ufshc_state = UFSHC_RESUME;

	ret = ufs_tegra_enable_ufs_clks(ufs_tegra);
	if (ret)
		return ret;

	ret = ufs_tegra_enable_mphylane_clks(ufs_tegra);
	if (ret)
		goto out_disable_ufs_clks;
	ufs_tegra_mphy_deassert_reset(ufs_tegra);
	ufs_tegra_ufs_deassert_reset(ufs_tegra);
	ufs_tegra_ufs_aux_prog(ufs_tegra);

	if (ufs_tegra->soc->chip_id != TEGRA234) {
		if (ufs_tegra->ufs_pinctrl &&
			!IS_ERR_OR_NULL(ufs_tegra->dpd_disable)) {
			ret = pinctrl_select_state(ufs_tegra->ufs_pinctrl,
					   ufs_tegra->dpd_disable);
			if (ret) {
				dev_err(dev, "pinctrl power up fail %d\n", ret);
				goto out_disable_mphylane_clks;
			}
		}
		/*
		 * T234 does not require context restore
		 */
		ufs_tegra_context_restore(ufs_tegra);
	}

	ufs_tegra_set_clk_div(hba, UFS_VNDR_HCLKDIV_1US_TICK);

	if (ufs_tegra->x2config) {
		ret = ufs_tegra_mphy_receiver_calibration(ufs_tegra,
				ufs_tegra->mphy_l1_base);
		if (ret < 0)
			goto out_disable_mphylane_clks;
	}

	ret = ufs_tegra_mphy_receiver_calibration(ufs_tegra,
			ufs_tegra->mphy_l0_base);
	if (ret < 0)
		goto out_disable_mphylane_clks;

	pm_runtime_disable(dev);
	pm_runtime_set_active(dev);
	pm_runtime_enable(dev);

	return ret;

out_disable_mphylane_clks:
	ufs_tegra_disable_mphylane_clks(ufs_tegra);
out_disable_ufs_clks:
	ufs_tegra_disable_ufs_clks(ufs_tegra);

	return ret;
}

static void ufs_tegra_print_power_mode_config(struct ufs_hba *hba,
			struct ufs_pa_layer_attr *configured_params)
{
	u32 rx_gear;
	u32 tx_gear;
	const char *freq_series = "";

	rx_gear = configured_params->gear_rx;
	tx_gear = configured_params->gear_tx;

	if (configured_params->hs_rate) {
		if (configured_params->hs_rate == PA_HS_MODE_A)
			freq_series = "RATE_A";
		else if (configured_params->hs_rate == PA_HS_MODE_B)
			freq_series = "RATE_B";
		dev_info(hba->dev,
			"HS Mode RX_Gear:gear_%u TX_Gear:gear_%u %s series\n",
				rx_gear, tx_gear, freq_series);
	} else {
		dev_info(hba->dev,
			"PWM Mode RX_Gear:gear_%u TX_Gear:gear_%u\n",
				rx_gear, tx_gear);
	}
}

static void ufs_tegra_scramble_enable(struct ufs_hba *hba)
{
	u32 pa_val;

	ufshcd_dme_get(hba, UIC_ARG_MIB(PA_PEERSCRAMBLING), &pa_val);
	if (pa_val & SCREN) {
		ufshcd_dme_get(hba, UIC_ARG_MIB(PA_SCRAMBLING), &pa_val);
		pa_val |= SCREN;
		ufshcd_dme_set(hba, UIC_ARG_MIB(PA_SCRAMBLING), pa_val);
	}
}

static int ufs_tegra_pwr_change_notify(struct ufs_hba *hba,
		enum ufs_notify_change_status status,
		struct ufs_pa_layer_attr *dev_max_params,
		struct ufs_pa_layer_attr *dev_req_params)
{
	struct ufs_tegra_host *ufs_tegra = hba->priv;
	u32 vs_save_config;
	int ret = 0;
	u32 pa_reg_check;

	if (!dev_req_params) {
		pr_err("%s: incoming dev_req_params is NULL\n", __func__);
		ret = -EINVAL;
		goto out;
	}

	switch (status) {
	case PRE_CHANGE:
		/* Update VS_DebugSaveConfigTime Tref */
		ufshcd_dme_get(hba, UIC_ARG_MIB(VS_DEBUGSAVECONFIGTIME),
			&vs_save_config);
		/* Update VS_DebugSaveConfigTime st_sct */
		vs_save_config &= ~SET_ST_SCT(~0);
		vs_save_config |= SET_ST_SCT(VS_DEBUGSAVECONFIGTIME_ST_SCT);
		/* Update VS_DebugSaveConfigTime Tref */
		vs_save_config &= ~SET_TREF(~0);
		vs_save_config |= SET_TREF(VS_DEBUGSAVECONFIGTIME_TREF);

		ufshcd_dme_set(hba, UIC_ARG_MIB(VS_DEBUGSAVECONFIGTIME),
				vs_save_config);

		memcpy(dev_req_params, dev_max_params,
			sizeof(struct ufs_pa_layer_attr));

		ufs_tegra->enable_hs_mode = true;

		if ((ufs_tegra->enable_hs_mode) && (dev_max_params->hs_rate)) {
			if (ufs_tegra->max_hs_gear) {
				if (dev_max_params->gear_rx >
						ufs_tegra->max_hs_gear)
					dev_req_params->gear_rx =
						ufs_tegra->max_hs_gear;
				if (dev_max_params->gear_tx >
						ufs_tegra->max_hs_gear)
					dev_req_params->gear_tx =
						ufs_tegra->max_hs_gear;
			} else {
				dev_req_params->gear_rx = UFS_HS_G1;
				dev_req_params->gear_tx = UFS_HS_G1;
			}
			if (ufs_tegra->mask_fast_auto_mode) {
				dev_req_params->pwr_rx = FAST_MODE;
				dev_req_params->pwr_tx = FAST_MODE;
			} else {
				dev_req_params->pwr_rx = FASTAUTO_MODE;
				dev_req_params->pwr_tx = FASTAUTO_MODE;
			}
			if (ufs_tegra->mask_hs_mode_b) {
				dev_req_params->hs_rate = PA_HS_MODE_A;
				ufs_tegra_enable_ufs_uphy_pll3(ufs_tegra,
								false);
			} else {
				ufs_tegra_enable_ufs_uphy_pll3(ufs_tegra, true);
			}
			if (ufs_tegra->enable_scramble)
				ufs_tegra_scramble_enable(hba);

			/*
			 * Clock boost during power change
			 * is required as per T234 IAS document
			 */
			if (ufs_tegra->soc->chip_id == TEGRA234)
				ufs_tegra_pwr_change_clk_boost(ufs_tegra);
		} else {
			if (ufs_tegra->max_pwm_gear) {
				ufshcd_dme_get(hba,
					UIC_ARG_MIB(PA_MAXRXPWMGEAR),
					&dev_req_params->gear_rx);
				ufshcd_dme_peer_get(hba,
					UIC_ARG_MIB(PA_MAXRXPWMGEAR),
					&dev_req_params->gear_tx);
				if (dev_req_params->gear_rx >
						ufs_tegra->max_pwm_gear)
					dev_req_params->gear_rx =
						ufs_tegra->max_pwm_gear;
				if (dev_req_params->gear_tx >
						ufs_tegra->max_pwm_gear)
					dev_req_params->gear_tx =
						ufs_tegra->max_pwm_gear;
			} else {
				dev_req_params->gear_rx = UFS_PWM_G1;
				dev_req_params->gear_tx = UFS_PWM_G1;
			}
			dev_req_params->pwr_rx = SLOWAUTO_MODE;
			dev_req_params->pwr_tx = SLOWAUTO_MODE;
			dev_req_params->hs_rate = 0;
		}
		memcpy(&hba->max_pwr_info.info, dev_req_params,
			sizeof(struct ufs_pa_layer_attr));
		break;
	case POST_CHANGE:
		ufs_tegra_print_power_mode_config(hba, dev_req_params);
		ufshcd_dme_get(hba, UIC_ARG_MIB(PA_SCRAMBLING), &pa_reg_check);
		if (pa_reg_check & SCREN)
			dev_info(hba->dev, "ufs scrambling feature enabled\n");
		break;
	default:
		break;
	}
out:
	return ret;
}

static int ufs_tegra_hce_enable_notify(struct ufs_hba *hba,
		enum ufs_notify_change_status status)
{
	struct ufs_tegra_host *ufs_tegra = hba->priv;
	struct device *dev = ufs_tegra->hba->dev;
	int err = 0;

	switch (status) {
	case PRE_CHANGE:
		err = ufs_tegra_host_clk_enable(dev,
			"mphy_force_ls_mode",
			ufs_tegra->mphy_force_ls_mode);
		if (err)
			return err;
		udelay(500);
		ufs_aux_clear_bits(ufs_tegra->ufs_aux_base,
				UFSHC_DEV_RESET,
				UFSHC_AUX_UFSHC_DEV_CTRL_0);
		break;
	case POST_CHANGE:
		ufs_aux_clear_bits(ufs_tegra->ufs_aux_base,
					UFSHC_CG_SYS_CLK_OVR_ON,
					UFSHC_AUX_UFSHC_SW_EN_CLK_SLCG_0);
		ufs_tegra_ufs_aux_prog(ufs_tegra);
		ufs_tegra_set_clk_div(hba, UFS_VNDR_HCLKDIV_1US_TICK);
		clk_disable_unprepare(ufs_tegra->mphy_force_ls_mode);
		if (ufs_tegra->soc->chip_id == TEGRA234)
			ufs_tegra_ufs_mmio_axi(hba);
		break;
	default:
		break;
	}
	return err;
}

static void ufs_tegra_unipro_post_linkup(struct ufs_hba *hba)
{
	struct ufs_tegra_host *ufs_tegra = hba->priv;
	/* set cport connection status = 1 */
	ufshcd_dme_set(hba, UIC_ARG_MIB(T_CONNECTIONSTATE), 0x1);

	/* MPHY TX sync length changes to MAX */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TxHsG1SyncLength), 0x4f);
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TxHsG2SyncLength), 0x4f);
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_TxHsG3SyncLength), 0x4f);

	/* Local Timer Value Changes */
	ufshcd_dme_set(hba, UIC_ARG_MIB(DME_FC0PROTECTIONTIMEOUTVAL), 0x1fff);
	ufshcd_dme_set(hba, UIC_ARG_MIB(DME_TC0REPLAYTIMEOUTVAL), 0xffff);
	ufshcd_dme_set(hba, UIC_ARG_MIB(DME_AFC0REQTIMEOUTVAL), 0x7fff);

	/* PEER TIMER values changes - PA_PWRModeUserData */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA0), 0x1fff);
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA1), 0xffff);
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_PWRMODEUSERDATA2), 0x7fff);

	/* After link start configuration request from Host controller,
	 * burst closure delay needs to be configured.
	 */
	ufshcd_dme_set(hba, UIC_ARG_MIB(VS_TXBURSTCLOSUREDELAY),
					ufs_tegra->vs_burst);

}

static void ufs_tegra_unipro_pre_linkup(struct ufs_hba *hba)
{
	struct ufs_tegra_host *ufs_tegra = hba->priv;
	/* Unipro LCC disable */
	ufshcd_dme_set(hba, UIC_ARG_MIB(PA_Local_TX_LCC_Enable), 0x0);
	/* Before link start configuration request from Host controller,
	 * burst closure delay needs to be configured to 0[7:0]
	 */
	ufshcd_dme_get(hba, UIC_ARG_MIB(VS_TXBURSTCLOSUREDELAY),
			&ufs_tegra->vs_burst);
	ufshcd_dme_set(hba, UIC_ARG_MIB(VS_TXBURSTCLOSUREDELAY), 0x0);
}

static int ufs_tegra_link_startup_notify(struct ufs_hba *hba,
			enum ufs_notify_change_status status)
{
	struct ufs_tegra_host *ufs_tegra = hba->priv;
	int err = 0;

	switch (status) {
	case PRE_CHANGE:
		ufs_tegra_mphy_rx_sync_capability(ufs_tegra);
		ufs_tegra_unipro_pre_linkup(hba);
		break;
	case POST_CHANGE:
		/*POST_CHANGE case is called on success of link start-up*/
		dev_info(hba->dev, "dme-link-startup Successful\n");
		ufs_tegra_unipro_post_linkup(hba);
		if (ufs_tegra->x2config) {
			err = ufs_tegra_mphy_receiver_calibration(ufs_tegra,
					ufs_tegra->mphy_l1_base);
			if (err)
				return err;
		}

		err = ufs_tegra_mphy_receiver_calibration(ufs_tegra,
				ufs_tegra->mphy_l0_base);
		if (err)
			return err;
		break;
	default:
		break;
	}

	return err;
}

static void ufs_tegra_config_soc_data(struct ufs_tegra_host *ufs_tegra)
{
	struct device *dev = ufs_tegra->hba->dev;
	struct device_node *np = dev->of_node;

	ufs_tegra->enable_mphy_rx_calib =
		of_property_read_bool(np, "nvidia,enable-rx-calib");

	ufs_tegra->x2config =
		of_property_read_bool(np, "nvidia,enable-x2-config");

	ufs_tegra->enable_hs_mode =
		of_property_read_bool(np, "nvidia,enable-hs-mode");

	ufs_tegra->enable_38mhz_clk =
		of_property_read_bool(np, "nvidia,enable-38mhz-clk");

	ufs_tegra->mask_fast_auto_mode =
		of_property_read_bool(np, "nvidia,mask-fast-auto-mode");

	ufs_tegra->mask_hs_mode_b =
		of_property_read_bool(np, "nvidia,mask-hs-mode-b");

	ufs_tegra->enable_ufs_provisioning =
		of_property_read_bool(np, "nvidia,enable-ufs-provisioning");

	ufs_tegra->configure_uphy_pll3 =
		of_property_read_bool(np, "nvidia,configure-uphy-pll3");


	of_property_read_u32(np, "nvidia,max-hs-gear", &ufs_tegra->max_hs_gear);
	of_property_read_u32(np, "nvidia,max-pwm-gear",
					&ufs_tegra->max_pwm_gear);

	ufs_tegra->enable_scramble =
		of_property_read_bool(np, "nvidia,enable-scramble");
}

static void ufs_tegra_eq_timeout(struct ufs_tegra_host *ufs_tegra)
{
	mphy_writel(ufs_tegra->mphy_l0_base, MPHY_EQ_TIMEOUT,
			MPHY_RX_APB_VENDOR3B_0_T234);
	mphy_update(ufs_tegra->mphy_l0_base, MPHY_GO_BIT, MPHY_RX_APB_VENDOR2_0_T234);

	if (ufs_tegra->x2config) {
		mphy_writel(ufs_tegra->mphy_l1_base, MPHY_EQ_TIMEOUT,
				MPHY_RX_APB_VENDOR3B_0_T234);
		mphy_update(ufs_tegra->mphy_l1_base, MPHY_GO_BIT, MPHY_RX_APB_VENDOR2_0_T234);
	}
}

/**
 * ufs_tegra_init - bind phy with controller
 * @hba: host controller instance
 *
 * Binds PHY with controller and powers up UPHY enabling clocks
 * and regulators.
 *
 * Returns -EPROBE_DEFER if binding fails, returns negative error
 * on phy power up failure and returns zero on success.
 */
static int ufs_tegra_init(struct ufs_hba *hba)
{
	struct ufs_tegra_host *ufs_tegra;
	struct device *dev = hba->dev;
	int err = 0;
	resource_size_t ufs_aux_base_addr, ufs_aux_addr_range, mphy_addr_range;

	ufs_tegra = devm_kzalloc(dev, sizeof(*ufs_tegra), GFP_KERNEL);
	if (!ufs_tegra) {
		err = -ENOMEM;
		dev_err(dev, "no memory for tegra ufs host\n");
		goto out;
	}

	ufs_tegra->soc = (struct ufs_tegra_soc_data *)of_device_get_match_data(dev);
	if (!ufs_tegra->soc)
		return -EINVAL;

	ufs_tegra->ufshc_state = UFSHC_INIT;
	ufs_tegra->hba = hba;
	hba->priv = (void *)ufs_tegra;

	ufs_tegra_config_soc_data(ufs_tegra);
	hba->spm_lvl = UFS_PM_LVL_3;
	hba->rpm_lvl = UFS_PM_LVL_1;
	hba->caps |= UFSHCD_CAP_INTR_AGGR;

	ufs_aux_base_addr = NV_ADDRESS_MAP_T23X_UFSHC_AUX_BASE;
	ufs_aux_addr_range = UFS_AUX_ADDR_RANGE_23X;
	mphy_addr_range = MPHY_ADDR_RANGE_T234;

	ufs_tegra->ufs_aux_base = devm_ioremap(dev,
			ufs_aux_base_addr, ufs_aux_addr_range);
	if (!ufs_tegra->ufs_aux_base) {
		err = -ENOMEM;
		dev_err(dev, "ufs_aux_base ioremap failed\n");
		goto out;
	}

	ufs_tegra->mphy_l0_base = devm_ioremap(dev,
			NV_ADDRESS_MAP_MPHY_L0_BASE, mphy_addr_range);
	if (!ufs_tegra->mphy_l0_base) {
		err = -ENOMEM;
		dev_err(dev, "mphy_l0_base ioremap failed\n");
		goto out;
	}

	ufs_tegra->mphy_l1_base = devm_ioremap(dev,
			NV_ADDRESS_MAP_MPHY_L1_BASE, mphy_addr_range);
	if (!ufs_tegra->mphy_l1_base) {
		err = -ENOMEM;
		dev_err(dev, "mphy_l1_base ioremap failed\n");
		goto out;
	}

	err = ufs_tegra_init_ufs_clks(ufs_tegra);
	if (err)
		goto out_host_free;

	err = ufs_tegra_init_mphy_lane_clks(ufs_tegra);
	if (err)
		goto out_host_free;
	err = ufs_tegra_init_uphy_pll3(ufs_tegra);
	if (err)
		goto out_host_free;
	err = ufs_tegra_host_clk_enable(dev, "mphy_force_ls_mode",
			ufs_tegra->mphy_force_ls_mode);
	if (err)
		goto out_host_free;
	usleep_range(1000, 2000);
	clk_disable_unprepare(ufs_tegra->mphy_force_ls_mode);
	usleep_range(1000, 2000);

	err = ufs_tegra_enable_ufs_clks(ufs_tegra);
	if (err)
		goto out_host_free;

	err = ufs_tegra_enable_mphylane_clks(ufs_tegra);
	if (err)
		goto out_disable_ufs_clks;

	err = ufs_tegra_mphy_reset_init(ufs_tegra);
	if (err)
		goto out_disable_mphylane_clks;

	ufs_tegra_mphy_deassert_reset(ufs_tegra);

	err = ufs_tegra_ufs_reset_init(ufs_tegra);
	if (err)
		goto out_disable_mphylane_clks;

	ufs_tegra_ufs_deassert_reset(ufs_tegra);
	ufs_tegra_mphy_rx_advgran(ufs_tegra);
	ufs_tegra_ufs_aux_ref_clk_disable(ufs_tegra);
	ufs_tegra_aux_reset_enable(ufs_tegra);
	ufs_tegra_ufs_aux_prog(ufs_tegra);
	ufs_tegra_set_clk_div(hba, UFS_VNDR_HCLKDIV_1US_TICK);
	ufs_tegra_eq_timeout(ufs_tegra);

	return err;

out_disable_mphylane_clks:
	ufs_tegra_disable_mphylane_clks(ufs_tegra);
out_disable_ufs_clks:
	ufs_tegra_disable_ufs_clks(ufs_tegra);
out_host_free:
		hba->priv = NULL;
out:
	return err;
}

static void ufs_tegra_exit(struct ufs_hba *hba)
{
	struct ufs_tegra_host *ufs_tegra = hba->priv;

	ufs_tegra_disable_mphylane_clks(ufs_tegra);
}

/**
 * struct ufs_hba_tegra_vops - UFS TEGRA specific variant operations
 *
 * The variant operations configure the necessary controller and PHY
 * handshake during initialization.
 */
struct ufs_hba_variant_ops ufs_hba_tegra_vops = {
	.name                   = "ufs-tegra",
	.init                   = ufs_tegra_init,
	.exit                   = ufs_tegra_exit,
	.suspend		= ufs_tegra_suspend,
	.resume			= ufs_tegra_resume,
	.hce_enable_notify      = ufs_tegra_hce_enable_notify,
	.link_startup_notify	= ufs_tegra_link_startup_notify,
	.pwr_change_notify      = ufs_tegra_pwr_change_notify,
};

static int ufs_tegra_probe(struct platform_device *pdev)
{
	int err;
	struct device *dev = &pdev->dev;

	/* Perform generic probe */
	err = ufshcd_pltfrm_init(pdev, &ufs_hba_tegra_vops);
	if (err) {
		if (err != -EPROBE_DEFER)
			dev_err(dev, "ufshcd_pltfrm_init() failed %d\n", err);
	}
	return err;
}

static int ufs_tegra_remove(struct platform_device *pdev)
{
	struct ufs_hba *hba =  platform_get_drvdata(pdev);

	pm_runtime_get_sync(&(pdev)->dev);
	ufshcd_remove(hba);
	return 0;

}

static struct ufs_tegra_soc_data tegra194_soc_data = {
	.chip_id = TEGRA194,
};

static struct ufs_tegra_soc_data tegra234_soc_data = {
	.chip_id = TEGRA234,
};

static const struct of_device_id ufs_tegra_of_match[] = {
	{
		.compatible = "tegra194,ufs_variant",
		.data = &tegra194_soc_data,
	}, {
		.compatible = "tegra234,ufs_variant",
		.data = &tegra234_soc_data,
	},
	{},
};

static const struct dev_pm_ops ufs_tegra_pm_ops = {
	SET_SYSTEM_SLEEP_PM_OPS(ufshcd_system_suspend, ufshcd_system_resume)
	SET_RUNTIME_PM_OPS(ufshcd_runtime_suspend, ufshcd_runtime_resume, NULL)
};

static struct platform_driver ufs_tegra_platform = {
	.probe = ufs_tegra_probe,
	.remove = ufs_tegra_remove,
	.driver = {
		.name = "ufs_tegra",
		.pm   = &ufs_tegra_pm_ops,
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(ufs_tegra_of_match),
	},
};

module_platform_driver(ufs_tegra_platform);
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Naveen Kumar Arepalli <naveenk@nvidia.com>");
MODULE_AUTHOR("Venkata Jagadish <vjagadish@nvidia.com>");
