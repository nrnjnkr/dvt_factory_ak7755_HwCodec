// COPYRIGHT (C) 2018 TEKNIQUE LIMITED
// ALL RIGHTS RESERVED. FOR LICENSING INFORMATION CONTACT LICENSE@TEKNIQUE.COM

// INCLUDES //////////////////////////////////////////////////////////////////

#ifndef ACCELEROMETER_H_
#define ACCELEROMETER_H_

typedef enum _acc_accel_range_t {
    ACC_RANGE_UNKNOWN,
    ACC_RANGE_2G,
    ACC_RANGE_4G,
    ACC_RANGE_8G,
    ACC_RANGE_16G,
    ACC_RANGE_MAX
} acc_accel_range_t;

typedef enum _acc_offset_target_t {
    ACC_OFFSET_TARGET_0G,
    ACC_OFFSET_TARGET_1G,
    ACC_OFFSET_TARGET_NEGATIVE_G,
    ACC_OFFSET_TARGET_MAX
} acc_offset_target_t;

typedef enum _acc_filter_bandwidth_t {
    ACC_FILTER_BANDWIDTH_7_81_HZ,
    ACC_FILTER_BANDWIDTH_15_63_HZ,
    ACC_FILTER_BANDWIDTH_31_25_HZ,
    ACC_FILTER_BANDWIDTH_62_5_HZ,
    ACC_FILTER_BANDWIDTH_125_HZ,
    ACC_FILTER_BANDWIDTH_250_HZ,
    ACC_FILTER_BANDWIDTH_500_HZ,
    ACC_FILTER_BANDWIDTH_1000_HZ,
    ACC_FILTER_BANDWIDTH_MAX
} acc_filter_bandwidth_t;

typedef enum _acc_power_mode_t {
    ACC_POWER_MODE_NORMAL,
    ACC_POWER_MODE_LOW_POWER1,
    ACC_POWER_MODE_SUSPEND,
    ACC_POWER_MODE_DEEP_SUSPEND,
    ACC_POWER_MODE_LOW_POWER2,
    ACC_POWER_MODE_STANDBY,
    ACC_POWER_MODE_MAX
} acc_power_mode_t;

typedef enum _acc_fifo_mode_t {
    ACC_FIFO_MODE_BYPASS,
    ACC_FIFO_MODE_FIFO,
    ACC_FIFO_MODE_STREAM,
    ACC_FIFO_MODE_MAX
} acc_fifo_mode_t;

typedef enum _acc_fifo_data_type_t {
    ACC_FIFO_DATA_TYPE_X_Y_Z,
    ACC_FIFO_DATA_TYPE_X,
    ACC_FIFO_DATA_TYPE_Y,
    ACC_FIFO_DATA_TYPE_Z,
    ACC_FIFO_DATA_TYPE_MAX
} acc_fifo_data_type_t;

typedef enum _acc_int_latch_type_t {
    ACC_INT_LATCH_DURN_NON_LATCH,
    ACC_INT_LATCH_DURN_250MS,
    ACC_INT_LATCH_DURN_500MS,
    ACC_INT_LATCH_DURN_1S,
    ACC_INT_LATCH_DURN_2S,
    ACC_INT_LATCH_DURN_4S,
    ACC_INT_LATCH_DURN_8S,
    ACC_INT_LATCH_DURN_LATCH,
    ACC_INT_LATCH_DURN_NON_LATCH1,
    ACC_INT_LATCH_DURN_250US,
    ACC_INT_LATCH_DURN_500US,
    ACC_INT_LATCH_DURN_1MS,
    ACC_INT_LATCH_DURN_12_5MS,
    ACC_INT_LATCH_DURN_25MS,
    ACC_INT_LATCH_DURN_50MS,
    ACC_INT_LATCH_DURN_LATCH1,
    ACC_INT_LATCH_MAX
} acc_int_latch_type_t;

typedef enum _acc_int_pin_t {
    ACC_INT_PIN_1,
    ACC_INT_PIN_2,
    ACC_INT_PIN_MAX
} acc_int_pin_t;

typedef enum _acc_int_src_type_t {
    ACC_INT_SRC_TYPE_FLAT,
    ACC_INT_SRC_TYPE_ORIENT,
    ACC_INT_SRC_TYPE_SIGNAL_TAP,
    ACC_INT_SRC_TYPE_DOUBLE_TAP,
    ACC_INT_SRC_TYPE_SLOPE_X,
    ACC_INT_SRC_TYPE_SLOPE_Y,
    ACC_INT_SRC_TYPE_SLOPE_Z,
    ACC_INT_SRC_TYPE_FIFO_WATERMARK,
    ACC_INT_SRC_TYPE_FIFO_FULL,
    ACC_INT_SRC_TYPE_DATA_READY,
    ACC_INT_SRC_TYPE_LOW_G,
    ACC_INT_SRC_TYPE_HIGH_G_X,
    ACC_INT_SRC_TYPE_HIGH_G_Y,
    ACC_INT_SRC_TYPE_HIGH_G_Z,
    ACC_INT_SRC_TYPE_SLOW_MOTION,
    ACC_INT_SRC_TYPE_SLOW_MOTION_X,
    ACC_INT_SRC_TYPE_SLOW_MOTION_Y,
    ACC_INT_SRC_TYPE_SLOW_MOTION_Z,
    ACC_INT_SRC_TYPE_MAX
} acc_int_src_type_t;

extern int accelerometer_probe(void);
extern int accelerometer_init(void);
extern int accelerometer_soft_reset(void);
extern int accelerometer_power_mode_set(acc_power_mode_t mode);
extern int accelerometer_accel_get(int16_t *x, int16_t *y, int16_t *z);
extern int accelerometer_temperature_get(int8_t *temp);
extern int accelerometer_offset_slow_auto_gen(int8_t x_en, int8_t y_en, int8_t z_en, int8_t cut_off_en);
extern int accelerometer_offset_fast_auto_gen(int8_t axis, acc_offset_target_t target);
extern int accelerometer_offset_clear();
extern int accelerometer_offset_get(int8_t *x, int8_t *y, int8_t *z);
extern int accelerometer_offset_set(int8_t x, int8_t y, int8_t z);
extern int accelerometer_range_set(acc_accel_range_t acc_range_val);
extern int accelerometer_range_get(acc_accel_range_t *acc_range_val);
extern int accelerometer_filter_enable_set(int8_t en);
extern int accelerometer_filter_enable_get(int8_t * en);
extern int accelerometer_filter_bandwidth_set(acc_filter_bandwidth_t bw);
extern int accelerometer_filter_bandwidth_get(acc_filter_bandwidth_t * bw);
extern int accelerometer_fifo_mode_set(acc_fifo_mode_t mode, acc_fifo_data_type_t data_type);
extern int accelerometer_fifo_mode_get(acc_fifo_mode_t *mode, acc_fifo_data_type_t * data_type);
extern int accelerometer_fifo_data_read(uint16_t *value);
extern int accelerometer_fifo_watermark_level_set(uint8_t level);
extern int accelerometer_fifo_watermark_level_get(uint8_t *level);
extern int accelerometer_fifo_status_get(uint8_t * frame_count, uint8_t * over_run);
extern int accelerometer_int_mode_set(uint8_t clear_latched_int, acc_int_latch_type_t latch);
extern int accelerometer_int_out_ctrl(acc_int_pin_t pin, int8_t open_drain, int8_t high_active);
extern int accelerometer_int_filter_enable(acc_int_src_type_t int_type, int8_t enable);
extern int accelerometer_int_map_to_pin1(acc_int_src_type_t int_type, int8_t enable);
extern int accelerometer_int_map_to_pin2(acc_int_src_type_t int_type, int8_t enable);
extern int accelerometer_int_enable(acc_int_src_type_t int_type, int8_t enable);
extern int accelerometer_int_status(uint32_t *int_status);
extern int accelerometer_int_low_g_config_set(uint16_t delay_time,
                                                uint16_t low_g_threshold,
                                                uint8_t low_g_mode,
                                                uint8_t low_hysteresis);
extern int accelerometer_int_high_g_config_set(uint16_t delay_time,
                                                uint16_t high_g_threshold,
                                                uint8_t high_hysteresis);
extern int accelerometer_int_slope_config_set(uint8_t slope_dur,
                                                uint8_t slope_threshold);
extern int accelerometer_int_slow_config_set(uint8_t slow_no_dur,
                                                uint8_t slow_no_threshold);
extern int accelerometer_int_tap_config_set(uint8_t quiet,
                                                uint8_t shock,
                                                uint8_t dur,
                                                uint8_t samples,
                                                uint8_t threshold);
extern int accelerometer_int_orient_config_set(uint8_t hysteresis,
                                                uint8_t blocking,
                                                uint8_t mode,
                                                uint8_t up_down_en,
                                                uint8_t block_angle);
extern int accelerometer_int_flat_config_set(uint8_t angle,
                                                uint8_t hold_time,
                                                uint8_t hysteresis);



#endif
