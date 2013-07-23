/*
 *  ------------------------------------------------------------------------
 *  stm_nand_emi.c STMicroelectronics NAND Flash driver: "EMI bit-banging"
 *  ------------------------------------------------------------------------
 *
 *  Copyright (c) 2008-2009 STMicroelectronics Limited
 *  Author: Angus Clark <Angus.Clark@st.com>
 *
 *  ------------------------------------------------------------------------
 *  May be copied or modified under the terms of the GNU General Public
 *  License Version 2.0 only.  See linux/COPYING for more information.
 *  ------------------------------------------------------------------------
 *
 *  Changelog:
 *      2009-03-09 Angus Clark <Angus.Clark@st.com>
 *              - moved EMI configuration from SoC setup to device probe
 *              - updated timing specification
 *	2008-02-19 Angus Clark <Angus.Clark@st.com>
 *		- Added FDMA support for Data IO
 *	2008-03-04 Angus Clark <Angus.Clark@st.com>
 *		- Support for Data IO though cache line to minimise impact
 *                of STBus latency
 */

#include <linux/io.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/completion.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/partitions.h>
#include <linux/dma-mapping.h>
#include <linux/clk.h>
#include <linux/gpio.h>
#include <linux/semaphore.h>
#include <linux/stm/dma.h>
#include <linux/stm/emi.h>
#include <linux/stm/platform.h>
#include <linux/stm/nand.h>
#include <asm/dma.h>
#include "../mtdcore.h"

#define NAME	"stm-nand-emi"

/*
 * Private data for stm_emi_nand driver.  Concurrency and device locking
 * handled by MTD layers.
 */
struct stm_nand_emi {
	struct nand_chip	chip;
	struct mtd_info		mtd;

	unsigned int		emi_bank;
	unsigned int		emi_base;
	unsigned int		emi_size;

	int			rbn_gpio;	/* Ready not busy pin         */

	void __iomem		*io_base;	/* EMI base for NAND chip     */
	void __iomem		*io_data;	/* Address for data IO        */
						/*        (possibly cached)   */
	void __iomem		*io_cmd;	/* CMD output (emi_addr(17))  */
	void __iomem		*io_addr;	/* ADDR output (emi_addr(17)) */

	spinlock_t		lock; /* save/restore IRQ flags */
	int			nr_parts;	/* Partition Table	      */
	struct mtd_partition	*parts;

#ifdef CONFIG_STM_NAND_EMI_FDMA
	unsigned long		nand_phys_addr;
	unsigned long		init_fdma_jiffies;	/* Rate limit init    */
	struct dma_chan		*dma_chan;		/* FDMA channel       */
	struct completion	dma_rd_comp;		/* FDMA read done     */
	struct completion	dma_wr_comp;		/* FDMA write done    */
#endif
};

/*
 * A group of NAND devices which hang off a single platform device, and
 * share some hardware (normally a single Ready/notBusy signal).
 */
struct stm_nand_emi_group {
	int rbn_gpio;
	int nr_banks;
	struct stm_nand_emi *banks[0];
};

static const char *part_probes[] = { "cmdlinepart", NULL };

/*
 * Routines for FDMA transfers.
 */
#if defined(CONFIG_STM_NAND_EMI_FDMA)
static bool fdma_filter_fn(struct dma_chan *chan, void *fn_param)
{
	/* Grant the first channel we come to */
	return true;
}

static int init_fdma_nand(struct stm_nand_emi *data)
{
	struct dma_slave_config config;
	dma_cap_mask_t mask;

	/* Set the DMA channel capabilities we want */
	dma_cap_zero(mask);
	dma_cap_set(DMA_SLAVE, mask);

	/* Request a matching DMA channel for NAND transactions */
	data->dma_chan = dma_request_channel(mask, fdma_filter_fn, NULL);
	if (!data->dma_chan) {
		printk(KERN_ERR NAME ": dma_request_channel failed!\n");
		return -EBUSY;
	}

	/* Set configuration (free-running only needs direction & FIFO addr) */
	config.direction = DMA_DEV_TO_MEM;
	config.src_addr = data->nand_phys_addr;

	if (dmaengine_slave_config(data->dma_chan, &config)) {
		printk(KERN_ERR NAME ": dmaengine_slave_config failed!\n");
		dma_release_channel(data->dma_chan);
		return -EBUSY;
	}

	/* Initiallise semaphores for read/write to wait on */
	init_completion(&data->dma_rd_comp);
	init_completion(&data->dma_wr_comp);

	printk(KERN_INFO NAME ": %s assigned %s(%d)\n",
	       data->mtd.name, dev_name(data->dma_chan->device->dev),
	       data->dma_chan->chan_id);

	return 0;
}

/* Ratelimit attempts to initialise FDMA */
static int init_fdma_nand_ratelimit(struct stm_nand_emi *data)
{
	if (printk_timed_ratelimit(&data->init_fdma_jiffies,  500))
		return init_fdma_nand(data);
	return -EBUSY;
}

static void exit_fdma_nand(struct stm_nand_emi *data)
{
	/* Release DMA channel */
	if (data->dma_chan)
		dma_release_channel(data->dma_chan);
}

static void nand_callback_dma(void *param)
{
	complete(param);
}

/*
 * Setup FDMA transfer. Add 'oob' to list, if present.  Assumes FDMA channel
 * has been initialised, and data areas are suitably aligned.
 */
static int nand_read_dma(struct mtd_info *mtd, uint8_t *buf, int buf_len,
			      uint8_t *oob, int oob_len)
{
	struct nand_chip *chip = mtd->priv;
	struct stm_nand_emi *data = chip->priv;
	struct scatterlist sg[2];
	struct dma_async_tx_descriptor *dma_desc;

	/* Map DMA addresses */
	sg_init_table(sg, 2);
	sg_dma_address(&sg[0]) = dma_map_single(NULL, buf, buf_len,
						DMA_FROM_DEVICE);
	sg_dma_len(&sg[0]) = buf_len;

	/* Are we doing data+oob transfer? */
	if (oob) {
		sg_dma_address(&sg[1]) = dma_map_single(NULL, oob, oob_len,
						DMA_FROM_DEVICE);
		sg_dma_len(&sg[1]) = oob_len;
	}

	/* Prepare the DMA transfer (on completion buffers will be unmapped) */
	dma_desc = data->dma_chan->device->device_prep_slave_sg(
			data->dma_chan, sg, oob ? 2 : 1, DMA_DEV_TO_MEM,
			DMA_CTRL_ACK | DMA_COMPL_SKIP_SRC_UNMAP |
			DMA_COMPL_DEST_UNMAP_SINGLE);

	dma_desc->callback = nand_callback_dma;
	dma_desc->callback_param = &data->dma_rd_comp;

	/* Submit the DMA transfer */
	(void) dma_desc->tx_submit(dma_desc);

	/* Wait for completion... */
	wait_for_completion(&data->dma_rd_comp);

	return 0;
}

/*
 * Setup FDMA transfer. Add 'oob' to list, if present.  Assumes FDMA channel
 * has been initialised, and data areas are suitably aligned.
 */
static int nand_write_dma(struct mtd_info *mtd, const uint8_t *buf, int buf_len,
			  uint8_t *oob, int oob_len)
{
	struct nand_chip *chip = mtd->priv;
	struct stm_nand_emi *data = chip->priv;
	struct scatterlist sg[2];
	struct dma_async_tx_descriptor *dma_desc;

	/* Map DMA addresses */
	sg_init_table(sg, 2);
	sg_dma_address(&sg[0]) = dma_map_single(NULL, (void *) buf, buf_len,
						DMA_TO_DEVICE);
	sg_dma_len(&sg[0]) = buf_len;

	/* Are we doing data+oob linked transfer? */
	if (oob) {
		sg_dma_address(&sg[1]) = dma_map_single(NULL, oob, oob_len,
						DMA_TO_DEVICE);
		sg_dma_len(&sg[1]) = oob_len;
	}

	/* Prepare the DMA transfer (on completion buffers will be unmapped) */
	dma_desc = data->dma_chan->device->device_prep_slave_sg(
			data->dma_chan, sg, oob ? 2 : 1, DMA_MEM_TO_DEV,
			DMA_CTRL_ACK | DMA_COMPL_SRC_UNMAP_SINGLE |
			DMA_COMPL_SKIP_DEST_UNMAP);

	dma_desc->callback = nand_callback_dma;
	dma_desc->callback_param = &data->dma_wr_comp;

	/* Submit the DMA transfer */
	(void) dma_desc->tx_submit(dma_desc);

	/* Wait for completion... */
	wait_for_completion(&data->dma_wr_comp);

	return 0;
}

/*
 * Write buf to NAND chip.  Attempt DMA write for 'large' bufs.  Fall-back to
 * writesl for small bufs and if FDMA fails to initialise.
 */
static void nand_write_buf_dma(struct mtd_info *mtd,
			       const uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct stm_nand_emi *data = chip->priv;
	unsigned long dma_align = dma_get_cache_alignment() - 1UL;
	int i;

	if (len >= 512 &&
	    virt_addr_valid(buf) &&
	    (data->dma_chan || init_fdma_nand_ratelimit(data) == 0)) {

		/* Read up to cache line boundary */
		while ((unsigned long)buf & dma_align) {
			writeb(*buf++, chip->IO_ADDR_W);
			len--;
		}

		/* Do DMA transfer, fall-back to writesl if fail */
		if (nand_write_dma(mtd, buf, len & ~dma_align, NULL, 0) != 0)
			writesl(chip->IO_ADDR_W, buf, (len & ~dma_align)/4);

		/* Mop up trailing bytes */
		for (i = (len & ~dma_align); i < len; i++)
			writeb(buf[i], chip->IO_ADDR_W);
	} else {
		/* write buf up to 4-byte boundary */
		while ((unsigned int)buf & 0x3) {
			writeb(*buf++, chip->IO_ADDR_W);
			len--;
		}

		writesl(chip->IO_ADDR_W, buf, len/4);

		/* mop up trailing bytes */
		for (i = (len & ~0x3); i < len; i++)
			writeb(buf[i], chip->IO_ADDR_W);
	}
}

/*
 * Read NAND chip to buf.  Attempt DMA read for 'large' bufs.  Fall-back to
 * readsl for small bufs and if FDMA fails to initialise.
 */
static void nand_read_buf_dma(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct stm_nand_emi *data = chip->priv;
	unsigned long dma_align = dma_get_cache_alignment() - 1UL;
	int i;

	if (len >= 512 &&
	    virt_addr_valid(buf) &&
	    (data->dma_chan || init_fdma_nand_ratelimit(data) == 0)) {

		/* Read up to cache-line boundary */
		while ((unsigned long)buf & dma_align) {
			*buf++ = readb(chip->IO_ADDR_R);
			len--;
		}

		/* Do DMA transfer, fall-back to readsl if fail */
		if (nand_read_dma(mtd, buf, len & ~dma_align, NULL, 0) != 0)
			readsl(chip->IO_ADDR_R, buf, (len & ~dma_align)/4);

		/* Mop up trailing bytes */
		for (i = (len & ~dma_align); i < len; i++)
			buf[i] = readb(chip->IO_ADDR_R);
	} else {

		/* Read buf up to 4-byte boundary */
		while ((unsigned int)buf & 0x3) {
			*buf++ = readb(chip->IO_ADDR_R);
			len--;
		}

		readsl(chip->IO_ADDR_R, buf, len/4);

		/* Mop up trailing bytes */
		for (i = (len & ~0x3); i < len; i++)
			buf[i] = readb(chip->IO_ADDR_R);
	}
}
#endif /* CONFIG_STM_NAND_EMI_FDMA */

#if defined(CONFIG_STM_NAND_EMI_LONGSL)
static void nand_readsl_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	/* read buf up to 4-byte boundary */
	while ((unsigned int)buf & 0x3) {
		*buf++ = readb(chip->IO_ADDR_R);
		len--;
	}

	readsl(chip->IO_ADDR_R, buf, len/4);

	/* mop up trailing bytes */
	for (i = (len & ~0x3); i < len; i++)
		buf[i] = readb(chip->IO_ADDR_R);
}
#endif /* CONFIG_STM_NAND_EMI_LONGSL */

#if defined(CONFIG_STM_NAND_EMI_LONGSL) || defined(CONFIG_STM_NAND_EMI_CACHED)
static void nand_writesl_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	/* write buf up to 4-byte boundary */
	while ((unsigned int)buf & 0x3) {
		writeb(*buf++, chip->IO_ADDR_W);
		len--;
	}

	writesl(chip->IO_ADDR_W, buf, len/4);

	/* mop up trailing bytes */
	for (i = (len & ~0x3); i < len; i++)
		writeb(buf[i], chip->IO_ADDR_W);
}
#endif

#if defined(CONFIG_STM_NAND_EMI_CACHED)
static void nand_read_buf_cached(struct mtd_info *mtd, uint8_t *buf, int len)
{
	struct nand_chip *chip = mtd->priv;
	struct stm_nand_emi *data = chip->priv;
	unsigned long irq_flags;

	int lenaligned = len & ~(L1_CACHE_BYTES-1);
	int lenleft = len & (L1_CACHE_BYTES-1);

	while (lenaligned > 0) {
		spin_lock_irqsave(&(data->lock), irq_flags);
		invalidate_ioremap_region(data->emi_base,
					  data->io_data, 0, L1_CACHE_BYTES);
		memcpy_fromio(buf, data->io_data, L1_CACHE_BYTES);
		spin_unlock_irqrestore(&(data->lock), irq_flags);

		buf += L1_CACHE_BYTES;
		lenaligned -= L1_CACHE_BYTES;
	}

#ifdef CONFIG_STM_L2_CACHE
	/* If L2 cache is enabled, we must ensure the cacheline is evicted prior
	 * to non-cached accesses..
	 */
	invalidate_ioremap_region(data->emi_base,
				  data->io_data, 0, L1_CACHE_BYTES);
#endif
	/* Mop up any remaining bytes */
	while (lenleft > 0) {
		*buf++ = readb(chip->IO_ADDR_R);
		lenleft--;
	}
}
#endif /* CONFIG_STM_NAND_EMI_CACHED */

/* Command control function for EMI 'bit-banging'.  AL and CL signals are
 * toggled via EMI address lines emi_addr_17, emi_addr_18.
 */
static void nand_cmd_ctrl_emi(struct mtd_info *mtd, int cmd, unsigned int ctrl)
{
	struct nand_chip *this = mtd->priv;
	struct stm_nand_emi *data = this->priv;

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_CLE) {
			this->IO_ADDR_W = data->io_cmd;
		} else if (ctrl & NAND_ALE) {
			this->IO_ADDR_W = data->io_addr;
		} else {
			this->IO_ADDR_W = data->io_base;
		}
	}

	if (cmd != NAND_CMD_NONE)
		writeb(cmd, this->IO_ADDR_W);
}

/* Device ready functio, for boards where nand_rbn is available via GPIO pin */
static int nand_device_ready(struct mtd_info *mtd)
{
	struct nand_chip *this = mtd->priv;
	struct stm_nand_emi *data = this->priv;

	return gpio_get_value(data->rbn_gpio);
}

#define GET_CLK_CYCLES(X, T)	(((X) + (T) - 1) / (T))
/* Configure EMI Bank for NAND access */
static int nand_config_emi(int bank, struct stm_nand_timing_data *td)
{
	struct clk *emi_clk;
	uint32_t emi_t_ns;
	uint32_t emi_p_ns;

	unsigned long config[4];

	uint32_t rd_cycle, wr_cycle;
	uint32_t iord_start, iord_end;
	uint32_t iowr_start, iowr_end;
	uint32_t rd_latch;
	uint32_t bus_release;
	uint32_t wait_active_low;

	printk(KERN_INFO NAME ": Configuring EMI Bank %d for NAND access\n",
	       bank);

	if (!td) {
		printk(KERN_ERR NAME "No timing data specified in platform "
		       "data\n");
		return 1;
	}

	/* Timings set in terms of EMI clock... */
	emi_clk = clk_get(NULL, "emi_clk");

	if (!emi_clk || IS_ERR(emi_clk)) {
		printk(KERN_WARNING NAME ": Failed to find EMI clock. "
		       "Using default 100MHz.\n");
		emi_t_ns = 10;
	} else {
		emi_t_ns = 1000000000UL / clk_get_rate(emi_clk);
	}
	emi_p_ns = emi_t_ns / 2;

	/* Convert nand timings to EMI compatible values */
	rd_cycle = GET_CLK_CYCLES(td->rd_on + td->rd_off, emi_t_ns) + 3;
	iord_start = 0;
	iord_end = GET_CLK_CYCLES(td->rd_on, emi_t_ns) + 2;
	rd_latch = GET_CLK_CYCLES(td->rd_on, emi_t_ns) + 2;
	bus_release = GET_CLK_CYCLES(50, emi_t_ns);
	wait_active_low = 0;
	wr_cycle = GET_CLK_CYCLES(td->wr_on + td->wr_off, emi_t_ns) + 3;
	iowr_start = 0;
	iowr_end = GET_CLK_CYCLES(td->wr_on, emi_t_ns) + 2;

	/* Set up EMI configuration data */
	config[0] = 0x04000699 |
		((bus_release & 0xf) << 11) |
		((rd_latch & 0x1f) << 20) |
		((wait_active_low & 0x1) << 25);

	config[1] =
		((rd_cycle & 0x7f) << 24) |
		((iord_start & 0xf) << 12) |
		((iord_end & 0xf) << 8);

	config[2] =
		((wr_cycle & 0x7f) << 24) |
		((iowr_start & 0xf) << 12) |
		((iowr_end & 0xf) << 8);

	config[3] = 0x00;

	/* Configure Bank */
	emi_bank_configure(bank, config);

	/* Disable PC mode */
	emi_config_pcmode(bank, 0);

	return 0;
}

/*
 * Probe for the NAND device.
 */
static struct stm_nand_emi * __init nand_probe_bank(
	struct stm_nand_bank_data *bank, int rbn_gpio,
	const char* name)
{
	struct stm_nand_emi *data;
	struct stm_nand_timing_data *tm;

	int res = 0;

	/* Allocate memory for the driver structure (and zero it) */
	data = kzalloc(sizeof(struct stm_nand_emi), GFP_KERNEL);
	if (!data) {
		printk(KERN_ERR NAME
		       ": Failed to allocate device structure.\n");
		return ERR_PTR(-ENOMEM);
	}

	/* Get EMI Bank base address */
	data->emi_bank = bank->csn;
	data->emi_base = emi_bank_base(data->emi_bank) +
		bank->emi_withinbankoffset;
	data->emi_size = (1 << 18) + 1;

	/* Configure EMI Bank */
	if (nand_config_emi(data->emi_bank, bank->timing_data) != 0) {
		printk(KERN_ERR NAME ": Failed to configure EMI bank "
		       "for NAND device\n");
		goto out1;
	}

	/* Request IO Memory */
	if (!request_mem_region(data->emi_base, data->emi_size, name)) {
		printk(KERN_ERR NAME ": Request mem 0x%x region failed\n",
		       data->emi_base);
		res = -ENODEV;
		goto out1;
	}

	/* Map base address */
	data->io_base = ioremap_nocache(data->emi_base, 4096);
	if (!data->io_base) {
		printk(KERN_ERR NAME ": ioremap failed for io_base 0x%08x\n",
		       data->emi_base);
		res = -ENODEV;
		goto out2;
	}

#ifdef CONFIG_STM_NAND_EMI_CACHED
	/* Map data address through cache line */
	data->io_data = ioremap_cache(data->emi_base, 4096);
	if (!data->io_data) {
		printk(KERN_ERR NAME ": ioremap failed for io_data 0x%08x\n",
		       data->emi_base);
		res = -ENOMEM;
		goto out3;
	}
#else
	data->io_data = data->io_base;
#endif
	/* Map cmd and addr addresses (emi_addr_17 and emi_addr_18) */
	data->io_cmd = ioremap_nocache(data->emi_base | (1 << 17), 1);
	if (!data->io_cmd) {
		printk(KERN_ERR NAME ": ioremap failed for io_cmd 0x%08x\n",
		       data->emi_base | (1 << 17));
		res = -ENOMEM;
		goto out4;
	}

	data->io_addr = ioremap_nocache(data->emi_base | (1 << 18), 1);
	if (!data->io_addr) {
		printk(KERN_ERR NAME ": ioremap failed for io_addr 0x%08x\n",
		       data->emi_base | (1 << 18));
		res = -ENOMEM;
		goto out5;
	}

	data->chip.priv = data;
	data->mtd.priv = &data->chip;
	data->mtd.owner = THIS_MODULE;

	/* Assign more sensible name (default is string from nand_ids.c!) */
	data->mtd.name = name;

	tm = bank->timing_data;

	data->chip.IO_ADDR_R = data->io_base;
	data->chip.IO_ADDR_W = data->io_base;
	data->chip.chip_delay = tm->chip_delay;
	data->chip.cmd_ctrl = nand_cmd_ctrl_emi;

	/* Do we have access to NAND_RBn? */
	if (gpio_is_valid(rbn_gpio)) {
		data->rbn_gpio = rbn_gpio;
		data->chip.dev_ready = nand_device_ready;
	} else {
		data->rbn_gpio = -1;
		if (data->chip.chip_delay == 0)
			data->chip.chip_delay = 30;
	}

	/* Set IO routines for acessing NAND pages */
#if defined(CONFIG_STM_NAND_EMI_FDMA)
	data->chip.read_buf = nand_read_buf_dma;
	data->chip.write_buf = nand_write_buf_dma;
	data->dma_chan = NULL;
	data->init_fdma_jiffies = 0;
	init_fdma_nand_ratelimit(data);
	data->nand_phys_addr = data->emi_base;

#elif defined(CONFIG_STM_NAND_EMI_LONGSL)
	data->chip.read_buf = nand_readsl_buf;
	data->chip.write_buf = nand_writesl_buf;

#elif defined(CONFIG_STM_NAND_EMI_CACHED)
	data->chip.read_buf = nand_read_buf_cached;
	data->chip.write_buf = nand_writesl_buf;

#elif defined(CONFIG_STM_NAND_EMI_BYTE)
	/* Default byte orientated routines */
#else
#error "Must specify CONFIG_STM_NAND_EMI_xxxx mode"
#endif

	data->chip.ecc.mode = NAND_ECC_SOFT;

	/* Copy chip options from platform data */
	data->chip.options = bank->options;
	data->chip.bbt_options = bank->bbt_options;

	/* Scan to find existance of the device */
	if (nand_scan(&data->mtd, 1)) {
		printk(KERN_ERR NAME ": nand_scan failed\n");
		res = -ENXIO;
		goto out6;
	}

	res = mtd_device_parse_register(&data->mtd,
			part_probes, 0,
			bank->partitions, bank->nr_partitions);
	if (!res)
		return data;

	/* Release resources on error */
 out6:

	nand_release(&data->mtd);
	iounmap(data->io_addr);
 out5:
	iounmap(data->io_cmd);
 out4:
#ifdef CONFIG_STM_NAND_EMI_CACHED
	iounmap(data->io_data);
 out3:
#endif
	iounmap(data->io_base);
 out2:
	release_mem_region(data->emi_base, data->emi_size);
 out1:
	kfree(data);
	return ERR_PTR(res);
}

static int __devinit stm_nand_emi_probe(struct platform_device *pdev)
{
	struct stm_plat_nand_emi_data *pdata = pdev->dev.platform_data;
	int res;
	int n;
	int rbn_gpio;
	struct stm_nand_emi_group *group;
	struct stm_nand_bank_data *bank;

	group = kzalloc(sizeof(struct stm_nand_emi_group) +
			(sizeof(struct stm_nand_emi *) * pdata->nr_banks),
			GFP_KERNEL);
	if (!group)
		return -ENOMEM;

	rbn_gpio = pdata->emi_rbn_gpio;
	if (gpio_is_valid(rbn_gpio)) {
		res = gpio_request(rbn_gpio, "nand_RBn");
		if (res == 0) {
			gpio_direction_input(rbn_gpio);
		} else {
			dev_err(&pdev->dev, "nand_rbn unavailable. "
				"Falling back to chip_delay\n");
			rbn_gpio = -1;
		}
	}

	group->rbn_gpio = rbn_gpio;
	group->nr_banks = pdata->nr_banks;

	bank = pdata->banks;
	for (n=0; n<pdata->nr_banks; n++) {
		group->banks[n] = nand_probe_bank(bank, rbn_gpio,
						  dev_name(&pdev->dev));
		bank++;
	}

	platform_set_drvdata(pdev, group);

	return 0;
}

/*
 * Remove a NAND device.
 */
static int __devexit stm_nand_emi_remove(struct platform_device *pdev)
{
	struct stm_nand_emi_group *group = platform_get_drvdata(pdev);
	struct stm_plat_nand_emi_data *pdata = pdev->dev.platform_data;
	int n;

	for (n=0; n<group->nr_banks; n++) {
		struct stm_nand_emi *data = group->banks[n];

		nand_release(&data->mtd);

		iounmap(data->io_addr);
		iounmap(data->io_cmd);
#ifdef CONFIG_STM_NAND_EMI_CACHED
		iounmap(data->io_data);
#endif
		iounmap(data->io_base);
		release_mem_region(data->emi_base, data->emi_size);
#ifdef CONFIG_STM_NAND_EMI_FDMA
		exit_fdma_nand(data);
#endif
		kfree(data);
	}

	if (gpio_is_valid(group->rbn_gpio))
		gpio_free(group->rbn_gpio);

	platform_set_drvdata(pdev, NULL);
	kfree(group);

	return 0;
}

static struct platform_driver plat_nand_driver = {
	.probe		= stm_nand_emi_probe,
	.remove		= stm_nand_emi_remove,
	.driver		= {
		.name	= NAME,
		.owner	= THIS_MODULE,
	},
};

static int __init stm_nand_emi_init(void)
{
	return platform_driver_register(&plat_nand_driver);
}

static void __exit stm_nand_emi_exit(void)
{
	platform_driver_unregister(&plat_nand_driver);
}

module_init(stm_nand_emi_init);
module_exit(stm_nand_emi_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Angus Clark");
MODULE_DESCRIPTION("STMicroelectronics NAND driver: 'EMI bit-banging'");
