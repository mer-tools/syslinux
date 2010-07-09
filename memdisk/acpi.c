/* ----------------------------------------------------------------------- *
 *
 *   Copyright 2010 Intel Corporation; author: H. Peter Anvin
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, Inc., 53 Temple Place Ste 330,
 *   Boston MA 02111-1307, USA; either version 2 of the License, or
 *   (at your option) any later version; incorporated herein by reference.
 *
 * ----------------------------------------------------------------------- */

#include <stdint.h>
#include "compiler.h"
#include "bda.h"
#include "dskprobe.h"
#include "e820.h"
#include "conio.h"
#include "version.h"
#include "memdisk.h"
#include "acpi.h"
#include "../version.h"

/*
 * acpi.c
 *
 * Routines to splice in code into the DSDT.
 *
 * WARNING: This doesn't currently even attempt to find and patch the
 * XSDT, even if both the XSDT and the DSDT pointed to by the XSDT are
 * both below 4 GiB.
 */

struct acpi_rsdp {
    uint8_t  magic[8];		/* "RSD PTR " */
    uint8_t  csum;
    char     oemid[6];
    uint8_t  rev;
    uint32_t rsdt_addr;
    uint32_t len;
    uint64_t xsdt_addr;
    uint8_t  xcsum;
    uint8_t  rsvd[3];
};

struct acpi_rsdt {
    struct acpi_description_header hdr;
    struct acpi_description_header *entry[0];
};

static struct acpi_rsdt *rsdt;
static struct acpi_description_header **dsdt_ptr;

static uint8_t checksum_range(const void *start, uint32_t size)
{
    const uint8_t *p = start;
    uint8_t csum = 0;

    while (size--)
	csum += *p++;

    return csum;
}

static void checksum_table(struct acpi_description_header *hdr)
{
    hdr->checksum -= checksum_range(hdr, hdr->length);
}

enum tbl_errs {
    ERR_NONE,			/* No errors */
    ERR_CSUM,			/* Invalid checksum */
    ERR_SIZE,			/* Impossibly large table */
    ERR_NOSIG			/* No signature */
};

static enum tbl_errs is_valid_table(const void *ptr)
{
    const struct acpi_description_header *hdr = ptr;

    if (hdr->signature[0] == 0)
	return ERR_NOSIG;

    if (hdr->length < 10 || hdr->length > (1 << 20)) {
	/* Either insane or too large to dump */
	return ERR_SIZE;
    }

    return checksum_range(hdr, hdr->length) == 0 ? ERR_NONE : ERR_CSUM;
}


static struct acpi_rsdp *scan_for_rsdp(uint32_t base, uint32_t end)
{
    for (base &= ~15; base < end-20; base += 16) {
	struct acpi_rsdp *rsdp = (struct acpi_rsdp *)base;

	if (memcmp(rsdp->magic, "RSD PTR ", 8))
	    continue;

	if (checksum_range(rsdp, 20))
	    continue;

	if (rsdp->rev > 0) {
	    if (base + rsdp->len >= end || checksum_range(rsdp, rsdp->len))
		continue;
	}

	return rsdp;
    }

    return NULL;
}

static struct acpi_rsdp *find_rsdp(void)
{
    uint32_t ebda;
    struct acpi_rsdp *rsdp;

    ebda = (*(uint16_t *)0x40e) << 4;
    if (ebda >= 0x10000 && ebda < 0xa0000) {
	rsdp = scan_for_rsdp(ebda, ebda+1024);

	if (rsdp)
	    return rsdp;
    }

    return scan_for_rsdp(0xe0000, 0x100000);
}

/* Generate a unique ID from the mBFT segment address */
/* This is basically hexadecimal with [A-P] instead of [0-9A-F] */
static void gen_id(uint8_t *p, const void *mbft)
{
    uint16_t mseg = (size_t)mbft >> 4;

    p[0] = ((mseg >> 12) & 15) + 'A';
    p[1] = ((mseg >>  8) & 15) + 'A';
    p[2] = ((mseg >>  4) & 15) + 'A';
    p[3] = ((mseg >>  0) & 15) + 'A';
}

static const uint8_t aml_template[] =
{
    0x10, 0x4c, 0x04, '_', 'S', 'B', '_', 0x5b, 0x82, 0x44,
    0x04, 'M', 'D', 'S', 'K', 0x08, '_', 'H', 'I', 'D',
    0x0c, 0x41, 0xd0, 0x0a, 0x05, 0x5b, 0x82, 0x32,
    'A', 'A', 'A', 'A',    /* Unique identifier for this memdisk device */
    0x08, '_', 'H', 'I', 'D', 0x0c, 0x22, 0x01, 0x00, 0x01,
    0x08, '_', 'C', 'R', 'S', 0x11, 0x1d, 0x0a, 0x1a, 0x86,
    0x09, 0x00, 0x01,
    0xaa, 0xaa, 0xaa, 0xaa,	/* Memory range 1 base */
    0xbb, 0xbb, 0xbb, 0xbb,	/* Memory range 1 size */
    0x86, 0x09, 0x00, 0x01,
    0xcc, 0xcc, 0xcc, 0xcc,	/* Memory range 2 base */
    0xdd, 0xdd, 0xdd, 0xdd,	/* Memory range 2 size */
    0x79, 0x00
};

/*
 * Returns the number of bytes we need to reserve for the replacement DSDT,
 * or 0 on failure
 */
size_t acpi_bytes_needed(void)
{
    struct acpi_rsdp *rsdp;
    struct acpi_description_header *dsdt;
    int i, n;

    rsdp = find_rsdp();
    if (!rsdp) {
	printf("ACPI: unable to locate an ACPI RSDP\n");
	return 0;
    }

    rsdt = (struct acpi_rsdt *)rsdp->rsdt_addr;
    if (is_valid_table(rsdt) != ERR_NONE) {
	printf("ACPI: unable to locate an ACPI RSDT\n");
	return 0;
    }

    n = (rsdt->hdr.length - sizeof(rsdt->hdr))/sizeof(rsdt->entry[0]);
    for (i = 0; i < n; i++) {
	enum tbl_errs err;
	dsdt = rsdt->entry[i];
	err = is_valid_table(dsdt);
	if (memcmp(dsdt->signature, "DSDT", 4) &&
	    memcmp(dsdt->signature, "SSDT", 4))
	    continue;
	if (err != ERR_NONE)
	    continue;
	dsdt_ptr = &rsdt->entry[i];
	printf("ACPI: found %4.4s at %p, size %u\n",
	       dsdt->signature, dsdt, dsdt->length);
	return dsdt->length + sizeof aml_template;
    }

    printf("ACPI: unable to locate an ACPI DSDT or SSDT\n");
    return 0;
}

void acpi_hack_dsdt(void *membuf, const void *mbft, uint32_t mbft_size,
		    const void *data, uint32_t data_size)
{
    struct acpi_description_header *dsdt = *dsdt_ptr;
    struct acpi_description_header *new_dsdt = membuf;
    uint8_t *p = membuf;
    uint8_t *newcode;

    if (!dsdt_ptr)
	return;			/* No valid DSDT pointer */

    p = mempcpy(p, dsdt, sizeof *dsdt); /* Copy header */
    p = mempcpy(newcode = p, aml_template, sizeof aml_template);
    memcpy(p, dsdt+1, dsdt->length - sizeof *dsdt); /* Copy body */

    /* Generate a unique ID from the mBFT segment address */
    gen_id(newcode+0x1c, mbft);

    /* Patch in the resources */
    *((uint32_t *)(newcode+0x37)) = (size_t)data;
    *((uint32_t *)(newcode+0x3b)) = data_size;
    *((uint32_t *)(newcode+0x43)) = (size_t)mbft;
    *((uint32_t *)(newcode+0x47)) = mbft_size;

    /* Checksum the new DSDT */
    new_dsdt->length += sizeof aml_template;
    checksum_table(new_dsdt);

    /* Patch the RDST */
    *dsdt_ptr = new_dsdt;
    checksum_table(&rsdt->hdr);
}
