/*
 * Copyright (c) 2021, NVIDIA CORPORATION.  All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ether_linux.h"

static irqreturn_t macsec_s_isr(int irq, void *data)
{
	struct macsec_priv_data *macsec_pdata = (struct macsec_priv_data *)data;
	struct ether_priv_data *pdata = macsec_pdata->ether_pdata;

	osi_macsec_s_isr(pdata->osi_core);

	return IRQ_HANDLED;
}

static irqreturn_t macsec_ns_isr(int irq, void *data)
{
	struct macsec_priv_data *macsec_pdata = (struct macsec_priv_data *)data;
	struct ether_priv_data *pdata = macsec_pdata->ether_pdata;

	osi_macsec_ns_isr(pdata->osi_core);

	return IRQ_HANDLED;
}

static int macsec_disable_car(struct macsec_priv_data *macsec_pdata)
{
	struct ether_priv_data *pdata = macsec_pdata->ether_pdata;
	struct device *dev = pdata->dev;
	void __iomem *addr = NULL;
	unsigned int val = 0;

	PRINT_ENTRY();
	if (!pdata->osi_core->pre_si) {
		if (macsec_pdata->ns_rst) {
			reset_control_assert(macsec_pdata->ns_rst);
		}

		if (!IS_ERR_OR_NULL(macsec_pdata->apb_pclk)) {
			clk_disable_unprepare(macsec_pdata->apb_pclk);
		}
		if (!IS_ERR_OR_NULL(macsec_pdata->tx_clk)) {
			clk_disable_unprepare(macsec_pdata->tx_clk);
		}
		if (!IS_ERR_OR_NULL(macsec_pdata->rx_clk)) {
			clk_disable_unprepare(macsec_pdata->rx_clk);
		}
	} else {
		/* For Pre-sil only, reset the MACsec controller directly.
		 * Assert the reset Bit 8 in CLK_RST_CONTROLLER_RST_DEV_MGBE_0.
		 */
		addr = devm_ioremap(dev, 0x21460018, 0x4);
		if (addr) {
			val = readl(addr);
			val |= BIT(8);
			writel(val, addr);
			devm_iounmap(dev, addr);
		}
		addr = devm_ioremap(dev, 0x21460080, 0x4);
		if (addr) {
			val = readl(addr);
			val &= ~BIT(2);
			writel(val, addr);
			devm_iounmap(dev, addr);
		}
	}

	PRINT_EXIT();
	return 0;
}

static int macsec_enable_car(struct macsec_priv_data *macsec_pdata)
{
	struct ether_priv_data *pdata = macsec_pdata->ether_pdata;
	struct device *dev = pdata->dev;
	void __iomem *addr = NULL;
	unsigned int val = 0;
	int ret = 0;

	PRINT_ENTRY();
	if (!pdata->osi_core->pre_si) {
		if (!IS_ERR_OR_NULL(macsec_pdata->apb_pclk)) {
			ret = clk_prepare_enable(macsec_pdata->apb_pclk);
			if (ret < 0) {
				dev_err(dev, "failed to enable macsec pclk\n");
				goto exit;
			}
		}
		if (!IS_ERR_OR_NULL(macsec_pdata->tx_clk)) {
			ret = clk_prepare_enable(macsec_pdata->tx_clk);
			if (ret < 0) {
				dev_err(dev,
					"failed to enable macsec_tx_clk\n");
				goto err_tx_clk;
			}
		}
		if (!IS_ERR_OR_NULL(macsec_pdata->rx_clk)) {
			ret = clk_prepare_enable(macsec_pdata->rx_clk);
			if (ret < 0) {
				dev_err(dev,
					"failed to enable macsec_rx_clk\n");
				goto err_rx_clk;
			}
		}
		/* TODO:
		 * 1. Any delay needed in silicon for clocks to stabilize ?
		 */
		if (macsec_pdata->ns_rst) {
			ret = reset_control_reset(macsec_pdata->ns_rst);
			if (ret < 0) {
				dev_err(dev, "failed to reset macsec\n");
				goto err_ns_rst;
			}
		}
	} else {
		/* For Pre-sil only, reset the MACsec controller directly.
		 * clk ungate first, followed by disabling reset
		 * Bit 8 in CLK_RST_CONTROLLER_RST_DEV_MGBE_0 register.
		 */
		addr = devm_ioremap(dev, 0x21460080, 0x4);
		if (addr) {
			val = readl(addr);
			val |= BIT(2);
			writel(val, addr);
			devm_iounmap(dev, addr);
		}

		/* Followed by disabling reset - Bit 8 in
		 * CLK_RST_CONTROLLER_RST_DEV_MGBE_0 register.
		 */
		addr = devm_ioremap(dev, 0x21460018, 0x4);
		if (addr) {
			val = readl(addr);
			val &= ~BIT(8);
			writel(val, addr);
			devm_iounmap(dev, addr);
		}
	}

	goto exit;

err_ns_rst:
	if (!IS_ERR_OR_NULL(macsec_pdata->rx_clk)) {
		clk_disable_unprepare(macsec_pdata->rx_clk);
	}
err_rx_clk:
	if (!IS_ERR_OR_NULL(macsec_pdata->tx_clk)) {
		clk_disable_unprepare(macsec_pdata->tx_clk);
	}
err_tx_clk:
	if (!IS_ERR_OR_NULL(macsec_pdata->apb_pclk)) {
		clk_disable_unprepare(macsec_pdata->apb_pclk);
	}
exit:
	PRINT_EXIT();
	return ret;
}

int macsec_close(struct macsec_priv_data *macsec_pdata)
{
	struct ether_priv_data *pdata = macsec_pdata->ether_pdata;
	struct device *dev = pdata->dev;
	int ret = 0;

	PRINT_ENTRY();
#ifdef DEBUG_MACSEC
	macsec_disable_car(macsec_pdata);
#endif
	/* 1. Disable the macsec controller */
	ret = osi_macsec_en(pdata->osi_core, OSI_DISABLE);
	if (ret < 0) {
		dev_err(dev, "%s: Failed to enable macsec Tx/Rx, %d\n",
			__func__, ret);
		return ret;
	}
	macsec_pdata->enabled = OSI_DISABLE;
	osi_macsec_deinit(pdata->osi_core);

	devm_free_irq(dev, macsec_pdata->ns_irq, macsec_pdata);
	devm_free_irq(dev, macsec_pdata->s_irq, macsec_pdata);
	PRINT_EXIT();

	return ret;
}

int macsec_open(struct macsec_priv_data *macsec_pdata)
{
	struct ether_priv_data *pdata = macsec_pdata->ether_pdata;
	struct device *dev = pdata->dev;
	int ret = 0;

	PRINT_ENTRY();
	/* 1. Request macsec irqs */
	snprintf(macsec_pdata->irq_name[0], MACSEC_IRQ_NAME_SZ, "%s.macsec_s",
		 netdev_name(pdata->ndev));
	ret = devm_request_irq(dev, macsec_pdata->s_irq, macsec_s_isr,
			       IRQF_TRIGGER_NONE, macsec_pdata->irq_name[0],
			       macsec_pdata);
	if (ret < 0) {
		dev_err(dev, "failed to request irq %d\n", __LINE__);
		goto exit;
	}
	pr_err("%s: requested s_irq %d: %s\n", __func__, macsec_pdata->s_irq,
	       macsec_pdata->irq_name[0]);

	snprintf(macsec_pdata->irq_name[1], MACSEC_IRQ_NAME_SZ, "%s.macsec_ns",
		 netdev_name(pdata->ndev));
	ret = devm_request_irq(dev, macsec_pdata->ns_irq, macsec_ns_isr,
			       IRQF_TRIGGER_NONE, macsec_pdata->irq_name[1],
			       macsec_pdata);
	if (ret < 0) {
		dev_err(dev, "failed to request irq %d\n", __LINE__);
		goto err_ns_irq;
	}
	pr_err("%s: requested ns_irq %d: %s\n", __func__, macsec_pdata->ns_irq,
	       macsec_pdata->irq_name[1]);

#ifdef DEBUG_MACSEC
	/* 2. Enable CAR */
	ret = macsec_enable_car(macsec_pdata);
	if (ret < 0) {
		dev_err(dev, "Unable to enable macsec clks & reset\n");
		goto err_car;
	}
#endif

	/* 3. invoke OSI HW initialization, initialize standard BYP entries */
	ret = osi_macsec_init(pdata->osi_core);
	if (ret < 0) {
		dev_err(dev, "osi_macsec_init failed, %d\n", ret);
		goto err_osi_init;
	}

	/* 4. Enable the macsec controller */
	ret = osi_macsec_en(pdata->osi_core,
			    (OSI_MACSEC_TX_EN | OSI_MACSEC_RX_EN));
	if (ret < 0) {
		dev_err(dev, "%s: Failed to enable macsec Tx/Rx, %d\n",
			__func__, ret);
		goto err_osi_init;
	}
	macsec_pdata->enabled = (OSI_MACSEC_TX_EN | OSI_MACSEC_RX_EN);

	goto exit;

err_osi_init:
#ifdef DEBUG_MACSEC
	macsec_disable_car(macsec_pdata);
err_car:
#endif
	devm_free_irq(dev, macsec_pdata->ns_irq, macsec_pdata);
err_ns_irq:
	devm_free_irq(dev, macsec_pdata->s_irq, macsec_pdata);
exit:
	PRINT_EXIT();
	return ret;
}

static int macsec_get_platform_res(struct macsec_priv_data *macsec_pdata)
{
	struct ether_priv_data *pdata = macsec_pdata->ether_pdata;
	struct device *dev = pdata->dev;
	struct platform_device *pdev = to_platform_device(dev);
	int ret = 0;

	PRINT_ENTRY();
	if (!pdata->osi_core->pre_si) {
		/* 1. Get resets */
		macsec_pdata->ns_rst = devm_reset_control_get(dev,
							      "macsec_rst");
		if (IS_ERR_OR_NULL(macsec_pdata->ns_rst)) {
			dev_err(dev, "Failed to get macsec_ns_rst\n");
			ret = PTR_ERR(macsec_pdata->ns_rst);
			goto exit;
		}

		/* 2. Get clks */
		macsec_pdata->apb_pclk = devm_clk_get(dev, "macsec_pclk");
		if (IS_ERR(macsec_pdata->apb_pclk)) {
			dev_err(dev, "failed to get macsec_pclk\n");
			ret = PTR_ERR(macsec_pdata->apb_pclk);
			goto exit;
		}
		macsec_pdata->tx_clk = devm_clk_get(dev, "macsec_tx_clk");
		if (IS_ERR(macsec_pdata->tx_clk)) {
			dev_err(dev, "failed to get macsec_tx_clk\n");
			ret = PTR_ERR(macsec_pdata->tx_clk);
			goto exit;
		}
		macsec_pdata->rx_clk = devm_clk_get(dev, "macsec_rx_clk");
		if (IS_ERR(macsec_pdata->rx_clk)) {
			dev_err(dev, "failed to get macsec_rx_clk\n");
			ret = PTR_ERR(macsec_pdata->rx_clk);
			goto exit;
		}
	}

	/* 3. Get irqs */
	macsec_pdata->ns_irq = platform_get_irq_byname(pdev, "macsec-ns-irq");
	if (macsec_pdata->ns_irq < 0) {
		dev_err(dev, "failed to get macsec-ns-irq\n");
		ret = macsec_pdata->ns_irq;
		goto exit;
	}

	macsec_pdata->s_irq = platform_get_irq_byname(pdev, "macsec-s-irq");
	if (macsec_pdata->s_irq < 0) {
		dev_err(dev, "failed to get macsec-s-irq\n");
		ret = macsec_pdata->s_irq;
		goto exit;
	}

	/* Sucess */
exit:
	PRINT_EXIT();
	return ret;
}

static void macsec_release_platform_res(struct macsec_priv_data *macsec_pdata)
{
	struct ether_priv_data *pdata = macsec_pdata->ether_pdata;
	struct device *dev = pdata->dev;

	PRINT_ENTRY();
	if (!pdata->osi_core->pre_si) {
		if (!IS_ERR_OR_NULL(macsec_pdata->apb_pclk)) {
			devm_clk_put(dev, macsec_pdata->apb_pclk);
		}
		if (!IS_ERR_OR_NULL(macsec_pdata->tx_clk)) {
			devm_clk_put(dev, macsec_pdata->tx_clk);
		}
		if (!IS_ERR_OR_NULL(macsec_pdata->rx_clk)) {
			devm_clk_put(dev, macsec_pdata->rx_clk);
		}
	}
	PRINT_EXIT();
}

static struct macsec_priv_data *genl_to_macsec_pdata(struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct net_device *ndev;
	struct ether_priv_data *pdata = NULL;
	struct macsec_priv_data *macsec_pdata = NULL;
	char ifname[IFNAMSIZ];

	PRINT_ENTRY();

	nla_strlcpy(ifname, attrs[NV_MACSEC_ATTR_IFNAME], sizeof(ifname));
	ndev = dev_get_by_name(genl_info_net(info),
			       ifname);
	if (!ndev) {
		pr_err("%s: Unable to get netdev\n", __func__);
		goto exit;
	}

	pdata = netdev_priv(ndev);
	macsec_pdata = pdata->macsec_pdata;
	dev_put(ndev);
exit:
	PRINT_EXIT();
	return macsec_pdata;
}

static int macsec_set_prot_frames(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct macsec_priv_data *macsec_pdata;
	int ret = 0;

	PRINT_ENTRY();
	if (!attrs[NV_MACSEC_ATTR_IFNAME] ||
	    !attrs[NV_MACSEC_ATTR_PROT_FRAMES_EN]) {
		ret = -EINVAL;
		goto exit;
	}

	macsec_pdata = genl_to_macsec_pdata(info);
	if (!macsec_pdata) {
		ret = -EOPNOTSUPP;
		goto exit;
	}

	macsec_pdata->protect_frames =
		nla_get_u32(attrs[NV_MACSEC_ATTR_PROT_FRAMES_EN]);

exit:
	PRINT_EXIT();
	return ret;
}

static int macsec_set_cipher(struct sk_buff *skb, struct genl_info *info)
{
	return -1;
}

static int macsec_set_controlled_port(struct sk_buff *skb,
				      struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct macsec_priv_data *macsec_pdata;
	unsigned int enable = 0;
	unsigned int macsec_en = 0;
	int ret = 0;

	PRINT_ENTRY();
	if (!attrs[NV_MACSEC_ATTR_IFNAME] ||
	    !attrs[NV_MACSEC_ATTR_CTRL_PORT_EN]) {
		ret = -EINVAL;
		goto exit;
	}

	macsec_pdata = genl_to_macsec_pdata(info);
	if (!macsec_pdata) {
		ret = -EOPNOTSUPP;
		goto exit;
	}

	enable = nla_get_u32(attrs[NV_MACSEC_ATTR_CTRL_PORT_EN]);
	if (enable) {
		macsec_en |= OSI_MACSEC_RX_EN;
		if (macsec_pdata->protect_frames)
			macsec_en |= OSI_MACSEC_TX_EN;
	}

	ret = osi_macsec_en(macsec_pdata->ether_pdata->osi_core, macsec_en);
	if (ret < 0) {
		ret = -EOPNOTSUPP;
		goto exit;
	}
	macsec_pdata->enabled = macsec_en;

exit:
	PRINT_EXIT();
	return ret;
}

static int parse_sa_config(struct nlattr **attrs, struct nlattr **tb_sa,
			   struct osi_macsec_sc_info *sc_info)
{
	if (!attrs[NV_MACSEC_ATTR_SA_CONFIG])
		return -EINVAL;

	if (nla_parse_nested(tb_sa, NV_MACSEC_SA_ATTR_MAX,
			     attrs[NV_MACSEC_ATTR_SA_CONFIG],
			     nv_macsec_sa_genl_policy, NULL))
		return -EINVAL;

	if (tb_sa[NV_MACSEC_SA_ATTR_SCI]) {
		memcpy(sc_info->sci, nla_data(tb_sa[NV_MACSEC_SA_ATTR_SCI]),
			sizeof(sc_info->sci));
	}
	if (tb_sa[NV_MACSEC_SA_ATTR_AN]) {
		sc_info->curr_an = nla_get_u8(tb_sa[NV_MACSEC_SA_ATTR_AN]);
	}
	if (tb_sa[NV_MACSEC_SA_ATTR_PN]) {
		sc_info->next_pn = nla_get_u32(tb_sa[NV_MACSEC_SA_ATTR_PN]);
	}
	if (tb_sa[NV_MACSEC_SA_ATTR_KEY]) {
		memcpy(sc_info->sak, nla_data(tb_sa[NV_MACSEC_SA_ATTR_KEY]),
			sizeof(sc_info->sak));
	}

	return 0;
}

static int macsec_dis_rx_sa(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct macsec_priv_data *macsec_pdata;
	struct ether_priv_data *pdata;
	struct osi_macsec_sc_info rx_sa;
	struct nlattr *tb_sa[NUM_NV_MACSEC_SA_ATTR];
	int ret = 0, i = 0;

	PRINT_ENTRY();
	if (!attrs[NV_MACSEC_ATTR_IFNAME] ||
	    parse_sa_config(attrs, tb_sa, &rx_sa)) {
		pr_err("%s: failed to parse nlattrs", __func__);
		ret = -EINVAL;
		goto exit;
	}

	macsec_pdata = genl_to_macsec_pdata(info);
	if (macsec_pdata) {
		pdata = macsec_pdata->ether_pdata;
	} else {
#ifndef TEST
		ret = -EOPNOTSUPP;
		goto exit;
#endif
	}

	pr_err("%s:\n"
		"\tsci: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n"
		"\tan: %u\n"
		"\tpn: %u",
		__func__,
		rx_sa.sci[0], rx_sa.sci[1], rx_sa.sci[2], rx_sa.sci[3],
		rx_sa.sci[4], rx_sa.sci[5], rx_sa.sci[6], rx_sa.sci[7],
		rx_sa.curr_an, rx_sa.next_pn);
	pr_err("\tkey: ");
	for (i = 0; i < 16; i++) {
		pr_cont(" %02x", rx_sa.sak[i]);
	}
	pr_err("");

#ifndef TEST
	ret = osi_macsec_config(pdata->osi_core, &rx_sa, OSI_DISABLE,
				CTLR_SEL_RX);
	if (ret < 0) {
		pr_err("%s: failed to disable Rx SA", __func__);
	}
#endif

exit:
	PRINT_EXIT();
	return ret;
}

static int macsec_en_rx_sa(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct macsec_priv_data *macsec_pdata;
	struct ether_priv_data *pdata;
	struct osi_macsec_sc_info rx_sa;
	struct nlattr *tb_sa[NUM_NV_MACSEC_SA_ATTR];
	int ret = 0, i = 0;

	PRINT_ENTRY();
	if (!attrs[NV_MACSEC_ATTR_IFNAME] ||
	    parse_sa_config(attrs, tb_sa, &rx_sa)) {
		pr_err("%s: failed to parse nlattrs", __func__);
		ret = -EINVAL;
		goto exit;
	}

	macsec_pdata = genl_to_macsec_pdata(info);
	if (macsec_pdata) {
		pdata = macsec_pdata->ether_pdata;
	} else {
#ifndef TEST
		ret = -EOPNOTSUPP;
		goto exit;
#endif
	}

	pr_err("%s:\n"
		"\tsci: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n"
		"\tan: %u\n"
		"\tpn: %u",
		__func__,
		rx_sa.sci[0], rx_sa.sci[1], rx_sa.sci[2], rx_sa.sci[3],
		rx_sa.sci[4], rx_sa.sci[5], rx_sa.sci[6], rx_sa.sci[7],
		rx_sa.curr_an, rx_sa.next_pn);
	pr_err("\tkey: ");
	for (i = 0; i < 16; i++) {
		pr_cont(" %02x", rx_sa.sak[i]);
	}
	pr_err("");

#ifndef TEST
	ret = osi_macsec_config(pdata->osi_core, &rx_sa, OSI_ENABLE,
				CTLR_SEL_RX);
	if (ret < 0) {
		pr_err("%s: failed to enable Rx SA", __func__);
	}
#endif

exit:
	PRINT_EXIT();
	return ret;
}

static int macsec_dis_tx_sa(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct macsec_priv_data *macsec_pdata;
	struct ether_priv_data *pdata;
	struct osi_macsec_sc_info tx_sa;
	struct nlattr *tb_sa[NUM_NV_MACSEC_SA_ATTR];
	int ret = 0, i = 0;

	PRINT_ENTRY();
	if (!attrs[NV_MACSEC_ATTR_IFNAME] ||
	    parse_sa_config(attrs, tb_sa, &tx_sa)) {
		pr_err("%s: failed to parse nlattrs", __func__);
		ret = -EINVAL;
		goto exit;
	}

	macsec_pdata = genl_to_macsec_pdata(info);
	if (macsec_pdata) {
		pdata = macsec_pdata->ether_pdata;
	} else {
#ifndef TEST
		ret = -EOPNOTSUPP;
		goto exit;
#endif
	}

	pr_err("%s:\n"
		"\tsci: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n"
		"\tan: %u\n"
		"\tpn: %u",
		__func__,
		tx_sa.sci[0], tx_sa.sci[1], tx_sa.sci[2], tx_sa.sci[3],
		tx_sa.sci[4], tx_sa.sci[5], tx_sa.sci[6], tx_sa.sci[7],
		tx_sa.curr_an, tx_sa.next_pn);
	pr_err("\tkey: ");
	for (i = 0; i < 16; i++) {
		pr_cont(" %02x", tx_sa.sak[i]);
	}
	pr_err("");

#ifndef TEST
	ret = osi_macsec_config(pdata->osi_core, &tx_sa, OSI_DISABLE,
				CTLR_SEL_TX);
	if (ret < 0) {
		pr_err("%s: failed to disable Tx SA", __func__);
	}
#endif

exit:
	PRINT_EXIT();
	return ret;
}

static int macsec_en_tx_sa(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct macsec_priv_data *macsec_pdata;
	struct ether_priv_data *pdata;
	struct osi_macsec_sc_info tx_sa;
	struct nlattr *tb_sa[NUM_NV_MACSEC_SA_ATTR];
	int ret = 0, i = 0;

	PRINT_ENTRY();
	if (!attrs[NV_MACSEC_ATTR_IFNAME] ||
	    parse_sa_config(attrs, tb_sa, &tx_sa)) {
		pr_err("%s: failed to parse nlattrs", __func__);
		ret = -EINVAL;
		goto exit;
	}

	macsec_pdata = genl_to_macsec_pdata(info);
	if (macsec_pdata) {
		pdata = macsec_pdata->ether_pdata;
	} else {
#ifndef TEST
		ret = -EOPNOTSUPP;
		goto exit;
#endif
	}

	pr_err("%s:\n"
		"\tsci: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n"
		"\tan: %u\n"
		"\tpn: %u",
		__func__,
		tx_sa.sci[0], tx_sa.sci[1], tx_sa.sci[2], tx_sa.sci[3],
		tx_sa.sci[4], tx_sa.sci[5], tx_sa.sci[6], tx_sa.sci[7],
		tx_sa.curr_an, tx_sa.next_pn);
	pr_err("\tkey: ");
	for (i = 0; i < 16; i++) {
		pr_cont(" %02x", tx_sa.sak[i]);
	}
	pr_err("");

#ifndef TEST
	ret = osi_macsec_config(pdata->osi_core, &tx_sa, OSI_ENABLE,
				CTLR_SEL_TX);
	if (ret < 0) {
		pr_err("%s: failed to enable Tx SA", __func__);
	}
#endif

exit:
	PRINT_EXIT();
	return ret;
}

static int macsec_deinit(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct macsec_priv_data *macsec_pdata;
	int ret = 0;

	PRINT_ENTRY();
	if (!attrs[NV_MACSEC_ATTR_IFNAME]) {
		ret = -EINVAL;
		goto exit;
	}

	macsec_pdata = genl_to_macsec_pdata(info);
	if (!macsec_pdata) {
		ret = -EOPNOTSUPP;
		goto exit;
	}

	ret = macsec_close(macsec_pdata);
	//TODO - check why needs -EOPNOTSUPP, why not pass ret val
	if (ret < 0) {
		ret = -EOPNOTSUPP;
	}

exit:
	PRINT_EXIT();
	return ret;
}

static int macsec_init(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	struct macsec_priv_data *macsec_pdata;
	int ret = 0;

	PRINT_ENTRY();
	if (!attrs[NV_MACSEC_ATTR_IFNAME]) {
		ret = -EINVAL;
		goto exit;
	}

	macsec_pdata = genl_to_macsec_pdata(info);
	if (!macsec_pdata) {
		ret = -EOPNOTSUPP;
		goto exit;
	}

	ret = macsec_open(macsec_pdata);
	//TODO - check why needs -EOPNOTSUPP, why not pass ret val
	if (ret < 0) {
		ret = -EOPNOTSUPP;
	}

exit:
	PRINT_EXIT();
	return ret;
}

static int macsec_set_replay_prot(struct sk_buff *skb, struct genl_info *info)
{
	struct nlattr **attrs = info->attrs;
	unsigned int replay_prot, window;

	if (!attrs[NV_MACSEC_ATTR_IFNAME] ||
	    !attrs[NV_MACSEC_ATTR_REPLAY_PROT_EN] ||
	    !attrs[NV_MACSEC_ATTR_REPLAY_WINDOW]) {
		return -EINVAL;
	}

	replay_prot = nla_get_u32(attrs[NV_MACSEC_ATTR_REPLAY_PROT_EN]);
	window = nla_get_u32(attrs[NV_MACSEC_ATTR_REPLAY_WINDOW]);
	pr_err("replay_prot(window): %u(%u)\n",
		replay_prot, window);


	/* TODO - set replay window for all active SA's.
	 * Store window in macsec_pdata so that future SA's can be updated
	 */
	return 0;
}

static const struct genl_ops nv_macsec_genl_ops[] = {
	{
		.cmd = NV_MACSEC_CMD_INIT,
		.doit = macsec_init,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_SET_PROT_FRAMES,
		.doit = macsec_set_prot_frames,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_SET_REPLAY_PROT,
		.doit = macsec_set_replay_prot,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_SET_CIPHER,
		.doit = macsec_set_cipher,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_SET_CONTROLLED_PORT,
		.doit = macsec_set_controlled_port,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_DEINIT,
		.doit = macsec_deinit,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_EN_TX_SA,
		.doit = macsec_en_tx_sa,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_DIS_TX_SA,
		.doit = macsec_dis_tx_sa,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_EN_RX_SA,
		.doit = macsec_en_rx_sa,
		.flags = GENL_ADMIN_PERM,
	},
	{
		.cmd = NV_MACSEC_CMD_DIS_RX_SA,
		.doit = macsec_dis_rx_sa,
		.flags = GENL_ADMIN_PERM,
	},
};

static struct genl_family nv_macsec_fam __ro_after_init = {
	.name = NV_MACSEC_GENL_NAME,
	.hdrsize = 0,
	.version = NV_MACSEC_GENL_VERSION,
	.maxattr = NV_MACSEC_ATTR_MAX,
	.module = THIS_MODULE,
	.ops = nv_macsec_genl_ops,
	.n_ops = ARRAY_SIZE(nv_macsec_genl_ops),
};

void macsec_remove(struct ether_priv_data *pdata)
{
	struct macsec_priv_data *macsec_pdata = NULL;

	PRINT_ENTRY();
	macsec_pdata = pdata->macsec_pdata;

	if (macsec_pdata) {
		/* 1. Unregister generic netlink */
		genl_unregister_family(&nv_macsec_fam);

		/* 2. Release platform resources */
		macsec_release_platform_res(macsec_pdata);
	}
	PRINT_EXIT();
}

#ifdef TEST
int macsec_genl_register(void)
{
	int ret = 0;
	ret = genl_register_family(&nv_macsec_fam);
	if (ret < 0) {
		pr_err("Srini:failed to register genl\n");
	}
	return ret;
}

void macsec_genl_unregister(void)
{
	genl_unregister_family(&nv_macsec_fam);
}
#endif /* TEST */

int macsec_probe(struct ether_priv_data *pdata)
{
	struct device *dev = pdata->dev;
	struct platform_device *pdev = to_platform_device(dev);
	struct osi_core_priv_data *osi_core = pdata->osi_core;
	struct macsec_priv_data *macsec_pdata = NULL;
	struct resource *res = NULL;
	int ret = 0;

	PRINT_ENTRY();
	/* 1. Check if MACsec is enabled in DT, if so map the I/O base addr */
	res = platform_get_resource_byname(pdev, IORESOURCE_MEM, "macsec-base");
	if (res) {
		osi_core->macsec_base = devm_ioremap_resource(dev, res);
		if (IS_ERR(osi_core->macsec_base)) {
			dev_err(dev, "failed to ioremap MACsec base addr\n");
			ret = PTR_ERR(osi_core->macsec_base);
			goto exit;
		}
	} else {
		/* MACsec not enabled in DT, nothing more to do */
		osi_core->macsec_base = NULL;
		osi_core->tz_base = NULL;
		pdata->macsec_pdata = NULL;

		/* Return positive value to indicate MACsec not enabled in DT */
		ret = 1;
		goto exit;
	}

	//TODO: Move to TZ window
	osi_core->tz_base = devm_ioremap(dev, 0x68C0000, 0x10000);
	if (IS_ERR(osi_core->tz_base)) {
		dev_err(dev, "failed to ioremap TZ base addr\n");
		ret = PTR_ERR(osi_core->tz_base);
		goto exit;
	}

	/* 2. Alloc macsec priv data structure */
	macsec_pdata = devm_kzalloc(dev, sizeof(struct macsec_priv_data),
				    GFP_KERNEL);
	if (macsec_pdata == NULL) {
		dev_err(dev, "failed to alloc macsec_priv_data\n");
		ret = -ENOMEM;
		goto exit;
	}
	macsec_pdata->ether_pdata = pdata;
	pdata->macsec_pdata = macsec_pdata;

	/* 3. Get OSI MACsec ops */
	if (osi_init_macsec_ops(osi_core) != 0) {
		dev_err(dev, "osi_init_macsec_ops failed\n");
		ret = -1;
		goto exit;
	}

	/* 4. Get platform resources - clks, resets, irqs.
	 * CAR is not enabled and irqs not requested until macsec_init()
	 */
	ret = macsec_get_platform_res(macsec_pdata);
	if (ret < 0) {
		dev_err(dev, "macsec_get_platform_res failed\n");
		goto exit;
	}

	/* 2. Enable CAR */
	ret = macsec_enable_car(macsec_pdata);
	if (ret < 0) {
		dev_err(dev, "Unable to enable macsec clks & reset\n");
		goto exit;
	}
	/* 5. Register macsec sysfs node - done from sysfs.c */

	/* 6. Register macsec generic netlink ops */
	ret = genl_register_family(&nv_macsec_fam);
	if (ret) {
		dev_err(dev, "Failed to register GENL ops\n");
		macsec_disable_car(macsec_pdata);
	}

exit:
	PRINT_EXIT();
	return ret;
}
