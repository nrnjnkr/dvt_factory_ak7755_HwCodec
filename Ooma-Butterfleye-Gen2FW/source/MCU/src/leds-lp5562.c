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

#define LP5562_I2C_SLAVE_ADDRESS 0x60
#define LP5562_ENABLE_GPIO_ID    (gpioGroup_RGB_EN)


#define LP5562_PROGRAM_LENGTH       32
#define LP5562_MAX_LEDS         4

/* ENABLE Register 00h */
#define LP5562_REG_ENABLE       0x00
#define LP5562_EXEC_ENG1_M      0x30
#define LP5562_EXEC_ENG2_M      0x0C
#define LP5562_EXEC_ENG3_M      0x03
#define LP5562_EXEC_M           0x3F
#define LP5562_MASTER_ENABLE        0x40    /* Chip master enable */
#define LP5562_LOGARITHMIC_PWM      0x80    /* Logarithmic PWM adjustment */
#define LP5562_EXEC_RUN         0x2A
#define LP5562_ENABLE_DEFAULT   \
    (LP5562_MASTER_ENABLE | LP5562_LOGARITHMIC_PWM)
#define LP5562_ENABLE_RUN_PROGRAM   \
    (LP5562_ENABLE_DEFAULT | LP5562_EXEC_RUN)

/* OPMODE Register 01h */
#define LP5562_REG_OP_MODE      0x01
#define LP5562_MODE_ENG1_M      0x30
#define LP5562_MODE_ENG2_M      0x0C
#define LP5562_MODE_ENG3_M      0x03
#define LP5562_LOAD_ENG1        0x10
#define LP5562_LOAD_ENG2        0x04
#define LP5562_LOAD_ENG3        0x01
#define LP5562_RUN_ENG1         0x20
#define LP5562_RUN_ENG2         0x08
#define LP5562_RUN_ENG3         0x02
#define LP5562_ENG1_IS_LOADING(mode)    \
    ((mode & LP5562_MODE_ENG1_M) == LP5562_LOAD_ENG1)
#define LP5562_ENG2_IS_LOADING(mode)    \
    ((mode & LP5562_MODE_ENG2_M) == LP5562_LOAD_ENG2)
#define LP5562_ENG3_IS_LOADING(mode)    \
    ((mode & LP5562_MODE_ENG3_M) == LP5562_LOAD_ENG3)

/* BRIGHTNESS Registers */
#define LP5562_REG_R_PWM        0x04
#define LP5562_REG_G_PWM        0x03
#define LP5562_REG_B_PWM        0x02
#define LP5562_REG_W_PWM        0x0E

/* CURRENT Registers */
#define LP5562_REG_R_CURRENT        0x07
#define LP5562_REG_G_CURRENT        0x06
#define LP5562_REG_B_CURRENT        0x05
#define LP5562_REG_W_CURRENT        0x0F

/* CONFIG Register 08h */
#define LP5562_REG_CONFIG       0x08
#define LP5562_PWM_HF           0x40
#define LP5562_PWRSAVE_EN       0x20
#define LP5562_CLK_INT          0x01    /* Internal clock */
#define LP5562_DEFAULT_CFG      (LP5562_PWM_HF | LP5562_PWRSAVE_EN)

/* RESET Register 0Dh */
#define LP5562_REG_RESET        0x0D
#define LP5562_RESET            0xFF

/* PROGRAM ENGINE Registers */
#define LP5562_REG_PROG_MEM_ENG1    0x10
#define LP5562_REG_PROG_MEM_ENG2    0x30
#define LP5562_REG_PROG_MEM_ENG3    0x50

/* LEDMAP Register 70h */
#define LP5562_REG_ENG_SEL      0x70
#define LP5562_ENG_SEL_PWM      0
#define LP5562_ENG_FOR_RGB_M        0x3F
#define LP5562_ENG_SEL_RGB      0x1B    /* R:ENG1, G:ENG2, B:ENG3 */
#define LP5562_ENG_FOR_W_M      0xC0
#define LP5562_ENG1_FOR_W       0x40    /* W:ENG1 */
#define LP5562_ENG2_FOR_W       0x80    /* W:ENG2 */
#define LP5562_ENG3_FOR_W       0xC0    /* W:ENG3 */

/* Program Commands */
#define LP5562_CMD_DISABLE      0x00
#define LP5562_CMD_LOAD         0x15
#define LP5562_CMD_RUN          0x2A
#define LP5562_CMD_DIRECT       0x3F
#define LP5562_PATTERN_OFF      0

// TYPEDEFS, STRUCTS, ENUMS //////////////////////////////////////////////////

// GLOBAL DATA ///////////////////////////////////////////////////////////////

// PRIVATE VARIABLES /////////////////////////////////////////////////////////

// PRIVATE FUNCTION DECLARATIONS /////////////////////////////////////////////

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////

static int lp55xx_write(uint8_t addr, uint8_t value)
{
#if 0
    I2C_TransferSeq_TypeDef transfer = { 0 };
    transfer.addr = LP5562_I2C_SLAVE_ADDRESS;
    transfer.flags = I2C_FLAG_WRITE_WRITE;
    transfer.buf[0].data = &addr;
    transfer.buf[0].len = 1;

    transfer.buf[1].data = &value;
    transfer.buf[1].len = sizeof(value);

    if (PerformI2CTransfer(&transfer) != i2cTransferDone) {
        LOG_DEBUG("ERROR: %s write: 0x%x -> 0x%x\n", __FUNCTION__, addr, value);
        return -1;
    }
    LOG_DEBUG("%s write: 0x%x -> 0x%x\n", __FUNCTION__, addr, value);
    return 0;
#else
    return i2c_write_byte(LP5562_I2C_SLAVE_ADDRESS, addr, value);
#endif
}

#if 0
static int lp55xx_read(uint8_t addr, uint8_t* p_value)
{
#if 0
    I2C_TransferSeq_TypeDef transfer = { 0 };
    uint8_t value = 0;

    transfer.addr = LP5562_I2C_SLAVE_ADDRESS;
    transfer.flags = I2C_FLAG_WRITE_READ;
    transfer.buf[0].data = &addr;
    transfer.buf[0].len = 1;

    transfer.buf[1].data = &value;
    transfer.buf[1].len = sizeof(value);

    if (PerformI2CTransfer(&transfer) != i2cTransferDone) {
        LOG_DEBUG("ERROR: %s read: 0x%x -> 0x%x\n", __FUNCTION__, addr, value);
        return -1;
    }
    LOG_DEBUG("%s read: 0x%x -> 0x%x\n", __FUNCTION__, addr, value);
    *p_value = value;
    return 0;
#else
    return i2c_read_byte(LP5562_I2C_SLAVE_ADDRESS, addr, p_value);
#endif
}
#endif

static inline void lp5562_wait_opmode_done(void)
{
    /* operation mode change needs to be longer than 153 us */
    Delay(1);
}

static inline void lp5562_wait_enable_done(void)
{
    /* it takes more 488 us to update ENABLE register */
    Delay(1);
}

#if 0
static void lp5562_set_led_current(struct lp55xx_led *led, u8 led_current)
{
    u8 addr[] = {
        LP5562_REG_R_CURRENT,
        LP5562_REG_G_CURRENT,
        LP5562_REG_B_CURRENT,
        LP5562_REG_W_CURRENT,
    };

    led->led_current = led_current;
    lp55xx_write(led->chip, addr[led->chan_nr], led_current);
}
#endif

// TODO: implement these advanced features if customer need them
#if 0
static void lp5562_load_engine(struct lp55xx_chip *chip)
{
    enum lp55xx_engine_index idx = chip->engine_idx;
    u8 mask[] = {
        [LP55XX_ENGINE_1] = LP5562_MODE_ENG1_M,
        [LP55XX_ENGINE_2] = LP5562_MODE_ENG2_M,
        [LP55XX_ENGINE_3] = LP5562_MODE_ENG3_M,
    };

    u8 val[] = {
        [LP55XX_ENGINE_1] = LP5562_LOAD_ENG1,
        [LP55XX_ENGINE_2] = LP5562_LOAD_ENG2,
        [LP55XX_ENGINE_3] = LP5562_LOAD_ENG3,
    };

    lp55xx_update_bits(chip, LP5562_REG_OP_MODE, mask[idx], val[idx]);

    lp5562_wait_opmode_done();
}

static void lp5562_stop_engine(struct lp55xx_chip *chip)
{
    lp55xx_write(chip, LP5562_REG_OP_MODE, LP5562_CMD_DISABLE);
    lp5562_wait_opmode_done();
}

static void lp5562_run_engine(struct lp55xx_chip *chip, bool start)
{
    int ret;
    u8 mode;
    u8 exec;

    /* stop engine */
    if (!start) {
        lp55xx_write(chip, LP5562_REG_ENABLE, LP5562_ENABLE_DEFAULT);
        lp5562_wait_enable_done();
        lp5562_stop_engine(chip);
        lp55xx_write(chip, LP5562_REG_ENG_SEL, LP5562_ENG_SEL_PWM);
        lp55xx_write(chip, LP5562_REG_OP_MODE, LP5562_CMD_DIRECT);
        lp5562_wait_opmode_done();
        return;
    }

    /*
     * To run the engine,
     * operation mode and enable register should updated at the same time
     */

    ret = lp55xx_read(chip, LP5562_REG_OP_MODE, &mode);
    if (ret)
        return;

    ret = lp55xx_read(chip, LP5562_REG_ENABLE, &exec);
    if (ret)
        return;

    /* change operation mode to RUN only when each engine is loading */
    if (LP5562_ENG1_IS_LOADING(mode)) {
        mode = (mode & ~LP5562_MODE_ENG1_M) | LP5562_RUN_ENG1;
        exec = (exec & ~LP5562_EXEC_ENG1_M) | LP5562_RUN_ENG1;
    }

    if (LP5562_ENG2_IS_LOADING(mode)) {
        mode = (mode & ~LP5562_MODE_ENG2_M) | LP5562_RUN_ENG2;
        exec = (exec & ~LP5562_EXEC_ENG2_M) | LP5562_RUN_ENG2;
    }

    if (LP5562_ENG3_IS_LOADING(mode)) {
        mode = (mode & ~LP5562_MODE_ENG3_M) | LP5562_RUN_ENG3;
        exec = (exec & ~LP5562_EXEC_ENG3_M) | LP5562_RUN_ENG3;
    }

    lp55xx_write(chip, LP5562_REG_OP_MODE, mode);
    lp5562_wait_opmode_done();

    lp55xx_update_bits(chip, LP5562_REG_ENABLE, LP5562_EXEC_M, exec);
    lp5562_wait_enable_done();
}

static int lp5562_update_firmware(struct lp55xx_chip *chip,
                    const u8 *data, size_t size)
{
    enum lp55xx_engine_index idx = chip->engine_idx;
    u8 pattern[LP5562_PROGRAM_LENGTH] = {0};
    u8 addr[] = {
        [LP55XX_ENGINE_1] = LP5562_REG_PROG_MEM_ENG1,
        [LP55XX_ENGINE_2] = LP5562_REG_PROG_MEM_ENG2,
        [LP55XX_ENGINE_3] = LP5562_REG_PROG_MEM_ENG3,
    };
    unsigned cmd;
    char c[3];
    int program_size;
    int nrchars;
    int offset = 0;
    int ret;
    int i;

    /* clear program memory before updating */
    for (i = 0; i < LP5562_PROGRAM_LENGTH; i++)
        lp55xx_write(chip, addr[idx] + i, 0);

    i = 0;
    while ((offset < size - 1) && (i < LP5562_PROGRAM_LENGTH)) {
        /* separate sscanfs because length is working only for %s */
        ret = sscanf(data + offset, "%2s%n ", c, &nrchars);
        if (ret != 1)
            goto err;

        ret = sscanf(c, "%2x", &cmd);
        if (ret != 1)
            goto err;

        pattern[i] = (u8)cmd;
        offset += nrchars;
        i++;
    }

    /* Each instruction is 16bit long. Check that length is even */
    if (i % 2)
        goto err;

    program_size = i;
    for (i = 0; i < program_size; i++)
        lp55xx_write(chip, addr[idx] + i, pattern[i]);

    return 0;

err:
    dev_err(&chip->cl->dev, "wrong pattern format\n");
    return -EINVAL;
}

static void lp5562_firmware_loaded(struct lp55xx_chip *chip)
{
    const struct firmware *fw = chip->fw;

    if (fw->size > LP5562_PROGRAM_LENGTH) {
        dev_err(&chip->cl->dev, "firmware data size overflow: %zu\n",
            fw->size);
        return;
    }

    /*
     * Program momery sequence
     *  1) set engine mode to "LOAD"
     *  2) write firmware data into program memory
     */

    lp5562_load_engine(chip);
    lp5562_update_firmware(chip, fw->data, fw->size);
}

static void lp5562_write_program_memory(struct lp55xx_chip *chip,
                    u8 base, const u8 *rgb, int size)
{
    int i;

    if (!rgb || size <= 0)
        return;

    for (i = 0; i < size; i++)
        lp55xx_write(chip, base + i, *(rgb + i));

    lp55xx_write(chip, base + i, 0);
    lp55xx_write(chip, base + i + 1, 0);
}

/* check the size of program count */
static inline bool _is_pc_overflow(struct lp55xx_predef_pattern *ptn)
{
    return (ptn->size_r >= LP5562_PROGRAM_LENGTH ||
        ptn->size_g >= LP5562_PROGRAM_LENGTH ||
        ptn->size_b >= LP5562_PROGRAM_LENGTH);
}

static int lp5562_run_predef_led_pattern(struct lp55xx_chip *chip, int mode)
{
    struct lp55xx_predef_pattern *ptn;
    int i;

    if (mode == LP5562_PATTERN_OFF) {
        lp5562_run_engine(chip, false);
        return 0;
    }

    ptn = chip->pdata->patterns + (mode - 1);
    if (!ptn || _is_pc_overflow(ptn)) {
        dev_err(&chip->cl->dev, "invalid pattern data\n");
        return -EINVAL;
    }

    lp5562_stop_engine(chip);

    /* Set LED map as RGB */
    lp55xx_write(chip, LP5562_REG_ENG_SEL, LP5562_ENG_SEL_RGB);

    /* Load engines */
    for (i = LP55XX_ENGINE_1; i <= LP55XX_ENGINE_3; i++) {
        chip->engine_idx = i;
        lp5562_load_engine(chip);
    }

    /* Clear program registers */
    lp55xx_write(chip, LP5562_REG_PROG_MEM_ENG1, 0);
    lp55xx_write(chip, LP5562_REG_PROG_MEM_ENG1 + 1, 0);
    lp55xx_write(chip, LP5562_REG_PROG_MEM_ENG2, 0);
    lp55xx_write(chip, LP5562_REG_PROG_MEM_ENG2 + 1, 0);
    lp55xx_write(chip, LP5562_REG_PROG_MEM_ENG3, 0);
    lp55xx_write(chip, LP5562_REG_PROG_MEM_ENG3 + 1, 0);

    /* Program engines */
    lp5562_write_program_memory(chip, LP5562_REG_PROG_MEM_ENG1,
                ptn->r, ptn->size_r);
    lp5562_write_program_memory(chip, LP5562_REG_PROG_MEM_ENG2,
                ptn->g, ptn->size_g);
    lp5562_write_program_memory(chip, LP5562_REG_PROG_MEM_ENG3,
                ptn->b, ptn->size_b);

    /* Run engines */
    lp5562_run_engine(chip, true);

    return 0;
}

static ssize_t lp5562_store_pattern(struct device *dev,
                struct device_attribute *attr,
                const char *buf, size_t len)
{
    struct lp55xx_led *led = i2c_get_clientdata(to_i2c_client(dev));
    struct lp55xx_chip *chip = led->chip;
    struct lp55xx_predef_pattern *ptn = chip->pdata->patterns;
    int num_patterns = chip->pdata->num_patterns;
    unsigned long mode;
    int ret;

    ret = kstrtoul(buf, 0, &mode);
    if (ret)
        return ret;

    if (mode > num_patterns || !ptn)
        return -EINVAL;

    mutex_lock(&chip->lock);
    ret = lp5562_run_predef_led_pattern(chip, mode);
    mutex_unlock(&chip->lock);

    if (ret)
        return ret;

    return len;
}

static ssize_t lp5562_store_engine_mux(struct device *dev,
                     struct device_attribute *attr,
                     const char *buf, size_t len)
{
    struct lp55xx_led *led = i2c_get_clientdata(to_i2c_client(dev));
    struct lp55xx_chip *chip = led->chip;
    u8 mask;
    u8 val;

    /* LED map
     * R ... Engine 1 (fixed)
     * G ... Engine 2 (fixed)
     * B ... Engine 3 (fixed)
     * W ... Engine 1 or 2 or 3
     */

    if (sysfs_streq(buf, "RGB")) {
        mask = LP5562_ENG_FOR_RGB_M;
        val = LP5562_ENG_SEL_RGB;
    } else if (sysfs_streq(buf, "W")) {
        enum lp55xx_engine_index idx = chip->engine_idx;

        mask = LP5562_ENG_FOR_W_M;
        switch (idx) {
        case LP55XX_ENGINE_1:
            val = LP5562_ENG1_FOR_W;
            break;
        case LP55XX_ENGINE_2:
            val = LP5562_ENG2_FOR_W;
            break;
        case LP55XX_ENGINE_3:
            val = LP5562_ENG3_FOR_W;
            break;
        default:
            return -EINVAL;
        }

    } else {
        dev_err(dev, "choose RGB or W\n");
        return -EINVAL;
    }

    mutex_lock(&chip->lock);
    lp55xx_update_bits(chip, LP5562_REG_ENG_SEL, mask, val);
    mutex_unlock(&chip->lock);

    return len;
}
#endif

/*
static uint8_t get_reg_value_from_current(uint32_t uA)
{
    uint32_t 10th_mA = uA / 100;
    if (uint32_t > 0xFF)
    {
        LOG_DEBUG("invalid current: %d uA\n", uA);
        return 0;
    }
    return (uint8_t)10th_mA;
}
*/

// PUBLIC FUNCTION DEFINITIONS ///////////////////////////////////////////////
#if 0
static void _led_red(bool on)
{
    if (on)
    {
        lp55xx_write(LP5562_REG_R_PWM, 0xFF);
    }
    else
    {
        lp55xx_write(LP5562_REG_R_PWM, 0x00);
    }
}

static void _led_green(bool on)
{
    if (on)
    {
        lp55xx_write(LP5562_REG_G_PWM, 0xFF);
    }
    else
    {
        lp55xx_write(LP5562_REG_G_PWM, 0x00);
    }
}

static void _led_blue(bool on)
{
    if (on)
    {
        lp55xx_write(LP5562_REG_B_PWM, 0xFF);
    }
    else
    {
        lp55xx_write(LP5562_REG_B_PWM, 0x00);
    }
}
#endif

static int lp5562_init_device(void)
{
    int ret;
    uint8_t cfg = LP5562_DEFAULT_CFG;
    LOG_DEBUG("%s -->\n", __FUNCTION__);

    ret = lp55xx_write(LP5562_REG_ENABLE, LP5562_MASTER_ENABLE);
    if (ret)
        return ret;

    /* Set all PWMs to direct control mode */
    ret = lp55xx_write(LP5562_REG_OP_MODE, LP5562_CMD_DIRECT);
    if (ret)
        return ret;

    lp5562_wait_opmode_done();

    // Using internal clock
#if 0
    /* Update configuration for the clock setting */
    if (!lp55xx_is_extclk_used(chip))
        cfg |= LP5562_CLK_INT;
#else
    cfg |= LP5562_CLK_INT;
#endif

    ret = lp55xx_write(LP5562_REG_CONFIG, cfg);
    if (ret)
        return ret;

    /* Initialize all channels PWM to zero -> leds off */
    lp55xx_write(LP5562_REG_R_PWM, 0);
    lp55xx_write(LP5562_REG_G_PWM, 0);
    lp55xx_write(LP5562_REG_B_PWM, 0);
    lp55xx_write(LP5562_REG_W_PWM, 0);

#if 0
    /* Set LED map as register PWM by default */
    lp55xx_write(LP5562_REG_ENG_SEL, LP5562_ENG_SEL_PWM);
#endif

    return 0;
}


//////////////////////////////////////////////////////

void led_init(void)
{
    // Enable Chip
    GPIO_PinOutSet(GPIO_GROUP[LP5562_ENABLE_GPIO_ID].port, GPIO_GROUP[LP5562_ENABLE_GPIO_ID].pin);
    int ret = lp5562_init_device();
    if (ret != 0)
    {
        LOG_DEBUG("%s lp5562_init_device failed\n", __FUNCTION__);
    }
}

void led_deinit()
{
    lp55xx_write(LP5562_REG_ENABLE, 0);
    GPIO_PinOutClear(GPIO_GROUP[LP5562_ENABLE_GPIO_ID].port, GPIO_GROUP[LP5562_ENABLE_GPIO_ID].pin);
}

void ConfigLEDStatus(LED_Type_TypeDef led, LED_Status_TypeDef status, uint8_t timer)
{
    // TODO: implement I2C driver
    LOG_DEBUG("I2C LED driver is not implemented\n");
}

void LED_Toggle(void)
{
    // TODO: implement I2C driver
    LOG_DEBUG("I2C LED driver is not implemented\n");
}

LED_Type_TypeDef GetLEDType(Work_Mode_Def work_mode)
{
    // TODO: implement I2C driver
    //LOG_DEBUG("I2C DC is not implemented\n");
    return ledType_Interval;
}

LED_Status_TypeDef GetLEDStatus(Work_Mode_Def work_mode)
{
    // TODO: implement I2C driver
    LOG_DEBUG("I2C LED driver is not implemented\n");
    return ledStatus_Off;
}

int led_flash(uint8_t color)
{
    static int cnt = 0;

    if (cnt == 1000) {
        led_set_pwm(0, 0);
        led_set_pwm(1, 0);
        led_set_pwm(2, 0);
        led_set_pwm(color, 0xff);
    } else if (cnt == 2000) {
        led_set_pwm(0, 0);
        led_set_pwm(1, 0);
        led_set_pwm(2, 0);
        cnt = 0;
    }
    cnt++;
    return 0;
}

void led_flash_color(uint8_t color)
{
		led_set_pwm(0, 0);
		led_set_pwm(1, 0);
		led_set_pwm(2, 0);
		led_set_pwm(color, 0xff);
}

void led_flash_off() {

	led_set_pwm(0, 0);
	led_set_pwm(1, 0);
	led_set_pwm(2, 0);
}

int led_set_pwm(uint8_t color, uint8_t pwm)
{
    uint8_t addr[] = {LP5562_REG_R_PWM,
                      LP5562_REG_G_PWM,
                      LP5562_REG_B_PWM,
                      LP5562_REG_W_PWM};

    if (color > 3) return -1;
    if(lp55xx_write(addr[color], pwm))
    {
        return -1;
    }
    return 0;
}

int led_test(void)
{
    // Default current is 17.5 mA
    // Should be good for testing
    for(int i=0; i < 0xFF; i++)
    {
        if (led_set_pwm(2,0))
        {
            return -1;
        }
        Delay(5);
        if (led_set_pwm(2,i))
        {
            return -1;
        }
    }
    return 0;
}

#endif

// PRIVATE FUNCTION DEFINITIONS //////////////////////////////////////////////

