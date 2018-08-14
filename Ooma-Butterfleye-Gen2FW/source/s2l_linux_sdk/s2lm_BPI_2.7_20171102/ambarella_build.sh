echo 'Copy file <../ambarella/build/env/Linaro-armv7ahf-gcc4.9.env> to <ambarella/build/env/Linaro-armv7ahf-gcc4.9.env>.\n'
mkdir -p ../ambarella/build/env
cp ambarella/build/env/Linaro-armv7ahf-gcc4.9.env ../ambarella/build/env/Linaro-armv7ahf-gcc4.9.env

echo 'Remove file <../ambarella/build/env/Linaro-multilib-gcc4.9.env>.\n'
rm ../ambarella/build/env/Linaro-multilib-gcc4.9.env

echo 'Copy file <../ambarella/build/env/Linaro-multilib-gcc4.9_2014.09.env> to <ambarella/build/env/Linaro-multilib-gcc4.9_2014.09.env>.\n'
mkdir -p ../ambarella/build/env
cp ambarella/build/env/Linaro-multilib-gcc4.9_2014.09.env ../ambarella/build/env/Linaro-multilib-gcc4.9_2014.09.env

echo 'Copy file <../ambarella/build/env/armv7ahf-linaro-gcc.env> to <ambarella/build/env/armv7ahf-linaro-gcc.env>.\n'
mkdir -p ../ambarella/build/env
cp ambarella/build/env/armv7ahf-linaro-gcc.env ../ambarella/build/env/armv7ahf-linaro-gcc.env

echo 'Copy file <../ambarella/build/bin/create_private_mkcfg> to <ambarella/build/bin/create_private_mkcfg>.\n'
mkdir -p ../ambarella/build/bin
cp ambarella/build/bin/create_private_mkcfg ../ambarella/build/bin/create_private_mkcfg

