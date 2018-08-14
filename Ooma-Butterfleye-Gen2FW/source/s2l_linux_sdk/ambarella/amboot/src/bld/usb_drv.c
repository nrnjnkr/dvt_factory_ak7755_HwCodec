/**
 * bld/usb_drv.c
 *
 * History:
 *    2005/09/07 - [Arthur Yang] created file
 *
 *    Notes: The data pointer for USB DMA TX/RX should be aligned to 8-bytes
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include <amboot.h>
#include <irq.h>
#include <ambhw/cache.h>
#include <ambhw/usbdc.h>
#include <bldfunc.h>
#include "descriptor.h"
#include "hwusbreg.h"
#include "hwusbcfg.h"

#include "bld_usb_descriptors.c"

#define _DEBUG_

#ifdef _DEBUG_
#define _DEBUG_PRINTK(x,y) 	do { putstr(x); \
			  		if(y) \
						puthex(y); \
			  		putstr("\r\n"); \
				} while (0)
#else
#define _DEBUG_PRINTK(...)
#endif

#define QUEUE_DEPTH		10
#define DEFAULT_PLL_MODE	7

#define DMA_DESC_ADDR(x) 	dma_desc_buf[x]
#define DMA_BUF_ADDR(x) 	dma_data_buf[x]
#define RETRY_COUNT		2
#define FAILED			1
#define SUCCESSED		0
#define TASK_TIMEOUT		10000	/* 3 second */
#define TEST_NUM		100

#define FLAG_LAST_TRANS		0x01
#define FLAG_ADDRESS		0x02
#define FLAG_JUMP_EXEC		0x04
#define FLAG_COMMAND		0x08
#define FLAG_FORCE_FINISH	0x10
#define bulk_process_trylock()		do { \
										if (!bulk_process_lock) \
											bulk_process_lock = 1; \
											} while(0)

#define bulk_process_tryunlock()	do { \
										if (bulk_process_lock) \
											bulk_process_lock = 0; \
											} while(0)

#define bulk_process_is_locked()	(!!bulk_process_lock)

typedef struct _buf_info {
	uintptr_t addr;
	u32 length;
	u32 crc_value;
} buf_info_t;

typedef struct _transfer_info {
	volatile u32	flag;
	volatile u32	task_finished;
	uintptr_t	jump_address;
	u32		total_length;
	buf_info_t 	tx;
	buf_info_t 	rx;
} transfer_info_t;

/* Fixed descriptor address */
extern u8 *uds_bld_descriptors[];
extern u8 uds_bld_configuration1_hs[];
extern u8 uds_bld_configuration1_fs[];

static u32 setup_desc[16] __attribute__ ((aligned(32), section(".bss.noinit")));
static u32 dma_desc_buf[EP_NUM][16] __attribute__ ((aligned(32), section(".bss.noinit")));
static u8 dma_data_buf[EP_NUM][1024] __attribute__ ((aligned(32), section(".bss.noinit")));
static DEV_RSP dev_response __attribute__ ((aligned(32), section(".bss.noinit")));

static u8 twiddle[] ={'\\','|','/','-'};

volatile COMMAND_QUEUE command[QUEUE_DEPTH] __attribute__ ((section(".bss.noinit")));
volatile WAIT_QUEUE response[QUEUE_DEPTH] __attribute__ ((section(".bss.noinit")));
volatile QUEUE_INDEX qindex={0};

transfer_info_t transfer;
static u32 rx_state = COMMAND_PHASE;
static int enable_usb = 0;	/* identify if usb is initialized */
static u32 usb_speed = USB_HS;
static volatile int ep_event[EP_NUM] = {0};
/* FIXME: When USB device is transmitting binary from host, the host may request a
 * configuration description(why?). We need ignore this request. */
static volatile int bulk_process_lock = 0;


/* =========================================================
 * Global Function Call
 * =========================================================*/

void usb_dump(u32 ep, void *data, int size)
{
#ifdef USB_DUMP
	int i;
	char *ctrl = data;
	int *bulk = data;
	int sz = size > 32 ? 32 : size;
	char *epx[4] = {"CTRL_IN", "BULK_IN", "CTRL_OUT", "BULK_OUT"};

	if ((!ctrl[0] && !ctrl[1] && !ctrl[2] && !ctrl[3])
			|| (size == 512))
		return ;

	printf("EP: %s	SZ: %d", epx[ep], size);

	if (ep == CTRL_IN || (ep == CTRL_OUT))
	{
		for (i = 0; i < sz; i++) {

			if (i % 12 == 0)
				printf("\n");
			printf("%02x ", ctrl[i]);
		}
	} else {

		for (i = 0; i < sz / 4; i++) {
			if (i % 4 == 0)
				printf("\n");
			printf("%08x ", bulk[i]);
		}
	}
	printf("\n\n");
#endif
}

static void usb_isr(void *data)
{
	u32 value = 0;

	if(!enable_usb) /* this could be ethernet interrupt on A2, just return */
		return;

	/* 1. check if device interrupt */

	value = readl(USB_DEV_INTR_REG);
	if(value) {
		/* ack the interrupt */
		writel(USB_DEV_INTR_REG, value);

		device_interrupt(value);

	}
	/* 2. check if endpoint interrupt */
	value = readl(USB_DEV_EP_INTR_REG);
	if(value) {

		if(value & USB_DEV_EP_OUT) {

			_clean_flush_d_cache();
			endpoint_out_interrupt(value);
		}
		else if(value & USB_DEV_EP_IN) {

			_clean_flush_d_cache();
			endpoint_in_interrupt(value);

		} else { /* something wrong in endpoint interrupt */


		}
	}
}

/*
 * Name: device_interrupt
 * Description:
 *	Process related device interrupt
 *
 * Input : int_value
 * Return:
 */
void device_interrupt(u32 int_value)
{
	u32 value = 0, serviced_int = 0;

	/* case 1. enumeration complete */
	if(int_value & USB_DEV_ENUM_CMPL) {

		init_udc_reg();

		serviced_int = USB_DEV_ENUM_CMPL;
		value = readl(USB_DEV_STS_REG) & USB_DEV_ENUM_SPD ;

		usb_speed = _SPEED_(value);

		if(usb_speed == USB_HS) {  /* high speed */

//			_DEBUG_PRINTK("enum HS",0);
			/* set BULK_IN size register */
			writel(USB_EP_IN_MAX_PKT_SZ_REG(BULK_IN_ID),
						USB_EP_BULKIN_MAX_PKT_SZ_HI);
			value = readl(USB_UDC_REG(BULK_IN));
			value &= (~USB_UDC_MAX_PKT_SZ);
			value |= USB_UDC_BULKIN_MAX_PKT_SZ_HI;
			writel(USB_UDC_REG(BULK_IN), value);


			/* set BULK_OUT size register */
			writel(USB_EP_OUT_MAX_PKT_SZ_REG(BULK_OUT_ID),
						USB_EP_BULKOUT_MAX_PKT_SZ_HI);
			value = readl(USB_UDC_REG(BULK_OUT));
			value &= (~USB_UDC_MAX_PKT_SZ);
			value |= USB_UDC_BULKIN_MAX_PKT_SZ_HI;
			writel(USB_UDC_REG(BULK_OUT), value);

		} else if (usb_speed == USB_FS) { /* full speed */

//			_DEBUG_PRINTK("enum FS",0);
			/* set BULK_IN size register */
			writel(USB_EP_IN_MAX_PKT_SZ_REG(BULK_IN_ID),
						USB_EP_BULKIN_MAX_PKT_SZ_FU);
			value = readl(USB_UDC_REG(BULK_IN));
			value &= (~USB_UDC_MAX_PKT_SZ);
			value |= USB_UDC_BULKIN_MAX_PKT_SZ_FU;
			writel(USB_UDC_REG(BULK_IN), value);

			/* set BULK_IN size register */
			writel(USB_EP_OUT_MAX_PKT_SZ_REG(BULK_OUT_ID),
						USB_EP_BULKOUT_MAX_PKT_SZ_FU);
			value = readl(USB_UDC_REG(BULK_OUT));
			value &= (~USB_UDC_MAX_PKT_SZ);
			value |= USB_UDC_BULKIN_MAX_PKT_SZ_FU;
			writel(USB_UDC_REG(BULK_OUT), value);

		} else {

		}

	} /* ENUM COMPLETE */
	/* case 2. Get set_config or set_interface request from host */
	if (int_value & (USB_DEV_SET_CFG | USB_DEV_SET_INTF)) {

//		_DEBUG_PRINTK("set config/interface",0);

		if(int_value & USB_DEV_SET_CFG)
			serviced_int |= USB_DEV_SET_CFG;
		if(int_value & USB_DEV_SET_INTF)
			serviced_int |= USB_DEV_SET_INTF;

	  	/* unlock endpoint stall status */
		clrbitsl(USB_EP_IN_CTRL_REG(CTRL_IN_ID), USB_EP_STALL);

		/* told UDC the configuration is done, and to ack HOST */
		value = readl(USB_DEV_CTRL_REG);
		value |= USB_DEV_CSR_DONE;
		writel(USB_DEV_CTRL_REG, value);

		/* The device is full configured, data IN/OUT can be started */
		start_rx(BULK_OUT, DMA_BUF_ADDR(BULK_OUT));


	}
	/* case 3. Get reset signal */


	/* acknowledge all serviced interrupt */
	if(serviced_int != int_value) {
		/* some interrupts are not serviced, why ? */
		_DEBUG_PRINTK("serviced_int= ",serviced_int);
		writel(USB_DEV_INTR_REG, serviced_int);
	}
}

void endpoint_in_interrupt(u32 int_value)
{
	struct USB_DATA_DESC *desc = NULL;
	u32 int_status = 0;
	volatile u32 value = 0;

	if(int_value & USB_DEV_EP1_IN) { /* BULK_IN */

		/* ack the device interrupt */
		writel(USB_DEV_EP_INTR_REG, USB_DEV_EP1_IN);

		int_status = readl(USB_EP_IN_STS_REG(BULK_IN_ID));
//		_DEBUG_PRINTK("bulk in interrupt",int_status);

		if(int_status & USB_EP_TRN_DMA_CMPL) {

			/* if a previous TX DMA complete interrupt happens
			   disable interrupt before next TX DMA is prepared */
			writel(USB_DEV_EP_INTR_MSK_REG, readl(USB_DEV_EP_INTR_MSK_REG)|USB_DEV_EP1_IN);

			/* acknowledge the status bit */
			writel(USB_EP_IN_STS_REG(BULK_IN_ID), USB_EP_TRN_DMA_CMPL);

			/* Tx DMA finished, wakeup */
			ep_event[BULK_IN_ID] = 1;

			/* FIXME: this is a delay for PHY to finish sending and
			   prevent from stopping transmitting */
			for(value=0;value<0x8ff;value++);

			/* if this is a issued send data command
			   then update the information in the command
			   structure	*/
			if(command[qindex.cmd_start].lock) {
				/* update tx data size */
				desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(BULK_IN);

				command[qindex.cmd_start].done_size +=
					(desc->status & USB_DMA_RXTX_BYTES);
				command[qindex.cmd_start].pBuf +=
					(desc->status & USB_DMA_RXTX_BYTES);

//				_DEBUG_PRINTK("BULK IN sent data length=",
//					desc->status & USB_DMA_RXTX_BYTES);

				/* if there is pending response, clear it */
				if(qindex.rsp_index)
					qindex.rsp_index --;
				if(qindex.rsp_index)
					_DEBUG_PRINTK("failed in ISR",0);
			}

		}
		if (int_status & USB_EP_IN_PKT) {

			/* if a IN token is received */
			bulk_in_handler();

			/* NAK the request before next IN packet is ready */
			setbitsl(USB_EP_IN_CTRL_REG(BULK_IN_ID), USB_EP_SET_NAK);
			/* acknowledge the status bit */
			writel(USB_EP_IN_STS_REG(BULK_IN_ID), USB_EP_IN_PKT);

		}
		if (int_status & USB_EP_BUF_NOT_AVAIL) {
			writel(USB_EP_IN_STS_REG(BULK_IN_ID), USB_EP_BUF_NOT_AVAIL);
			_DEBUG_PRINTK("BNA", 0);
		}
		if (int_status & USB_EP_HOST_ERR) {
			writel(USB_EP_IN_STS_REG(BULK_IN_ID), USB_EP_HOST_ERR);
			_DEBUG_PRINTK("HE", 0);
		}
		if (int_status & USB_EP_RCV_CLR_STALL) {
			writel(USB_EP_IN_STS_REG(BULK_IN_ID), USB_EP_RCV_CLR_STALL);
		}

	} else {  			/* CTRL_IN */
		int_status = readl(USB_EP_IN_STS_REG(CTRL_IN_ID));

		/* ack the device interrupt */
		/* disable interrupt before next TX DMA is prepared */

		if(int_status & USB_EP_TRN_DMA_CMPL) {

			/* disable EP0 IN interrupt */
			writel(USB_DEV_EP_INTR_MSK_REG, readl(USB_DEV_EP_INTR_MSK_REG)| USB_DEV_EP0_IN);

			/* acknowledge the DMA complete status */
			writel(USB_EP_IN_STS_REG(CTRL_IN_ID), USB_EP_TRN_DMA_CMPL);

			/* Tx DMA finished, wakeup */
			ep_event[CTRL_IN_ID] = 1;

			/* update tx data size */
			desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(CTRL_IN);

			command[qindex.cmd_start].done_size +=
					(desc->status & USB_DMA_RXTX_BYTES);

			/* if there is pending response, clear it */
			if(qindex.rsp_index)
				qindex.rsp_index --;
			if(qindex.rsp_index)
				_DEBUG_PRINTK("response index failed",0);

		} else if (int_status & USB_EP_IN_PKT) {
			writel(USB_EP_IN_STS_REG(CTRL_IN_ID), USB_EP_IN_PKT);

		} else if (int_status & USB_EP_BUF_NOT_AVAIL) {
			writel(USB_EP_IN_STS_REG(CTRL_IN_ID), USB_EP_BUF_NOT_AVAIL);

		} else if (int_status & USB_EP_HOST_ERR) {
			writel(USB_EP_IN_STS_REG(CTRL_IN_ID), USB_EP_HOST_ERR);

		} else {  /* other interrupt reasons */
			writel(USB_EP_IN_STS_REG(CTRL_IN_ID), int_status);
			//_DEBUG_PRINTK("ep(in) int status=",int_status);
		}

		writel(USB_DEV_EP_INTR_REG, USB_DEV_EP0_IN);
	}
}

void endpoint_out_interrupt(u32 int_value)
{
	struct USB_DATA_DESC *desc;
	u32 dma_status = 0, int_status;
	u32 ep_status = 0;
	u32 rx_length = 0;

	if(int_value & USB_DEV_EP1_OUT) { /*** BULK_OUT ***/
		/* ack the device interrupt */
		writel(USB_DEV_EP_INTR_REG, USB_DEV_EP1_OUT);

		desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(BULK_OUT);

		dma_status = desc->status;

		/* received data */
		if((dma_status & USB_DMA_BUF_STS) == USB_DMA_BUF_DMA_DONE) {

			rx_length = dma_status & USB_DMA_RXTX_BYTES;

			if (rx_length == 0)
				_DEBUG_PRINTK("rx_length is 0",0);

			/* disable OUT endpoint */
			writel(USB_DEV_EP_INTR_MSK_REG, readl(USB_DEV_EP_INTR_MSK_REG)| USB_DEV_EP1_OUT);

			/* set NAK until next rx is ready */
			setbitsl(USB_EP_OUT_CTRL_REG(BULK_OUT_ID), USB_EP_SET_NAK);

			/* queue reuest for background processing */
			qindex.cmd_end ++;
			qindex.cmd_end %= QUEUE_DEPTH; /* wrap around */

			command[qindex.cmd_end].TX_RX = RX_DIR;
			command[qindex.cmd_end].pendding = 1;
			command[qindex.cmd_end].pipe = BULK_OUT;
			command[qindex.cmd_end].need_size = rx_length;
			command[qindex.cmd_end].done_size = rx_length;
			command[qindex.cmd_end].pBuf = desc->data_ptr;
			command[qindex.cmd_end].lock = 0;

		}

		ep_status = readl(USB_EP_OUT_STS_REG(BULK_OUT_ID));

		/* check if any endpoint error status */
		/* case 1. buffer is not available */
		if(ep_status & USB_EP_BUF_NOT_AVAIL) {
			writel(USB_EP_OUT_STS_REG(BULK_OUT_ID), USB_EP_BUF_NOT_AVAIL);
		}
		/* case 2. host error */
		if(ep_status & USB_EP_HOST_ERR) {
			writel(USB_EP_OUT_STS_REG(BULK_OUT_ID), USB_EP_HOST_ERR);
		}

		/* Receive Clear Stall interrupt */
		if (ep_status & USB_EP_RCV_CLR_STALL) {
			writel(USB_EP_OUT_STS_REG(BULK_OUT_ID), USB_EP_RCV_CLR_STALL);
		}

		/* acknowledge endpoint status bit */
		if(ep_status & USB_EP_OUT_PKT) {
			writel(USB_EP_OUT_STS_REG(BULK_OUT_ID), USB_EP_OUT_PKT);
		}

	} else { 				/*** CTRL_OUT ***/
		/* ack the device interrupt */
		writel(USB_DEV_EP_INTR_REG, USB_DEV_EP0_OUT);

		/* check the status bits for what kind of packets in */
		int_status = readl(USB_EP_OUT_STS_REG(CTRL_OUT_ID));

		if((int_status & USB_EP_OUT_PKT_MSK) == USB_EP_OUT_PKT) {
			writel(USB_EP_OUT_STS_REG(CTRL_OUT_ID), USB_EP_OUT_PKT);

			desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(CTRL_OUT);
			rx_length = desc->status & USB_DMA_RXTX_BYTES;

			if(rx_length == 0) {
				writel(USB_DEV_CTRL_REG, readl(USB_DEV_CTRL_REG) | USB_DEV_RCV_DMA_EN);
				init_setup_descriptor(CTRL_OUT);
				//return;

			} /* rx_length==0 */

			control_out_handler();
			return;

		} else if ((int_status & USB_EP_OUT_PKT_MSK) == USB_EP_SETUP_PKT) {

			/* SETUP packet, always 8 bytes */
			/* ack setup packet status */
			writel(USB_EP_OUT_STS_REG(CTRL_OUT_ID), USB_EP_SETUP_PKT);

			/* If transmitting binary is in process, ignore the SETUP packet */
			if (bulk_process_is_locked()) {
				printf("WARNNING: get configuration request! Ignore it ...\n");
				return ;
			}

			decode_request();

		} else { /* none or reserved */

		}
	}
	return;
}

/*
 * Name: bulk_in_handler
 * Description:
 *	Process the BULK IN request from host
 * Input :
 * Return:
 */
void bulk_in_handler(void)
{
	/* not implmented */
}

void control_out_handler(void)
{
	struct USB_DATA_DESC *desc;
	u32 dma_status, ep_status;

	/* check if the descriptor has fresh data */
	desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(CTRL_OUT);
	dma_status = desc->status;

	if((dma_status & USB_DMA_BUF_STS) == USB_DMA_BUF_DMA_DONE) {
		if((dma_status & USB_DMA_RXTX_STS) == USB_DMA_RXTX_SUCC) {
			ep_status = readl(USB_EP_OUT_STS_REG(CTRL_OUT_ID));
			if(ep_status & USB_EP_BUF_NOT_AVAIL)
				writel(USB_EP_OUT_STS_REG(CTRL_OUT_ID), USB_EP_BUF_NOT_AVAIL);
			if(ep_status & USB_EP_HOST_ERR)
				writel(USB_EP_OUT_STS_REG(CTRL_OUT_ID), USB_EP_HOST_ERR);

		}
	}
}

/*
 * Name: decode_request
 * Description:
 *	Request from host through the default control pipe
 *	Decode what's the request
 *	host
 * Input : none
 * Return: none
 */
void decode_request(void)
{
	struct USB_SETUP_PKT *setup;
	u32 setup_data[2];
	u8 bmRequestType;
	u8 bRequest;
	u8 request;
	USB_DEVICE_REQUEST *req;

	/* read out setup packet */
	setup = (struct USB_SETUP_PKT *)setup_desc;
	setup_data[0] = setup->data0;
	setup_data[1] = setup->data1;

	req = (USB_DEVICE_REQUEST *)setup_data;

	bmRequestType = req->bmRequestType;
	bRequest = req->bRequest;

//	_DEBUG_PRINTK("bmRequestType= ",bmRequestType);
//	_DEBUG_PRINTK("bRequest= ",bRequest);

	request = bmRequestType & USB_DEV_REQ_TYPE_TYPE;

	switch(request) {
	case USB_DEV_REQ_TYPE_STANDARD: 	/* Standard request */
		switch(bRequest) {
              	case USB_GET_DESCRIPTOR:   	/* get descriptor    */
			get_descriptor(req);
			break;
		default:
			_DEBUG_PRINTK("unknown request= ",bRequest);
			break;
		} /* bRequest */
		break;
	case USB_CLASS:

		/* not implmented */

		break;
	case USB_VENDOR:

		/* not implmented */

		break;
	default:

		break;
	}/* switch(request) */

	/* reinitialize setup packet */
	init_setup_descriptor(CTRL_OUT);
}

void get_descriptor(USB_DEVICE_REQUEST *req)
{
	u16 type, index;
	u32 tx_size = 0;
	u8 *descriptor_p = NULL;

	type = (req->wValue & 0xff00) >> 8;
	index = (req->wValue & 0xff);

//	_DEBUG_PRINTK("host get descriptor",0);
//	_DEBUG_PRINTK("type=",type);
//	_DEBUG_PRINTK("index=",index);

	/* get different types of descriptors */
	switch(type) {
	case USB_DEVICE_DESCRIPTOR:
		descriptor_p = (u8 *)uds_bld_descriptors[DEVICE_DESC];
		tx_size = sizeof(USB_DEVICE_DESC);
		if(tx_size >= req->wLength)
			tx_size = req->wLength;
		break;
	case USB_CONFIGURATION_DESCRIPTOR:
		if(usb_speed == USB_HS) {
			descriptor_p = (u8 *)uds_bld_descriptors[CONFIG_HS];
			if(req->wLength == 0xff)
				tx_size = sizeof(uds_bld_configuration1_hs);
			else
				tx_size = req->wLength;
		} else {
			descriptor_p = (u8 *)uds_bld_descriptors[CONFIG_FS];
			if(req->wLength == 0xff)
				tx_size = sizeof(uds_bld_configuration1_fs);
			else
				tx_size = req->wLength;
		}
		break;
	case USB_STRING_DESCRIPTOR:
		if(index == 0)
			descriptor_p = (u8 *)uds_bld_descriptors[STRING_0];
		else if(index == 1)
			descriptor_p = (u8 *)uds_bld_descriptors[STRING_1];
		else if(index == 2)
			descriptor_p = (u8 *)uds_bld_descriptors[BLD_STRING2];
		else if(index == 3)
			descriptor_p = (u8 *)uds_bld_descriptors[STRING_3];
		else
			_DEBUG_PRINTK("Unknown string index",0);

		/* XXX: old mac OS send the SETUP with shorter wLength(wLength = 2).
		 * Howerver , descriptor_p[0] is larger than that , so the reponse
		 * tx_size = min(wLength, descriptor_p[0]) */
		if (descriptor_p)
			tx_size = descriptor_p[0] > req->wLength ? req->wLength : descriptor_p[0];
		break;
	case USB_DEVICE_QUALIFIER:
		descriptor_p = (u8 *)uds_bld_descriptors[DEVICE_QUA];
		tx_size = sizeof(USB_DEVICE_QUALIFIER_DESC);
		if(tx_size >= req->wLength)
			tx_size = req->wLength;
		break;
	case USB_OTHER_SPEED_CONFIGURATION:
		_DEBUG_PRINTK("get other speed configuration",0);
		break;

	default:

		break;
	}

	qindex.cmd_end ++;
	qindex.cmd_end %= QUEUE_DEPTH; /* wrap around */

	command[qindex.cmd_end].TX_RX = TX_DIR;
	command[qindex.cmd_end].pendding = 1;
	command[qindex.cmd_end].pipe = CTRL_IN;
	command[qindex.cmd_end].need_size = tx_size;
	command[qindex.cmd_end].done_size = 0;
	command[qindex.cmd_end].pBuf = (uintptr_t)descriptor_p;
	command[qindex.cmd_end].lock = 0;
}

void init_setup_descriptor(int ep_num)
{
	struct USB_SETUP_PKT *setup;

	setup = (struct USB_SETUP_PKT *)setup_desc;

	setup->status 	= USB_DMA_BUF_HOST_RDY;
	setup->reserved = 0xffffffff;
	setup->data0	= 0xffffffff;
	setup->data1	= 0xffffffff;

	clean_d_cache(setup, sizeof(struct USB_SETUP_PKT));

	writel(USB_EP_OUT_SETUP_BUF_PTR_REG(CTRL_OUT_ID), (uintptr_t)setup);
}

/*
 * Name: init_endpoint
 * Description:
 *	Configure each endpoint registers, and also assign UDC register value
 * Input : ep_num - endpoint number
 * Return: none
 */
void init_endpoint(int ep_num, int speed)
{
	u32 value;

	if(ep_num == CTRL_OUT) {	/*** initialize CTROL_OUT ***/
		writel(USB_EP_OUT_CTRL_REG(CTRL_OUT_ID), USB_EP_TYPE_CTRL);
		writel(USB_EP_OUT_MAX_PKT_SZ_REG(CTRL_OUT_ID), USB_EP_CTRLOUT_MAX_PKT_SZ);
		init_setup_descriptor(CTRL_OUT);
		init_data_descriptor(CTRL_OUT);

	} else if(ep_num == CTRL_IN) {	/*** initialize CTROL_IN ***/
		writel(USB_EP_IN_CTRL_REG(CTRL_IN_ID), USB_EP_TYPE_CTRL);
		writel(USB_EP_IN_BUF_SZ_REG(CTRL_IN_ID), USB_TXFIFO_DEPTH_CTRLIN);
		writel(USB_EP_IN_MAX_PKT_SZ_REG(CTRL_IN_ID), USB_EP_CTRLIN_MAX_PKT_SZ);
		init_data_descriptor(CTRL_IN);

	} else if(ep_num == BULK_OUT) {	/*** initialize BULK_OUT ***/
		writel(USB_EP_OUT_CTRL_REG(BULK_OUT_ID), USB_EP_TYPE_BULK | USB_EP_SET_NAK);
		if(speed == USB_HS)
			value = USB_EP_BULKOUT_MAX_PKT_SZ_HI;
		else	/* either FS or LS */
			value = USB_EP_BULKOUT_MAX_PKT_SZ_FU;
		writel(USB_EP_OUT_MAX_PKT_SZ_REG(BULK_OUT_ID), value);

		init_data_descriptor(BULK_OUT);

	} else {			/*** initialize BULK_IN ***/
		writel(USB_EP_IN_CTRL_REG(BULK_IN_ID), USB_EP_TYPE_BULK | USB_EP_SET_NAK);
		writel(USB_EP_IN_BUF_SZ_REG(BULK_IN_ID), USB_TXFIFO_DEPTH_BULKIN);
		if(speed == USB_HS)
			value = USB_EP_BULKIN_MAX_PKT_SZ_HI;
		else
			value = USB_EP_BULKIN_MAX_PKT_SZ_FU;
		writel(USB_EP_IN_MAX_PKT_SZ_REG(BULK_IN_ID), value);

		init_data_descriptor(BULK_IN);
	}
}

/*
 * Name: setup_data_descriptor
 * Description:
 *	Initialize the descriptor data structure
 * Input : ep_num - endpoint number
 * Return: none
 */
void init_data_descriptor(u32 ep_num)
{
	struct USB_DATA_DESC *desc;

	desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(ep_num);
	desc->status = (USB_DMA_BUF_DMA_DONE | USB_DMA_LAST);
	desc->reserved = 0xffffffff;
	desc->data_ptr = (uintptr_t)DMA_BUF_ADDR(ep_num);
	desc->next_desc_ptr = (uintptr_t)desc;

	clean_d_cache(desc, sizeof(struct USB_DATA_DESC));

	switch (ep_num) {
	case CTRL_IN:
		writel(USB_EP_IN_DAT_DESC_PTR_REG(CTRL_IN_ID), (uintptr_t)desc);
		break;
	case CTRL_OUT:
		writel(USB_EP_OUT_DAT_DESC_PTR_REG(CTRL_OUT_ID), (uintptr_t)desc);
		break;
	case BULK_IN:
		writel(USB_EP_IN_DAT_DESC_PTR_REG(BULK_IN_ID), (uintptr_t)desc);
		break;
	case BULK_OUT:
		writel(USB_EP_OUT_DAT_DESC_PTR_REG(BULK_OUT_ID), (uintptr_t)desc);
		break;
	default:
		printf("Invalid ep_num: %d\n", ep_num);
		break;
	}
}

/*
 * Name: init_usb_hardware
 * Description:
 *	Configure USB hardware registers
 * Input :
 * Return:
 */
u32 init_usb_hardware(void)
{
	u32 value = 0;

	rct_usb_reset();

	/* initialize each endpoint */
	init_endpoint(CTRL_IN,USB_HS);
	init_endpoint(CTRL_OUT,USB_HS);
	init_endpoint(BULK_IN,USB_HS);
	init_endpoint(BULK_OUT,USB_HS);

	/* device config register */
	value = USB_DEV_SPD_HI |
		USB_DEV_SELF_POWER |
		USB_DEV_PHY_8BIT |
		USB_DEV_UTMI_DIR_BI |
		USB_DEV_HALT_ACK |
		USB_DEV_SET_DESC_STALL |
		USB_DEV_DDR |
		USB_DEV_REMOTE_WAKEUP_EN;

	writel(USB_DEV_CFG_REG, value);

	/* device control register */
	value = USB_DEV_RCV_DMA_EN |
		USB_DEV_TRN_DMA_EN |
		USB_DEV_DESC_UPD_PYL |
		USB_DEV_LITTLE_ENDN |
		USB_DEV_DMA_MD;
	writel(USB_DEV_CTRL_REG, value);

	/* device interrupt mask register */
	/* ack previous interrupt, but necessary ? */
	writel(USB_DEV_INTR_REG, 0x0000007f);
	writel(USB_DEV_EP_INTR_REG, 0xffffffff);
	writel(USB_DEV_INTR_MSK_REG, 0x0000007f);
	writel(USB_DEV_EP_INTR_MSK_REG, 0xffffffff);

	/* enable device interrupt for
		Set_Configure
		Set_Interface
		Speed Enumerate Complete
	 */
	value = ~(USB_DEV_MSK_SET_CFG |
			  USB_DEV_MSK_SET_INTF |
			  USB_DEV_MSK_SPD_ENUM_CMPL);
	writel(USB_DEV_INTR_MSK_REG, value);

	/* enable endpoint interrupt */
	value = ~(USB_DEV_MSK_EP0_IN |
			  USB_DEV_MSK_EP0_OUT |
			  USB_DEV_MSK_EP1_IN |
			  USB_DEV_MSK_EP1_OUT);
	writel(USB_DEV_EP_INTR_MSK_REG, value);


	_DEBUG_PRINTK("init_usb_hardware done",0);

	return 0;
}

/*
 * Name: init_usb_dev
 * Description:
 *	Initialize USB device data structure
 * Input : none
 * Return: none
 */
void init_usb_dev(void)
{
	/* init command/response queue */
	memzero ((u8 *)command, sizeof(COMMAND_QUEUE) * QUEUE_DEPTH);
	memzero ((u8 *)response, sizeof(WAIT_QUEUE) * QUEUE_DEPTH);
	memzero ((u8 *)&qindex, sizeof(QUEUE_INDEX));
	memzero ((u8 *)&transfer, sizeof(transfer_info_t));

	_DEBUG_PRINTK("init_usb_dev done",0);
}


/*
 * Name: init_usb_pll
 * Description:
 *	Initialize USB PLL mode
 *	default mode is 11
 * Input : none
 * Return: none
 */
void init_usb_pll(void)
{
	rct_enable_usb();
	rct_timer_dly_ms(1);
	set_softdisc();
}


/*
 * Name: init_udc_reg
 * Description:
 *	Initialize UDC registers (only in enumeration timing window)
 * Input : none
 * Return: none
 */
void init_udc_reg(void)
{
	u32 value;

	value = USB_UDC_EP0_NUM |
		USB_UDC_OUT |
		USB_UDC_CTRL |
		USB_UDC_CFG_NUM |
		USB_UDC_INTF_NUM |
		USB_UDC_ALT_SET |
		USB_UDC_CTRLOUT_MAX_PKT_SZ;
	writel(USB_UDC_REG(CTRL_OUT), value);

	value = USB_UDC_EP0_NUM |
		USB_UDC_IN  |
		USB_UDC_CTRL |
		USB_UDC_CFG_NUM |
		USB_UDC_INTF_NUM |
		USB_UDC_ALT_SET |
		USB_UDC_CTRLIN_MAX_PKT_SZ;
	writel(USB_UDC_REG(CTRL_IN), value);

	value = USB_UDC_EP1_NUM |
		USB_UDC_OUT |
		USB_UDC_BULK |
		USB_UDC_CFG_NUM |
		USB_UDC_INTF_NUM |
		USB_UDC_ALT_SET;
	value |= USB_UDC_BULKOUT_MAX_PKT_SZ_HI;
	writel(USB_UDC_REG(BULK_OUT), value);

	value = USB_UDC_EP1_NUM |
		USB_UDC_IN  |
		USB_UDC_BULK |
		USB_UDC_CFG_NUM |
		USB_UDC_INTF_NUM |
		USB_UDC_ALT_SET;
	value |= USB_UDC_BULKIN_MAX_PKT_SZ_HI;
	writel(USB_UDC_REG(BULK_IN), value);

	writel(USB_DEV_CFG_REG, readl(USB_DEV_CFG_REG) | USB_DEV_CSR_PRG_EN);

}

/*
 * Name: usb_init
 * Description:
 *	USB initialization
 * Input : none
 * Return: none
 */
void usb_init (void)
{
	/* initialize software data structure */
	init_usb_dev();

	/* initialize hardware */
	init_usb_hardware();

	request_irq(USBC_IRQ, IRQ_LEVEL_HIGH, usb_isr, NULL);

	/* re-enable USB connection by SW */
	writel(USB_DEV_CTRL_REG, readl(USB_DEV_CTRL_REG) | USB_DEV_REMOTE_WAKEUP);
}

u32 start_tx(u32 ep_num, void *buf_ptr, u32 *pkt_size)
{
	struct USB_DATA_DESC *desc;
	volatile u32 value = 0;
	u32 send_size, ep_id, max_pkt_size;

	usb_dump(ep_num, buf_ptr, *pkt_size);

	if (ep_num == CTRL_IN)
		ep_id = CTRL_IN_ID;
	else
		ep_id = BULK_IN_ID;

	max_pkt_size = readl(USB_EP_IN_MAX_PKT_SZ_REG(ep_id));

	/* sending NULL packet, *buf_ptr should be NULL	*/
	if(*pkt_size == 0) {
		buf_ptr = DMA_BUF_ADDR(ep_num);
		send_size = 0;
	} else {
		send_size = min(*pkt_size, max_pkt_size);

	}

	desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(ep_num);
	desc->data_ptr = (uintptr_t)buf_ptr; /* assign new packet buffer pointer */
	desc->status = USB_DMA_BUF_HOST_RDY | USB_DMA_LAST | send_size;
	clean_d_cache(desc, sizeof(struct USB_DATA_DESC));
	clean_d_cache(buf_ptr, send_size);

	irq_disable(USBC_IRQ);

	setbitsl(USB_EP_IN_CTRL_REG(ep_id), USB_EP_CLR_NAK);

	/*
	 * Wait IN token before issue DMA polling
	 * NOTE: Please add your own timeout here.
	 * Or it will wait forever ....
	 */
	while(!(readl(USB_EP_IN_STS_REG(ep_id)) & USB_EP_IN_PKT));

	writel(USB_EP_IN_STS_REG(ep_id), USB_EP_IN_PKT);

	value = readl(USB_DEV_EP_INTR_MSK_REG);
	if(ep_num == CTRL_IN)
		value &= ~(USB_DEV_MSK_EP0_IN);
	else /* if(ep_num == BULK_IN) */
		value &= ~(USB_DEV_MSK_EP1_IN);

	/* enable the corresponding EP interrupt */
	writel(USB_DEV_EP_INTR_MSK_REG, value);

	/* issue polling bit */
	setbitsl(USB_EP_IN_CTRL_REG(ep_id), USB_EP_POLL_DEMAND);

	/*
	 * FIXME: No need to delay for macbook and verified with other OS,
	 * so remove it.
	 */
#if 0
	for(value=0;value<0xffff;value++);
#endif

	/* update data buffer and size in current command element*/
	buf_ptr += send_size;
	*pkt_size -= send_size;

	irq_enable(USBC_IRQ);

	/* wait tx DMA finished, woke up by DMA complete interrupt */
	while(!ep_event[ep_id]);
	ep_event[ep_id] = 0;

	/* FIXME: Host will failed to get description sometimes. this delay is
	 * in order to makesure packet tx finished */
	rct_timer_dly_ms(1);

	if((send_size == 0) || (send_size < max_pkt_size)) {
		/* the last packet */
		if(*pkt_size)
			_DEBUG_PRINTK("remaind size is not 0",0);
		return 0;
	} else {
		/* still packet to send */

		return 1;
	}
}

void start_rx(u32 ep_num, void *buf_ptr)
{
	struct USB_DATA_DESC *desc;
	volatile u32 value=0;
	u32 ep_id = (ep_num == CTRL_OUT) ? CTRL_OUT_ID : BULK_OUT_ID;

	irq_disable(USBC_IRQ); /* critical section */

	desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(ep_num);
	desc->data_ptr = (unsigned long)buf_ptr;
	desc->status = USB_DMA_BUF_HOST_RDY | USB_DMA_LAST;
	clean_d_cache(desc, sizeof(struct USB_DATA_DESC));

	irq_enable(USBC_IRQ); /* critical section */

	value = readl(USB_DEV_EP_INTR_MSK_REG);

	if(ep_num == CTRL_OUT)
		value &= ~(USB_DEV_MSK_EP0_OUT);
	else /* if(ep_num == BULK_OUT) */
		value &= ~(USB_DEV_MSK_EP1_OUT);

	writel(USB_DEV_EP_INTR_MSK_REG, value);

	// clear NAK
	writel(USB_DEV_CTRL_REG, readl(USB_DEV_CTRL_REG) | USB_DEV_RCV_DMA_EN);

	/*
	 * FIXME: No need to delay for macbook and verified with other OS,
	 * so remove it.
	 */
#if 0
	for(value=0;value < 0x100;value++);
#endif

	setbitsl(USB_EP_OUT_CTRL_REG(ep_id), USB_EP_CLR_NAK);
}

/*
 * Name: send_response
 * Description:
 *	Send response to host
 *
 * Input : rsp - response code
 *	   r0 - parameter 0 of response
 *	   r1 - parameter 1 of response
 * Return: none
 */
void send_response (u32 rsp, u32 r0, u32 r1)
{
	u32 rsp_length = sizeof(DEV_RSP);

	dev_response.signature = STATUS_TOKEN;
	dev_response.response = rsp;
	dev_response.parameter0 = r0;
	dev_response.parameter1 = r1;

	start_tx(BULK_IN, &dev_response, &rsp_length);
	//_DEBUG_PRINTK ("Send response", rsp);
}

/*
 * Name: process_cmd_send_fw
 * Description:
 *	Process USB_CMD_GET_FW_XXX commands of USB_CMD_RDY_TO_SND
 *
 * Input : cmd - the command from host
 *	   addr - the address of data to send
 *	   length - the length of data to send
 * Return: <0: failed, 0: success
 */
int process_cmd_send_fw (u32 cmd, uintptr_t *addr, u32 *length)
{
	flpart_table_t ptb;
	flpart_t *part;
	uintptr_t mem_addr = (uintptr_t)bld_hugebuf_addr;
	u32 header_len = sizeof(partimg_header_t);

	if ((addr == NULL) || (length == NULL))
		return -1;

	/* Read partition table */
	if (flprog_get_part_table(&ptb) < 0)
		return -2;

	/* Load image into memory */
	switch (cmd) {
	case USB_CMD_GET_FW_PTB:
		memcpy ((void *)mem_addr, &ptb, sizeof(flpart_table_t));
		*addr = mem_addr;
		*length = sizeof (flpart_table_t);
		return 0;
	default:
		putstr("process_cmd_send_fw: Invalid cmd\r\n");
		putdec(cmd);
		putstr("\r\n");
		return -3;
	}

	if (part->img_len == 0) {
		if (length)
			*length = 0;
		return -4;
	}

	/* Prepare header */
	memzero((void *)mem_addr, header_len);
	memcpy((void *)mem_addr, part, sizeof (flpart_t));
	/* Get raw data */
	//nand_load(sblk, nblk, mem_addr + header_len, part->img_len);

	*addr = mem_addr;
	*length = part->img_len + header_len;
	return 0;
}

/*
 * Name: process_cmd_recv
 * Description:
 *	Process USB_CMD_RDY_TO_RCV command
 *
 * Input : host_cmd - the command from host
 * Return: none
 */
void process_cmd_recv (HOST_CMD *host_cmd)
{
	u32 r0 = 0;
	flpart_table_t ptb;
	int rval;

	/* Get transfer paremeters */
	transfer.flag = host_cmd->parameter0;
	if (transfer.flag & FLAG_ADDRESS) {
		transfer.rx.addr = host_cmd->parameter1;
		if (transfer.rx.addr < DRAM_START_ADDR ||
			transfer.rx.addr > (DRAM_START_ADDR + DRAM_SIZE - 1)) {
			_DEBUG_PRINTK("Invalid recv address: 0x",
				transfer.rx.addr);
			transfer.rx.addr = (uintptr_t)bld_hugebuf_addr;
			_DEBUG_PRINTK("Revert to default address: ",
				transfer.rx.addr);
		}
	}
	if (transfer.flag & FLAG_JUMP_EXEC) {
		transfer.jump_address = host_cmd->parameter2;
		if (transfer.jump_address < DRAM_START_ADDR ||
			transfer.jump_address > (DRAM_START_ADDR + DRAM_SIZE - 1)) {
			_DEBUG_PRINTK("Invalid jump address: 0x",
				transfer.jump_address);
			transfer.jump_address = (uintptr_t)bld_hugebuf_addr;
			_DEBUG_PRINTK("Revert to default address: 0x",
				transfer.jump_address);
		}
	}
	if (transfer.flag & FLAG_COMMAND) {
		switch (host_cmd->parameter1) {
		case USB_CMD_SET_MEMORY_READ:
			r0 = readl(host_cmd->parameter2);
			break;
		case USB_CMD_SET_MEMORY_WRITE:
			writel (host_cmd->parameter2, host_cmd->parameter3);
			break;
		case USB_CMD_SET_FW_PTB:
			rval = flprog_get_part_table(&ptb);
			if (rval < 0) {
				putstr("PTB load error!\r\n");
			} else {
				if ((*(u32 *)transfer.rx.addr) != FLPART_MAGIC) {
					/* add MAC:offset * 4 only use for eth0_mac ? Others? */
					memcpy (((u8 *)&ptb+(host_cmd->parameter2*4)),
						(u8 *)transfer.rx.addr, transfer.rx.length);
					rval = flprog_set_part_table(&ptb);
					if (rval < 0) {
						putstr("PTB save error!\r\n");
					}
				}
			}
			break;
		}
	}
	if (transfer.flag & FLAG_FORCE_FINISH)
		transfer.task_finished = 1;

	/* reponse */
	send_response (USB_RSP_SUCCESS, r0, 0);
}

/*
 * Name: process_cmd_recv_data
 * Description:
 *	Process USB_CMD_RCV_DATA command
 *
 * Input : host_cmd - the command from host
 * Return: none
 */
u32 * process_cmd_recv_data (HOST_CMD *host_cmd)
{
	transfer.rx.crc_value = host_cmd->parameter0;
	_DEBUG_PRINTK ("start to recv data, crc=0x", transfer.rx.crc_value);

	rx_state = RCV_DATA_PHASE;
	return (u32 *)transfer.rx.addr;
}

/*
 * Name: process_cmd_send
 * Description:
 *	Process USB_CMD_RDY_TO_SND command
 *
 * Input : host_cmd - the command from host
 * Return: none
 */
void process_cmd_send (HOST_CMD *host_cmd)
{
	buf_info_t *tx = &transfer.tx;
	u32 rsp;

	/* Init response */
	rsp = USB_RSP_SUCCESS;
	tx->addr = 0;
	tx->length = 0;

	/* check application finished */
	transfer.flag = host_cmd->parameter0;

	switch (host_cmd->parameter1) {
	case USB_CMD_GET_MEMORY:
		/* Save transfer information */
		tx->addr = host_cmd->parameter2;
		tx->length = host_cmd->parameter3;
		break;

	case USB_CMD_GET_FW_BST:
	case USB_CMD_GET_FW_BLD:
	case USB_CMD_GET_FW_PRI:
	case USB_CMD_GET_FW_RMD:
	case USB_CMD_GET_FW_ROM:
	case USB_CMD_GET_FW_DSP:
	case USB_CMD_GET_FW_PTB:
		if (process_cmd_send_fw (host_cmd->parameter1,
					&tx->addr, &tx->length) != 0)
			rsp = USB_RSP_FAILED;

		break;

	default:
		_DEBUG_PRINTK ("Unknown Get CMD=", host_cmd->parameter1);
		break;
	}

	/* Calculate CRC */
	if (tx->length)
		tx->crc_value = crc32((void *)tx->addr, tx->length);

	/* reponse with length and CRC */
	send_response (rsp, tx->length, tx->crc_value);
}

/*
 * Name: process_cmd_send_data
 * Description:
 *	Process USB_CMD_SND_DATA command
 *
 * Input : host_cmd - the command from host
 * Return: none
 */
void process_cmd_send_data (HOST_CMD *host_cmd)
{
	putstr ("Start to send data ");
	putdec (transfer.tx.length);
	putstr (" bytes...\r\n");

	/* Add a new command to command queue */
	qindex.cmd_end ++;
	qindex.cmd_end %= QUEUE_DEPTH;

	command[qindex.cmd_end].TX_RX = TX_DIR;
	command[qindex.cmd_end].pendding = 1;
	command[qindex.cmd_end].pipe = BULK_IN;
	command[qindex.cmd_end].need_size = transfer.tx.length;
	command[qindex.cmd_end].done_size = 0;
	command[qindex.cmd_end].pBuf = transfer.tx.addr;
	command[qindex.cmd_end].lock = 1;
}

/*
 * Name: process_cmd_inquiry
 * Description:
 *	Process USB_CMD_INQUIRY_STATUS command
 *
 * Input : host_cmd - the command from host
 * Return: none
 */
void process_cmd_inquiry (HOST_CMD *host_cmd)
{
	if (host_cmd->parameter0 == USB_BLD_INQUIRY_CHIP) {
		send_response(USB_RSP_SUCCESS, CHIP_REV, 0);
	} else if (host_cmd->parameter0 == USB_BLD_INQUIRY_ADDR) {
		send_response(USB_RSP_SUCCESS, AMBOOT_BLD_RAM_START, 0);
	} else if (host_cmd->parameter0 == USB_BLD_INQUIRY_REG) {
		send_response(USB_RSP_SUCCESS, readl(host_cmd->parameter1), 0);
	} else {
		/* reponse */
		send_response(USB_RSP_IN_BLD, 0, 0);
	}
}

/*
 * Name: rx_fsm
 * Description:
 *	Process the BULK OUT request from host as RX Finite State Machine
 * Input :
 * Return:
 */
void rx_fsm(COMMAND_QUEUE *cur_cmd)
{
	HOST_CMD *host_cmd = (HOST_CMD *)cur_cmd->pBuf;
	u32 *next_rx_ptr = NULL;
	u32 max_pkt_size = readl(USB_EP_OUT_MAX_PKT_SZ_REG(BULK_OUT_ID));

	/* Set next_rx_ptr to EP buffer for receive next command */
	next_rx_ptr = (u32 *)DMA_BUF_ADDR(BULK_OUT);

	usb_dump(BULK_OUT, (void *)cur_cmd->pBuf, cur_cmd->done_size);

	switch (rx_state) {
	case COMMAND_PHASE:
		/* Check host command */
		if (host_cmd->signature != COMMAND_TOKEN) {
			_DEBUG_PRINTK ("Wrong signature", host_cmd->signature);
			break;
		}

		/* Process command */
		switch (host_cmd->command) {
		case USB_CMD_RDY_TO_RCV:
			process_cmd_recv (host_cmd);
			break;
		case USB_CMD_RCV_DATA:
			next_rx_ptr = process_cmd_recv_data (host_cmd);
			break;
		case USB_CMD_RDY_TO_SND:
			process_cmd_send (host_cmd);
			break;
		case USB_CMD_SND_DATA:
			process_cmd_send_data (host_cmd);
			break;
		case USB_CMD_INQUIRY_STATUS:
			process_cmd_inquiry (host_cmd);
			break;
		default:
			_DEBUG_PRINTK ("Unknown command", host_cmd->command);
			break;
		}
		break;

	case SND_DATA_PHASE:
		break;

	case RCV_DATA_PHASE:
		 /* done_size = need_size = received packet size */
		 if (cur_cmd->done_size < max_pkt_size) {
		 	u32 cal_crc_value;

			/* Short packet received */
		 	transfer.rx.length = (uintptr_t)cur_cmd->pBuf +
		 			     cur_cmd->done_size -
		 			     transfer.rx.addr;
			transfer.total_length += transfer.rx.length;

			/* Calculate CRC */
			_clean_flush_d_cache();
			cal_crc_value = crc32((void *) transfer.rx.addr,
				transfer.rx.length);

			putstr("total rcv data length = ");
			putdec(transfer.rx.length);
			putstr(" bytes, crc32 = 0x");
			puthex(cal_crc_value);
			putstr("\r\n");

			/* send response according to CRC check */
			if(cal_crc_value != transfer.rx.crc_value)
		 		send_response (USB_RSP_FAILED, 0, 0);
			else
		 		send_response (USB_RSP_SUCCESS, 0, 0);

			transfer.task_finished= 1;
		 	rx_state = COMMAND_PHASE;

			 /* finish transmitting the binary, unlock the bulk process */
			bulk_process_tryunlock();

		 } else {

			 /* in transmitting process, lock the bulk process */
			 bulk_process_trylock();

			/* advanced rx buffer pointer */
			/* done_size is "byte", but address is "word" */
			next_rx_ptr = (u32 *)cur_cmd->pBuf;
			next_rx_ptr += (cur_cmd->done_size >> 2);

			print_twiddle();
		 }
		 break;
	default:
		 _DEBUG_PRINTK("Unknown rx_state", rx_state);
		 break;
	} /* switch */

	cur_cmd->pendding = 0;
	start_rx(BULK_OUT, next_rx_ptr);
}

/*
 * Name: process_bulk_in
 * Description:
 *	Process the command to BULK_IN pipe
 *
 * Input : cur_cmd - the current processing command
 * Return: none
 */
void process_bulk_in (COMMAND_QUEUE *cur_cmd)
{
	/* being here only as in SND_DATA state */
	if(cur_cmd->need_size == 0) {
		/* finish sending, then just ACK */

		transfer.total_length += transfer.tx.length;
		qindex.rsp_index ++;

		send_response (USB_RSP_SUCCESS, 0, 0);

		_DEBUG_PRINTK ("Send completed!",0);

		cur_cmd->pendding = 0;
		transfer.task_finished = 1;

	} else {	/* need_size != 0 */

		/* prepare wait response data */
		/* wait for TX DMA finish */
		qindex.rsp_index ++;
		start_tx(BULK_IN, (void *)cur_cmd->pBuf, &cur_cmd->need_size);

		print_twiddle ();
	}
}

/*
 * Name: process_ctrl_in
 * Description:
 *	Process the command to CTRL_IN pipe
 *
 * Input : cur_cmd - the current processing command
 * Return: none
 */
void process_ctrl_in (COMMAND_QUEUE *cur_cmd)
{
	u32 value = readl(USB_EP_IN_MAX_PKT_SZ_REG(CTRL_IN_ID));

	if(cur_cmd->need_size < value) {
		/* the last packet */
		cur_cmd->pendding = 0;
	}

	/* prepare wait response data */
	/* wait for TX DMA finish */
	if(qindex.rsp_index) {
		_DEBUG_PRINTK("wrong rsp_index =", qindex.rsp_index);
		/* reset response index if this is an error one */
		qindex.rsp_index = 0;
	}

	qindex.rsp_index ++;
	start_tx(CTRL_IN, (void *)cur_cmd->pBuf, &cur_cmd->need_size);

	/* prepare the status stage */
	start_rx(CTRL_OUT, DMA_BUF_ADDR(CTRL_OUT));
}

/*
 * Name: usb_task
 * Description:
 *	This subroutine is called from command line
 *	Its function is to send/receive data
 *
 * Input : none
 * Return: 1: failed, 0: success
 */
int usb_task(void)
{
	COMMAND_QUEUE *cur_cmd = NULL;
	struct USB_DATA_DESC *desc;

	_DEBUG_PRINTK("entering usb_task",0);

	cur_cmd = NULL;

	transfer.task_finished = 0;
	while(!transfer.task_finished) {
		/* issuing RX DMA enable all the time if in RCV_DATA */
		if(rx_state == RCV_DATA_PHASE) {
			irq_disable(USBC_IRQ);
			/* check if the descriptor has fresh data */
			desc = (struct USB_DATA_DESC *)DMA_DESC_ADDR(BULK_OUT);
			_clean_flush_d_cache();

			/* if descriptor is ready to receive */
			if(!(desc->status & USB_DMA_BUF_STS))
				writel(USB_DEV_CTRL_REG, readl(USB_DEV_CTRL_REG) | USB_DEV_RCV_DMA_EN);
			irq_enable(USBC_IRQ);
		}

		/* the rsp_index is set caused by previous command,
		   and will be cleared in ISR */
		if(qindex.rsp_index) {
			/* a timeout is needed ? */
			continue;
		}

		/* If no current command or current command has finished,
		 * try to dequeue one from command queue. */
		if ((!cur_cmd) || (cur_cmd->pendding == 0)) {

			if (qindex.cmd_start == qindex.cmd_end) {
				/* Command queue is empty */
				cur_cmd = NULL;
				continue;
			} else {
				/* get a new command from command queue */
				qindex.cmd_start ++;
				qindex.cmd_start %= QUEUE_DEPTH;
				cur_cmd = (COMMAND_QUEUE *)&command[qindex.cmd_start];
			}
		}

		/* process the pendding or new command */
		if(cur_cmd->TX_RX == RX_DIR) {		/* RX_DIR */

			if (cur_cmd->pipe == BULK_OUT)
				rx_fsm(cur_cmd);

		} else if (cur_cmd->TX_RX == TX_DIR) {	/* TX_DIR */

			if (cur_cmd->pipe == BULK_IN)
				process_bulk_in (cur_cmd);
			else if (cur_cmd->pipe == CTRL_IN)
				process_ctrl_in (cur_cmd);

		}
	} /* finish_download */

	return SUCCESSED;

}

/*
 * Name: usb_download
 * Description:
 *	main task called through command shell
 * Input : addr - target address
 *	   exec - jump to the target address or not
 *	   flag - download flags, see definition above
 * Return: transmitted/received length
 */
u32 usb_download(void *addr, int exec, int flag)
{
	void (*jump)(void) = addr;
	u32 connected_at_boot;
	void *ptr;

	enable_usb = 1;
	connected_at_boot = exec;

	/* USB initialization */
	usb_init ();

	/* Handling transfer request */
	if (flag & USB_FLAG_TEST_MASK) {

		usb_test (flag);

	} else if ((flag & USB_FLAG_KERNEL) || (flag & USB_FLAG_FW_PROG)) {

		usb_download_kernel (flag & USB_FLAG_FW_PROG);

		/* disable TX/RX DMA */
		writel(USB_DEV_CTRL_REG, readl(USB_DEV_CTRL_REG) & 0xfffffff3);

		/* mask device/endpoint interrupt */
		writel(USB_DEV_INTR_MSK_REG, 0xffffffff);
		writel(USB_DEV_EP_INTR_MSK_REG, 0xffffffff);

		/* set soft-disconnect
		 * set_softdisc() got problem because it set the VIC again
		 */
		writel(USB_DEV_CTRL_REG, UDC_CTRL_SD);
		writel(USB_DEV_CFG_REG, UDC_CFG_RWKP);

	} else if (flag & USB_FLAG_MEMORY) {
		transfer.rx.addr = (uintptr_t) addr;
		usb_task();

	} else {
		while ((transfer.flag & FLAG_LAST_TRANS) == 0)
			usb_task ();
		if (transfer.flag & FLAG_JUMP_EXEC) {
			exec = 1;
			jump = (void *) transfer.jump_address;
		}


		/* disable TX/RX DMA */
		writel(USB_DEV_CTRL_REG, readl(USB_DEV_CTRL_REG) & 0xfffffff3);

		/* mask device/endpoint interrupt */
		writel(USB_DEV_INTR_MSK_REG, 0xffffffff);
		writel(USB_DEV_EP_INTR_MSK_REG, 0xffffffff);

		/* set soft-disconnect
		 * set_softdisc() got problem because it set the VIC again
		 */
		writel(USB_DEV_CTRL_REG, UDC_CTRL_SD);    // Soft disconnect
		writel(USB_DEV_CFG_REG, UDC_CFG_RWKP);    // Enable remote wakeup
	}

	/* Tranfer finished */
	if(!connected_at_boot) {
		writel(USB_DEV_CFG_REG, 0);
		writel(USB_DEV_CTRL_REG, 0);
		writel(USB_DEV_INTR_REG, 0x7f);
		writel(USB_DEV_EP_INTR_REG, 0xffffffff);
		writel(USB_DEV_INTR_MSK_REG, 0x7f);
		writel(USB_DEV_EP_INTR_MSK_REG, 0xffffffff);
	}

	if (exec) {
		putstr("Start to run...\r\n");
		_clean_flush_all_cache();
		_disable_icache();
		_disable_dcache();
		disable_interrupts();

		/* put the return address in 0xc00ffffc
		 * and let jump() to read and return back */
		ptr = &&_return_;
		*(volatile uintptr_t *)(DRAM_START_ADDR + 0x000ffffc) = (uintptr_t) ptr;

		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");

		jump ();

		/* being here only if REBOOT_AFTER_BURNING */
_return_:
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");
		__asm__ __volatile__ ("nop");

	}

	return transfer.total_length;
}

/*
 * Name: print_twiddle
 * Description:
 *	Just funny stuff to indicate that it's alive
 * Input : none
 * Return: none
 */
void print_twiddle(void)
{
	static int i=0;
	static int loop = 0;

	if(loop && !(loop % 100)) {
		putchar('\b');
		putchar(twiddle[(i++)%4]);
	}
	loop ++;
}

/*
 * Name: set_softdisc
 * Description:
 *	If VBUS is on (USB cable is connected) as initializing,
 *	just set USB as disconnect, and make it reconnect later by SW
 * Input : none
 * Return: none
 */
void set_softdisc(void)
{
	/* note don't change the order */
	writel(UDC_CTRL_REG, readl(UDC_CTRL_REG) | UDC_CTRL_SD);
	writel(UDC_CFG_REG, readl(UDC_CFG_REG) | UDC_CFG_RWKP);
}

/*
 * Name: usb_boot_logo
 * Description:
 *	Show usb boot information
 * Input : none
 * Return: none
 */
void usb_boot_logo(void)
{
	rct_show_boot_from(rct_boot_from());
	rct_show_pll();
}

/*
 * Name: usb_boot
 * Description:
 *	Boot from USB downloading
 * Input : usbdl_mode - download mode
 * Return: none
 */
void usb_boot(u8 usbdl_mode)
{
	int flag = 0;

	usb_boot_logo();

	init_usb_pll();
	switch (usbdl_mode)
	{
	case 0x80:	/* Useless? */
		flag = USB_FLAG_FW_PROG;
		break;
	case USB_DL_NORMAL:
		flag = USB_FLAG_KERNEL;
		break;
	case USB_DL_DIRECT_USB:
		flag = 0;
		break;
	}

	putstr("Entered USB download mode (");
	putbyte(usbdl_mode);
	putstr(")...\r\n");
	usb_download((void *)MEMFWPROG_RAM_START, 0, flag);
}

/*
 * Name: usb_disconnect
 * Description:
 *	Disconnect USB from host
 * Input : none
 * Return: none
 */
void usb_disconnect(void)
{
	init_usb_pll();
}

/*
 * Name: do_reboot
 * Description:
 * 	Reboot whole system
 * Input : none
 * Return: none
 */
static void do_reboot(void)
{
	disable_interrupts();
	_clean_flush_all_cache();
	_disable_icache();
	_disable_dcache();

	rct_reset_chip();

	while (1);
}

/*
 * Name: usb_download_file
 * Description:
 * 	Save one file from USB to specific address
 * Input : addr - target address
 * Return: none
 */
void usb_download_file (uintptr_t addr)
{
	unsigned int retry_counter;

	putstr("Target address: 0x");
	puthex(addr);
	putstr("\r\n");

	/* run the background task */
	transfer.rx.addr = addr;
	retry_counter = 0;
	while(usb_task() == FAILED) {
		retry_counter ++;
		if(retry_counter == RETRY_COUNT) {
			putstr("Retry failed\r\n");
			do_reboot();
		}
	}
}

/*
 * Name: usb_download_kernel
 * Description:
 * 	Download 4 kernel files from USB to specific address.
 * Input : is_fw_prog - Is the image to be loaded a firmware program.
 * Return: none
 */
void usb_download_kernel (int is_fw_prog)
{

	putstr("=============================");
	putstr("\r\n");
	putstr("USB download ");
	putstr("\r\n");
	putstr("Built @ ");
	putstr(__DATE__);		/* compiling time */
	putstr("  ");
	putstr(__TIME__);
	putstr("\r\n");
	putstr("=============================");
	putstr("\r\n");

	if (is_fw_prog) {
		putstr("Download 1 firmware programming file\r\n");
		usb_download_file((uintptr_t)bld_hugebuf_addr);
		putstr("firmware program is loaded\r\n");
	} else {
		putstr("Download 4 kernel files\r\n");

		usb_download_file((uintptr_t)bld_hugebuf_addr);
		putstr("Prkernel is loaded\r\n");

		/* Flow broken if PC application ask to finish */
		if (transfer.flag & FLAG_LAST_TRANS)
			return;

		usb_download_file (DRAM_START_ADDR + 0x03e00000);
		putstr("code is loaded\r\n");

		usb_download_file (DRAM_START_ADDR + 0x03f00000);
		putstr("memd is loaded\r\n");

		usb_download_file (DRAM_START_ADDR + 0x03eb9c00);
		putstr("default_bin is loaded\r\n");
	}
}

/*
 * Name: usb_test_download
 * Description:
 * 	Test USB download flow TEST_NUM times.
 * Input : none
 * Return: none
 */
void usb_test_download (void)
{
	uintptr_t addr = (uintptr_t)bld_hugebuf_addr;
	uintptr_t mem_start = DRAM_START_ADDR;
	uintptr_t mem_end = DRAM_END_ADDR;
	u32 mem_jump = 0x500000;	// 5MB
	u32 max_file_size = 0x400000;	// 4MB
	int i;

	putstr("USB download test:\r\n");
	for (i = 0; i < TEST_NUM; i++) {
		usb_download_file (addr);

		/* Change and wrapper memory address */
		addr += mem_jump;
		if ((addr + max_file_size) > mem_end) {
			addr += mem_jump;
			addr -= (mem_end - mem_start);
		}
		_DEBUG_PRINTK ("test #", i);
	}
	_DEBUG_PRINTK ("test times=", i);
}

/*
 * Name: usb_test
 * Description:
 *	Test USB related functionalities
 * Input : flag - download flags, see definition above
 * Return: none
 */
void usb_test (int flag)
{
	if (flag & USB_FLAG_TEST_DOWNLOAD)
		usb_test_download ();
}
