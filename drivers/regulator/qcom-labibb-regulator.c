// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2019, The Linux Foundation. All rights reserved.

#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/of_irq.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/platform_device.h>
#include <linux/regmap.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>

#define REG_PERPH_TYPE                  0x04
#define QCOM_LAB_TYPE			0x24
#define QCOM_IBB_TYPE			0x20

#define REG_LABIBB_STATUS1		0x08
#define REG_LABIBB_ENABLE_CTL		0x46
#define LABIBB_STATUS1_VREG_OK_BIT	BIT(7)
#define LABIBB_CONTROL_ENABLE		BIT(7)
#define LABIBB_STATUS1_SC_DETECT_BIT	BIT(6)

#define LAB_ENABLE_CTL_MASK		BIT(7)
#define IBB_ENABLE_CTL_MASK		(BIT(7) | BIT(6))

#define POWER_DELAY			8000
#define POLLING_SCP_DONE_INTERVAL_US	5000
#define POLLING_SCP_TIMEOUT		16000


struct labibb_regulator {
	struct regulator_desc		desc;
	struct device			*dev;
	struct regmap			*regmap;
	struct regulator_dev		*rdev;
	u16				base;
	int				sc_irq;
	int				vreg_enabled;
	u8				type;
};

struct qcom_labibb {
	struct device			*dev;
	struct regmap			*regmap;
	struct labibb_regulator		lab;
	struct labibb_regulator		ibb;
};

struct labibb_regulator_data {
	u16				base;
	const char			*name;
	const char			*irq_name;
	u8				type;
};

static int qcom_labibb_regulator_is_enabled(struct regulator_dev *rdev)
{
	int ret;
	u8 val;
	struct labibb_regulator *reg = rdev_get_drvdata(rdev);

	ret = regmap_bulk_read(reg->regmap, reg->base +
			       REG_LABIBB_STATUS1, &val, 1);
	if (ret < 0) {
		dev_err(reg->dev, "Read register failed ret = %d\n", ret);
		return ret;
	}

	if (val & LABIBB_STATUS1_VREG_OK_BIT)
		return 1;
	else
		return 0;
}

static int _check_enabled_with_retries(struct regulator_dev *rdev,
			int retries, int enabled)
{
	int ret;
	struct labibb_regulator *reg = rdev_get_drvdata(rdev);

	while (retries--) {
		/* Wait for a small period before checking REG_LABIBB_STATUS1 */
		usleep_range(POWER_DELAY, POWER_DELAY + 200);

		ret = qcom_labibb_regulator_is_enabled(rdev);

		if (ret < 0) {
			dev_err(reg->dev, "Can't read %s regulator status\n",
				reg->desc.name);
			return ret;
		}

		if (ret == enabled)
			return ret;

	}

	return -EINVAL;
}

static int qcom_labibb_regulator_enable(struct regulator_dev *rdev)
{
	int ret, retries = 10;
	struct labibb_regulator *reg = rdev_get_drvdata(rdev);

	ret = regulator_enable_regmap(rdev);

	if (ret < 0) {
		dev_err(reg->dev, "Write failed: enable %s regulator\n",
			reg->desc.name);
		return ret;
	}

	ret = _check_enabled_with_retries(rdev, retries, 1);
	if (ret < 0) {
		dev_err(reg->dev, "retries exhausted: enable %s regulator\n",
			reg->desc.name);
		return ret;
	}

	if (ret) {
		reg->vreg_enabled = 1;
		return 0;
	}

	dev_err(reg->dev, "Can't enable %s\n", reg->desc.name);
	return -EINVAL;
}

static int qcom_labibb_regulator_disable(struct regulator_dev *rdev)
{
	int ret, retries = 2;
	struct labibb_regulator *reg = rdev_get_drvdata(rdev);

	ret = regulator_disable_regmap(rdev);

	if (ret < 0) {
		dev_err(reg->dev, "Write failed: disable %s regulator\n",
			reg->desc.name);
		return ret;
	}

	ret = _check_enabled_with_retries(rdev, retries, 0);
	if (ret < 0) {
		dev_err(reg->dev, "retries exhausted: disable %s regulator\n",
			reg->desc.name);
		return ret;
	}

	if (!ret) {
		reg->vreg_enabled = 0;
		return 0;
	}

	dev_err(reg->dev, "Can't disable %s\n", reg->desc.name);
	return -EINVAL;
}

static struct regulator_ops qcom_labibb_ops = {
	.enable			= qcom_labibb_regulator_enable,
	.disable		= qcom_labibb_regulator_disable,
	.is_enabled		= qcom_labibb_regulator_is_enabled,
};


static irqreturn_t labibb_sc_err_handler(int irq, void *_reg)
{
	int ret, count;
	u16 reg;
	u8 sc_err_mask;
	unsigned int val;
	struct labibb_regulator *labibb_reg = (struct labibb_regulator *)_reg;
	bool in_sc_err, reg_en, scp_done = false;

	if (irq == labibb_reg->sc_irq)
		reg = labibb_reg->base + REG_LABIBB_STATUS1;
	else
		return IRQ_HANDLED;

	sc_err_mask = LABIBB_STATUS1_SC_DETECT_BIT;

	ret = regmap_bulk_read(labibb_reg->regmap, reg, &val, 1);
	if (ret < 0) {
		dev_err(labibb_reg->dev, "Read failed, ret=%d\n", ret);
		return IRQ_HANDLED;
	}
	dev_dbg(labibb_reg->dev, "%s SC error triggered! STATUS1 = %d\n",
		labibb_reg->desc.name, val);

	in_sc_err = !!(val & sc_err_mask);

	/*
	 * The SC(short circuit) fault would trigger PBS(Portable Batch
	 * System) to disable regulators for protection. This would
	 * cause the SC_DETECT status being cleared so that it's not
	 * able to get the SC fault status.
	 * Check if the regulator is enabled in the driver but
	 * disabled in hardware, this means a SC fault had happened
	 * and SCP handling is completed by PBS.
	 */
	if (!in_sc_err) {

		reg = labibb_reg->base + REG_LABIBB_ENABLE_CTL;

		ret = regmap_read_poll_timeout(labibb_reg->regmap,
					reg, val,
					!(val & LABIBB_CONTROL_ENABLE),
					POLLING_SCP_DONE_INTERVAL_US,
					POLLING_SCP_TIMEOUT);

		if (!ret && labibb_reg->vreg_enabled) {
			dev_dbg(labibb_reg->dev,
				"%s has been disabled by SCP\n",
				labibb_reg->desc.name);
			scp_done = true;
		}
	}

	if (in_sc_err || scp_done) {
		regulator_lock(labibb_reg->rdev);
		regulator_notifier_call_chain(labibb_reg->rdev,
						REGULATOR_EVENT_OVER_CURRENT,
						NULL);
		regulator_unlock(labibb_reg->rdev);
	}
	return IRQ_HANDLED;
}

static int register_labibb_regulator(struct qcom_labibb *labibb,
				const struct labibb_regulator_data *reg_data,
				struct device_node *of_node)
{
	int ret;
	struct labibb_regulator *reg;
	struct regulator_config cfg = {};

	if (reg_data->type == QCOM_LAB_TYPE) {
		reg = &labibb->lab;
		reg->desc.enable_mask = LAB_ENABLE_CTL_MASK;
	} else {
		reg = &labibb->ibb;
		reg->desc.enable_mask = IBB_ENABLE_CTL_MASK;
	}

	reg->dev = labibb->dev;
	reg->base = reg_data->base;
	reg->type = reg_data->type;
	reg->regmap = labibb->regmap;
	reg->desc.enable_reg = reg->base + REG_LABIBB_ENABLE_CTL;
	reg->desc.enable_val = LABIBB_CONTROL_ENABLE;
	reg->desc.of_match = reg_data->name;
	reg->desc.name = reg_data->name;
	reg->desc.owner = THIS_MODULE;
	reg->desc.type = REGULATOR_VOLTAGE;
	reg->desc.ops = &qcom_labibb_ops;

	reg->sc_irq = -EINVAL;
	ret = of_irq_get_byname(of_node, reg_data->irq_name);
	if (ret < 0)
		dev_dbg(labibb->dev,
			"Unable to get %s, ret = %d\n",
			reg_data->irq_name, ret);
	else
		reg->sc_irq = ret;

	if (reg->sc_irq > 0) {
		ret = devm_request_threaded_irq(labibb->dev,
						reg->sc_irq,
						NULL, labibb_sc_err_handler,
						IRQF_ONESHOT |
						IRQF_TRIGGER_RISING,
						reg_data->irq_name, labibb);
		if (ret) {
			dev_err(labibb->dev, "Failed to register '%s' irq ret=%d\n",
				reg_data->irq_name, ret);
			return ret;
		}
	}

	cfg.dev = labibb->dev;
	cfg.driver_data = reg;
	cfg.regmap = labibb->regmap;
	cfg.of_node = of_node;

	reg->rdev = devm_regulator_register(labibb->dev, &reg->desc,
							&cfg);
	if (IS_ERR(reg->rdev)) {
		ret = PTR_ERR(reg->rdev);
		dev_err(labibb->dev,
			"unable to register %s regulator\n", reg_data->name);
		return ret;
	}
	return 0;
}

static const struct labibb_regulator_data pmi8998_labibb_data[] = {
	{0xde00, "lab", "lab-sc-err", QCOM_LAB_TYPE},
	{0xdc00, "ibb", "ibb-sc-err", QCOM_IBB_TYPE},
	{ },
};

static const struct of_device_id qcom_labibb_match[] = {
	{ .compatible = "qcom,pmi8998-lab-ibb", .data = &pmi8998_labibb_data},
	{ },
};
MODULE_DEVICE_TABLE(of, qcom_labibb_match);

static int qcom_labibb_regulator_probe(struct platform_device *pdev)
{
	struct qcom_labibb *labibb;
	struct device_node *child;
	const struct of_device_id *match;
	const struct labibb_regulator_data *reg;
	u8 type;
	int ret;

	labibb = devm_kzalloc(&pdev->dev, sizeof(*labibb), GFP_KERNEL);
	if (!labibb)
		return -ENOMEM;

	labibb->regmap = dev_get_regmap(pdev->dev.parent, NULL);
	if (!labibb->regmap) {
		dev_err(&pdev->dev, "Couldn't get parent's regmap\n");
		return -ENODEV;
	}

	labibb->dev = &pdev->dev;

	match = of_match_device(qcom_labibb_match, &pdev->dev);
	if (!match)
		return -ENODEV;

	for (reg = match->data; reg->name; reg++) {
		child = of_get_child_by_name(pdev->dev.of_node, reg->name);

		/* TODO: This validates if the type of regulator is indeed
		 * what's mentioned in DT.
		 * I'm not sure if this is needed, but we'll keep it for now.
		 */
		ret = regmap_bulk_read(labibb->regmap,
					reg->base + REG_PERPH_TYPE,
					&type, 1);
		if (ret < 0) {
			dev_err(labibb->dev,
				"Peripheral type read failed ret=%d\n",
				ret);
			return -EINVAL;
		}

		if ((type != QCOM_LAB_TYPE) && (type != QCOM_IBB_TYPE)) {
			dev_err(labibb->dev,
				"qcom_labibb: unknown peripheral type\n");
			return -EINVAL;
		} else if (type != reg->type) {
			dev_err(labibb->dev,
				"qcom_labibb: type read %x doesn't match DT %x\n",
				type, reg->type);
			return -EINVAL;
		}

		ret = register_labibb_regulator(labibb, reg, child);
		if (ret < 0) {
			dev_err(&pdev->dev,
				"qcom_labibb: error registering %s regulator: %d\n",
				child->full_name, ret);
			return ret;
		}
	}

	dev_set_drvdata(&pdev->dev, labibb);
	return 0;
}

static struct platform_driver qcom_labibb_regulator_driver = {
	.driver		= {
		.name		= "qcom-lab-ibb-regulator",
		.of_match_table	= qcom_labibb_match,
	},
	.probe		= qcom_labibb_regulator_probe,
};
module_platform_driver(qcom_labibb_regulator_driver);

MODULE_DESCRIPTION("Qualcomm labibb driver");
MODULE_LICENSE("GPL v2");
