#include <linux/device.h>
#include <linux/interrupt.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/pci.h>
#include <linux/uio_driver.h>

#define DRV_NAME	"uio_led"
#define DRV_VERSION	"0.2"

#define BAR0 0

#define CRA_ENABLE_REG 0x0050

static irqreturn_t button_int(int irq, struct uio_info *dev_info)
{

	readw(dev_info->mem[0].internal_addr + BUTTON_VALUE);

	return IRQ_HANDLED;
}

static int led_probe(struct pci_dev *pdev, const struct pci_device_id *id)
{
	struct uio_info *info;
	void __iomem * const * iomap;
	u16 value;
	u32 value32;
	int ret = -EAGAIN;
	int msi_num, i;

	info = devm_kzalloc(&pdev->dev, sizeof(*info), GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	ret = pcim_enable_device(pdev);
	if (ret)
		return ret;

	ret = pcim_iomap_regions(pdev, (1 << BAR0), DRV_NAME);
	if (ret)
		return ret;

	iomap = pcim_iomap_table(pdev);

	info->mem[0].name = "UIO_LED";
	info->mem[0].size = pci_resource_len(pdev, BAR0);
	info->mem[0].addr = pci_resource_start(pdev, BAR0);
	info->mem[0].internal_addr = iomap[BAR0];
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

	pci_set_master(pdev);

	/* activate PCIe interruption (BAR0 Cra 0x50) */
	writel(0x01, info->mem[0].internal_addr + CRA_ENABLE_REG);

	dev_info(&pdev->dev, "%s version = %s\n", DRV_NAME, DRV_VERSION);
	pci_set_drvdata(pdev, info);

	return 0;
}

static void led_remove(struct pci_dev *pdev) {
	struct uio_info *info = pci_get_drvdata(pdev);

	dev_info(&pdev->dev, "removing %s\n", DRV_NAME);
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
