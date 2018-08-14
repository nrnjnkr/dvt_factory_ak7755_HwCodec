echo 'Remove file <../ambarella/boards/s2lm_kiwi/config/s2lm_kiwi_emmc_cpio_config>.\n'
rm ../ambarella/boards/s2lm_kiwi/config/s2lm_kiwi_emmc_cpio_config

echo 'Copy file <../ambarella/boards/s2lm_kiwi/bsp/iav/imx322.bin> to <ambarella/boards/s2lm_kiwi/bsp/iav/imx322.bin>.\n'
mkdir -p ../ambarella/boards/s2lm_kiwi/bsp/iav
cp ambarella/boards/s2lm_kiwi/bsp/iav/imx322.bin ../ambarella/boards/s2lm_kiwi/bsp/iav/imx322.bin

