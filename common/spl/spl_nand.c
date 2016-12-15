/*
 * Copyright (C) 2011
 * Corscience GmbH & Co. KG - Simon Schwarz <schwarz@corscience.de>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <config.h>
#include <spl.h>
#include <asm/io.h>
#include <nand.h>

#if defined(CONFIG_SPL_NAND_RAW_ONLY)
void spl_nand_load_image(void)
{
	nand_init();

	nand_spl_load_image(CONFIG_SYS_NAND_U_BOOT_OFFS,
			    CONFIG_SYS_NAND_U_BOOT_SIZE,
			    (void *)CONFIG_SYS_NAND_U_BOOT_DST);
	spl_set_header_raw_uboot();
	nand_deselect();
}
#else
#include <packimg.h>
#include <fdt_support.h>
#include <asm/smp.h>

static uint32_t calc_crc(void *buff, int size)
{
	int i;
	uint32_t ret = 0;
	for (i = 0; i < size; i += 4)
		ret += *(uint32_t *)(buff + i);
	return ret;
}

int nand_spl_load_packimg( uint32_t nand_off)
{
	int i, err, do_smp_boot = 0;
	size_t size;
	char *buff = (void *)(CONFIG_SYS_TEXT_BASE - 0x1000);
	struct pack_header *ph;
	struct pack_entry *pe;
	uint32_t offs = nand_off, crc;
	void *fdt;
		
	size = 512;
	err = nand_spl_load_image(nand_off, size, (void *)buff);
	if (err) {
		printf("nand read offset %x fail\n", offs);
		return err;
	}
	ph = (void *)buff;
	pe = (void *)buff + sizeof(*ph);

	// check valid header
	if (ph->magic != PACK_MAGIC)
		goto next_block;
	crc = calc_crc(pe, ph->nentry * sizeof(*pe));
	if (ph->crc != crc)
		goto next_block;
		
#ifdef CONFIG_SPL_SMP_BOOT
	if (imx_get_boot_arg() == SMP_BOOT_DONE_SIGNATURE) {
		imx_set_boot_arg(SMP_START_LOAD_INITRD_SIGNATURE);
		do_smp_boot = 1;
	}
	else
#endif
	{
		// load all entries
		for (i = 0; i < ph->nentry; i++) {
			printf("spl nand load packimg now at entry 0x%x , size 0x%x, ldaddr 0x%x\n", i,  pe[i].size, pe[i].ldaddr);
			size = pe[i].size;
			err = nand_spl_load_image(offs + pe[i].offset, size, (void *)pe[i].ldaddr);
			if (err) {
				printf("nand read offset %x size %x to %x fail\n",
					   offs + pe[i].offset, pe[i].size, pe[i].ldaddr);
				return err;
			}
			
			//aes_dec(pe[i].ldaddr, pe[i].size);

			#if 0
			crc = calc_crc((void *)pe[i].ldaddr, pe[i].size);
			if (pe[i].crc != crc)
				goto next_block;
			#endif
		}
	}

	fdt = (void *)CONFIG_SYS_SPL_ARGS_ADDR;
	fdt_open_into(fdt, fdt, fdt_totalsize(fdt) + 0x10000);
	fdt_fixup_memory(fdt, CONFIG_SYS_SDRAM_BASE, PHYS_SDRAM_SIZE);

	fdt_initrd(fdt, pe->ldaddr, pe->ldaddr + pe->size);
	fdt_pack(fdt);


	if (do_smp_boot) {
		ulong spl_start = CONFIG_SPL_RANGE_BEGIN;
		ulong spl_end = CONFIG_SPL_RANGE_END;

		err = fdt_add_mem_rsv(fdt, CONFIG_SPL_RANGE_BEGIN, CONFIG_SPL_RANGE_END - CONFIG_SPL_RANGE_BEGIN);
		if (err < 0)
			printf("fdt reserve %x - %x fail\n", CONFIG_SPL_RANGE_BEGIN, CONFIG_SPL_RANGE_END);

		err = fdt_set_chosen(fdt, " maxcpus=1", &spl_start, &spl_end);
		if (err < 0)
			printf("fdt change boot cpu number fail\n");
	}	

	printf("load packimg at %x success\n", offs);
	return pe->size;
	//return 0;

next_block:
	printf("invalid packimg at offset %x\n", offs);

	printf("load packimg from %x  fail\n", nand_off);
	return -1;
}

void spl_nand_load_image(void)
{
	struct image_header *header;
	int *src __attribute__((unused));
	int *dst __attribute__((unused));

	debug("spl: nand - using hw ecc\n");

	nand_init();

	/*use CONFIG_SYS_TEXT_BASE as temporary storage area */
	//header = (struct image_header *)(CONFIG_SYS_TEXT_BASE);
	header = (struct image_header *)(CONFIG_SYS_TEXT_BASE - 0x1000);
#ifdef CONFIG_SPL_OS_BOOT
	if (!spl_start_uboot()) {
		/*
		 * load parameter image
		 * load to temp position since nand_spl_load_image reads
		 * a whole block which is typically larger than
		 * CONFIG_CMD_SPL_WRITE_SIZE therefore may overwrite
		 * following sections like BSS
		 */
		#if 0
		nand_spl_load_image(CONFIG_CMD_SPL_NAND_OFS,
			CONFIG_CMD_SPL_WRITE_SIZE,
			(void *)CONFIG_SYS_TEXT_BASE);
		/* copy to destintion */
		for (dst = (int *)CONFIG_SYS_SPL_ARGS_ADDR,
				src = (int *)CONFIG_SYS_TEXT_BASE;
				src < (int *)(CONFIG_SYS_TEXT_BASE +
				CONFIG_CMD_SPL_WRITE_SIZE);
				src++, dst++) {
			writel(readl(src), dst);
		}
		#else
		nand_spl_load_image(CONFIG_CMD_SPL_NAND_DTB_OFS,
			CONFIG_CMD_SPL_DTB_WRITE_SIZE, (void *)CONFIG_SYS_SPL_ARGS_ADDR);
		#endif
		/* load linux */
		nand_spl_load_image(CONFIG_SYS_NAND_SPL_KERNEL_OFFS,
			sizeof(*header), (void *)header);

		spl_parse_image_header(header);
		if (header->ih_os == IH_OS_LINUX) {
			/* happy - was a linux */
			nand_spl_load_image(CONFIG_SYS_NAND_SPL_KERNEL_OFFS,
				spl_image.size, (void *)spl_image.load_addr);
			nand_spl_load_packimg(CONFIG_INITRAMFS_ADDR);
			nand_deselect();
			return;
		} else {
			puts("The Expected Linux image was not "
				"found. Please check your NAND "
				"configuration.\n");
			puts("Trying to start u-boot now...\n");
		}
	}
#endif
#ifdef CONFIG_NAND_ENV_DST
	nand_spl_load_image(CONFIG_ENV_OFFSET,
		sizeof(*header), (void *)header);
	spl_parse_image_header(header);
	nand_spl_load_image(CONFIG_ENV_OFFSET, spl_image.size,
		(void *)spl_image.load_addr);
#ifdef CONFIG_ENV_OFFSET_REDUND
	nand_spl_load_image(CONFIG_ENV_OFFSET_REDUND,
		sizeof(*header), (void *)header);
	spl_parse_image_header(header);
	nand_spl_load_image(CONFIG_ENV_OFFSET_REDUND, spl_image.size,
		(void *)spl_image.load_addr);
#endif
#endif
	/* Load u-boot */
	nand_spl_load_image(CONFIG_SYS_NAND_U_BOOT_OFFS,
		sizeof(*header), (void *)header);
	spl_parse_image_header(header);
	nand_spl_load_image(CONFIG_SYS_NAND_U_BOOT_OFFS,
		spl_image.size, (void *)spl_image.load_addr);
	nand_deselect();
}
#endif
