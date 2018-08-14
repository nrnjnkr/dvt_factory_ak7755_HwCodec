#include <osl.h>
#include <dhd_linux.h>
#include <linux/gpio.h>

#ifdef CONFIG_PLAT_AMBARELLA
#define WIFI_DRIVER_NAME "bcmdhd"
#include <config.h>
#include <plat/sd.h>
#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 9, 0)
#include <mach/board.h>
#endif

#ifndef GPIO
#define	GPIO(X)		(X)	/* 0 <= X <= (DAVINCI_N_GPIO - 1) */
#endif
#endif

#ifdef CUSTOMER_HW_PLATFORM
#include <plat/sdhci.h>
#define	sdmmc_channel	sdmmc_device_mmc0
#endif /* CUSTOMER_HW_PLATFORM */

#if defined(BUS_POWER_RESTORE) && defined(BCMSDIO)
#include <linux/mmc/core.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/sdio_func.h>
#endif /* defined(BUS_POWER_RESTORE) && defined(BCMSDIO) */

#ifdef CONFIG_DHD_USE_STATIC_BUF
extern void *dhd_wlan_mem_prealloc(int section, unsigned long size);
#endif /* CONFIG_DHD_USE_STATIC_BUF */

static int gpio_wl_reg_on = -1; // WL_REG_ON is input pin of WLAN module
#ifdef CUSTOMER_OOB
static int gpio_wl_host_wake = -1; // WL_HOST_WAKE is output pin of WLAN module
#endif

#define MAC_FILE "/tmp/config/etc/butterfleye/mac_addr.bin"
#define MAC_LENGTH 13
static int
dhd_wlan_set_power(bool on
#ifdef BUS_POWER_RESTORE
, wifi_adapter_info_t *adapter
#endif /* BUS_POWER_RESTORE */
)
{
	int err = 0;

	if (on) {
		printf("======== PULL WL_REG_ON(%d) HIGH! ========\n", gpio_wl_reg_on);
		if (gpio_wl_reg_on >= 0) {
#ifdef CONFIG_PLAT_AMBARELLA
			err = gpio_direction_output(GPIO(gpio_wl_reg_on), GPIO_BCM_WL_REG_ON_ACTIVE);
#else
			err = gpio_direction_output(gpio_wl_reg_on, 1);
#endif
			if (err) {
				printf("%s: WL_REG_ON didn't output high\n", __FUNCTION__);
				return -EIO;
			}
		}
#if defined(BUS_POWER_RESTORE)
#if defined(BCMSDIO)
		if (adapter->sdio_func && adapter->sdio_func->card && adapter->sdio_func->card->host) {
			printf("======== mmc_power_restore_host! ========\n");
			mmc_power_restore_host(adapter->sdio_func->card->host);
		}
#elif defined(BCMPCIE)
		OSL_SLEEP(50); /* delay needed to be able to restore PCIe configuration registers */
		if (adapter->pci_dev) {
			printf("======== pci_set_power_state PCI_D0! ========\n");
			pci_set_power_state(adapter->pci_dev, PCI_D0);
			if (adapter->pci_saved_state)
				pci_load_and_free_saved_state(adapter->pci_dev, &adapter->pci_saved_state);
			pci_restore_state(adapter->pci_dev);
			err = pci_enable_device(adapter->pci_dev);
			if (err < 0)
				printf("%s: PCI enable device failed", __FUNCTION__);
			pci_set_master(adapter->pci_dev);
		}
#endif /* BCMPCIE */
#endif /* BUS_POWER_RESTORE */
		/* Lets customer power to get stable */
		mdelay(100);
	} else {
#if defined(BUS_POWER_RESTORE)
#if defined(BCMSDIO)
		if (adapter->sdio_func && adapter->sdio_func->card && adapter->sdio_func->card->host) {
			printf("======== mmc_power_save_host! ========\n");
			mmc_power_save_host(adapter->sdio_func->card->host);
		}
#elif defined(BCMPCIE)
		if (adapter->pci_dev) {
			printf("======== pci_set_power_state PCI_D3hot! ========\n");
			pci_save_state(adapter->pci_dev);
			adapter->pci_saved_state = pci_store_saved_state(adapter->pci_dev);
			if (pci_is_enabled(adapter->pci_dev))
				pci_disable_device(adapter->pci_dev);
			pci_set_power_state(adapter->pci_dev, PCI_D3hot);
		}
#endif /* BCMPCIE */
#endif /* BUS_POWER_RESTORE */
		printf("======== PULL WL_REG_ON(%d) LOW! ========\n", gpio_wl_reg_on);
		if (gpio_wl_reg_on >= 0) {
#ifdef CONFIG_PLAT_AMBARELLA
			err = gpio_direction_output(GPIO(gpio_wl_reg_on), !GPIO_BCM_WL_REG_ON_ACTIVE);
#else
			err = gpio_direction_output(gpio_wl_reg_on, 0);
#endif
			if (err) {
				printf("%s: WL_REG_ON didn't output low\n", __FUNCTION__);
				return -EIO;
			}
		}
	}

	return err;
}

static int dhd_wlan_set_reset(int onoff)
{
	return 0;
}

static int dhd_wlan_set_carddetect(bool present)
{
	int err = 0;

#if !defined(BUS_POWER_RESTORE)
	if (present) {
#if defined(BCMSDIO)
		printf("======== Card detection to detect SDIO card! Slot.num[%d] ========\n",
			WIFI_CONN_SD_SLOT_NUM);
#ifdef CUSTOMER_HW_PLATFORM
		err = sdhci_force_presence_change(&sdmmc_channel, 1);
#endif /* CUSTOMER_HW_PLATFORM */
#ifdef CONFIG_PLAT_AMBARELLA
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
		if (WIFI_CONN_SD_SLOT_NUM >= 0 && WIFI_CONN_SD_SLOT_NUM < 5) {
			ambarella_detect_sd_slot(WIFI_CONN_SD_SLOT_NUM, 1);
		}
#else
		if (ambarella_board_generic.wifi_sd_bus >= 0 && ambarella_board_generic.wifi_sd_bus < 5) {
			ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
				ambarella_board_generic.wifi_sd_slot, 1);
		}
#endif
#endif
#elif defined(BCMPCIE)
		printf("======== Card detection to detect PCIE card! ========\n");
#endif
	} else {
#if defined(BCMSDIO)
		printf("======== Card detection to remove SDIO card! Slot.num[%d] ========\n",
			WIFI_CONN_SD_SLOT_NUM);
#ifdef CUSTOMER_HW_PLATFORM
		err = sdhci_force_presence_change(&sdmmc_channel, 0);
#endif /* CUSTOMER_HW_PLATFORM */
#ifdef CONFIG_PLAT_AMBARELLA
#if LINUX_VERSION_CODE > KERNEL_VERSION(3, 9, 0)
		if (WIFI_CONN_SD_SLOT_NUM >= 0 && WIFI_CONN_SD_SLOT_NUM < 5) {
			ambarella_detect_sd_slot(WIFI_CONN_SD_SLOT_NUM, 0);
		}
#else
		if (ambarella_board_generic.wifi_sd_bus >= 0 && ambarella_board_generic.wifi_sd_bus < 5) {
			ambarella_detect_sd_slot(ambarella_board_generic.wifi_sd_bus,
				ambarella_board_generic.wifi_sd_slot, 0);
		}
#endif
#endif
#elif defined(BCMPCIE)
		printf("======== Card detection to remove PCIE card! ========\n");
#endif
	}
#endif /* BUS_POWER_RESTORE */

	return err;
}

int ConvertChar2Number(char c) {
    switch(c) {
        case '0': return 0;
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case 'a': return 0xA;
        case 'A': return 0xA;
        case 'b': return 0xB;
        case 'B': return 0xB;
        case 'c': return 0xC;
        case 'C': return 0xC;
        case 'd': return 0xD;
        case 'D': return 0xD;
        case 'e': return 0xE;
        case 'E': return 0xE;
        case 'f': return 0xF;
        case 'F': return 0xF;
    }
}
static int read_mac_from_file(char *buf) {
    struct file *filep;
    int ret;
    mm_segment_t fs;
    loff_t pos;
    fs = get_fs();
    set_fs(KERNEL_DS);

    filep = filp_open(MAC_FILE, O_RDONLY, 0);
    if (IS_ERR(filep)) {
        printf("Unable to open file %s\n", MAC_FILE);
        return 0;
    }
    pos = 0;
    vfs_read(filep, buf, MAC_LENGTH-1, &pos);
    filp_close(filep, NULL);
    return 1;
}

static int dhd_wlan_get_mac_addr(unsigned char *buf)
{
	int err = 0;
    int fd = -1;
    char buff[MAC_LENGTH] = {0};
    struct ether_addr ea_example = {{0x00, 0x11, 0x22, 0x33, 0x44, 0xFF}};

#ifdef EXAMPLE_GET_MAC
    bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
#else
	/* EXAMPLE code */
	{
        if(read_mac_from_file(buff)) {
            char str[2];
            int n = 0, i  = 0;
            memcpy(str, buff, 2);
            for (i = 0; i < 6; i++) {
                memset(str, 0, 2);
                memcpy(str, buff+i*2, 2);
                /*
                 * Mac validation done in factory tool only.
                 */
                // Fitting two integer digit into single byte as nibles.
                ea_example.octet[i] = ConvertChar2Number(str[0])<< 4;
                ea_example.octet[i] = ea_example.octet[i] | ConvertChar2Number(str[1]);
		        bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
            }
            printf("Mac %02x:%02x:%02x:%02x:%02x:%02x buff\n", buf[0],
                    buf[1], buf[2], buf[3], buf[4], buf[5]);
        } else {
		    bcopy((char *)&ea_example, buf, sizeof(struct ether_addr));
        }
	}
#endif /* EXAMPLE_GET_MAC */
#ifdef EXAMPLE_GET_MAC_VER2
	/* EXAMPLE code */
	{
		char mac[6] = {0x00,0x11,0x22,0x33,0x44,0xFF};
		char macpad[56]= {
		0x00,0xaa,0x9c,0x84,0xc7,0xbc,0x9b,0xf6,
		0x02,0x33,0xa9,0x4d,0x5c,0xb4,0x0a,0x5d,
		0xa8,0xef,0xb0,0xcf,0x8e,0xbf,0x24,0x8a,
		0x87,0x0f,0x6f,0x0d,0xeb,0x83,0x6a,0x70,
		0x4a,0xeb,0xf6,0xe6,0x3c,0xe7,0x5f,0xfc,
		0x0e,0xa7,0xb3,0x0f,0x00,0xe4,0x4a,0xaf,
		0x87,0x08,0x16,0x6d,0x3a,0xe3,0xc7,0x80};
		bcopy(mac, buf, sizeof(mac));
		bcopy(macpad, buf+6, sizeof(macpad));
	}
#endif /* EXAMPLE_GET_MAC_VER2 */

	return err;
}

#if !defined(WL_WIRELESS_EXT)
struct cntry_locales_custom {
	char iso_abbrev[WLC_CNTRY_BUF_SZ];	/* ISO 3166-1 country abbreviation */
	char custom_locale[WLC_CNTRY_BUF_SZ];	/* Custom firmware locale */
	int32 custom_locale_rev;		/* Custom local revisin default -1 */
};
#endif

static struct cntry_locales_custom brcm_wlan_translate_custom_table[] = {
	/* Table should be filled out based on custom platform regulatory requirement */
	{"",   "XT", 49},  /* Universal if Country code is unknown or empty */
	{"US", "US", 0},
};

#ifdef CUSTOM_FORCE_NODFS_FLAG
struct cntry_locales_custom brcm_wlan_translate_nodfs_table[] = {
	{"",   "XT", 50},  /* Universal if Country code is unknown or empty */
	{"US", "US", 0},
};
#endif

static void *dhd_wlan_get_country_code(char *ccode
#ifdef CUSTOM_FORCE_NODFS_FLAG
	, u32 flags
#endif
)
{
	struct cntry_locales_custom *locales;
	int size;
	int i;

	if (!ccode)
		return NULL;

#ifdef CUSTOM_FORCE_NODFS_FLAG
	if (flags & WLAN_PLAT_NODFS_FLAG) {
		locales = brcm_wlan_translate_nodfs_table;
		size = ARRAY_SIZE(brcm_wlan_translate_nodfs_table);
	} else {
#endif
		locales = brcm_wlan_translate_custom_table;
		size = ARRAY_SIZE(brcm_wlan_translate_custom_table);
#ifdef CUSTOM_FORCE_NODFS_FLAG
	}
#endif

	for (i = 0; i < size; i++)
		if (strcmp(ccode, locales[i].iso_abbrev) == 0)
			return &locales[i];
	return NULL;
}

struct resource dhd_wlan_resources[] = {
	[0] = {
		.name	= "bcmdhd_wlan_irq",
		.start	= 0, /* Dummy */
		.end	= 0, /* Dummy */
		.flags	= IORESOURCE_IRQ | IORESOURCE_IRQ_SHAREABLE
			| IORESOURCE_IRQ_HIGHLEVEL, /* Dummy */
	},
};

struct wifi_platform_data dhd_wlan_control = {
	.set_power	= dhd_wlan_set_power,
	.set_reset	= dhd_wlan_set_reset,
	.set_carddetect	= dhd_wlan_set_carddetect,
	.get_mac_addr	= dhd_wlan_get_mac_addr,
#ifdef CONFIG_DHD_USE_STATIC_BUF
	.mem_prealloc	= dhd_wlan_mem_prealloc,
#endif /* CONFIG_DHD_USE_STATIC_BUF */
	.get_country_code = dhd_wlan_get_country_code,
};

int dhd_wlan_init_gpio(void)
{
	int err = 0;
#ifdef CUSTOMER_OOB
	int host_oob_irq = -1;
	uint host_oob_irq_flags = 0;
#endif

	/* Please check your schematic and fill right GPIO number which connected to
	* WL_REG_ON and WL_HOST_WAKE.
	*/
#ifdef CONFIG_PLAT_AMBARELLA
	gpio_wl_reg_on = GPIO_BCM_WL_REG_ON;
#ifdef CUSTOMER_OOB
	gpio_wl_host_wake = GPIO_BCM_WL_HOST_WAKE;
#endif
#endif

	printf("%s: GPIO(WL_REG_ON) = %d\n", __FUNCTION__, gpio_wl_reg_on);
	if (gpio_wl_reg_on >= 0) {
#ifdef CONFIG_PLAT_AMBARELLA
		err = gpio_request(GPIO(gpio_wl_reg_on), WIFI_DRIVER_NAME);
#else
		err = gpio_request(gpio_wl_reg_on, "WL_REG_ON");
#endif
		if (err < 0) {
			printf("%s: Faiiled to request gpio %d for WL_REG_ON\n",
				__FUNCTION__, gpio_wl_reg_on);
			gpio_wl_reg_on = -1;
		}
	}

#ifdef CUSTOMER_OOB
	printf("%s: GPIO(WL_HOST_WAKE) = %d\n", __FUNCTION__, gpio_wl_host_wake);
	if (gpio_wl_host_wake >= 0) {
#ifdef CONFIG_PLAT_AMBARELLA
		err = gpio_request(GPIO(gpio_wl_host_wake), WIFI_DRIVER_NAME);
#else
		err = gpio_request(gpio_wl_host_wake, "bcmdhd");
#endif
		if (err < 0) {
			printf("%s: gpio_request failed\n", __FUNCTION__);
			return -1;
		}
#ifdef CONFIG_PLAT_AMBARELLA
		err = gpio_direction_input(GPIO(gpio_wl_host_wake));
#else
		err = gpio_direction_input(gpio_wl_host_wake);
#endif
		if (err < 0) {
			printf("%s: gpio_direction_input failed\n", __FUNCTION__);
			gpio_free(gpio_wl_host_wake);
			return -1;
		}
#ifdef CONFIG_PLAT_AMBARELLA
		host_oob_irq = gpio_to_irq(GPIO(gpio_wl_host_wake));
#else
		host_oob_irq = gpio_to_irq(gpio_wl_host_wake);
#endif
		if (host_oob_irq < 0) {
			printf("%s: gpio_to_irq failed\n", __FUNCTION__);
			gpio_free(gpio_wl_host_wake);
			return -1;
		}
	}
	printf("%s: host_oob_irq: %d\n", __FUNCTION__, host_oob_irq);

#ifdef HW_OOB
#ifdef HW_OOB_LOW_LEVEL
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_LOWLEVEL | IORESOURCE_IRQ_SHAREABLE;
#else
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHLEVEL | IORESOURCE_IRQ_SHAREABLE;
#endif
#else
	host_oob_irq_flags = IORESOURCE_IRQ | IORESOURCE_IRQ_HIGHEDGE | IORESOURCE_IRQ_SHAREABLE;
#endif

	dhd_wlan_resources[0].start = dhd_wlan_resources[0].end = host_oob_irq;
	dhd_wlan_resources[0].flags = host_oob_irq_flags;
	printf("%s: host_oob_irq_flags=0x%x\n", __FUNCTION__, host_oob_irq_flags);
#endif /* CUSTOMER_OOB */

	return 0;
}

static void dhd_wlan_deinit_gpio(void)
{
	if (gpio_wl_reg_on >= 0) {
		printf("%s: gpio_free(WL_REG_ON %d)\n", __FUNCTION__, gpio_wl_reg_on);
#ifdef CONFIG_PLAT_AMBARELLA
		gpio_free(GPIO(gpio_wl_reg_on));
#else
		gpio_free(gpio_wl_reg_on);
#endif
		gpio_wl_reg_on = -1;
	}
#ifdef CUSTOMER_OOB
	if (gpio_wl_host_wake >= 0) {
		printf("%s: gpio_free(WL_HOST_WAKE %d)\n", __FUNCTION__, gpio_wl_host_wake);
#ifdef CONFIG_PLAT_AMBARELLA
		gpio_free(GPIO(gpio_wl_host_wake));
#else
		gpio_free(gpio_wl_host_wake);
#endif
		gpio_wl_host_wake = -1;
	}
#endif /* CUSTOMER_OOB */
}

int dhd_wlan_init_plat_data(void)
{
	int err = 0;

	printf("======== %s ========\n", __FUNCTION__);
	err = dhd_wlan_init_gpio();
	return err;
}

void dhd_wlan_deinit_plat_data(wifi_adapter_info_t *adapter)
{
	printf("======== %s ========\n", __FUNCTION__);
	dhd_wlan_deinit_gpio();
}

