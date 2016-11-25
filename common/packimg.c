//#define DEBUG
#include <common.h>
#include <packimg.h>
#include <aes-packimg.h>

static uint32_t calc_crc(void *buff, int size)
{
	int i;
	uint32_t ret = 0;
	for (i = 0; i < size; i += 4)
		ret += *(uint32_t *)(buff + i);
	return ret;
}

#define MAX_PACK_ENTRY	8

#if defined(CONFIG_SPL_SPI_SUPPORT)
#include <spi_flash.h>
/*
	failed: return -1
	success: return 0, name==NULL or can't find
			return <name>@address
*/
int sf_load_packimg(struct spi_flash *flash, uint32_t offs, char *name)
{
	struct pack_header *ph;
	struct pack_entry *pe;
	uint8_t buf[sizeof(*ph)+sizeof(*pe)*MAX_PACK_ENTRY];
	uint32_t crc;
	int ret, i;

	ret = spi_flash_read(flash, offs, sizeof(buf), buf);
	if(ret<0){
		printf("%s: load head failed\n", __FUNCTION__);
		return ret;
	}
	ph = (struct pack_header *)buf;
	pe = (struct pack_entry *)(ph+1);

	// check valid header
	if (ph->magic != PACK_MAGIC){
		printf("load packimg from 0x%x fail\n", offs);
		return -1;
	}
	offs+=ret;
	crc = calc_crc(pe, ph->nentry * sizeof(*pe));
	if (ph->crc != crc){
		printf("packimg head crc error 0x%x should be 0x%x\n", ph->crc, crc);
		return -1;
	}
	if(ph->nentry>MAX_PACK_ENTRY){
		printf("Warning packimg entry %d bigger than %d\n", ph->nentry, MAX_PACK_ENTRY);
		ph->nentry = MAX_PACK_ENTRY;
	}

	// load all entries
	for (i = 0; i < ph->nentry; i++){
		if(name && strcmp(name, pe[i].name)==0) //find target
			ret = pe[i].ldaddr;
		
		debug("load %s@0x%x to ram 0x%x\n", pe[i].name, offs+pe[i].offset, pe[i].ldaddr);
		spi_flash_read(flash, offs+pe[i].offset, pe[i].size, (void *)pe[i].ldaddr);
		crc = calc_crc((void *)pe[i].ldaddr, pe[i].size);
		if (pe[i].crc != crc){
			printf("packimg data crc error 0x%x should be 0x%x\n", pe[i].crc, crc);
			return -1;
		}
	}
	debug("load packimg success\n");
	return ret;
}
#endif //#if defined(CONFIG_SPL_SPI_SUPPORT)

#if (!defined(CONFIG_SPL_BUILD) && defined(CONFIG_CMD_MMC_PACKIMG)) || defined(CONFIG_SPL_MMC_SUPPORT)

#include <mmc.h>
#define ROUND_UP(n,log2)	(((n) + (1<<(log2)) - 1) >> (log2))

struct pack_header *mmc_get_packimg_header(void)
{
	return (struct pack_header *)(CONFIG_SYS_TEXT_BASE - 0x1000);
}

struct pack_entry *mmc_get_packimg_entry_by_name(const char *name)
{
	struct pack_header *ph = mmc_get_packimg_header();
	struct pack_entry *pe = (struct pack_entry *)(ph + 1);
	int i;

	for (i = 0; i < ph->nentry; i++)
		if (strcmp(name, pe[i].name) == 0)
			return pe + i;

	return NULL;
}

struct pack_entry *mmc_get_packimg_entry_by_index(int index)
{
	struct pack_header *ph = mmc_get_packimg_header();
	struct pack_entry *pe = (struct pack_entry *)(ph + 1);

	if (index > 0 && index < ph->nentry)
		return pe + index;

	return NULL;
}

int mmc_load_packimg_header(struct mmc *mmc, uint32_t offs_sector)
{
	struct pack_header *ph = mmc_get_packimg_header();
	struct pack_entry *pe = (struct pack_entry *)(ph + 1);
	uint32_t crc, nblk;
	int ret;

	nblk = ROUND_UP(sizeof(*pe) + sizeof(*ph) * MAX_PACK_ENTRY, 
					mmc->block_dev.log2blksz);

	ret = mmc->block_dev.block_read(mmc->block_dev.dev, offs_sector, nblk, (char *)ph);
	if (ret < 0) {
		printf("%s: load head failed\n", __FUNCTION__);
		return ret;
	}

	aes_dec(ph, nblk<<mmc->block_dev.log2blksz);

	// check valid header
	if (ph->magic != PACK_MAGIC) {
		printf("mmc load packimg from sector 0x%x fail\n", offs_sector);
		return -1;
	}

	crc = calc_crc(pe, ph->nentry * sizeof(*pe));
	if (ph->crc != crc) {
		printf("packimg head crc error 0x%x should be 0x%x\n", ph->crc, crc);
		return -1;
	}

	if (ph->nentry > MAX_PACK_ENTRY) {
		printf("Warning packimg entry %d bigger than %d\n", ph->nentry, MAX_PACK_ENTRY);
		ph->nentry = MAX_PACK_ENTRY;
	}

	return 0;
}

int mmc_load_packimg_entry(struct mmc *mmc, uint32_t offs_sector, struct pack_entry *pe)
{
	uint32_t nblk;
	int err;

	debug("load %s@0x%x to ram 0x%x\n", pe->name,
		  offs_sector + (pe->offset >> mmc->block_dev.log2blksz),
		  pe->ldaddr);

	nblk = ROUND_UP(pe->size, mmc->block_dev.log2blksz);

	err = mmc->block_dev.block_read(mmc->block_dev.dev,
		  offs_sector + (pe->offset >> mmc->block_dev.log2blksz),
		  nblk, (void *)pe->ldaddr);
	if (err < 0) {
		printf("load packimg entry fail\n");
		return err;
	}

	aes_dec(pe->ldaddr, nblk<<mmc->block_dev.log2blksz);

#if 0
	crc = calc_crc((void *)pe->ldaddr, pe->size);
	if (pe->crc != crc){
		printf("packimg data crc error 0x%x should be 0x%x\n", pe->crc, crc);
		return -1;
	}
#endif
	return 0;
}

int mmc_load_packimg(struct mmc *mmc, uint32_t offs_sector)
{
	struct pack_header *ph;
	struct pack_entry *pe;
	int err, i;

	init_aes();

	err = mmc_load_packimg_header(mmc, offs_sector);
	if (err < 0)
		return err;

	ph = mmc_get_packimg_header();
	pe = (struct pack_entry *)(ph + 1);

	// load all entries
	for (i = 0; i < ph->nentry; i++) {
		err = mmc_load_packimg_entry(mmc, offs_sector, pe + i);
		if (err < 0)
			return err;
	}

	debug("load packimg success\n");
	return 0;
}
#endif //#if defined(CONFIG_SPL_MMC_SUPPORT)

#if defined(CONFIG_SYS_USE_NAND)
#include <nand.h>
#include <fdt_support.h>
#include <asm/smp.h>

int nand_packimg_read(nand_info_t *nand, uint32_t nand_off, uint32_t nand_size)
{
	int i, err, do_smp_boot = 0;
	size_t size;
	char buff[512];
	struct pack_header *ph;
	struct pack_entry *pe;
	uint32_t offs = nand_off, crc;
	void *fdt;

	if ((nand_off & (nand->erasesize - 1)) || (nand_size & (nand->erasesize - 1))) {
		printf("offset 0x%x and size 0x%x must be block aligned 0x%x, writesize 0x%x\n", nand_off, nand_size, nand->erasesize, nand->writesize);
		return -1;
	}
		
	while (offs < nand_off + nand_size) {
		size = 512;
		err = nand_read_skip_bad(nand, offs, &size, NULL, nand->size, (void *)buff);
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
			printf("load packimg now at entry 0x%x , size 0x%x\n", i,  pe[i].size);
			size = pe[i].size;
			err = nand_read_skip_bad(nand, offs + pe[i].offset, &size, NULL, nand->size, (void *)pe[i].ldaddr);
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

		fdt = (void *)0x18000000;
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
		offs += nand->erasesize;
	}

	printf("load packimg from %x to %x fail\n", nand_off, nand_size);
	return -1;
}

int nand_packimg_write(nand_info_t *nand, uint32_t nand_off, uint32_t nand_size, uint32_t mem_off, 
				  uint32_t mem_size, uint32_t max_copy)
{
	int err;
	int nsize = nand_size, msize, copies = 0;
	uint32_t noffs = nand_off, moffs;

	if ((nand_off & (nand->erasesize - 1)) || (nand_size & (nand->erasesize - 1))) {
		printf("offset %x and size %x must be block aligned\n", nand_off, nand_size);
		return -1;
	}

	while (nsize >= mem_size && copies < max_copy) {
		moffs = mem_off;
		msize = mem_size;

		while (msize > 0) {
			uint32_t size;
			while (nand_block_isbad(nand, noffs)) {
				noffs += nand->erasesize;
				nsize -= nand->erasesize;
				if (nsize < msize)
					goto out;
			}

			err = nand_erase(nand, noffs, nand->erasesize);
			if (err) {
				printf("erase block %x fail\n", noffs);
				goto out;
			}

			size = msize > nand->erasesize ? nand->erasesize : msize;
			err = nand_write(nand, noffs, &size, (void *)moffs);
			if (err) {
				printf("write block %x fail\n", noffs);
				goto out;
			}

			moffs += nand->erasesize;
			msize -= nand->erasesize;
			noffs += nand->erasesize;
			nsize -= nand->erasesize;
		}

		copies++;
	}

out:
	if (copies) {
		printf("success write %d/%d copies of packimg to %x\n", copies, max_copy, nand_off);
		return 0;
	}
	else {
		printf("fail write packimg to %x\n", nand_off);
		return -1;
	}
}
#endif //#if defined(CONFIG_SPL_NAND_SUPPORT)

#if 1
int nand_packimg_read_ext( uint32_t nand_off, uint32_t nand_size)
{	
	nand_info_t *nand;
	int dev = nand_curr_device;

	nand = &nand_info[dev];
	return nand_packimg_read(nand, nand_off, nand_size);
}

static int do_nandr_aes(cmd_tbl_t *cmdtp, int flag, int argc, char *const argv[])
{
	uint32_t src_addr, len;
	uint8_t *src_ptr;

	nand_info_t *nand;
	int dev = nand_curr_device;
	
	if (argc != 3)
		return CMD_RET_USAGE;

	src_addr = simple_strtoul(argv[1], NULL, 16);
	len = simple_strtoul(argv[2], NULL, 16);

	src_ptr = (uint8_t *)src_addr;

	/* First we expand the key. */
	//init_aes();

	spl_start_uboot();
	
	nand = &nand_info[dev];
	if(get_nr_cpus() <= 0)
		nand_packimg_read(nand, src_addr, len);
	
	return 0;
}

/***************************************************/
U_BOOT_CMD(
	nandr_aes, 3, 1, do_nandr_aes,
	"read data form NandFlash with aes",
	"nandr_aes src len - read block of data $len bytes long at address $src form nandflash with aes\n"
);
#endif

