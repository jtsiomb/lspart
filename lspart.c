#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include "ptype.h"

#define PATTR_ACT_BIT	(1 << 9)
#define PATTR_PRIM_BIT	(1 << 10)

#define PTYPE(attr)		((attr) & 0xff)
#define IS_ACT(attr)	((attr) & PATTR_ACT_BIT)
#define IS_PRIM(attr)	((attr) & PATTR_PRIM_BIT)

struct partition {
	uint32_t start_sect;
	size_t size_sect;

	unsigned int attr;

	struct partition *next;
};

struct part_record {
	uint8_t stat;
	uint8_t first_head, first_cyl, first_sect;
	uint8_t type;
	uint8_t last_head, last_cyl, last_sect;
	uint32_t first_lba;
	uint32_t nsect_lba;
} __attribute__((packed));

static void print_parlist(struct partition *plist);

static struct partition *load_parlist(void);
static uint16_t bootsig(const char *sect);
static int read_sector(void *buf, uint32_t sector);
static const char *printsz(unsigned int sz);
static const char *ptype_name(int type);

int bdev;

int main(int argc, char **argv)
{
	struct partition *plist;

	if(argc != 2) {
		fprintf(stderr, "usage: %s <block device>\n", argv[0]);
		return 1;
	}

	if((bdev = open(argv[1], O_RDONLY)) == -1) {
		fprintf(stderr, "failed to open %s: %s\n", argv[1], strerror(errno));
		return 1;
	}

	if((plist = load_parlist())) {
		printf("partitions:\n");
		print_parlist(plist);
	}

	return 0;
}

static void print_parlist(struct partition *plist)
{
	int idx = 0;

	while(plist) {
		printf("%d%c ", idx++, IS_ACT(plist->attr) ? '*' : ' ');
		printf("(%s) %-20s ", IS_PRIM(plist->attr) ? "pri" : "log", ptype_name(PTYPE(plist->attr)));
		printf("start: %-10lu ", (unsigned long)plist->start_sect);
		printf("size: %-10lu [%s]\n", (unsigned long)plist->size_sect, printsz(plist->size_sect));
		plist = plist->next;
	}
}


#define BOOTSIG_OFFS	510
#define PTABLE_OFFS		0x1be

#define BOOTSIG			0xaa55

#define IS_MBR			(sidx == 0)
#define IS_FIRST_EBR	(!IS_MBR && (first_ebr_offs == 0))
static struct partition *load_parlist(void)
{
	char *sect;
	struct partition *phead = 0, *ptail = 0;
	uint32_t sidx = 0;
	uint32_t first_ebr_offs = 0;
	int i, num_bootrec = 0;

	if(!(sect = malloc(512))) {
		perror("failed to allocate sector buffer");
		return 0;
	}


	do {
		int num_rec;
		struct part_record *prec;

		if(IS_FIRST_EBR) {
			first_ebr_offs = sidx;
		}

		if(read_sector(sect, sidx) == -1) {
			goto err;
		}
		if(bootsig(sect) != BOOTSIG) {
			fprintf(stderr, "invalid/corrupted partition table, sector %lu has no magic\n", (unsigned long)sidx);
			goto err;
		}
		prec = (struct part_record*)(sect + PTABLE_OFFS);

		/* MBR has 4 records, EBRs have 2 */
		num_rec = IS_MBR ? 4 : 2;

		for(i=0; i<num_rec; i++) {
			struct partition *pnode;

			/* ignore empty partitions in the MBR, stop if encountered in an EBR */
			if(prec[i].type == 0) {
				if(num_bootrec > 0) {
					sidx = 0;
					break;
				}
				continue;
			}

			/* ignore extended partitions and setup sector index to read
			 * the next logical partition afterwards.
			 */
			if(prec[i].type == PTYPE_EXT || prec[i].type == PTYPE_EXT_LBA) {
				/* all EBR start fields are relative to the first EBR offset */
				sidx = first_ebr_offs + prec[i].first_lba;
				continue;
			}

			if(!(pnode = malloc(sizeof *pnode))) {
				perror("failed to allocate partition list node");
				goto err;
			}

			pnode->attr = prec[i].type;

			if(prec[i].stat & 0x80) {
				pnode->attr |= PATTR_ACT_BIT;
			}
			if(IS_MBR) {
				pnode->attr |= PATTR_PRIM_BIT;
			}
			pnode->start_sect = prec[i].first_lba + first_ebr_offs;
			pnode->size_sect = prec[i].nsect_lba;
			pnode->next = 0;

			/* append to the list */
			if(!phead) {
				phead = ptail = pnode;
			} else {
				ptail->next = pnode;
				ptail = pnode;
			}
		}

		num_bootrec++;
	} while(sidx > 0);

	free(sect);
	return phead;

err:
	free(sect);
	while(phead) {
		void *tmp = phead;
		phead = phead->next;
		free(tmp);
	}
	return 0;
}

static uint16_t bootsig(const char *sect)
{
	return *(uint16_t*)(sect + BOOTSIG_OFFS);
}

static int read_sector(void *buf, uint32_t sector)
{
	if(lseek(bdev, (off_t)sector * 512, SEEK_SET) == -1) {
		fprintf(stderr, "read_sector: failed to seek: %s\n", strerror(errno));
		return -1;
	}
	if(read(bdev, buf, 512) != 512) {
		fprintf(stderr, "failed to read sector %lu: %s\n", (unsigned long)sector, strerror(errno));
		return -1;
	}
	return 0;
}

const char *printsz(unsigned int sz)
{
	int i = 0;
	const char *suffix[] = { "kb", "mb", "gb", "tb", "pb", 0 };
	static char buf[512];

	while(sz > 1024 && suffix[i + 1]) {
		sz /= 1024;
		i++;
	}

	sprintf(buf, "%u %s", sz, suffix[i]);
	return buf;
}

static const char *ptype_name(int type)
{
	int i;

	for(i=0; i<PTYPES_SIZE; i++) {
		if(partypes[i].type == type) {
			return partypes[i].name;
		}
	}
	return "unknown";
}
