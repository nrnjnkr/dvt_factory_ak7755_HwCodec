/*
 * Copyright (C) 2007 Nokia Corporation.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 */

/*
 * An utility to attach MTD devices to UBI.
 *
 * Author: Artem Bityutskiy
 */
#define PROGRAM_NAME    "ubiattach"

#include <stdio.h>
#include <stdint.h>
#include <getopt.h>
#include <stdlib.h>
#include <string.h>

#include <libubi.h>
#include "common.h"
#include "ubiutils-common.h"

#include "bpi_ubi.h"

#define DEFAULT_CTRL_DEV "/dev/ubi_ctrl"

/* The variables below are set by command line arguments */
struct args {
	int devn;
	int mtdn;
	int vidoffs;
	const char *node;
	const char *dev;
	int max_beb_per1024;
};

static struct args args = {
	.devn = UBI_DEV_NUM_AUTO,
	.mtdn = -1,
	.vidoffs = 0,
	.node = DEFAULT_CTRL_DEV,
	.dev = NULL,
	.max_beb_per1024 = 0,
};

int bpi_ubi_attach(int devn, int mtdn)
{
    int err;
    libubi_t libubi;
    struct ubi_info ubi_info;
    struct ubi_dev_info dev_info;
    struct ubi_attach_request req;

    libubi = libubi_open();
    if (!libubi) {
    	if (errno == 0){
    		return errmsg("UBI is not present in the system");
      }
    	return sys_errmsg("cannot open libubi");
    }

    /*
     * Make sure the kernel is fresh enough and this feature is supported.
     */
    err = ubi_get_info(libubi, &ubi_info);
    if (err) {
    	sys_errmsg("cannot get UBI information");
    	goto out_libubi;
    }

    if (ubi_info.ctrl_major == -1) {
    	errmsg("MTD attach/detach feature is not supported by your kernel");
    	goto out_libubi;
    }

    req.dev_num = devn;
    req.mtd_num = mtdn;
    req.vid_hdr_offset = args.vidoffs;
    req.mtd_dev_node = args.dev;
    req.max_beb_per1024 = args.max_beb_per1024;

    err = ubi_attach(libubi, args.node, &req);
    if (err < 0) {
    	if (args.dev)
    		sys_errmsg("cannot attach \"%s\"", args.dev);
    	else
    		sys_errmsg("cannot attach mtd%d", args.mtdn);
    	goto out_libubi;
    } else if (err == 1) {
    	/* The kernel did not support the 'max_beb_per1024' parameter */
    	warnmsg("the --max-beb-per1024=%d parameter was ignored", args.max_beb_per1024);
    	normsg("the UBI kernel driver does not support does not allow changing the reserved PEBs count");
    	normsg("the support was added in kernel version 3.7, probably you are running older kernel?");
    	goto out_libubi;
    }

    /* Print some information about the new UBI device */
    err = ubi_get_dev_info1(libubi, req.dev_num, &dev_info);
    if (err) {
    	sys_errmsg("cannot get information about newly created UBI device");
    	goto out_libubi;
    }

    printf("UBI device number %d, total %d LEBs (", dev_info.dev_num, dev_info.total_lebs);
    ubiutils_print_bytes(dev_info.total_bytes, 0);
    printf("), available %d LEBs (", dev_info.avail_lebs);
    ubiutils_print_bytes(dev_info.avail_bytes, 0);
    printf("), LEB size ");
    ubiutils_print_bytes(dev_info.leb_size, 1);
    printf("\n");

    libubi_close(libubi);
    return 0;

out_libubi:
    libubi_close(libubi);
    return -1;

}

int bpi_ubi_detach(int devn, int mtdn){
    int err;
    libubi_t libubi;
    struct ubi_info ubi_info;

    libubi = libubi_open();
    if (!libubi) {
    	if (errno == 0){
    		return errmsg("UBI is not present in the system");
      }
    	return sys_errmsg("cannot open libubi");
    }

    /*
     * Make sure the kernel is fresh enough and this feature is supported.
     */
    err = ubi_get_info(libubi, &ubi_info);
    if (err) {
    	sys_errmsg("cannot get UBI information");
    	goto out_libubi;
    }

    if (ubi_info.ctrl_major == -1) {
    	errmsg("MTD detach/detach feature is not supported by your kernel");
    	goto out_libubi;
    }

    args.devn = devn;
    args.mtdn = mtdn;
    if (args.devn != -1) {
    	err = ubi_remove_dev(libubi, args.node, args.devn);
    	if (err) {
    		sys_errmsg("cannot remove ubi%d", args.devn);
    		goto out_libubi;
    	}
    } else {
    	if (args.dev != NULL) {
    		err = ubi_detach(libubi, args.node, args.dev);
    		if (err) {
    			sys_errmsg("cannot detach \"%s\"", args.dev);
    			goto out_libubi;
    		}
    	} else {
    		err = ubi_detach_mtd(libubi, args.node, args.mtdn);
    		if (err) {
    			sys_errmsg("cannot detach mtd%d", args.mtdn);
    			goto out_libubi;
    		}
    	}
    }

    libubi_close(libubi);
    return 0;

out_libubi:
    libubi_close(libubi);
    return -1;
}