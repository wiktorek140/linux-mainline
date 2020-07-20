// SPDX-License-Identifier: GPL-2.0-only
/*
 * Copyright (c) 2019, The Linux Foundation. All rights reserved.
 */

#include <linux/qcom_scm.h>
#include <linux/dma-mapping.h>

#include "arm-smmu.h"

#define to_qcom_smmu(var) container_of(var, struct qcom_smmu, smmu)

struct qcom_smmu {
	struct arm_smmu_device smmu;
	unsigned int sec_id;
	u64 attached_cbs;
	bool protected;
	int halt;
};

static inline void qcom_smmu_restore_sec_cfg(struct qcom_smmu *qsmmu, u8 cb)
{
	int ret;

	if (!qsmmu->sec_id)
		return;

	ret = qcom_scm_restore_sec_cfg(qsmmu->sec_id, cb);
	if (ret)
		dev_err(qsmmu->smmu.dev, "qcom_scm_restore_sec_cfg failed: %d\n", ret);
}

static int qcom_iommu_sec_ptbl_init(struct device *dev)
{
	size_t psize = 0;
	unsigned int spare = 0;
	void *cpu_addr;
	dma_addr_t paddr;
	unsigned long attrs;
	static bool allocated = false;
	int ret;

	if (allocated)
		return 0;

	ret = qcom_scm_iommu_secure_ptbl_size(spare, &psize);
	if (ret) {
		dev_err(dev, "failed to get iommu secure pgtable size (%d)\n", ret);
		return ret;
	}

	attrs = DMA_ATTR_NO_KERNEL_MAPPING;

	cpu_addr = dma_alloc_attrs(dev, psize, &paddr, GFP_KERNEL, attrs);
	if (!cpu_addr) {
		dev_err(dev, "failed to allocate %zu bytes for pgtable\n",
			psize);
		return -ENOMEM;
	}

	ret = qcom_scm_iommu_secure_ptbl_init(paddr, psize, spare);
	if (ret) {
		dev_err(dev, "failed to init iommu pgtable (%d)\n", ret);
		goto free_mem;
	}

	allocated = true;
	return 0;

free_mem:
	dma_free_attrs(dev, psize, cpu_addr, paddr, attrs);
	return ret;
}

static bool qcom_smmu_access_reg(struct arm_smmu_device *smmu, int page, bool write)
{
	struct qcom_smmu *qsmmu = to_qcom_smmu(smmu);
	int cb = page - smmu->numpage;

	if (cb >= 0 && !(qsmmu->attached_cbs & BIT(cb))) {
		return false;
	}

	if (cb < 0 && qsmmu->halt == 1) {
		qcom_smmu_restore_sec_cfg(qsmmu, 0);
		qsmmu->halt = 2;
	}

	if (!write || !qsmmu->protected)
		return true;

	switch (page) {
		case ARM_SMMU_GR0:
		case ARM_SMMU_GR1:
			return false;
		default:
			return true;
	}
}

static void qcom_smmu_writel(struct arm_smmu_device *smmu,
		int page, int offset, u32 val)
{
	if (!qcom_smmu_access_reg(smmu, page, true))
		return;

	writel_relaxed(val, arm_smmu_page(smmu, page) + offset);
}

static void qcom_smmu_writeq(struct arm_smmu_device *smmu,
		int page, int offset, u64 val)
{
	if (!qcom_smmu_access_reg(smmu, page, true))
		return;

	writeq_relaxed(val, arm_smmu_page(smmu, page) + offset);
}

static u32 qcom_smmu_readl(struct arm_smmu_device *smmu,
		int page, int offset)
{
	if (!qcom_smmu_access_reg(smmu, page, false))
		return 0;

	return readl_relaxed(arm_smmu_page(smmu, page) + offset);
}

static u64 qcom_smmu_readq(struct arm_smmu_device *smmu,
		int page, int offset)
{
	if (!qcom_smmu_access_reg(smmu, page, false))
		return 0;

	return readq_relaxed(arm_smmu_page(smmu, page) + offset);
}

static bool qcom_smmu_resume(struct arm_smmu_device *smmu)
{
	struct qcom_smmu *qsmmu = to_qcom_smmu(smmu);

	if (qsmmu->halt)
		qcom_smmu_restore_sec_cfg(qsmmu, 0);

	if (qsmmu->protected)
		return true;

	return false;
}

static int qcom_smmu_init_context(struct arm_smmu_domain *smmu_domain)
{
	struct qcom_smmu *qsmmu = to_qcom_smmu(smmu_domain->smmu);
	struct iommu_fwspec *fwspec = smmu_domain->fwspec;
	struct arm_smmu_cfg *cfg = &smmu_domain->cfg;

	if (WARN_ON(fwspec == NULL))
		return -EINVAL;

	if (fwspec->num_ids) {
		cfg->irptndx = cfg->asid = cfg->cbndx = fwspec->ids[0];
		qsmmu->attached_cbs |= BIT(cfg->cbndx);
		qcom_smmu_restore_sec_cfg(qsmmu, cfg->cbndx);
		return 0;
	}

	return -EINVAL;
}

static int qcom_smmu_cfg_probe(struct arm_smmu_device *smmu)
{
	struct qcom_smmu *qsmmu = to_qcom_smmu(smmu);

	smmu->features &= ~(ARM_SMMU_FEAT_FMT_AARCH64_64K
			|ARM_SMMU_FEAT_FMT_AARCH64_16K
			|ARM_SMMU_FEAT_FMT_AARCH64_4K);

	if (smmu->smrs) {
		devm_kfree(smmu->dev, smmu->smrs);
		smmu->smrs = NULL;
	}
	return 0;
}

static const struct arm_smmu_impl qcom_smmu_impl_msm8953 = {
	.write_reg = qcom_smmu_writel,
	.write_reg64 = qcom_smmu_writeq,
	.read_reg = qcom_smmu_readl,
	.read_reg64 = qcom_smmu_readq,
	.init_context = qcom_smmu_init_context,
	.cfg_probe = qcom_smmu_cfg_probe,
	.resume = qcom_smmu_resume,
};

static int qcom_sdm845_smmu500_reset(struct arm_smmu_device *smmu)
{
	int ret;

	arm_mmu500_reset(smmu);

	/*
	 * To address performance degradation in non-real time clients,
	 * such as USB and UFS, turn off wait-for-safe on sdm845 based boards,
	 * such as MTP and db845, whose firmwares implement secure monitor
	 * call handlers to turn on/off the wait-for-safe logic.
	 */
	ret = qcom_scm_qsmmu500_wait_safe_toggle(0);
	if (ret)
		dev_warn(smmu->dev, "Failed to turn off SAFE logic\n");

	return ret;
}

static const struct arm_smmu_impl qcom_smmu_impl = {
	.reset = qcom_sdm845_smmu500_reset,
};

struct arm_smmu_device *qcom_smmu_impl_init(struct arm_smmu_device *smmu)
{
	struct qcom_smmu *qsmmu;

	qsmmu = devm_kzalloc(smmu->dev, sizeof(*qsmmu), GFP_KERNEL);
	if (!qsmmu)
		return ERR_PTR(-ENOMEM);

	qsmmu->smmu = *smmu;

	qsmmu->smmu.impl = &qcom_smmu_impl;

	if (of_device_is_compatible(smmu->dev->of_node, "qcom,msm8953-smmu-v2")) {
		int ret;

		if (!qcom_scm_is_available())
			return ERR_PTR(-EPROBE_DEFER);

		ret = qcom_iommu_sec_ptbl_init(smmu->dev);
		if (ret)
			return ERR_PTR(ret);

		qsmmu->smmu.impl = &qcom_smmu_impl_msm8953;
		if (of_property_read_u32(smmu->dev->of_node, "qcom,iommu-secure-id",
					 &qsmmu->sec_id))
			return ERR_PTR(-ENODATA);

		qsmmu->protected = of_property_read_bool(smmu->dev->of_node, "qcom,protected");
		qsmmu->halt = of_property_read_bool(smmu->dev->of_node, "qcom,smmu-halt");
	}

	devm_kfree(smmu->dev, smmu);

	return &qsmmu->smmu;
}
