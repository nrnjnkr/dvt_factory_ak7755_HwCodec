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

#if defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)
// DEFINES ///////////////////////////////////////////////////////////////////

//
// #define BQ24250_I2C_SLAVE_ADDRESS 0x6A
//#define BQ24250_I2C_SLAVE_ADDRESS 0xD4
// I2C slave address changed for ooma board
#define BQ24250_I2C_SLAVE_ADDRESS 0xC4

/* Registers */
#define BQ24250_STATUS_CONTROL_REG  0x00
#define BQ24250_CONTROL_REG     0x01
#define BQ24250_BATTERY_VOLTAGE_REG 0x02
#define BQ24250_TERM_CURR_CHARG_REG 0x03
#define BQ24250_VIN_DPM_REG     0x04
#define BQ24250_TIMER_NTC_MONITOR_REG   0x05
#define BQ24250_VOVP_FRC_BATDET_PTM_REG 0x06

/* STATUS_CONTROL Reg */
#define BQ24250_WD_FAULT    BIT(7)
#define BQ24250_WD_EN       BIT(6)
#define BQ24250_STATUS      (BIT(5) | BIT(4))
#define BQ24250_STATUS_MASK 0x03
#define BQ24250_STATUS_SHIFT    0x4
#define BQ24250_FAULT       (BIT(3) | BIT(2) | BIT(1) | BIT(0))
#define BQ24250_FAULT_MASK  0x0F

/* CONTROL Reg */
#define BQ24250_RESET       BIT(7)
#define BQ24250_IIN_LIMIT   (BIT(6) | BIT(5) | BIT(4))
#define BQ24250_EN_STAT     BIT(3)
#define BQ24250_TERM_EN     BIT(2)
#define BQ24250_CE      BIT(1)
#define BQ24250_HZ_MODE     BIT(0)

/* BATTERY_VOLTAGE Reg */
#define BQ24250_BATTERY_VOLTAGE     \
            (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3) | BIT(2))
#define BQ24250_BATTERY_VOLTAGE_MASK    0xFC
#define BQ24250_BATTERY_VOLTAGE_SHIFT   0x2
#define BQ24250_USB_DET     (BIT(1) | BIT(0))
#define BQ24250_USB_DET_MASK    0x03

/* CURR_TERM_CHARG Reg */
#define BQ24250_CHARGE_CURR (BIT(7) | BIT(6) | BIT(5) | BIT(4) | BIT(3))
#define BQ24250_CHARGE_CURR_MASK    0x1f
#define BQ24250_CHARGE_CURR_SHIFT   0x3
#define BQ24250_TERM_CURR   (BIT(2) | BIT(1) | BIT(0))
#define BQ24250_TERM_CURR_MASK  0x7

/* VIN_DPM Reg */
#define BQ24250_LOOP_STATUS (BIT(7) | BIT(6))
#define BQ24250_LOOP_STATUS_SHIFT   0x5
#define BQ24250_LOW_CHG     BIT(5)
#define BQ24250_DPDM_EN     BIT(4)
#define BQ24250_CS_STATUS   BIT(3)
#define BQ24250_VIN_DPM     (BIT(2) | BIT(1) | BIT(0))

/* TIMER_NTC_MONITOR Reg */
#define BQ24250_2X_TMR_EM   BIT(7)
#define BQ24250_TMR     (BIT(6) | BIT(5))
#define BQ24250_TMR_SHIFT   0x5
#define BQ24250_TMR_MASK    0x3
#define BQ24250_TMR_DISABLE 0x3
#define BQ24250_SYSOFF      BIT(4)
#define BQ24250_TS_EN       BIT(3)
#define BQ24250_TS_FAULT    (BIT(2) | BIT(1) | BIT(0))
#define BQ24250_TS_MASK     0x7

/* Vovp_FRC_BATDET_PTM_REG */
#define BQ24250_VOVP        (BIT(7) | BIT(6) | BIT(5))
#define BQ24250_VOVP_SHIFT  0x5
#define BQ24250_CLR_VDP     BIT(4)
#define BQ24250_FRC_BATDET  BIT(3)
#define BQ24250_FRC_PTM     BIT(2)

#define BQ24250_MAX_REGISTER    0x6


// TYPEDEFS, STRUCTS, ENUMS //////////////////////////////////////////////////
#if 0
static struct reg_default bq24250_reg_defs[] = {
    {0, 0x00},
    {1, 0x2C},
    {2, 0x8F},
    {3, 0x00},
    {4, 0x06},
    {5, 0xA8},
    {6, 0xD0},
};
#endif

enum bq24250_charge_status {
    BQ24250_CHARGE_READY = 0,
    BQ24250_CHARGE_IN_PROGRESS,
    BQ24250_CHARGE_DONE,
    BQ24250_CHARGE_FAULT,
    BQ24250_CHARGE_MAX
};

enum bq24250_charge_fault {
    BQ24250_NORMAL = 0,
    BQ24250_INPUT_OVP,
    BQ24250_INPUT_UVLO,
    BQ24250_SLEEP,
    BQ24250_BATTERY_TS_FAULT,
    BQ24250_BATTERY_OVP,
    BQ24250_THERMAL_SHUTDOWN,
    BQ24250_TIMER_FAULT,
    BQ24250_NO_BATTERY_CONNECTED,
    BQ24250_ISET_SHORT,
    BQ24250_LDO_SHORT,
    BQ24250_MAX_CHG_FAULT,
};

static const char *desc[BQ24250_MAX_CHG_FAULT] = {
    [BQ24250_NORMAL] = "NORMAL",
    [BQ24250_INPUT_OVP] = "INPUT OVP",
    [BQ24250_INPUT_UVLO] = "INPUT UVLO",
    [BQ24250_SLEEP] = "SLEEP",
    [BQ24250_BATTERY_TS_FAULT] = "BATTERY TS FAULT",
    [BQ24250_BATTERY_OVP] = "BATTERY OVP",
    [BQ24250_THERMAL_SHUTDOWN] = "THERMAL SHUTDOWN",
    [BQ24250_TIMER_FAULT] = "TIMER FAULT",
    [BQ24250_NO_BATTERY_CONNECTED] = "NO BATTERY CONNECTED",
    [BQ24250_ISET_SHORT] = "ISET SHORT",
    [BQ24250_LDO_SHORT] = "LDO SHORT",
};

static const char *charge_status_str[BQ24250_CHARGE_MAX] = {
    [BQ24250_CHARGE_READY] = "BQ24250_CHARGE_READY",
    [BQ24250_CHARGE_IN_PROGRESS] = "BQ24250_CHARGE_IN_PROGRESS",
    [BQ24250_CHARGE_DONE] = "BQ24250_CHARGE_DONE",
    [BQ24250_CHARGE_FAULT] = "BQ24250_CHARGE_FAULT",
};


#define BQ24250_DCP_DETECTED    0   /* dedicated charger */
#define BQ24250_CDP_DETECTED    1   /* charging downstream */
#define BQ24250_SDP_DETECTED    2   /* standard downstream */
#define BQ24250_ACA_DETECTED    4   /* non-standard adaptor */


// GLOBAL DATA ///////////////////////////////////////////////////////////////

// PRIVATE VARIABLES /////////////////////////////////////////////////////////
/* Charge current in mA */
static const unsigned int cc_tbl[] = {
    500,
    550,
    600,
    650,
    700,
    750,
    800,
    850,
    900,
    950,
    1050,
    1100,
    1150,
    1200,
    1250,
    1300,
    1350,
    1400,
    1450,
    1500,
    1550,
    1600,
    1650,
    1700,
    1750,
    1800,
    1850,
    1900,
    1950,
    2000,
    0000,
};

/* Termination current in mA */
static const unsigned int tc_tbl[] = {
    50,
    75,
    100,
    125,
    150,
    175,
    200,
    225,
};

/* Input current limit in mA */
static const unsigned int icl_tbl[] = {
    100,
    150,
    500,
    900,
    1500,
    2000,
    0000, /* For External ILIM current limit */
    1111, /* No input current limit with internal clamp at 3A (PTM MODE) */

};

/* Battery Regulation Voltage in mV */
static const unsigned int brv_tbl[] = {
    0,
    20,
    40,
    60,
    80,
    100,
    120,
    140,
    160,
    180,
    200,
    220,
    240,
    260,
    280,
    300,
    320,
    340,
    360,
    380,
    400,
    420,
    440,
    460,
    480,
    500,
    520,
    540,
    560,
    580,
    600,
    620,
    640,
    660,
    680,
    700,
    720,
    740,
    760,
    780,
    800,
    820,
    840,
    860,
    880,
    900,
    920,
    940,
};

/* Safety timer time limit in minutes */
static const unsigned int stl_tbl[] = {
    45,
    360,
    540,
    0,
};

/* OVP voltage in mV */
static const unsigned int ovp_tbl[] = {
    6000,
    6500,
    7000,
    7500,
    8000,
    8500,
    9000,
    9500,
    10000,
    10500,
};

// PRIVATE FUNCTION DECLARATIONS /////////////////////////////////////////////

#if 0
static int BQ24250_write_byte(uint8_t addr, uint8_t value)
{
    return i2c_write_byte(BQ24250_I2C_SLAVE_ADDRESS, addr, value);
}
#endif

static int BQ24250_read_byte(uint8_t addr, uint8_t* p_value)
{
    return i2c_read_byte(BQ24250_I2C_SLAVE_ADDRESS, addr, p_value);
}


# if 0

/* Convert register value to current using lookup table */
static int hw_to_current(const unsigned int *tbl, size_t size, unsigned int val)
{
    if (val >= size)
        return -EINVAL;
    return tbl[val];
}

/* Convert current to register value using lookup table */
static int current_to_hw(const unsigned int *tbl, size_t size, unsigned int val)
{
    size_t i;

    for (i = 0; i < size; i++)
        if (val < tbl[i])
            break;
    return i > 0 ? i - 1 : -EINVAL;
}

/**
 * bq24250_update_ps_status - refreshes the power source status
 * @bq24250: pointer to bq24250 charger instance
 *
 * Function checks whether any power source is connected to the charger and
 * updates internal state accordingly. If there is a change to previous state
 * function returns %1, otherwise %0 and negative errno in case of errror.
 */
static int bq24250_update_ps_status(struct bq24250_charger *bq24250)
{
    bool usb = false;
    bool dc = false;
    unsigned int val;
    int ret;

    ret = regmap_read(bq24250->regmap, BQ24250_VIN_DPM_REG, &val);
    if (ret < 0)
        return ret;

    /*
     * DC and usb are set depending on whether they are enabled in
     * platform data _and_ whether corresponding undervoltage is set.
     */
    if (bq24250->pdata->use_mains)
        dc = !(val & BQ24250_DPDM_EN);
    if (bq24250->pdata->use_usb)
        usb = !(val & BQ24250_CS_STATUS);

    ret = bq24250->mains_online != dc || bq24250->usb_online != usb;
    bq24250->mains_online = dc;
    bq24250->usb_online = usb;

    return 0;
}

enum power_supply_property bq24250_charger_properties[] = {
    POWER_SUPPLY_PROP_ONLINE,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
    POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
};

static int bq24250_update_usb_type(struct bq24250_charger *bq24250)
{
    int ret;
    unsigned int type;

    ret = regmap_read(bq24250->regmap, BQ24250_BATTERY_VOLTAGE_REG, &type);

    if (ret < 0)
        return ret;

    type &= BQ24250_USB_DET_MASK;

    switch (type) {
    case BQ24250_DCP_DETECTED:
        bq24250->usb.type = POWER_SUPPLY_TYPE_USB_DCP;
        break;
    case BQ24250_CDP_DETECTED:
        bq24250->usb.type = POWER_SUPPLY_TYPE_USB_CDP;
        break;
    case BQ24250_SDP_DETECTED:
        bq24250->usb.type = POWER_SUPPLY_TYPE_USB;
        break;
    case BQ24250_ACA_DETECTED:
        bq24250->usb.type = POWER_SUPPLY_TYPE_USB_ACA;
        break;
    default:
        bq24250->usb.type = POWER_SUPPLY_TYPE_UNKNOWN;
        break;
    }

    return 0;
}


/*
 * bq24250_is_ps_online - returns whether input power source is connected
 * @bq24250: pointer to bq24250 charger instance
 *
 * Returns %true if input power source is connected. Note that this is
 * dependent on what platform has configured for usable power sources. For
 * example if USB is disabled, this will return %false even if the USB cable
 * is connected.
 */
static bool bq24250_is_ps_online(struct bq24250_charger *bq24250)
{
    return bq24250->usb_online || bq24250->mains_online;
}

#endif

/**
 * bq24250_charging_status - returns status of charging
 *
 * Function returns charging status.
 * %0 means ready, %1 charging is in progress,
 * %2charging done and %3 fault.
 */
static int bq24250_charging_status()
{
    uint8_t val;
    int ret;

    ret = BQ24250_read_byte(BQ24250_STATUS_CONTROL_REG, &val);
    if (ret < 0)
    {
        LOG_DEBUG("BQ24250_read_byte failed!\n");
        return ret;
    }

    val >>= BQ24250_STATUS_SHIFT;
    val &= BQ24250_STATUS_MASK;

    return val;
}

#if 0
static int bq24250_vsysoff_set(struct bq24250_charger *bq24250, bool enable)
{
    int ret = 0;

    if (bq24250->pdata->enable_control != BQ24250_CE) {
        dev_dbg(bq24250->dev, "vsys enable/disable in SW disabled\n");
        return 0;
    }

    if (bq24250->vsysoff_enabled != enable) {
        ret = regmap_update_bits(bq24250->regmap,
                     BQ24250_TIMER_NTC_MONITOR_REG,
                     BQ24250_SYSOFF, enable ? 0 : 1);
        if (!ret)
            bq24250->vsysoff_enabled = enable;
    }
    return ret;
}

static inline int bq24250_vsystem_enable(struct bq24250_charger *bq24250)
{
    return bq24250_vsysoff_set(bq24250, false);
}

static inline int bq24250_vsystem_disable(struct bq24250_charger *bq24250)
{
    return bq24250_vsysoff_set(bq24250, true);
}

static int bq24250_timer_suspend(struct bq24250_charger *bq24250)
{
    int ret = 0;

    if (bq24250->pdata->enable_control != BQ24250_CE) {
        dev_dbg(bq24250->dev, "vsys enable/disable in SW disabled\n");
        return 0;
    }

    ret = regmap_update_bits(bq24250->regmap, BQ24250_TIMER_NTC_MONITOR_REG,
                 BQ24250_TMR, BQ24250_TMR_DISABLE);
    if (!ret)
        dev_err(bq24250->dev, "failed to suspend timer\n");

    return ret;
}

static int bq24250_timer_reset(struct bq24250_charger *bq24250)
{
    int ret;
    unsigned int tmr;

    if (bq24250->pdata->enable_control != BQ24250_CE) {
        dev_dbg(bq24250->dev, "vsys enable/disable in SW disabled\n");
        return 0;
    }

    /* Reset with previous time value, when fault occur */
    ret = regmap_read(bq24250->regmap, BQ24250_TIMER_NTC_MONITOR_REG, &tmr);

    tmr >>= BQ24250_TMR_SHIFT;
    tmr &= BQ24250_TMR_MASK;

    ret = regmap_update_bits(bq24250->regmap, BQ24250_TIMER_NTC_MONITOR_REG,
                 BQ24250_TMR, tmr);
    if (!ret)
        dev_err(bq24250->dev, "failed to reset timer\n");

    return ret;

}

static int bq24250_charging_set(struct bq24250_charger *bq24250, bool enable)
{
    int ret = 0;

    if (bq24250->pdata->enable_control != BQ24250_CE) {
        dev_dbg(bq24250->dev, "charging enable/disable in SW disabled\n");
        return 0;
    }

    if (bq24250->charging_enabled != enable) {
        ret = regmap_update_bits(bq24250->regmap, BQ24250_CONTROL_REG,
                     BQ24250_CE, enable ? 0 : 1);
        if (!ret)
            bq24250->charging_enabled = enable;
    }
    return ret;
}

static inline int bq24250_charging_enable(struct bq24250_charger *bq24250)
{
    return bq24250_charging_set(bq24250, true);
}

static inline int bq24250_charging_disable(struct bq24250_charger *bq24250)
{
    return bq24250_charging_set(bq24250, false);
}

static int bq24250_start_stop_charging(struct bq24250_charger *bq24250)
{
    int ret;

    /*
     * Depending on whether valid power source is connected or not, we
     * disable or enable the charging. We do it manually because it
     * depends on how the platform has configured the valid inputs.
     */
    if (bq24250_is_ps_online(bq24250)) {
        ret = bq24250_charging_enable(bq24250);
        if (ret < 0)
            dev_err(bq24250->dev, "failed to enable charging\n");
    } else {
        ret = bq24250_charging_disable(bq24250);
        if (ret < 0)
            dev_err(bq24250->dev, "failed to disable charging\n");
    }

    return ret;
}

static int bq24250_set_charge_current(struct bq24250_charger *bq24250)
{
    int ret;

    if (bq24250->pdata->max_charge_current) {
        ret = current_to_hw(cc_tbl, ARRAY_SIZE(cc_tbl),
                    bq24250->pdata->max_charge_current);

        if (ret < 0)
            return ret;

        ret = regmap_update_bits(bq24250->regmap,
                     BQ24250_TERM_CURR_CHARG_REG,
                     BQ24250_CHARGE_CURR, ret);

        if (ret < 0)
            return ret;
    }

    if (bq24250->pdata->termination_current) {
        ret = current_to_hw(tc_tbl, ARRAY_SIZE(tc_tbl),
                    bq24250->pdata->termination_current);
        if (ret < 0)
            return ret;

        ret = regmap_update_bits(bq24250->regmap,
                     BQ24250_TERM_CURR_CHARG_REG,
                     BQ24250_TERM_CURR, ret);

        if (ret < 0)
            return ret;
    }

    return 0;
}

static int bq24250_set_current_limits(struct bq24250_charger *bq24250)
{
    int ret;

    if (bq24250->pdata->mains_current_limit) {
        ret = current_to_hw(icl_tbl, ARRAY_SIZE(icl_tbl),
                    bq24250->pdata->mains_current_limit);
        if (ret < 0)
            return ret;

        ret = regmap_update_bits(bq24250->regmap, BQ24250_CONTROL_REG,
                     BQ24250_IIN_LIMIT, ret);
        if (ret < 0)
            return ret;
    }

    if (bq24250->pdata->usb_hc_current_limit) {
        ret = current_to_hw(icl_tbl, ARRAY_SIZE(icl_tbl),
                    bq24250->pdata->usb_hc_current_limit);
        if (ret < 0)
            return ret;

        ret = regmap_update_bits(bq24250->regmap, BQ24250_CONTROL_REG,
                     BQ24250_IIN_LIMIT, ret);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int bq24250_set_voltage_limits(struct bq24250_charger *bq24250)
{
    int ret;

    if (bq24250->pdata->max_charge_voltage) {
        ret = bq24250->pdata->max_charge_voltage;

        if (ret < 0)
            return ret;

        ret = regmap_update_bits(bq24250->regmap,
                     BQ24250_BATTERY_VOLTAGE_REG,
                     BQ24250_BATTERY_VOLTAGE, ret);
        if (ret < 0)
            return ret;
    }

    return 0;
}

static int bq24250_set_ovp_limits(struct bq24250_charger *bq24250)
{
    /* Set default - 6V as OVP */
    return regmap_update_bits(bq24250->regmap,
                  BQ24250_VOVP_FRC_BATDET_PTM_REG,
                  BQ24250_VOVP, 0);
}

static int bq24250_hw_init(struct bq24250_charger *bq24250)
{
    int ret;

    /*
     * Program the platform specific configuration values to the device
     * first.
     */
    ret = bq24250_set_charge_current(bq24250);
    if (ret < 0)
        goto fail;

    ret = bq24250_set_current_limits(bq24250);
    if (ret < 0)
        goto fail;

    ret = bq24250_set_voltage_limits(bq24250);
    if (ret < 0)
        goto fail;

    ret = bq24250_set_ovp_limits(bq24250);
    if (ret < 0)
        goto fail;

    ret = bq24250_update_ps_status(bq24250);
    if (ret < 0)
        goto fail;

    ret = bq24250_start_stop_charging(bq24250);

fail:
    return ret;
}

static irqreturn_t bq24250_interrupt(int irq, void *data)
{
    struct bq24250_charger *bq24250 = data;
    unsigned int irqstat;
    bool handled = false;
    int ret;

    dev_info(bq24250->dev, " In %s - Started\n", __func__);
    ret = regmap_read(bq24250->regmap, BQ24250_STATUS_CONTROL_REG,
              &irqstat);
    if (ret < 0) {
        dev_warn(bq24250->dev, "reading STATUS_CONTROL_REG failed\n");
        return IRQ_NONE;
    }

    irqstat &= BQ24250_FAULT_MASK;

    /*
     * If we get charger error we report the error back to user and
     * disable charging.
     */
    switch (irqstat)  {
    case BQ24250_NORMAL:    /* Normal  - No fault */
        handled = true;
        break;
    case BQ24250_INPUT_OVP: /* Input OVP */
    case BQ24250_SLEEP: /* SLEEP */
    case BQ24250_THERMAL_SHUTDOWN:  /* TS SHUTDOWN */
    case BQ24250_LDO_SHORT: /* LDO Short */
        dev_info(bq24250->dev,
            "error in charger, disabling charging with \
             \"%s\" fault\n", desc[irqstat]);
        bq24250_vsystem_disable(bq24250);
        bq24250_charging_disable(bq24250);
        bq24250_timer_suspend(bq24250);
        power_supply_changed(&bq24250->battery);
        handled = true;
        break;
    case BQ24250_INPUT_UVLO:    /* Input UVLO */
        /* If we got an under voltage interrupt it means that
            AC/USB input was connected or disconnected.  */
        dev_info(bq24250->dev,
            "error in charger, disabling charging with \
             \"%s\" fault\n", desc[irqstat]);
        bq24250_vsystem_disable(bq24250);
        bq24250_charging_disable(bq24250);
        bq24250_timer_reset(bq24250);
        if (bq24250->pdata->use_mains)
            power_supply_changed(&bq24250->mains);
        if (bq24250->pdata->use_usb)
            power_supply_changed(&bq24250->usb);
        handled = true;
        break;
    case BQ24250_BATTERY_TS_FAULT:  /* BAT TS FAULT */
        dev_info(bq24250->dev,
            "error in charger, disabling charging with \
             \"%s\" fault\n", desc[irqstat]);
        bq24250_vsystem_enable(bq24250);
        bq24250_charging_disable(bq24250);
        bq24250_timer_suspend(bq24250);
        if (bq24250->pdata->use_mains)
            power_supply_changed(&bq24250->mains);
        if (bq24250->pdata->use_usb)
            power_supply_changed(&bq24250->usb);
        handled = true;
        break;
    case BQ24250_BATTERY_OVP:   /* BAT OVP */
        dev_info(bq24250->dev,
            "error in charger, disabling charging with \
             \"%s\" fault\n", desc[irqstat]);
        bq24250_vsystem_disable(bq24250);
        bq24250_charging_disable(bq24250);
        bq24250_timer_suspend(bq24250);
        if (bq24250->pdata->use_mains)
            power_supply_changed(&bq24250->mains);
        if (bq24250->pdata->use_usb)
            power_supply_changed(&bq24250->usb);
        handled = true;
        break;
    case BQ24250_TIMER_FAULT:   /* Timer Fault */
        dev_info(bq24250->dev,
            "error in charger, disabling charging with \
             \"%s\" fault\n", desc[irqstat]);
        bq24250_vsystem_enable(bq24250);
        bq24250_charging_disable(bq24250);
        bq24250_timer_reset(bq24250);
        handled = true;
        break;
    case BQ24250_NO_BATTERY_CONNECTED:  /* No BAT Connected */
    case BQ24250_ISET_SHORT:    /* ISET Short */
        dev_info(bq24250->dev,
            "error in charger, disabling charging with \
             \"%s\" fault\n", desc[irqstat]);
        bq24250_vsystem_enable(bq24250);
        bq24250_charging_disable(bq24250);
        bq24250_timer_suspend(bq24250);
        if (bq24250->pdata->use_mains)
            power_supply_changed(&bq24250->mains);
        if (bq24250->pdata->use_usb)
            power_supply_changed(&bq24250->usb);
        handled = true;
        break;
    default:
        dev_err(bq24250->dev,
            "Charger Fault, disabling charging with \"%s\" unknown fault\n", desc[irqstat]);
        handled = false;
        break;
    }

    return handled ? IRQ_HANDLED : IRQ_NONE;
}

static int bq24250_irq_set(struct bq24250_charger *bq24250, bool enable)
{
    int ret;

    /*
    * Enable/disable interrupts for:
    *      - status chage using the STAT pin
    *      - charger fault error usign the INT pin
    */
    ret = regmap_update_bits(bq24250->regmap, BQ24250_STATUS_CONTROL_REG,
                 BQ24250_FAULT, enable ? BQ24250_FAULT : 0);
    if (ret < 0)
        goto fail;

    ret = regmap_update_bits(bq24250->regmap, BQ24250_CONTROL_REG,
                 BQ24250_EN_STAT, enable ? 1 : 0);

    if (ret < 0)
        goto fail;

fail:
    return ret;
}

static inline int bq24250_irq_enable(struct bq24250_charger *bq24250)
{
    return bq24250_irq_set(bq24250, true);
}

static inline int bq24250_irq_disable(struct bq24250_charger *bq24250)
{
    return bq24250_irq_set(bq24250, false);
}

static int bq24250_irq_init(struct bq24250_charger *bq24250, int irq)
{
    int ret;

    if (irq <= 0) {
        dev_info(bq24250->dev, "invalid irq number: %d\n", irq);
        goto out;
    }

    /*
    * Configure the STAT output to be suitable for interrupts: disable
    * all other output (except interrupts) and make it active low.
    */
    regmap_update_bits(bq24250->regmap, BQ24250_STATUS_CONTROL_REG,
               BQ24250_EN_STAT, 1);

    ret = request_threaded_irq(irq, NULL, bq24250_interrupt,
                   IRQF_TRIGGER_RISING | IRQF_ONESHOT,
                   "bq24250_irq", bq24250);
    if (ret)
        return ret;

    bq24250->irq = irq;
    bq24250_irq_enable(bq24250);
out:
    return 0;
}

#endif

static void bq24250_show_charger_fault(void)
{
    uint8_t val;
    int ret;

    ret = BQ24250_read_byte(BQ24250_STATUS_CONTROL_REG, &val);
    if (ret < 0)
    {
        LOG_DEBUG("BQ24250_read_byte failed!\n");
        return ;
    }
    val &= BQ24250_FAULT_MASK;

    LOG_DEBUG("BQ24250 fault status: %s\n", desc[val]);
}

#if 0
static DEVICE_ATTR(charger_fault, S_IRUSR, bq24250_show_charger_fault, NULL);

static struct attribute *bq24250_charger_attr[] = {
    &dev_attr_charger_fault.attr,
    NULL,
};

static const struct attribute_group bq24250_attr_group = {
    .attrs = bq24250_charger_attr,
};


/*
 * Returns the constant charge current programmed
 * into the charger in mA.
 */
static int get_const_charge_current(struct bq24250_charger *bq24250)
{
    int ret;
    unsigned int val, intval;

    if (!bq24250_is_ps_online(bq24250))
        return -ENODATA;

    ret = regmap_read(bq24250->regmap, BQ24250_TERM_CURR_CHARG_REG, &val);
    if (ret < 0)
        return ret;

    val >>= BQ24250_CHARGE_CURR_SHIFT;
    val &= BQ24250_CHARGE_CURR;

    intval = hw_to_current(cc_tbl, ARRAY_SIZE(cc_tbl), val);

    if (intval == 0)
        intval = 0xf;
    else
        intval = 500 + val * 50;


    dev_info(bq24250->dev, "const charge current is %d mA\n", intval);
    return intval;
}

/*
 * Returns the constant charge voltage programmed
 * into the charger in mV.
 */
static int get_const_charge_voltage(struct bq24250_charger *bq24250)
{
    int ret;
    unsigned int val;
    int intval;

    if (!bq24250_is_ps_online(bq24250))
        return -ENODATA;

    ret = regmap_read(bq24250->regmap, BQ24250_BATTERY_VOLTAGE_REG, &val);
    if (ret < 0)
        return ret;

    val >>= BQ24250_BATTERY_VOLTAGE_SHIFT;
    val &= BQ24250_BATTERY_VOLTAGE;

    if (val > 0x30)
        val = 0x30;

    intval = hw_to_current(brv_tbl, ARRAY_SIZE(brv_tbl), val);

    /* intval = (3500 + val)/1000; */
    intval += 3500;
    dev_info(bq24250->dev, "const charge voltage is=%d mV\n", intval);
    intval = 3500 + val * 20;
    dev_info(bq24250->dev, "const charge voltage is=%d mV\n", intval);

    return intval;
}

static int bq24250_mains_get_property(struct power_supply *psy,
                     enum power_supply_property prop,
                     union power_supply_propval *val)
{
    struct bq24250_charger *bq24250 =
        container_of(psy, struct bq24250_charger, mains);
    int ret;

    switch (prop) {
    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = bq24250->mains_online;
        break;

    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
        ret = get_const_charge_voltage(bq24250);
        if (ret < 0)
            return ret;
        else
            val->intval = ret;
        break;

    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
        ret = get_const_charge_current(bq24250);
        if (ret < 0)
            return ret;
        else
            val->intval = ret;
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static int bq24250_usb_get_property(struct power_supply *psy,
                   enum power_supply_property prop,
                   union power_supply_propval *val)
{
    struct bq24250_charger *bq24250 =
        container_of(psy, struct bq24250_charger, usb);
    int ret;

    switch (prop) {
    case POWER_SUPPLY_PROP_ONLINE:
        val->intval = bq24250->usb_online;
        break;

    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
        ret = get_const_charge_voltage(bq24250);
        if (ret < 0)
            return ret;
        else
            val->intval = ret;
        break;

    case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
        ret = get_const_charge_current(bq24250);
        if (ret < 0)
            return ret;
        else
            val->intval = ret;
        break;

    default:
        return -EINVAL;
    }

    bq24250_update_usb_type(bq24250);

    return 0;
}

static int bq24250_battery_get_property(struct power_supply *psy,
                       enum power_supply_property prop,
                       union power_supply_propval *val)
{
    struct bq24250_charger *bq24250 =
            container_of(psy, struct bq24250_charger, battery);
    const struct bq24250_charger_platform_data *pdata = bq24250->pdata;
    int ret;

    ret = bq24250_update_ps_status(bq24250);
    if (ret < 0)
        return ret;

    switch (prop) {
    case POWER_SUPPLY_PROP_STATUS:
        if (!bq24250_is_ps_online(bq24250)) {
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
            break;
        }

        switch (bq24250_charging_status(bq24250)) {
        case 0:
            val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
            break;
        case 1:
            val->intval = POWER_SUPPLY_STATUS_CHARGING;
            break;
        case 2:
            val->intval = POWER_SUPPLY_STATUS_FULL;
            break;
        case 3:
            val->intval = POWER_SUPPLY_STATUS_UNKNOWN; /* FAULT */
            break;
        }
        break;

    case POWER_SUPPLY_PROP_CHARGE_TYPE:
        if (!bq24250_is_ps_online(bq24250))
            return -ENODATA;

        /*
         * We handle trickle and pre-charging the same, and taper
         * and none the same.
         */
        switch (bq24250_charging_status(bq24250)) {
        case 1:
            val->intval = POWER_SUPPLY_CHARGE_TYPE_TRICKLE;
            break;
        case 2:
            val->intval = POWER_SUPPLY_CHARGE_TYPE_FAST;
            break;
        default:
            val->intval = POWER_SUPPLY_CHARGE_TYPE_NONE;
            break;
        }
        break;

    case POWER_SUPPLY_PROP_TECHNOLOGY:
        val->intval = pdata->battery_info.technology;
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
        val->intval = pdata->battery_info.voltage_min_design;
        break;

    case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
        val->intval = pdata->battery_info.voltage_max_design;
        break;

    case POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN:
        val->intval = pdata->battery_info.charge_full_design;
        break;

    case POWER_SUPPLY_PROP_MODEL_NAME:
        val->strval = pdata->battery_info.name;
        break;

    default:
        return -EINVAL;
    }

    return 0;
}

static enum power_supply_property bq24250_battery_properties[] = {
    POWER_SUPPLY_PROP_STATUS,
    POWER_SUPPLY_PROP_CHARGE_TYPE,
    POWER_SUPPLY_PROP_TECHNOLOGY,
    POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN,
    POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN,
    POWER_SUPPLY_PROP_CHARGE_FULL_DESIGN,
    POWER_SUPPLY_PROP_MODEL_NAME,
};

static int bq24250_power_supply_register(struct bq24250_charger *bq24250)
{
    static char *battery[] = { "bq24250-battery" };
    int ret;

    if (bq24250->pdata->use_mains) {
        bq24250->mains.name = "bq24250-mains";
        bq24250->mains.type = POWER_SUPPLY_TYPE_MAINS;
        bq24250->mains.get_property = bq24250_mains_get_property;
        bq24250->mains.properties = bq24250_charger_properties;
        bq24250->mains.num_properties =
                    ARRAY_SIZE(bq24250_charger_properties);
        bq24250->mains.supplied_to = battery;
        bq24250->mains.num_supplicants = ARRAY_SIZE(battery);

        ret = power_supply_register(bq24250->dev, &bq24250->mains);
        if (ret < 0)
            return ret;
    }

    if (bq24250->pdata->use_usb) {
        bq24250->usb.name = "bq24250-usb";
        bq24250->usb.type = POWER_SUPPLY_TYPE_USB;
        bq24250->usb.get_property = bq24250_usb_get_property;
        bq24250->usb.properties = bq24250_charger_properties;
        bq24250->usb.num_properties =
                    ARRAY_SIZE(bq24250_charger_properties);
        bq24250->usb.supplied_to = battery;
        bq24250->usb.num_supplicants = ARRAY_SIZE(battery);

        ret = power_supply_register(bq24250->dev, &bq24250->usb);
        if (ret < 0) {
            if (bq24250->pdata->use_mains)
                power_supply_unregister(&bq24250->mains);
            return ret;
        }
    }

    bq24250->battery.name = "bq24250-battery";
    bq24250->battery.type = POWER_SUPPLY_TYPE_BATTERY;
    bq24250->battery.get_property = bq24250_battery_get_property;
    bq24250->battery.properties = bq24250_battery_properties;
    bq24250->battery.num_properties =
                ARRAY_SIZE(bq24250_battery_properties);

    ret = power_supply_register(bq24250->dev, &bq24250->battery);
    if (ret < 0) {
        if (bq24250->pdata->use_usb)
            power_supply_unregister(&bq24250->usb);
        if (bq24250->pdata->use_mains)
            power_supply_unregister(&bq24250->mains);
        return ret;
    }

    return 0;
}

static void bq24250_power_supply_unregister(struct bq24250_charger *bq24250)
{
    power_supply_unregister(&bq24250->battery);
    if (bq24250->pdata->use_usb)
        power_supply_unregister(&bq24250->usb);
    if (bq24250->pdata->use_mains)
        power_supply_unregister(&bq24250->mains);
}

#endif

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////

int charger_init(void)
{
    LOG_DEBUG("%s \n", __FUNCTION__);
    return 0;
}

int charger_test(void)
{
#if 0
    uint8_t status;
    if (0 != BQ24250_read_byte(0x00, &status))
    {
        LOG_DEBUG("%s BQ24250_read_byte: 0x%x failed! \n", __FUNCTION__, 0x00);
        return -1;
    }
    LOG_DEBUG("%s BQ24250 status: 0x%x \n", __FUNCTION__, status);
#endif
    int charge_status = bq24250_charging_status();
    if (charge_status < 0)
    {
        return -1;
    }
    else
    {
        if (charge_status < BQ24250_CHARGE_MAX)
        {
            LOG_DEBUG("BQ24250 charge status: %s \n", charge_status_str[charge_status]);
        }
        else
        {
            LOG_DEBUG("Invalid BQ24250 charge status: %d \n", charge_status);
        }
    }
    bq24250_show_charger_fault();

    return 0;
}


#endif

// PRIVATE FUNCTION DEFINITIONS //////////////////////////////////////////////
