/*
 * kernel/private/drivers/lens/amba_lens.c
 *
 * History:
 *    2014/10/10 - [Peter Jiao]
 *
 * Copyright (c) 2017 Ambarella, Inc.
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


#include <config.h>

#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/io.h>
#include <linux/ioport.h>
#include <linux/irq.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/ambbus.h>
#include <linux/delay.h>
#include <linux/dma-mapping.h>
#include <linux/ioctl.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/vmalloc.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/idr.h>
#include <linux/seq_file.h>
#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/i2c.h>
#include <linux/firmware.h>
#include <linux/string.h>
#include <linux/fb.h>
#include <linux/spi/spi.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/clk.h>
#include <asm/dma.h>
#include <asm/uaccess.h>
#include <linux/semaphore.h>
#include <iav_devnum.h>
#include <plat/iav_helper.h>
#include <plat/gpio.h>

#include "amba_lens.h"


static u32 amba_timer_input_freq = 0;
static int amba_zoom_timer_irq = 0;
static int amba_focus_timer_irq = 0;
static int amba_iris_timer_irq = 0;

static int amba_lens_gpio_num = 0;
static int * amba_lens_gpio_id = NULL;

static const char * amba_lens_name = "amb_lens";
static int amba_lens_major = AMBA_DEV_MAJOR;
static int amba_lens_minor = (AMBA_DEV_MINOR_PUBLIC_START + 18);
static int amba_lens_registered = 0;
static struct cdev amba_lens_cdev;

static struct amba_lens_operations impl_fops = {
	.lens_dev_name = NULL,
	.lens_dev_ioctl = NULL,
	.lens_dev_open = NULL,
	.lens_dev_release = NULL
};

int amba_lens_exchange_resource(struct amba_lens_interrupts *lens_irq, struct amba_lens_gpios *lens_gpio)
{
	if(!lens_irq || !lens_gpio) {
		printk(KERN_ERR "Invalid resource for %s\n", amba_lens_name);
		return -1;
	}

	if(amba_lens_registered) {
		printk(KERN_ERR "%s already has device, others can't get resource\n", amba_lens_name);
		return -1;
	}

	if(lens_gpio->gpio_num) {
		amba_lens_gpio_id = kmalloc(lens_gpio->gpio_num * sizeof(int), GFP_KERNEL);
		if(!amba_lens_gpio_id) {
			printk(KERN_ERR "Kmalloc gpio failed for %s\n", amba_lens_name);
			return -1;
		}
		memcpy(amba_lens_gpio_id, lens_gpio->gpio_val, lens_gpio->gpio_num * sizeof(int));
		amba_lens_gpio_num = lens_gpio->gpio_num;
	}

	if(lens_irq->timer_freq)
		*lens_irq->timer_freq = amba_timer_input_freq;
	if(lens_irq->zoom_timer)
		*lens_irq->zoom_timer = amba_zoom_timer_irq;
	if(lens_irq->focus_timer)
		*lens_irq->focus_timer = amba_focus_timer_irq;
	if(lens_irq->iris_timer)
		*lens_irq->iris_timer = amba_iris_timer_irq;

	return 0;
}
EXPORT_SYMBOL(amba_lens_exchange_resource);

int amba_lens_register(struct amba_lens_operations *lens_fops)
{
	if((!lens_fops->lens_dev_name)||(!lens_fops->lens_dev_ioctl)||(!lens_fops->lens_dev_open)||(!lens_fops->lens_dev_release)) {
		printk(KERN_ERR "Invalid lens params, failed %s device register\n", amba_lens_name);
		return -1;
	}

	if(amba_lens_registered) {
		printk(KERN_ERR "%s already has device, failed %s register\n", amba_lens_name, lens_fops->lens_dev_name);
		return -1;
	}

	impl_fops.lens_dev_name = lens_fops->lens_dev_name;
	impl_fops.lens_dev_ioctl = lens_fops->lens_dev_ioctl;
	impl_fops.lens_dev_open = lens_fops->lens_dev_open;
	impl_fops.lens_dev_release = lens_fops->lens_dev_release;

	amba_lens_registered = 1;

	printk("%s registered on %s.\n", impl_fops.lens_dev_name, amba_lens_name);

	return 0;
}
EXPORT_SYMBOL(amba_lens_register);

void amba_lens_logoff(void)
{
	impl_fops.lens_dev_name = NULL;
	impl_fops.lens_dev_ioctl = NULL;
	impl_fops.lens_dev_open = NULL;
	impl_fops.lens_dev_release = NULL;

	if(amba_lens_gpio_id) {
		kfree(amba_lens_gpio_id);
		amba_lens_gpio_id = NULL;
	}
	amba_lens_gpio_num = 0;

	amba_lens_registered = 0;
}
EXPORT_SYMBOL(amba_lens_logoff);

static int request_gpio(int gpio_id)
{
	struct ambsvc_gpio gpio_svc;

	printk(KERN_DEBUG "Request GPIO %d\n", gpio_id);

	gpio_svc.svc_id = AMBSVC_GPIO_REQUEST;
	gpio_svc.gpio = GPIO(gpio_id);
	if(ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL))
		return -1;

	return 0;
}

static void release_gpio(int gpio_id)
{
	struct ambsvc_gpio gpio_svc;

    printk(KERN_DEBUG "Release GPIO %d\n", gpio_id);

	gpio_svc.svc_id = AMBSVC_GPIO_FREE;
	gpio_svc.gpio = GPIO(gpio_id);
	ambarella_request_service(AMBARELLA_SERVICE_GPIO, &gpio_svc, NULL);
}

static long amba_lens_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	if(!impl_fops.lens_dev_ioctl) {
		printk(KERN_ERR "No registered device, failed %s ioctl\n", amba_lens_name);
		return -1;
	}

	return impl_fops.lens_dev_ioctl(filp, cmd, arg);
}

static int amba_lens_open(struct inode *inode, struct file *filp)
{
	int index = 0;

	for(index=0; index<amba_lens_gpio_num; index++) {
		if(request_gpio(amba_lens_gpio_id[index])) {
			printk(KERN_ERR "Request gpio[%d] failed for %s\n", amba_lens_gpio_id[index], amba_lens_name);
			goto RELEASE_EXIT;
		}
	}

	if(!impl_fops.lens_dev_open) {
		printk(KERN_ERR "No registered device, failed %s open\n", amba_lens_name);
		goto RELEASE_EXIT;
	}

	if(impl_fops.lens_dev_open(inode, filp) < 0)
		goto RELEASE_EXIT;

	return 0;

RELEASE_EXIT:
	while(index > 0) {
		release_gpio(amba_lens_gpio_id[index-1]);
		index--;
	}

	return -1;
}

static int amba_lens_release(struct inode *inode, struct file *filp)
{
	int index, ret = 0;

	if(!impl_fops.lens_dev_release) {
		printk(KERN_ERR "No registered device, failed %s release\n", amba_lens_name);
		ret = -1;
	}
	else
		ret = impl_fops.lens_dev_release(inode, filp);

	for(index=0; index<amba_lens_gpio_num; index++)
		release_gpio(amba_lens_gpio_id[index]);

	return ret;
}

static struct file_operations amba_lens_fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = amba_lens_ioctl,
	.open = amba_lens_open,
	.release = amba_lens_release
};

static int ambarella_lens_probe(struct platform_device *pdev)
{
	dev_t dev_id;
	int rval;

	amba_timer_input_freq = clk_get_rate(clk_get(NULL, "gclk_apb"));
	if(!amba_timer_input_freq) {
		printk(KERN_ERR "failed to get timer input freq for %s.\n", amba_lens_name);
		return -1;
	}

	amba_zoom_timer_irq = platform_get_irq(pdev, 0);
	if(amba_zoom_timer_irq < 0) {
		printk(KERN_ERR "failed to get zoom_timer_irq for %s.\n", amba_lens_name);
		return amba_zoom_timer_irq;
	}

	amba_focus_timer_irq = platform_get_irq(pdev, 1);
	if(amba_focus_timer_irq < 0) {
		printk(KERN_ERR "failed to get focus_timer_irq for %s.\n", amba_lens_name);
		return amba_focus_timer_irq;
	}

	amba_iris_timer_irq = platform_get_irq(pdev, 2);
	if(amba_iris_timer_irq < 0) {
		printk(KERN_ERR "failed to get iris_timer_irq for %s.\n", amba_lens_name);
		return amba_iris_timer_irq;
	}

	amba_lens_logoff(); //clear off registered lens

	if (amba_lens_major) {
		dev_id = MKDEV(amba_lens_major, amba_lens_minor);
		rval = register_chrdev_region(dev_id, 1, amba_lens_name);
	} else {
		rval = alloc_chrdev_region(&dev_id, 0, 1, amba_lens_name);
		amba_lens_major = MAJOR(dev_id);
	}

	if (rval) {
		printk(KERN_ERR "failed to get dev region for %s.\n", amba_lens_name);
		return rval;
	}

	cdev_init(&amba_lens_cdev, &amba_lens_fops);
	amba_lens_cdev.owner = THIS_MODULE;
	rval = cdev_add(&amba_lens_cdev, dev_id, 1);
	if (rval) {
		printk(KERN_ERR "cdev_add failed for %s, error = %d.\n", amba_lens_name, rval);
		unregister_chrdev_region(dev_id, 1);
		return rval;
	}

	printk("%s_created %d:%d.\n", amba_lens_name, MAJOR(dev_id), MINOR(dev_id));

	return 0;
}

static int ambarella_lens_remove(struct platform_device *pdev)
{
	dev_t dev_id;

	cdev_del(&amba_lens_cdev);

	dev_id = MKDEV(amba_lens_major, amba_lens_minor);
	unregister_chrdev_region(dev_id, 1);

	return 0;
}

static const struct of_device_id ambarella_lens_of_match[] = {
	{.compatible = "ambarella,lens", },
	{},
};
MODULE_DEVICE_TABLE(of, ambarella_lens_of_match);

static struct platform_driver amba_lens_driver = {
	.probe		= ambarella_lens_probe,
	.remove		= ambarella_lens_remove,
	.driver = {
		.name	= "ambarella-lens",
		.of_match_table = ambarella_lens_of_match,
	},
};
module_platform_driver(amba_lens_driver);

MODULE_DESCRIPTION("Ambarella lens driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Peter Jiao <hgjiao@ambarella.com>");
MODULE_ALIAS("lens-driver");
