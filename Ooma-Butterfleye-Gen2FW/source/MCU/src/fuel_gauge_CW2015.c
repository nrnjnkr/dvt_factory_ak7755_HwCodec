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

#include "fuel_gauge.h"

#if defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)
// DEFINES ///////////////////////////////////////////////////////////////////

//
#define CW2015_I2C_SLAVE_ADDRESS 0xC4

#define CW2015_VERSION 0x6F

#define REG_VERSION     0x0
#define REG_VCELL_MSB   0x2
#define REG_VCELL_LSB   0x3
#define REG_SOC         0x4
#define REG_RRT_ALERT   0x6
#define REG_CONFIG      0x8
#define REG_MODE        0xA
#define REG_BATINFO     0x10

#define SIZE_BATINFO    64

#define MODE_SLEEP_MASK (0x3<<6)
#define MODE_SLEEP      (0x3<<6)
#define MODE_NORMAL     (0x0<<6)
#define MODE_QUICK_START (0x3<<4)
#define MODE_RESTART    (0xf<<0)

#define CONFIG_UPDATE_FLG (0x1<<1)
#define ATHD (0xa<<3)   //ATHD =10%

#define VCELL_RESOLUTION_MICROVOTES 305

// TYPEDEFS, STRUCTS, ENUMS //////////////////////////////////////////////////

// GLOBAL DATA ///////////////////////////////////////////////////////////////

// PRIVATE VARIABLES /////////////////////////////////////////////////////////

// PRIVATE FUNCTION DECLARATIONS /////////////////////////////////////////////

static int CW2015_write_byte(uint8_t addr, uint8_t value)
{
    return i2c_write_byte(CW2015_I2C_SLAVE_ADDRESS, addr, value);
}

static int CW2015_read_byte(uint8_t addr, uint8_t* p_value)
{
    return i2c_read_byte(CW2015_I2C_SLAVE_ADDRESS, addr, p_value);
}

static int cw2015_get_mvolts(void)
{
    // TODO: implement I2C word/streams read
    uint8_t voltage_msb;
    uint8_t voltage_lsb;
    if (0 != CW2015_read_byte(REG_VCELL_MSB, &voltage_msb))
    {
        LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_VCELL_MSB);
        return -1;
    }
    if (0 != CW2015_read_byte(REG_VCELL_LSB, &voltage_lsb))
    {
        LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_VCELL_LSB);
        return -1;
    }
    int32_t voltage = voltage_lsb | (voltage_msb << 8);
    // VCELL_RESOLUTION_MICROVOTES
    /* 1 voltage LSB is 305uV, ~312/1024mV */
    // voltage = value16 * 305 / 1000;
    voltage = voltage * 312 / 1024;
    return voltage;
}

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////

int fuel_gauge_init(void)
{
    LOG_DEBUG("%s \n", __FUNCTION__);
    uint8_t mode;
    if (0 != CW2015_read_byte(REG_MODE, &mode))
    {
        LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_MODE);
        // return -1;
    }
    if (0 != CW2015_write_byte(REG_MODE, (mode & 0x3F)))
    {
        LOG_DEBUG("%s CW2015_write_byte: 0x%x failed! \n", __FUNCTION__, REG_MODE);
        // return -1;
    }

    return 0;
}

int fuel_gauge_test(void)
{
    uint8_t chip_version;
    if (0 != CW2015_read_byte(REG_VERSION, &chip_version))
    {
        LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_VERSION);
        return -1;
    }
    if (CW2015_VERSION != chip_version)
    {
        LOG_DEBUG("WARNING: %s Chip version: 0x%x != CW2015_VERSION: 0x%x \n", __FUNCTION__, chip_version, CW2015_VERSION);
        // return -1;
    }
    else
    {
        LOG_DEBUG("%s Chip id: 0x%x == CW2015_VERSION: 0x%x \n", __FUNCTION__, chip_version, CW2015_VERSION);
    }
    int32_t volts = cw2015_get_mvolts();
    if (volts <= 0)
    {
        LOG_DEBUG("%s cw2015_get_mvolts failed!\n", __FUNCTION__);
        return -1;
    }
    else
    {
        LOG_DEBUG("%s battery voltage: %d mV \n", __FUNCTION__, volts);
    }

    {
        uint8_t alert;
        if (0 != CW2015_read_byte(REG_SOC, &alert))
        {
            LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_SOC);
            return -1;
        }
    }
    {
        uint8_t alert;
        if (0 != CW2015_read_byte(REG_RRT_ALERT, &alert))
        {
            LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_RRT_ALERT);
            return -1;
        }
    }
    {
        uint8_t alert;
        if (0 != CW2015_read_byte(REG_CONFIG, &alert))
        {
            LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_CONFIG);
            return -1;
        }
    }
    {
        uint8_t alert;
        if (0 != CW2015_read_byte(REG_MODE, &alert))
        {
            LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_MODE);
            return -1;
        }
    }
    {
        uint8_t alert;
        if (0 != CW2015_read_byte(REG_BATINFO, &alert))
        {
            LOG_DEBUG("%s CW2015_read_byte: 0x%x failed! \n", __FUNCTION__, REG_BATINFO);
            return -1;
        }
    }
    return 0;
}

int fuelgauge_probe()
{
    uint8_t mode;
    if (CW2015_read_byte(REG_MODE, &mode) != 0)
    {
        LOG_DEBUG("read mode failed");
        return -1;
    }

    // write 00 to Sleep to wake up device
    if (CW2015_write_byte(REG_MODE, mode & 0x3F) != 0)
    {
        LOG_DEBUG("wake up device failed");
        return -1;
    }

    return 0;
}

const char* fuelgauge_get_name()
{
    return "CW2015";
}

uint32_t fuelgauge_get_voltage_mV() {
    uint8_t volH, volL;
    uint32_t ret = 0;

    if (CW2015_read_byte(REG_VCELL_MSB, &volH) != 0
            || CW2015_read_byte(REG_VCELL_LSB, &volL) != 0)
    {
        LOG_DEBUG("read vcell failed");
        return 0;
    }

    ret |= volH << 8;
    ret |= volL;

    // 305uV resolution
    // Fraction('305/1000') == Fraction(61, 200)
    ret *= 61;
    ret /= 200;

    return ret;
}

uint32_t fuelgauge_get_full_capacity_mAh()
{
    // not accessable from this IC
    return 0;
}

uint32_t fuelgauge_get_remaining_capacity_mAh()
{
    // not accessable from this IC
    return 0;
}

uint32_t fuelgauge_get_state_of_charge_percent()
{
    // LSB ignored, only 1% precision is required
    uint8_t percent;
    if (CW2015_read_byte(REG_SOC, &percent) != 0)
    {
        return 0;
    }

    return percent;
}

uint32_t fuelgauge_get_state_of_health_percent()
{
    // not accessable from this IC
    return 0;
}

float fuelgauge_get_temperature_c()
{
    // not accessable from this IC
    return 0.0f;
}

int32_t fuelgauge_get_average_current_mA()
{
    // not accessable from this IC
    return 0;
}

#endif

// PRIVATE FUNCTION DEFINITIONS //////////////////////////////////////////////
