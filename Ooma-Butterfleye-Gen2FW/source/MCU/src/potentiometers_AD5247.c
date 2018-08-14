// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////
#include "em_i2c.h"
// #include "em_cmu.h"
// #include "em_emu.h"
#include "em_gpio.h"

#include "config.h"
#include "am_common.h"
#include "am_i2c.h"
#include "am_led.h"
#include "am_log.h"
#include "potentiometer.h"

#if defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)
// DEFINES ///////////////////////////////////////////////////////////////////

#define DPOT_CONF(features, wipers, max_pos, uid) \
        (((features) << 18) | (((wipers) & 0xFF) << 10) | \
        ((max_pos & 0xF) << 6) | (uid & 0x3F))

#define DPOT_UID(conf)      (conf & 0x3F)
#define DPOT_MAX_POS(conf)  ((conf >> 6) & 0xF)
#define DPOT_WIPERS(conf)   ((conf >> 10) & 0xFF)
#define DPOT_FEAT(conf)     (conf >> 18)
#define BRDAC0          (1 << 0)
#define BRDAC1          (1 << 1)
#define BRDAC2          (1 << 2)
#define BRDAC3          (1 << 3)
#define BRDAC4          (1 << 4)
#define BRDAC5          (1 << 5)
#define MAX_RDACS       6

#define F_CMD_INC       (1 << 0)    /* Features INC/DEC ALL, 6dB */
#define F_CMD_EEP       (1 << 1)    /* Features EEPROM */
#define F_CMD_OTP       (1 << 2)    /* Features OTP */
#define F_CMD_TOL       (1 << 3)    /* RDACS feature Tolerance REG */
#define F_RDACS_RW      (1 << 4)    /* RDACS are Read/Write  */
#define F_RDACS_WONLY       (1 << 5)    /* RDACS are Write only */
#define F_AD_APPDATA        (1 << 6)    /* RDAC Address append to data */
#define F_SPI_8BIT      (1 << 7)    /* All SPI XFERS are 8-bit */
#define F_SPI_16BIT     (1 << 8)    /* All SPI XFERS are 16-bit */
#define F_SPI_24BIT     (1 << 9)    /* All SPI XFERS are 24-bit */

#define F_RDACS_RW_TOL  (F_RDACS_RW | F_CMD_EEP | F_CMD_TOL)
#define F_RDACS_RW_EEP  (F_RDACS_RW | F_CMD_EEP)
#define F_SPI       (F_SPI_8BIT | F_SPI_16BIT | F_SPI_24BIT)

#define DPOT_RDAC0      0
#define DPOT_RDAC1      1
#define DPOT_RDAC2      2
#define DPOT_RDAC3      3
#define DPOT_RDAC4      4
#define DPOT_RDAC5      5

#define DPOT_RDAC_MASK      0x1F

#define DPOT_REG_TOL        0x18
#define DPOT_TOL_RDAC0      (DPOT_REG_TOL | DPOT_RDAC0)
#define DPOT_TOL_RDAC1      (DPOT_REG_TOL | DPOT_RDAC1)
#define DPOT_TOL_RDAC2      (DPOT_REG_TOL | DPOT_RDAC2)
#define DPOT_TOL_RDAC3      (DPOT_REG_TOL | DPOT_RDAC3)
#define DPOT_TOL_RDAC4      (DPOT_REG_TOL | DPOT_RDAC4)
#define DPOT_TOL_RDAC5      (DPOT_REG_TOL | DPOT_RDAC5)

/* RDAC-to-EEPROM Interface Commands */
#define DPOT_ADDR_RDAC      (0x0 << 5)
#define DPOT_ADDR_EEPROM    (0x1 << 5)
#define DPOT_ADDR_OTP       (0x1 << 6)
#define DPOT_ADDR_CMD       (0x1 << 7)
#define DPOT_ADDR_OTP_EN    (0x1 << 9)

#define DPOT_DEC_ALL_6DB    (DPOT_ADDR_CMD | (0x4 << 3))
#define DPOT_INC_ALL_6DB    (DPOT_ADDR_CMD | (0x9 << 3))
#define DPOT_DEC_ALL        (DPOT_ADDR_CMD | (0x6 << 3))
#define DPOT_INC_ALL        (DPOT_ADDR_CMD | (0xB << 3))


//
#define AD5247_ID DPOT_CONF(F_RDACS_RW, BRDAC0, 7, 38),

// Schematic says it is 0x53
// #define AD5247_I2C_SLAVE_ADDRESS 0x53
// Datasheet says it is 0x5C
#define AD5247_I2C_SLAVE_ADDRESS 0x5C

#define AD5247_MAX_LATCH 128

// TYPEDEFS, STRUCTS, ENUMS //////////////////////////////////////////////////

// GLOBAL DATA ///////////////////////////////////////////////////////////////

// PRIVATE VARIABLES /////////////////////////////////////////////////////////
uint8_t g_percentage;

// PRIVATE FUNCTION DECLARATIONS /////////////////////////////////////////////

static int AD5247_write_byte(uint8_t addr, uint8_t value)
{
    return i2c_write_byte(AD5247_I2C_SLAVE_ADDRESS, addr, value);
}

#if 0
static int AD5247_read_byte(uint8_t addr, uint8_t* p_value)
{
    return i2c_read_byte(AD5247_I2C_SLAVE_ADDRESS, addr, p_value);
}
#endif

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////

int potentiometer_init()
{
    LOG_DEBUG("%s \n", __FUNCTION__);
    return 0;
}

int potentiometer_test()
{
    // LOG_DEBUG("%s \n", __FUNCTION__);
    // Can not find it in its datasheet
    // But you can see the details from its driver:
    // https://git.kernel.org/pub/scm/linux/kernel/git/torvalds/linux.git/tree/drivers/misc/ad525x_dpot-i2c.c?id=HEAD
    uint8_t latch_addr_0 = DPOT_ADDR_RDAC | DPOT_RDAC0;
    uint8_t i;
    for (i = 0; i < AD5247_MAX_LATCH; i++)
    {
        if (0 != AD5247_write_byte(latch_addr_0, i))
        {
            LOG_DEBUG("%s AD5247_write_byte: 0x%x failed! \n", __FUNCTION__, latch_addr_0);
            return 1;
        }
        // LOG_DEBUG("AD5247_write_byte: 0x%x latch: 0x%x \n", latch_addr_0, i);
        Delay(20);
    }
    return 0;
}

uint8_t potentiometer_get()
{
    return g_percentage;
}

uint8_t potentiometer_set(uint8_t percentage)
{
    uint8_t latch_addr_0 = DPOT_ADDR_RDAC | DPOT_RDAC0;
    int latch = (AD5247_MAX_LATCH * percentage)/100;
    
    if (0 != AD5247_write_byte(latch_addr_0, latch))
    {
        LOG_DEBUG("%s AD5247_write_byte: 0x%x failed! \n", __FUNCTION__, latch_addr_0);
        return 1;
    }

    g_percentage = percentage;
    LOG_DEBUG("%s set AD5247 %d%% sensitive, latch value: %d \n", __FUNCTION__, percentage, latch);
    return 0;
}


#endif

// PRIVATE FUNCTION DEFINITIONS //////////////////////////////////////////////
