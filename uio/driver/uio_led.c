/*
 * uio_led.c
 *
 * Copyright 2015 Sébastien Szymanski <sebastien.szymanski@armadeus.com>
 *
 */

#include <linux/device.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/uio_driver.h>

#define DRV_NAME       "uio_led"
#define DRV_VERSION    "0.1"

static irqreturn_t led_int(int irq, struct uio_info *dev_info)
{
	return IRQ_HANDLED;
}

static int led_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct uio_info *info;
	void __iomem * const * iomap;
	dma_addr_t dma_handle;
	int ret, i, msi_num;
	u32 version;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	ret = pcim_iomap_regions(pdev,
				(1 << led_CTRL_BAR) | (1 << CTBS_NUCT_BAR),
				DRV_NAME);
	if (ret)
		return ret;

	iomap = pcim_iomap_table(pdev);

	info->mem[0].name = "led_CTRL_BAR";
	info->mem[0].size = pci_resource_len(pdev, led_CTRL_BAR);
	info->mem[0].addr = pci_resource_start(pdev, led_CTRL_BAR);
	info->mem[0].internal_addr = iomap[led_CTRL_BAR];
	info->mem[0].memtype = UIO_MEM_PHYS;

	info->mem[1].name = "led_NUCT_BAR";
	info->mem[1].size = pci_resource_len(pdev, led_NUCT_BAR);
	info->mem[1].addr = pci_resource_start(pdev, led_NUCT_BAR);
	info->mem[1].internal_addr = iomap[led_NUCT_BAR];
	info->mem[1].memtype = UIO_MEM_PHYS;

	info->name = DRV_NAME;
	info->version = DRV_VERSION;

	ret = dma_set_mask_and_coherent(&pdev->dev, DMA_BIT_MASK(32));
	if (ret) {
		dev_err(&pdev->dev, "DMA not available\n");
		return ret;
	}

	info->mem[2].internal_addr = dmam_alloc_coherent(&pdev->dev,
						led_DMA_PAGE_SIZE,
						&dma_handle, GFP_KERNEL);
	if (!info->mem[2].internal_addr) {
		dev_err(&pdev->dev, "DMA alloc failed\n");
		return -ENOMEM;
	}

	info->mem[2].name = "led_IMAGE_0";
	info->mem[2].addr = dma_handle;
	info->mem[2].size = led_DMA_IMG_SIZE;
	info->mem[2].memtype = UIO_MEM_PHYS;

	info->mem[3].internal_addr = dmam_alloc_coherent(&pdev->dev,
						led_DMA_PAGE_SIZE,
						&dma_handle, GFP_KERNEL);
	if (!info->mem[3].internal_addr) {
		dev_err(&pdev->dev, "DMA alloc failed\n");
		return -ENOMEM;
	}

	info->mem[3].name = "led_IMAGE_1";
	info->mem[3].addr = dma_handle;
	info->mem[3].size = led_DMA_IMG_SIZE;
	info->mem[3].memtype = UIO_MEM_PHYS;

	/* Writing dma vector page in PCIe Cra */
	for (i=0 ; i < led_DMA_PAGES ; i++) {
		writel(info->mem[i+2].addr,
		       info->mem[0].internal_addr + 0x1000 + i*8);
		writel(0, info->mem[0].internal_addr + 0x1000 + i*8 + 4);
	}

	/* Set up a single MSI interrupt */
	msi_num = pci_enable_msi_range(pdev, 1, 1);
	if (msi_num != 1) {
		dev_err(&pdev->dev, "failed to enable MSI interrupt\n");
		return msi_num;
	}

	info->irq = pdev->irq;
	info->irq_flags = IRQF_SHARED;
	info->handler = led_int;

	ret = uio_register_device(&pdev->dev, info);
	if (ret) {
		dev_err(&pdev->dev, "failed to register uio info\n");
		return ret;
	}

	ret = sysfs_create_groups(&pdev->dev.kobj, led_groups);
	if (ret) {
		dev_err(&pdev->dev, "failed to register sysfs files\n");
		uio_unregister_device(info);
		return ret;
	}

	/* activate PCIe interruption (BAR0 Cra 0x50) */
	writel(0x01, info->mem[0].internal_addr + CRA_ENABLE_REG);

	dev_info(&pdev->dev, "led fpga version = 0x%x\n", version);
	dev_info(&pdev->dev, "%s version = %s\n", DRV_NAME, DRV_VERSION);

	pci_set_drvdata(pdev, info);
	pci_set_master(pdev);

	return 0;
}

static void led_remove(struct pci_dev *pdev) {
	struct uio_info *info = pci_get_drvdata(pdev);

	uio_unregister_device(info);
	sysfs_remove_groups(&pdev->dev.kobj, led_groups);
}

static const struct pci_device_id led_pci_ids[] = {
	{ PCI_DEVICE(0x1172, 0xE001), 0 },
	{ }
};
MODULE_DEVICE_TABLE(pci, led_pci_ids);

static struct pci_driver led_pci_driver = {
	.name                   = DRV_NAME,
	.id_table               = led_pci_ids,
	.probe                  = led_probe,
	.remove			= led_remove,
};
module_pci_driver(led_pci_driver);

MODULE_AUTHOR("Fabien Marteau <mail@fabienm.eu>");
MODULE_DESCRIPTION("Simple UIO led driver");
MODULE_LICENSE("GPL");
