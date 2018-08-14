CC=gcc 
CFLAGS=-I/home/naresh.k/Projects/Butterfleye/Ambarella/OOMA_2.7_BPI_SDK_release/S2Lm/SDK/s2l_linux_sdk/ambarella/app/ipcam/pp_cam/cloud
LDFLAGS=-lmosquitto -L/home/naresh.k/Projects/Butterfleye/Ambarella/OOMA_2.7_BPI_SDK_release/S2Lm/SDK/s2l_linux_sdk/ambarella/app/ipcam/pp_cam/cloud/mosquitto/lib

all:
	gcc cloud_thread.c client_shared.c config.c $(CFLAGS)  $(LDFLAGS) -o pub_client
