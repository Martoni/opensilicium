#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/uio_driver.h>

#define DRV_NAME	"uio_led"
#define DRV_VERSION	"0.1"

#define BAR0 0

#define CRA_ENABLE_REG 0x0050

static irqreturn_t button_int(int irq, struct uio_info *dev_info)
{
	return IRQ_HANDLED;
}

static int led_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct uio_info *info;
	int ret, msi_num;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	ret = pcim_iomap_regions(pdev, (1 << BAR0), DRV_NAME);
	if (ret)
		return ret;

	info->mem[0].name = "BAR0";
	info->mem[0].size = pci_resource_len(pdev, BAR0);
	info->mem[0].addr = pci_resource_start(pdev, BAR0);
	info->mem[0].memtype = UIO_MEM_PHYS;

	info->name = DRV_NAME;
	info->version = DRV_VERSION;

	/* Set up a single MSI interrupt */
	msi_num = pci_enable_msi_range(pdev, 1, 1);
	if (msi_num != 1) {
		dev_err(&pdev->dev, "failed to enable MSI interrupt\n");
		return msi_num;
	}

	info->irq = pdev->irq;
	info->irq_flags = IRQF_SHARED;
	info->handler = button_int;

	ret = uio_register_device(&pdev->dev, info);
	if (ret) {
		dev_err(&pdev->dev, "failed to register uio info\n");
		return ret;
	}

	/* activate PCIe interruption (BAR0 Cra 0x50) */
	writel(0x01, info->mem[0].internal_addr + CRA_ENABLE_REG);

	dev_info(&pdev->dev, "%s version = %s\n", DRV_NAME, DRV_VERSION);

	pci_set_drvdata(pdev, info);

	return 0;
}

static void led_remove(struct pci_dev *pdev) {
	struct uio_info *info = pci_get_drvdata(pdev);

	uio_unregister_device(info);
}

static const struct pci_device_id led_pci_ids[] = {
	{ PCI_DEVICE(0x1172, 0xE001), 0 },
	{ }
};
MODULE_DEVICE_TABLE(pci, led_pci_ids);

static struct pci_driver led_pci_driver = {
	.name		= DRV_NAME,
	.id_table	= led_pci_ids,
	.probe		= led_probe,
	.remove		= led_remove,
};
module_pci_driver(led_pci_driver);

MODULE_AUTHOR("Fabien Marteau <mail@fabienm.eu>");
MODULE_DESCRIPTION("Simple UIO led driver");
MODULE_LICENSE("GPL");
