s2lm_BPI_2.7_20171102.tar.bz2
====================================================
Patch to SDK2.6.0 (s2l_2.6.0_20150928.tar.xz)

Usage:
    1. Extract Linux kernel in s2l_linux_sdk.
        $ cd s2l_linux_sdk/ambarella
        $ source build/env/Linaro-multilib-gcc4.9.env
		$ cd boards/hawthorn
        $ make sync_build_mkcfg
        $ make s2l_ipcam_config

    2. Extract s2lm_BPI_2.7_20171102.tar.bz2 under s2l_linux_sdk/ (in the same folder as ambarella/)
        $ cd s2l_linux_sdk/
        $ tar xjf s2lm_BPI_2.7_20171102.tar.bz2
        
    3. Apply the patch for source files.
        $ cd s2lm_BPI_2.7_20171102
        $ chmod +x apply.sh
        $ ./apply.sh
        
    4. Make
        $ cd s2l_linux_sdk/ambarella/boards/s2lm_elektra
	$ source ../../build/env/armv7ahf-linaro-gcc.env
        $ make sync_build_mkcfg
        $ make s2lm_elektra_fastboot_config
        $ make defconfig_public_linux
        $ make
        
