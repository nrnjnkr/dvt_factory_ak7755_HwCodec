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

#include "accelerometer.h"

#if defined(CONFIG_PROJECT_TEKNIQUE_BTFL_OOMA)
// DEFINES ///////////////////////////////////////////////////////////////////
#define GET_ACC_VALUE(H, L) ((int16_t)(((int16_t)((int8_t)(H)))<<8|((L)&0xf0)))>>4
#define CHECK_SLOW_AUOT_OFFSET_TARGET(O) ((O) == -1 || (O) == 0 || (O) == 1)
#define SET_BIT(R, O, Ret) do {\
    uint8_t _v; \
    if (BMA253_read_byte((R), &_v) != 0) {(Ret) = -1; break; } \
    _v |= (1<<(O)); \
    if (BMA253_write_byte((R), _v) != 0) {(Ret) = -1; break; } \
    (Ret) = 0;\
} while(0)
#define CLEAR_BIT(R, O, Ret) do {\
        uint8_t _v; \
        if (BMA253_read_byte((R), &_v) != 0) {(Ret) = -1; break; } \
        _v &= ~(1<<(O)); \
        if (BMA253_write_byte((R), _v) != 0) {(Ret) = -1; break; } \
        (Ret) = 0; \
} while(0)

//
// #define BMA253_I2C_SLAVE_ADDRESS 0x18
// #define BMA253_I2C_SLAVE_ADDRESS 0x19

// Left Shit 1 bit
#define BMA253_I2C_SLAVE_ADDRESS 0x30
// #define BMA253_I2C_SLAVE_ADDRESS 0x32

// b'1111'1010
#define BMA253_CHIP_ID         (0xFA)


/**************************************************************/
/**\name    REGISTER ADDRESS DEFINITIONS    */
/**************************************************************/
#define BMA253_EEP_OFFSET                       (0x16)
#define BMA253_IMAGE_BASE                       (0x38)
#define BMA253_IMAGE_LEN                        (22)
#define BMA253_CHIP_ID_ADDR                     (0x00)
/** DATA ADDRESS DEFINITIONS */
#define BMA253_X_AXIS_LSB_ADDR                  (0x02)
#define BMA253_X_AXIS_MSB_ADDR                  (0x03)
#define BMA253_Y_AXIS_LSB_ADDR                  (0x04)
#define BMA253_Y_AXIS_MSB_ADDR                  (0x05)
#define BMA253_Z_AXIS_LSB_ADDR                  (0x06)
#define BMA253_Z_AXIS_MSB_ADDR                  (0x07)
#define BMA253_TEMP_ADDR                        (0x08)
/**STATUS ADDRESS DEFINITIONS */
#define BMA253_STAT1_ADDR                       (0x09)
#define BMA253_STAT2_ADDR                       (0x0A)
#define BMA253_STAT_TAP_SLOPE_ADDR              (0x0B)
#define BMA253_STAT_ORIENT_HIGH_ADDR            (0x0C)
#define BMA253_STAT_FIFO_ADDR                   (0x0E)
/**STATUS ADDRESS DEFINITIONS */
#define BMA253_RANGE_SELECT_ADDR                (0x0F)
#define BMA253_BW_SELECT_ADDR                   (0x10)
#define BMA253_MODE_CTRL_ADDR                   (0x11)
#define BMA253_LOW_NOISE_CTRL_ADDR              (0x12)
#define BMA253_DATA_CTRL_ADDR                   (0x13)
#define BMA253_RST_ADDR                         (0x14)
/**INTERUPT ADDRESS DEFINITIONS */
#define BMA253_INTR_ENABLE1_ADDR                (0x16)
#define BMA253_INTR_ENABLE2_ADDR                (0x17)
#define BMA253_INTR_SLOW_NO_MOTION_ADDR         (0x18)
#define BMA253_INTR1_PAD_SELECT_ADDR            (0x19)
#define BMA253_INTR_DATA_SELECT_ADDR            (0x1A)
#define BMA253_INTR2_PAD_SELECT_ADDR            (0x1B)
#define BMA253_INTR_SOURCE_ADDR                 (0x1E)
#define BMA253_INTR_SET_ADDR                    (0x20)
#define BMA253_INTR_CTRL_ADDR                   (0x21)
/** FEATURE ADDRESS DEFINITIONS */
#define BMA253_LOW_DURN_ADDR                    (0x22)
#define BMA253_LOW_THRES_ADDR                   (0x23)
#define BMA253_LOW_HIGH_HYST_ADDR               (0x24)
#define BMA253_HIGH_DURN_ADDR                   (0x25)
#define BMA253_HIGH_THRES_ADDR                  (0x26)
#define BMA253_SLOPE_DURN_ADDR                  (0x27)
#define BMA253_SLOPE_THRES_ADDR                 (0x28)
#define BMA253_SLOW_NO_MOTION_THRES_ADDR        (0x29)
#define BMA253_TAP_PARAM_ADDR                   (0x2A)
#define BMA253_TAP_THRES_ADDR                   (0x2B)
#define BMA253_ORIENT_PARAM_ADDR                (0x2C)
#define BMA253_THETA_BLOCK_ADDR                 (0x2D)
#define BMA253_THETA_FLAT_ADDR                  (0x2E)
#define BMA253_FLAT_HOLD_TIME_ADDR              (0x2F)
#define BMA253_SELFTEST_ADDR                    (0x32)
#define BMA253_EEPROM_CTRL_ADDR                 (0x33)
#define BMA253_SERIAL_CTRL_ADDR                 (0x34)
/**OFFSET ADDRESS DEFINITIONS */
#define BMA253_OFFSET_CTRL_ADDR                 (0x36)
#define BMA253_OFFSET_PARAMS_ADDR               (0x37)
#define BMA253_OFFSET_X_AXIS_ADDR               (0x38)
#define BMA253_OFFSET_Y_AXIS_ADDR               (0x39)
#define BMA253_OFFSET_Z_AXIS_ADDR               (0x3A)
/**GP ADDRESS DEFINITIONS */
#define BMA253_GP0_ADDR                         (0x3B)
#define BMA253_GP1_ADDR                         (0x3C)
/**FIFO ADDRESS DEFINITIONS */
#define BMA253_FIFO_MODE_ADDR                   (0x3E)
#define BMA253_FIFO_DATA_OUTPUT_ADDR            (0x3F)
#define BMA253_FIFO_WML_TRIG                    (0x30)

// TYPEDEFS, STRUCTS, ENUMS //////////////////////////////////////////////////

// GLOBAL DATA ///////////////////////////////////////////////////////////////

// PRIVATE VARIABLES /////////////////////////////////////////////////////////

// PRIVATE FUNCTION DECLARATIONS /////////////////////////////////////////////
static int BMA253_read_byte(uint8_t addr, uint8_t* p_value)
{
    return i2c_read_byte(BMA253_I2C_SLAVE_ADDRESS, addr, p_value);
}
static int BMA253_write_byte(uint8_t addr, uint8_t value)
{
    return i2c_write_byte(BMA253_I2C_SLAVE_ADDRESS, addr, value);
}

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////
int accelerometer_init()
{
    accelerometer_power_mode_set(ACC_POWER_MODE_NORMAL);
    accelerometer_soft_reset();
    accelerometer_range_set(ACC_RANGE_2G);

    accelerometer_int_mode_set(1, ACC_INT_LATCH_DURN_250MS);
    accelerometer_int_out_ctrl(ACC_INT_PIN_1, 1, 0);
    accelerometer_int_out_ctrl(ACC_INT_PIN_2, 1, 0);

    accelerometer_int_enable(ACC_INT_SRC_TYPE_SIGNAL_TAP, true);
    accelerometer_int_map_to_pin1(ACC_INT_SRC_TYPE_SIGNAL_TAP, true);
    accelerometer_int_enable(ACC_INT_SRC_TYPE_ORIENT, true);
    accelerometer_int_map_to_pin1(ACC_INT_SRC_TYPE_ORIENT, true);

    return 0;
}

int accelerometer_probe(void)
{
    uint8_t chip_id;
    if (0 != BMA253_read_byte(BMA253_CHIP_ID_ADDR, &chip_id))
    {
        LOG_DEBUG("%s BMA253_read_byte: 0x%x failed! \n", __FUNCTION__, BMA253_CHIP_ID_ADDR);
        return -1;
    }
    if (BMA253_CHIP_ID != chip_id)
    {
        LOG_DEBUG("%s Chip id: 0x%x != BMA253_CHIP_ID: 0x%x \n", __FUNCTION__, chip_id, BMA253_CHIP_ID);
        return -1;
    }
    else
    {
        LOG_DEBUG("%s Chip id: 0x%x == BMA253_CHIP_ID: 0x%x \n", __FUNCTION__, chip_id, BMA253_CHIP_ID);
    }
    return 0;
}

int accelerometer_soft_reset()
{
    if (BMA253_write_byte(BMA253_RST_ADDR, 0xb6) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

int accelerometer_power_mode_set(acc_power_mode_t mode)
{
    uint8_t lpw_ctrl, low_power;

    switch (mode) {
        case ACC_POWER_MODE_NORMAL:
            lpw_ctrl  = 0x00;
            low_power = 0x00;
            break;
        case ACC_POWER_MODE_LOW_POWER1:
            lpw_ctrl  = 0x40;
            low_power = 0x00;
            break;
        case ACC_POWER_MODE_SUSPEND:
            lpw_ctrl  = 0x80;
            low_power = 0x00;
            break;
        case ACC_POWER_MODE_DEEP_SUSPEND:
            lpw_ctrl  = 0x20;
            break;
        case ACC_POWER_MODE_LOW_POWER2:
            lpw_ctrl  = 0x40;
            low_power = 0x40;
            break;
        case ACC_POWER_MODE_STANDBY:
            lpw_ctrl  = 0x80;
            low_power = 0x40;
        default:
            goto ERR;
    }

    if (BMA253_write_byte(BMA253_LOW_NOISE_CTRL_ADDR, low_power) != 0)
        goto ERR;
    Delay(1);

    if (BMA253_write_byte(BMA253_MODE_CTRL_ADDR, lpw_ctrl) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

int accelerometer_accel_get(int16_t *x, int16_t *y, int16_t *z)
{
    uint8_t h, l;

    // Must read LSB first
    if (BMA253_read_byte(BMA253_X_AXIS_LSB_ADDR, &l) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_X_AXIS_MSB_ADDR, &h) != 0)
        goto ERR;
    *x = GET_ACC_VALUE(h, l);
    
    if (BMA253_read_byte(BMA253_Y_AXIS_LSB_ADDR, &l) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_Y_AXIS_MSB_ADDR, &h) != 0)
        goto ERR;
    *y = GET_ACC_VALUE(h, l);
    
    if (BMA253_read_byte(BMA253_Z_AXIS_LSB_ADDR, &l) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_Z_AXIS_MSB_ADDR, &h) != 0)
        goto ERR;
    *z = GET_ACC_VALUE(h, l);
    
    return 0;
ERR:
    return -1;
}

int accelerometer_temperature_get(int8_t *temp)
{
    uint8_t data;

    if (BMA253_read_byte(BMA253_TEMP_ADDR, &data) != 0)
        goto ERR;

    // data = 0 = 23C = 296.15K, 0.5k/LSB
    *temp = 23 + (((int8_t)(data))/2);

    return 0;
ERR:
    return -1;
}

/*
 * target: -1, 0, 1
 */
int accelerometer_offset_slow_auto_gen(int8_t x_en, int8_t y_en, int8_t z_en, int8_t cut_off_en)
{
    uint8_t data = 0;

    if (x_en) {
        data |= 0x01;
    }
    if (y_en) {
        data |= 0x02;
    }
    if (z_en) {
        data |= 0x04;
    }
    if (BMA253_write_byte(BMA253_OFFSET_CTRL_ADDR, data) != 0)
        goto ERR;
    
    data = 0;
    if (cut_off_en) {
        data |= 0x1;
    }
    if (BMA253_write_byte(BMA253_OFFSET_PARAMS_ADDR, data) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;    
}

/*
 * axis: 0 = x, 1 = y, 2 = z
 */
int accelerometer_offset_fast_auto_gen(int8_t axis, acc_offset_target_t target)
{
    uint8_t data = 0;

    if ((axis < 0 || axis > 2)
        || (target > ACC_OFFSET_TARGET_MAX)) {
        goto ERR;
    }

    // range must be 2g
    if (accelerometer_range_set(ACC_RANGE_2G) != 0)
        goto ERR;

    data |= (target << (axis*2));
    // set target
    if (BMA253_write_byte(BMA253_OFFSET_PARAMS_ADDR, data) != 0) {
        goto ERR;
    }

    // start
    data = 0;
    if (axis == 0) {
        data = 0x20;
    } else if (axis == 1) {
        data = 0x40;
    } else {
        data = 0x60;
    }
    if (BMA253_write_byte(BMA253_OFFSET_CTRL_ADDR, data) != 0) {
        goto ERR;
    }

    // wait completed
    while (1) {
        Delay(10);
        if (BMA253_read_byte(BMA253_OFFSET_CTRL_ADDR, &data) != 0) {
            goto ERR;
        }
        if ((data & 0x10) != 0) {
            break;
        }
    }
    return 0;
ERR:
    return -1;
}


int accelerometer_offset_clear()
{
    if (BMA253_write_byte(BMA253_OFFSET_CTRL_ADDR, 0x80) != 0)
        return -1;
    return 0;
}
/*
 * mapping 
 * +127 -> +0.992g
 * 0 -> 0 g
 * -128 -> -1 g
 * the scaling is independent of the selected g-range
 */
int accelerometer_offset_get(int8_t *x, int8_t *y, int8_t *z)
{
    if (BMA253_read_byte(BMA253_OFFSET_X_AXIS_ADDR, (uint8_t *)x) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_OFFSET_Y_AXIS_ADDR, (uint8_t *)y) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_OFFSET_Z_AXIS_ADDR, (uint8_t *)z) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

/*
 * mapping 
 * +127 -> +0.992g
 * 0 -> 0 g
 * -128 -> -1 g
 * the scaling is independent of the selected g-range
 */
int accelerometer_offset_set(int8_t x, int8_t y, int8_t z)
{
    uint8_t data = 0;

    // Step 1: write new content to image registers
    if (BMA253_write_byte(BMA253_OFFSET_X_AXIS_ADDR, (uint8_t)x) != 0)
        goto ERR;
    if (BMA253_write_byte(BMA253_OFFSET_Y_AXIS_ADDR, (uint8_t)y) != 0)
        goto ERR;
    if (BMA253_write_byte(BMA253_OFFSET_Z_AXIS_ADDR, (uint8_t)z) != 0)
        goto ERR;

    // 3         2        1               0
    // nvm_load  nvm_rdy  nvm_prog_trig   nvm_prog_mode
    // Step 2: Unlock NVM
    if (BMA253_write_byte(BMA253_EEPROM_CTRL_ADDR, 0x1) != 0)
        goto ERR;
    // Step 3: trigger write process
    if (BMA253_write_byte(BMA253_EEPROM_CTRL_ADDR, 0x3) != 0)
        goto ERR;
    // wait until writing completed
    while (1) {
        if (BMA253_read_byte(BMA253_EEPROM_CTRL_ADDR, &data) != 0)
            goto ERR;
        if ((data & 0x04) != 0)
            break;
    }
    return 0;
ERR:
    return -1;
}
int accelerometer_range_set(acc_accel_range_t acc_range_val)
{
    uint8_t range;

    switch (acc_range_val) {
        case ACC_RANGE_2G:
            range = 0x3;
            break;
        case ACC_RANGE_4G:
            range = 0x5;
            break;
        case ACC_RANGE_8G:
            range = 0x8;
            break;
        case ACC_RANGE_16G:
            range = 0xc;
            break;
        default:
            goto ERR;
    }
    if (BMA253_write_byte(BMA253_RANGE_SELECT_ADDR, range) != 0)
        goto ERR;
    return 0;
ERR:
    return -1;
}
int accelerometer_range_get(acc_accel_range_t *acc_range_val)
{
    uint8_t range;

    if (BMA253_read_byte(BMA253_RANGE_SELECT_ADDR, &range) != 0)
        goto ERR;
    range = range &0x0f;

    switch (range) {
        case 0x3:
            *acc_range_val = ACC_RANGE_2G;
            break;
        case 0x5:
            *acc_range_val = ACC_RANGE_4G;
            break;
        case 0x8:
            *acc_range_val = ACC_RANGE_8G;
            break;
        case 0xc:
            *acc_range_val = ACC_RANGE_16G;
            break;
        default:
            *acc_range_val = ACC_RANGE_UNKNOWN;
            break;
    }

    return 0;
ERR:
    return -1;
}

int accelerometer_filter_enable_set(int8_t en)
{
    uint8_t data = 0;

    if (en == 0) {
        // unfiltered
        data |= 0x80;   
    }

    if (BMA253_write_byte(BMA253_DATA_CTRL_ADDR, data) != 0)
        return -1;
    return 0;
}

int accelerometer_filter_enable_get(int8_t * en)
{
    uint8_t data = 0;

    if (BMA253_read_byte(BMA253_DATA_CTRL_ADDR, &data) != 0)
        return -1;

    if ((data & 0x80) != 0)
        *en = 0;
    else
        *en = 1;

    return 0;
}

int accelerometer_filter_bandwidth_set(acc_filter_bandwidth_t bw)
{
    uint8_t data[ACC_FILTER_BANDWIDTH_MAX] = {0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

    if (bw >= ACC_FILTER_BANDWIDTH_MAX)
        goto ERR;

    if (BMA253_write_byte(BMA253_BW_SELECT_ADDR, data[bw]) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}


int accelerometer_filter_bandwidth_get(acc_filter_bandwidth_t * bw)
{
    uint8_t data;

    if (BMA253_read_byte(BMA253_BW_SELECT_ADDR, &data) != 0)
        goto ERR;
    data = data & 0x0f;
    *bw = data - 0x08;

    return 0;
ERR:
    return -1;
}

int accelerometer_fifo_mode_set(acc_fifo_mode_t mode,
                                                acc_fifo_data_type_t data_type)
{
    uint8_t data = 0;

    data = ((uint8_t)mode << 6) | (uint8_t)data_type;

    if (BMA253_write_byte(BMA253_FIFO_MODE_ADDR, data) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

int accelerometer_fifo_mode_get(acc_fifo_mode_t *mode,
                                                acc_fifo_data_type_t * data_type)
{
    uint8_t data = 0;

    if (BMA253_read_byte(BMA253_FIFO_MODE_ADDR, &data) != 0)
        goto ERR;

    *mode = (acc_fifo_mode_t)((data & 0xC0) >> 6);
    *data_type = (acc_fifo_data_type_t)(data & 0x02);

    return 0;
ERR:
    return -1;
}

int accelerometer_fifo_data_read(uint16_t *value)
{
    uint8_t *p_d = (uint8_t *)value;

    if (BMA253_read_byte(BMA253_FIFO_MODE_ADDR, p_d) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_FIFO_MODE_ADDR, &p_d[1]) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

int accelerometer_fifo_watermark_level_set(uint8_t level)
{
    if (level > 32)
        goto ERR;

    if (BMA253_write_byte(BMA253_FIFO_WML_TRIG, level) != 0)
        goto ERR;
    
    return 0;
ERR:
    return -1;
}

int accelerometer_fifo_watermark_level_get(uint8_t *level)
{
    if (BMA253_read_byte(BMA253_FIFO_WML_TRIG, level) != 0)
        goto ERR;
    
    return 0;
ERR:
    return -1;
}

int accelerometer_fifo_status_get(uint8_t * frame_count, uint8_t * over_run)
{
    uint8_t data;

    if (BMA253_read_byte(BMA253_STAT_FIFO_ADDR, &data) != 0)
        goto ERR;

    *over_run = (data & 0x80) >> 7;
    *frame_count = data & 0x7f;
    
    return 0;
ERR:
    return -1;
}

// Interrupt configrations
int accelerometer_int_mode_set(uint8_t clear_latched_int, acc_int_latch_type_t latch)
{
    uint8_t data;

    if (latch >= ACC_INT_LATCH_MAX)
        goto ERR;

    data = (clear_latched_int << 7) | ((uint8_t)latch);

    if (BMA253_write_byte(BMA253_INTR_CTRL_ADDR, data) != 0)
        goto ERR;
    
    return 0;
ERR:
    return -1;
}

int accelerometer_int_out_ctrl(acc_int_pin_t pin, int8_t open_drain, int8_t high_active)
{
    uint8_t data = 0;

    if (BMA253_read_byte(BMA253_INTR_SET_ADDR, &data) != 0)
        goto ERR;
    if (pin >= ACC_INT_PIN_MAX)
        goto ERR;
    if (pin == ACC_INT_PIN_1) {
        data &= 0xfc;
        data |= (((open_drain & 0x01) << 1) | (high_active &0x01));
    } else {
        data &= 0xf3;
        data |= (((open_drain & 0x01) << 3) | ((high_active &0x01) << 2));
    }

    if (BMA253_write_byte(BMA253_INTR_SET_ADDR, data) != 0)
        goto ERR;
    return 0;
ERR:
    return -1;
}

int accelerometer_int_filter_enable(acc_int_src_type_t int_type, int8_t enable)
{
    int8_t ret = -1;
    struct {
        uint8_t en_reg;
        uint8_t en_offset;
    } filter_offset[] = {
        {BMA253_INTR_SOURCE_ADDR, }, // ACC_INT_SRC_TYPE_FLAT
        {BMA253_INTR_SOURCE_ADDR,}, // ACC_INT_SRC_TYPE_ORIENT
        {BMA253_INTR_SOURCE_ADDR, 4}, // ACC_INT_SRC_TYPE_SIGNAL_TAP
        {BMA253_INTR_SOURCE_ADDR, 4}, // ACC_INT_SRC_TYPE_DOUBLE_TAP
        {0xff, 0}, // ACC_INT_SRC_TYPE_SLOPE_X
        {0xff, 0}, // ACC_INT_SRC_TYPE_SLOPE_Y
        {0xff, 0}, // ACC_INT_SRC_TYPE_SLOPE_Z
        {0xff, 0}, // ACC_INT_SRC_TYPE_FIFO_WATERMARK
        {0xff, 0}, // ACC_INT_SRC_TYPE_FIFO_FULL
        {BMA253_INTR_SOURCE_ADDR, 5}, // ACC_INT_SRC_TYPE_DATA_READY
        {BMA253_INTR_SOURCE_ADDR, 0}, // ACC_INT_SRC_TYPE_LOW_G
        {BMA253_INTR_SOURCE_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_X
        {BMA253_INTR_SOURCE_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_Y
        {BMA253_INTR_SOURCE_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_Z
        {BMA253_INTR_SOURCE_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION
        {BMA253_INTR_SOURCE_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_X
        {BMA253_INTR_SOURCE_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_Y
        {BMA253_INTR_SOURCE_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_Z
    };

    if (filter_offset[int_type].en_reg != 0xff) {
        if (enable) {
            SET_BIT(filter_offset[int_type].en_reg, filter_offset[int_type].en_offset, ret);
        } else {
            CLEAR_BIT(filter_offset[int_type].en_reg, filter_offset[int_type].en_offset, ret);
        }
    }

    return ret;
}

int accelerometer_int_map_to_pin1(acc_int_src_type_t int_type, int8_t enable)
{
    int8_t ret;
    struct {
        uint8_t en_reg;
        uint8_t en_offset;
    } map_offset[] = {
        {BMA253_INTR1_PAD_SELECT_ADDR, 7}, // ACC_INT_SRC_TYPE_FLAT
        {BMA253_INTR1_PAD_SELECT_ADDR, 6}, // ACC_INT_SRC_TYPE_ORIENT
        {BMA253_INTR1_PAD_SELECT_ADDR, 5}, // ACC_INT_SRC_TYPE_SIGNAL_TAP
        {BMA253_INTR1_PAD_SELECT_ADDR, 4}, // ACC_INT_SRC_TYPE_DOUBLE_TAP
        {BMA253_INTR1_PAD_SELECT_ADDR, 2}, // ACC_INT_SRC_TYPE_SLOPE_X
        {BMA253_INTR1_PAD_SELECT_ADDR, 2}, // ACC_INT_SRC_TYPE_SLOPE_Y
        {BMA253_INTR1_PAD_SELECT_ADDR, 2}, // ACC_INT_SRC_TYPE_SLOPE_Z
        {BMA253_INTR_DATA_SELECT_ADDR, 1}, // ACC_INT_SRC_TYPE_FIFO_WATERMARK
        {BMA253_INTR_DATA_SELECT_ADDR, 2}, // ACC_INT_SRC_TYPE_FIFO_FULL
        {BMA253_INTR_DATA_SELECT_ADDR, 0}, // ACC_INT_SRC_TYPE_DATA_READY
        {BMA253_INTR1_PAD_SELECT_ADDR, 0}, // ACC_INT_SRC_TYPE_LOW_G
        {BMA253_INTR1_PAD_SELECT_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_X
        {BMA253_INTR1_PAD_SELECT_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_Y
        {BMA253_INTR1_PAD_SELECT_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_Z
        {BMA253_INTR1_PAD_SELECT_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION
        {BMA253_INTR1_PAD_SELECT_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_X
        {BMA253_INTR1_PAD_SELECT_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_Y
        {BMA253_INTR1_PAD_SELECT_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_Z
    };
   
    if (enable) {
        SET_BIT(map_offset[int_type].en_reg, map_offset[int_type].en_offset, ret);
    } else {
        CLEAR_BIT(map_offset[int_type].en_reg, map_offset[int_type].en_offset, ret);
    }

    return ret;
}

int accelerometer_int_map_to_pin2(acc_int_src_type_t int_type, int8_t enable)
{
    int8_t ret;
    struct {
        uint8_t en_reg;
        uint8_t en_offset;
    } map_offset[] = {
        {BMA253_INTR2_PAD_SELECT_ADDR, 7}, // ACC_INT_SRC_TYPE_FLAT
        {BMA253_INTR2_PAD_SELECT_ADDR, 6}, // ACC_INT_SRC_TYPE_ORIENT
        {BMA253_INTR2_PAD_SELECT_ADDR, 5}, // ACC_INT_SRC_TYPE_SIGNAL_TAP
        {BMA253_INTR2_PAD_SELECT_ADDR, 4}, // ACC_INT_SRC_TYPE_DOUBLE_TAP
        {BMA253_INTR2_PAD_SELECT_ADDR, 2}, // ACC_INT_SRC_TYPE_SLOPE_X
        {BMA253_INTR2_PAD_SELECT_ADDR, 2}, // ACC_INT_SRC_TYPE_SLOPE_Y
        {BMA253_INTR2_PAD_SELECT_ADDR, 2}, // ACC_INT_SRC_TYPE_SLOPE_Z
        {BMA253_INTR_DATA_SELECT_ADDR, 6}, // ACC_INT_SRC_TYPE_FIFO_WATERMARK
        {BMA253_INTR_DATA_SELECT_ADDR, 5}, // ACC_INT_SRC_TYPE_FIFO_FULL
        {BMA253_INTR_DATA_SELECT_ADDR, 7}, // ACC_INT_SRC_TYPE_DATA_READY
        {BMA253_INTR2_PAD_SELECT_ADDR, 0}, // ACC_INT_SRC_TYPE_LOW_G
        {BMA253_INTR2_PAD_SELECT_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_X
        {BMA253_INTR2_PAD_SELECT_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_Y
        {BMA253_INTR2_PAD_SELECT_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_Z
        {BMA253_INTR2_PAD_SELECT_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION
        {BMA253_INTR2_PAD_SELECT_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_X
        {BMA253_INTR2_PAD_SELECT_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_Y
        {BMA253_INTR2_PAD_SELECT_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION_Z
    };
    
    if (enable) {
        SET_BIT(map_offset[int_type].en_reg, map_offset[int_type].en_offset, ret);
    } else {
        CLEAR_BIT(map_offset[int_type].en_reg, map_offset[int_type].en_offset, ret);
    }

    return ret;

}

int accelerometer_int_enable(acc_int_src_type_t int_type, int8_t enable)
{
    int8_t ret;
    struct {
        uint8_t en_reg;
        uint8_t en_offset;
    } interrupt_config[] = {
        {BMA253_INTR_ENABLE1_ADDR, 7}, // ACC_INT_SRC_TYPE_FLAT
        {BMA253_INTR_ENABLE1_ADDR, 6}, // ACC_INT_SRC_TYPE_ORIENT
        {BMA253_INTR_ENABLE1_ADDR, 5}, // ACC_INT_SRC_TYPE_SIGNAL_TAP
        {BMA253_INTR_ENABLE1_ADDR, 4}, // ACC_INT_SRC_TYPE_DOUBLE_TAP
        {BMA253_INTR_ENABLE1_ADDR, 0}, // ACC_INT_SRC_TYPE_SLOPE_X
        {BMA253_INTR_ENABLE1_ADDR, 1}, // ACC_INT_SRC_TYPE_SLOPE_Y
        {BMA253_INTR_ENABLE1_ADDR, 2}, // ACC_INT_SRC_TYPE_SLOPE_Z
        {BMA253_INTR_ENABLE2_ADDR, 6}, // ACC_INT_SRC_TYPE_FIFO_WATERMARK
        {BMA253_INTR_ENABLE2_ADDR, 5}, // ACC_INT_SRC_TYPE_FIFO_FULL
        {BMA253_INTR_ENABLE2_ADDR, 4}, // ACC_INT_SRC_TYPE_DATA_READY
        {BMA253_INTR_ENABLE2_ADDR, 3}, // ACC_INT_SRC_TYPE_LOW_G
        {BMA253_INTR_ENABLE2_ADDR, 0}, // ACC_INT_SRC_TYPE_HIGH_G_X
        {BMA253_INTR_ENABLE2_ADDR, 1}, // ACC_INT_SRC_TYPE_HIGH_G_Y
        {BMA253_INTR_ENABLE2_ADDR, 2}, // ACC_INT_SRC_TYPE_HIGH_G_Z
        {BMA253_INTR_SLOW_NO_MOTION_ADDR, 3}, // ACC_INT_SRC_TYPE_SLOW_MOTION
        {BMA253_INTR_SLOW_NO_MOTION_ADDR, 0}, // ACC_INT_SRC_TYPE_SLOW_MOTION_X
        {BMA253_INTR_SLOW_NO_MOTION_ADDR, 1}, // ACC_INT_SRC_TYPE_SLOW_MOTION_Y
        {BMA253_INTR_SLOW_NO_MOTION_ADDR, 2}, // ACC_INT_SRC_TYPE_SLOW_MOTION_Z
    };

    if (enable) {
        SET_BIT(interrupt_config[int_type].en_reg, interrupt_config[int_type].en_offset, ret);
    } else {
        CLEAR_BIT(interrupt_config[int_type].en_reg, interrupt_config[int_type].en_offset, ret);
    }

    return ret;
}

int accelerometer_int_status(uint32_t *int_status)
{
    uint8_t int_stat_1, int_stat_2, int_stat_3, int_stat_4;

    if (BMA253_read_byte(BMA253_STAT1_ADDR, &int_stat_1) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_STAT2_ADDR, &int_stat_2) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_STAT_TAP_SLOPE_ADDR, &int_stat_3) != 0)
        goto ERR;
    if (BMA253_read_byte(BMA253_STAT_ORIENT_HIGH_ADDR, &int_stat_4) != 0)
        goto ERR;

    *int_status = ((uint32_t)(int_stat_4&0xff) << 24)
                | ((uint32_t)(int_stat_3&0xff) << 16)
                | ((uint32_t)(int_stat_2&0xff) << 8)
                | ((uint32_t)(int_stat_1&0xff));

    return 0;
ERR:
    return -1;
}

/*
 * delay_time: actual delay = (delay_time + 1)*2ms
 * low_g_threashold: actual threadhold = low_g_threshold * 7.81mg
 * low_g_mode: 0 -> signal axis, 1 -> summing axis
 * low_hysteresis: actual low hysteresis = low_hysteresis * 125mg
 */
int accelerometer_int_low_g_config_set(uint16_t delay_time,
                                    uint16_t low_g_threshold,
                                    uint8_t low_g_mode,
                                    uint8_t low_hysteresis)
{
    uint8_t data;

    if (BMA253_write_byte(BMA253_LOW_DURN_ADDR, delay_time) != 0)
        goto ERR;
    if (BMA253_write_byte(BMA253_LOW_THRES_ADDR, low_g_threshold) != 0)
        goto ERR;

    if (BMA253_read_byte(BMA253_LOW_HIGH_HYST_ADDR, &data) != 0)
        goto ERR;
    // clear 0-2 bits
    data &= 0xfc;
    data |= (((low_g_mode&0x01) << 2) | low_hysteresis);
    if (BMA253_write_byte(BMA253_LOW_HIGH_HYST_ADDR, data) != 0)
        goto ERR;

    return 0;
ERR:
    return -1; 
}

/*
 * delay_time: actual delay = (delay_time + 1)*2ms
 * high_g_threshold: actual threadhold = low_g_threshold * 7.81mg (2-g range)
 *                                                         15.63 mg (4-g range),
 *                                                         31.25 mg (8-g range),
 *                                                         62.5 mg (16-g range)
 * high_hysteresis: actual low hysteresis = low_hysteresis * 125mg
 */
int accelerometer_int_high_g_config_set(uint16_t delay_time,
                                        uint16_t high_g_threshold,
                                        uint8_t high_hysteresis)
{
    uint8_t data;

    if (BMA253_write_byte(BMA253_HIGH_DURN_ADDR, delay_time) != 0)
        goto ERR;
    if (BMA253_write_byte(BMA253_HIGH_THRES_ADDR, high_g_threshold) != 0)
        goto ERR;

    if (BMA253_read_byte(BMA253_LOW_HIGH_HYST_ADDR, &data) != 0)
        goto ERR;
    // clear 6-7 bits
    data &= 0x3f;
    data |= (high_hysteresis << 6);
    if (BMA253_write_byte(BMA253_LOW_HIGH_HYST_ADDR, data) != 0)
        goto ERR;

    return 0;
ERR:
    return -1; 
}

/*
 * slope_dur: slope interrupt triggers if slope_dur+1 consecutive slope data points
 *            are above the slope_threshold
 * slope_threshold: actual slope threshold = slope_threshold * 3.91 mg (2-g range)
 *                                                             7.81 mg (4-g range)
 *                                                             15.63 mg (8-g range)
 *                                                             31.25 mg (16-g range)
 */
int accelerometer_int_slope_config_set(uint8_t slope_dur,
                                      uint8_t slope_threshold)
{
    uint8_t data;

    if (slope_dur > 3)
        goto ERR;

    if (BMA253_read_byte(BMA253_SLOPE_DURN_ADDR, &data) != 0)
        goto ERR;
    // clear 0-1 bits
    data &= 0x03;
    data |= slope_dur;
    if (BMA253_write_byte(BMA253_SLOPE_DURN_ADDR, data) != 0)
        goto ERR;

    if (BMA253_write_byte(BMA253_SLOPE_THRES_ADDR, slope_threshold) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

/*
 * slow_no_motion_threshold: actual slow/no motion threshold = slow_no_threshold * 3.91 mg (2-g range)
 *                                                                               7.81 mg (4-g range)
 *                                                                               15.63 mg (8-g range)
 *                                                                               31.25 mg (16-g range)
 */
int accelerometer_int_slow_config_set(uint8_t slow_no_dur,
                                     uint8_t slow_no_threshold)
{
    uint8_t data;

    // TODO : slow no dur how to define
    if (slow_no_dur > 3)
        goto ERR;

    if (BMA253_read_byte(BMA253_SLOPE_DURN_ADDR, &data) != 0)
        goto ERR;
    // clear 2-7 bits
    data &= 0xfc;
    data |= (slow_no_dur << 2);
    if (BMA253_write_byte(BMA253_SLOPE_DURN_ADDR, data) != 0)
        goto ERR;

    if (BMA253_write_byte(BMA253_SLOW_NO_MOTION_THRES_ADDR, slow_no_threshold) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;

}

/*
 * quiet: selects a tap quiet duration. 0 -> 30ms, 1 -> 20ms
 * shock: selects a tap shock duration. 0 -> 50ms, 1 -> 75ms
 * dur: selects the length of the time window for the second shock event for double tap detection.
 *      0 -> 50ms, 1 -> 100ms, 2 -> 150ms, 3 -> 200ms, 4 -> 250ms, 5 -> 375ms, 6 -> 500ms, 7 -> 700ms
 * samples: selects the number of samples that are processed after wake-up in the low-power mode
 *          0 -> 2 samples, 1 -> 4 samples, 2 -> 8 samples, 3 -> 16 samples.
 * threshold: threshold of the single/double-tap interrupt corresponding to an acceleration difference
 *            threshold * [62.5mg (2g-range), 125mg (4g- range), 250mg (8g-range), 500mg (16g- range)].
 */
int accelerometer_int_tap_config_set(uint8_t quiet,
                                    uint8_t shock,
                                    uint8_t dur,
                                    uint8_t samples,
                                    uint8_t threshold)
{
    uint8_t data = 0;

    if (quiet > 1 || shock > 1 || dur > 7 || samples > 3 || threshold > 0xf)
        goto ERR;

    data |= (quiet << 7) | (shock << 6) | dur;
    if (BMA253_write_byte(BMA253_TAP_PARAM_ADDR, data) != 0)
        goto ERR;
    
    data = 0;
    data |= (samples << 6) | threshold;
    if (BMA253_write_byte(BMA253_TAP_THRES_ADDR, data) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

/*
 * hysteresis: 1LSB = 62.5mg
 * blocking: 0 -> no blocking
 *           1 -> theta blocking or acceleration in any axis > 1.5g
 *           2 -> theta blocking or acceleration slope in any axis > 0.2 g or
 *                acceleration in any axis > 1.5g
 *           3 -> theta blocking or acceleration slope in any axis > 0.4 g or
 *                acceleration in any axis > 1.5g and value of orient is not stable for at least 100ms
 * mode: 0 -> symmetrical, 1 -> high-asymmetrical, 2 -> low-asymmetrical, 3 -> symmetrical
 * up_down_en: 1 enable, 0, disable
 * block_angle: blocking angle between 0° and 44.8°
 */
int accelerometer_int_orient_config_set(uint8_t hysteresis,
                                        uint8_t blocking,
                                        uint8_t mode,
                                        uint8_t up_down_en,
                                        uint8_t block_angle)
{
    uint8_t data = 0;

    if (hysteresis > 7 || blocking > 3 || mode > 3 || up_down_en > 1 || block_angle > 0x3f)
        goto ERR;

    data |= (hysteresis << 4) | (blocking << 2) | mode;
    if (BMA253_write_byte(BMA253_ORIENT_PARAM_ADDR, data) != 0)
        goto ERR;
    
    data = 0;
    data |= (up_down_en << 6) | block_angle;
    if (BMA253_write_byte(BMA253_THETA_BLOCK_ADDR, data) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

/*
 * angle: angle between 0° and 44.8°
 * hold_time: 0 -> 0ms, 1 -> 512ms, 2 -> 1024ms, 3 -> 2048ms
 * hysteresis: 0 -> disable
 */
int accelerometer_int_flat_config_set(uint8_t angle,
                                        uint8_t hold_time,
                                        uint8_t hysteresis)
{
    uint8_t data = 0;

    if (hold_time > 3 || hysteresis > 7 || angle > 0x3f)
        goto ERR;

    data |= angle;
    if (BMA253_write_byte(BMA253_THETA_FLAT_ADDR, data) != 0)
        goto ERR;
    
    data = 0;
    data |= (hold_time << 4) | hysteresis;
    if (BMA253_write_byte(BMA253_FLAT_HOLD_TIME_ADDR, data) != 0)
        goto ERR;

    return 0;
ERR:
    return -1;
}

#endif

// PRIVATE FUNCTION DEFINITIONS //////////////////////////////////////////////
