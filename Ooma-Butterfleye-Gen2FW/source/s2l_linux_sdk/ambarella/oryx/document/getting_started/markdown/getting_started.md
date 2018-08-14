<!--* getting_started.md
 *
 * History:
 *   2016-5-23 - [ghzheng] created file
 *
 * Copyright (c) 2016 Ambarella, Inc.
 *
 * This file and its contents ("Software") are protected by intellectual
 * property rights including, without limitation, U.S. and/or foreign
 * copyrights. This Software is also the confidential and proprietary
 * information of Ambarella, Inc. and its licensors. You may not use, reproduce,
 * disclose, distribute, modify, or otherwise prepare derivative works of this
 * Software or any portion thereof except pursuant to a signed license agreement
 * or nondisclosure agreement with Ambarella, Inc. or its authorized affiliates.
 * In the absence of such an agreement, you agree to promptly notify and return
 * this Software to Ambarella, Inc.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF NON-INFRINGEMENT,
 * MERCHANTABILITY, AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL AMBARELLA, INC. OR ITS AFFILIATES BE LIABLE FOR ANY DIRECT,
 * INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; COMPUTER FAILURE OR MALFUNCTION; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *-->
![Oryx logo](img/Oryx-Logo.jpg)
#Getting started

##Preparation

There are 3 kinds of things user should prepare:

* [Set up user's building machine](#Set up user's building machine): user can build codes, download binaries and do other developing things under this environment.
* [Install Toolchain and AmbaUSB](#Install Toolchain and AmbaUSB): A tool set for building codes and a tool for downloading binaries to the EVK board.
* [Prepare a Ambarella EVK board](#Prepare a Ambarella EVK board): For example, Antman.

##Building and downloading

If user have prepared all things mentioned at last chapter, user can build the codes int the next step. The document will take **S3L SDK** and **Antman** with ov2718 sensor Board for an example.

### Extract SDK package and build it.

Following the commands below:

```shell
 # tar xJvf s3l_linux_sdk_2016xxxx.tar.xz
 # cd s3l_linux_sdk/ambarella
 # source build/env/armv7ahf-linaro-gcc.env
 # cd boards/s3lm_antman/
 # make sync_build_mkcfg
 # make s3lm_2718_config
 # make defconfig_public_linux
 # make -j8
```

### Download firmware to board.

#### Sets on Serial terminal

##### Normal burning

In the serial console, press and hold the **Enter** key on the keyboard and then press the hardware **RESET** button. The development platform will switch to the AMBoot shell as shown in below. At last ,type the **u** to launch the mode of loading.

![image_loadshell](img/loadshell.png)

##### Burning in USB Boot Mode

If there is nothing to print on terminal, user need burn the board in USB Boot Mode. Toggle the USB switch on the debug board from 0 to 1 as shown below:


![image_hardburn](img/antman.JPG)

#### On PC side

Finish building, user will find the firmware: ```s3l_linux_sdk/ambarella/out/s3lm_antman/images/bst_bld_pba_kernel_lnx_release.bin```.
Following the steps shown in below:

**Step 1**: Launch AmbaUSB, select **S3L(H12)** for Boards Filter.

**Step 2**: Select **S3LM.DDR3.NAND** for the board's config menu, and the tool will auto detect the board.

**Step 3**: Select the binary from output location.

**Step 4**: Click the **blue** button.

![image_ambaUSBLoad](img/loadSDK.png)

```
----- Report -----
bst: success
bld: success
hal: success
pri: success
lnx: success
      - Program Terminated -
```

Finally, The serial console of the development platform will indicate the upgrading process. When the firmware is downloaded, the message "Program Terminated" will be displayed on the serial console. If users downloaded in USB Boot Mode, please toggle the USB switch back to 0. Refer to the information above.

>Note that there are minor differences in steps for choosing Chip Filter and Board's Config. Users should choose related sets based on chips.(up steps just an example for Antman)



##Getting it run

By operations above, users now can play live videos and audios through few simple procedures below:

Firstly, run the command of **apps_launcher** to start the service needed in the **Serial terminal**.

![image_laucher](img/laucher.png)

>The apps_launcher is designed to be a “programmer launcher tool” to launch all related “Air applications”
together and the parent process of all services. It manages the processes of img_svc, media_svc, video_svc, audio_svc, event_svc, net_svc and sys_svc.

Secondly, launch the VLC and open the media in the address of ``` rtsp://10.0.0.2/video=video0,audio=acc```

![imag_vlc](img/vlc.png)

Congratulations! Now user can play videos and make further progress.

## An example of customizing user's recording

The SDK allows users to customize a variety of configurations in different ways. The main procedures of configuration are shown in below:


![image_path](img/architecture.png)

The mechanisms of making coustomised functional configurations are separated into two ways:

- Editing static configs and restart services after editing config files such as source_buffer.acs, stream_fmt.acs, stream_cfg.acs etc.
- Real-time control through cmds such as test_video_service_dyn_air_api and so on.


In this chapter, The document will show an example of customized configuration items. The processes are shown in below:

![image_struct_path](img/example_struct.png)

There are 3 ways to set configuration items into config files:

* **use AIR api cmds to set**
* **set configuration from web**
* **directly modify the config files**

 Users prefer to use Air API commands or webpage to set configurations for decreasing risks of system errors. Before using these two ways to set, please run **apps_launcher** first.

### Set Buffers configs

> The Buffer mentioned in this document is specified. For the detail information, users could refer to related document such as Video Processing released together with SDK.

Use the command below to set buffers items:

```shell
# test_video_service_cfg_air_api -b 0 -t ENCODE -W 1920 -H 1080 -b 1 -t ENCODE -W 720 -H 480 -b 2 -t ENCODE -W 1280 -H 720
```

```shell
-b: Specify the buffer ID.
-t: The type of source buffer OFF|ENCODE|PREVIEW
-W: Source buffer width
-H: Source buffer height
```

> Note: All commands have the help text for typing: `test_vide_service_cfg_air_api --help`.

Using command to change configuration has the same effects as editing the config file of **`/etc/oryx/video/source_buffer.acs`**.

> The file with postfix of .acs is the config file specified by Amba.

![image_buffers_path](img/buffer.png)

"size" means buffer resolution.

"type" means buffer state, there are total 3 kinds of state: **encode**, **preview**, **off**.

> The config files of buffers and streams are in directory of `/etc/oryx/video/`. Users may check it for all config files. In this chapter, it just expand what is refered to.

### Set Streams configs

> The Stream mentioned in this document is specified. For the detail information, users could refer to related document such as Video Processing released together with SDK.


Use the command below to set streams items:


```shell
#test_video_service_cfg_air_api -s 0 -t H265 -W 1920 -H 1080 -s 1 -t H264 -W 720 -H 480 -s 2 -t MJPEG -W 1280 -H 720
```

```shell
-s: Specify the stream ID
-t: The type of encode
-W: Stream width
-H: Stream height
```

Using command to change configuration has the same effects as editing the config file of **`/etc/oryx/video/stream_fmt.acs`**.

![image_stream](img/stream_fmt.png)

"type": means encode type. As **H264**, **H265**, **MJPEG**

"source": means the source buffer which stream use to encode.

"enc_rec": encode resolution specified by struct.


### Set mp4 recording configs

The system supports two independent mp4 recording and related event recording. For convenient, the document just takes an example of mp4-0.
For recording mp4 files,users should set configuration items along **engine**->**filter**->**muxer**.

Firstly, edit **engine** configuration items in **`/etc/orxy/stream/engine/record-engine.acs`**, shown in below:

![image_engine](img/mp4-recording-engine.png)

The symbol of **"--"** means ignore the reladet item.


> Notes: The function of recording can be only set with editing config files.

then, edit **filter** configuration items in **`/etc/oryx/stream/filter/filter-file-muxer-0.acs`**, shown in below:

![image_filter](img/mp4-file-filter-0.png)

choose **"mp4","mp4-event0"** to **media_type**

Furthermore, edit **muxer** configuration items in **`/etc/oryx/stream/muxer/muxer-mp4-0.acs`**, shown in below:

![image_muxer_mp4](img/mp4-muxer-0.png)

Specify the location of storage in the item of **file_location**. If set the flag of **file_location_auto_parse** to **true**, it will specify storage location by parsing devices of USB and SD card. This document chooses default video ID 0 and other configuration items for recording in this example.

> Generally, the default path of USB device is /storage/sda and the default path of SD card is /sdcard/.

### Set mp4 event recording configs

The system allows user to record a video clip, start time is n minutes before users' trigger time and stop time in m minutes after trigger time.
With the config file below, choose the item of **media_type**, and make sure the item of mp4-event0 has been selected.
![image_avqueue_path](img/mp4-file-filter-0.png)

**history_duration**: the duration before users' trigger event time.

**future_duration**: the duration after uses' trigger event time.

Furthermore, edit **`/etc/oryx/stream/muxer/muxer-mp4-event-0.acs`**, most of the items are same as muxer-mp4.acs. For detail configuration as below:

![image_mp4_event0_path](img/mp4-event-0.png)



### Set mjpeg recording configs

For recording jpeg files, users should edit other two config files as **`/etc/oryx/stream/filter/filter-direct-muxer.acs`** and **`/etc/org/stream/muxer/muxer-jpeg.acs`**.

For **filter-direct-muxer.acs**:

![image_filter_direct_path](img/filter-direct-muxer.png)

For **muxer-jpeg.acs**:

![image_muxer_jpeg_path](img/muxer-jpeg.png)

### Set mjpeg event recording configs

For recording jpeg event files, users should edit another config file as **`/etc/oryx/stream/muxer/muxer-jpeg-event0.acs`** based on last chapter.

For **muxer-jpeg-event0.acs**:

![image_jpeg_event_path](img/muxer-jpeg-event0.png)


### Check the functions

At last, run the **#apps_launcher** to start the services. After that, user can try some cases to check the functions.

* Step 1: Check RTSP live streaming

    For playing video,open VLC with the address of `rtsp://10.0.0.2/video=video0,audio=acc`, also users could specify other video such as video1,video2 at this case.

* Step 2: Check mp4 file

    Type the commands of `test_media_service_air_api -m 0 --start-file-writing` to start file writing and `test_media_service_air_api -m 0 --stop-file-writing` to stop file writing. If it works well, user can find mp4 files in the path of file_location(e.g.`/sdcard/video/1`).

    > test_media_service_air_api -m 0: the format is **test_media_service_air_api -d (1 << muxer_id)**, muxer_id can be found in /etc/oryx/stream/muxer/muxer-mp4-0.acs

* Step 3: Check mp4 event recording

    Type the command of `test_media_service_air_api -v 0 --h26x-event-start --history-duration 10 --future-duration -1 ` to trigger the event. After that, user can find mp4 event files in the path of file_location(e.g.`/sdcard/video/event0`).
> history-duration: how many seconds save before trigger event; future-duration: how many seconds save after trigger event.

* Step 4: Check mjpeg recording

    Type the command of `test_media_service_air_api -d 256` to start jpeg file writing, `test_media_service_air_api -b 256` to stop jpeg file writing. If it works well, user can find jpeg files in the path of file_location(e.g.`/sdcard/video/jpeg`).

* Step 5: Check mjpeg event recording

    Type the command of `test_media_service_air_api -v 3 --jpeg-event-start --jpeg-history-num 2 -- jpeg-future-num 2` to trigger jpeg event. After that, user can find jpeg files in the path of file_location(e.g.`/sdcard/video/jpeg/event0`).
> jpeg-history-num: how many jpeg pictures save before trigger event; jpeg-future: how many jpeg pictures save after trigger event.

* Step 6: Check data exporting

    Type the command of `test_oryx_data_export -f /sdcard/export/`. It will create with related files such as "export_audio_0.acc", "export_audio_1.opus", "export_video_0.h265" in the path of /storage/sda/export/.

### Use config files
If users are bored about config sets or something failure during above steps, the SDK also provides config files already configured above examples. Users could directly use them in the path of `SDK/oryx/config_set/config_samples/` by the name of examples.tar.xz.

Users should extract the config files by the command of `tar -Jxf examples.tar.xz`. Then, cover config files in the path of `/etc/oryx/` with  folds of `video` and `stream`. At last, run the service with `apps_launcher`.

### CGI
Oryx Midlleware supports users interactively manipulate options through Web service.

Now the document begins with easy guide for users to access related services.

Type `10.0.0.2/oryx` into the address of web-browser with both username and password of `admin`.
Until then, it will access the Web as shown in below:

![image_cgi_full](img/cgi_full.png)

Now, users can make customized configurations with specific demands. As a guide, the document will show some details with examples shown in below:

* Start/Stop encode

* Make record

#### Start/Stop record

Choose the item of Video>>Encode, the web will go to web-site as shown in below:

![image_encode_path](img/cgi_encode.png)

Users can start/stop encode by triggering the button, also some parameters could be set through the list.

#### Event record

Choose the item of Media>>Recording, the web will go to the location as shown in below:

![image_record_path](img/cgh_record.png)

**Step 1:** Choose the format of file

**Step 2:** Trigger the button of **Start**

Trigger button of stop when it finishes. Then, user can find video file on specified directory.

Until now, users may customize video features.

#### Add overlay

Users could add overlays with customized design. The CGI allow users to add variety types of overlay, such as **String**, **Time**
, **Picture** and **Animation**. In this section, it will give an example of how to add overlay of type of **String**.

**Step 1:** Area Manipulate

The overlay added based on mechanism of **Area** and **Data Block**. The system firstly divide the screen into different areas, users
could set areas' color as background color. Then users could set data blocks in the areas. The overlay is eventually added in data block.
Firstly, user should get information of Overlay Parameters by the list button of **Get**, Check the areas already in screen, and make sure
the specified parameters. The related parameters are shown in below:

![image_cgi_paramters_path](img/CGI_overlay_parameters.png)

Based on the limitation of area number, the example don't add area in this section. The example add overlay in default area.


**Step 2:** Data Block Manipulate

Users could simply set data block to add overlay, type the list button of Add, then set the parameters shown in below:

![image_cgi_block_path](img/CGI_overlay_block.png)

It's should be reminded that the size of data block should not exceed the area.

Based on parameters set above, users could get the overlay shown in below:

![image_cgi_show_overlay](img/CGI_overlay_show.png)

# Oryx Middleware Framework
S2L/S2E/S3L series camera middleware framework has a unique name called "Oryx".
Oryx stands for strong, tough, robust, agile, and teamwork. So, this middleware
is also designed to make a high quality Camera reference design and help
customers to quickly customize it for products.
## Software Architecture
### System Block Diagram
The system block diagram is divided into 6 layers:

![image_oryx](img/Oryx_arc_final.png)

Oryx is in the layer of fifth, it allows users to customize products by up-layer
such as Home Kit, CGI and other Users apps.
### Work Flow in Oryx
The main work flow and function details in oryx are shown in below:

![image_oryx_fun](img/oryx_fun.png)
### Files and Directories in Oryx
All Oryx components are under `$(ambarella)/oryx/`, they are organized into
different folders by function.

![image_oryx_file_tree](img/oryx-file-tree.png)

# Features on Oryx

## Set Feature of LBR

LBR means low bit rate, it allow users to use low bit rate video optimization algorithm developed by Ambarella.

When users want to enable the feature of LBR. Please type the command of `test_video_service_cfg_air_api -s 0 --feature-bitrate lbr`

```shell
-s : Specify stream 0
--feature-bitrate: Specify the type of bitrate control
```
To enable the configuration, type the reset command of `test_video_service_cfg_air_api --apply`

Using the commands above also has the same effect to edit the config file of `/etc/oryx/video/features.acs` as shown in below:

![image_feature_path](img/feature-lbr.png)

## Set Feature of HDR

The system supports HDR mode set based on sensor feature, user could customize related feature as wish.

When users want to enable the feature of HDR. Please type command of `test_video_service_cfg_air_a --feature-hdr 2x`

To enable the configuration, type the reset command by `test_video_service_cfg_air_api --apply`

Using the commands above also has the same effect to edit the config file of `/etc/oryx/video/features.acs` as shown in below:

![image_hdr_path](img/hdr.png)

> The HDR feature should be supported by sensor, user should first make sure the sensor whether support HDR. User could type the
command **test_video_service_dyn_air_api --show-vin-info** to make sure the HDR feature.


## Set Feature of EIS

EIS(Electronic Image Stabilization) is a process where image stability is controlled through electronic processing procedures. If the device sensors detect camera shake, EIS responds by slightly moving the image so that it remains in the same place.

When users want to enable the feature of EIS. Please type the command of 'test_video_service_cfg_air_api --feature-dewarp eis'

To enable the configuration, type the reset command by `test_video_service_cfg_air_api --apply`

Modify the parameter of `dewarp_func = eis`  in `etc/oryx/video/features.acs`.

The EIS configuration file `etc/oryx/video/eis.acs` should be also modified to create a better EIS effect. For example, the parameter of `lens_focal_length_in_um` should be changed based on the focal length of your lens.

Using the commands above also has the same effect to edit the config file of `/etc/oryx/video/features.acs` as shown in below:

![image_eis_path](img/eis.png)

> ***NOTE: Before starting, make sure that there is a 6-axis gyro sensor mounted on the main board.***


## IR-CUT Filter Set

Currently, it is supported IR-CUT filter control in related boards. User could use the commend of **test_image_service_air_api**
to set IR-CUT filter off and on.

To turn on, type the command **test_image_service_air_api --ir-led-mode 1**, if user want to turn on in night, should type
**test_image_service_air_api --ir-led-mode 1 --day-night-mode 1**.

To turn off, type the command **test_image_service_air_api --ir-led-mode 0**, if user want to turn off in day, should type
**test_image_service_air_api --ir-led-mode 0 --day-night-mode 0**.

The IR-CUT filter is actually controlled by GPIO output, so user could also set it by editing file.

The GPIO's number is defined in:

ambarella/boards/s3lm_antman/devices.h (take s3lm_antman for example, if user use s2lm_ironman, just replace the board's name)

Use antman as example:

```
/* IR-CUT */
#define GPIO_ID_IR_CUT_CTRL 106

```

```shell
# echo 106 > /sys/class/gpio/export
# echo out > /sys/class/gpio/gpio106/direction
# echo 1 > /sys/class/gpio/gpio106/value
# echo 0 > /sys/class/gpio/gpio106/value
```

If the user finds the IR-CUT switcher is on the wrong state(removed on day mode, covered on night mode), user needs to revert
the polarity of IR-CUT switcher define in:

**ambarella/packages/img_mw/arch_s3l/include/mw_ir_led.h**

```C
typedef enum {
  IR_CUT_NIGHT = 0,
  IR_CUT_DAY = 1,
} ir_cut_state;

```
## Event Plugin

The SDK allows users to add events based on plugin. So far, it has key event, audio alarm.
So far, it has key event, audio alarm. The SDK allows users to add event based on plugin.

### Key input event



### Audio alarm event

## Firmare Upgrade
The SDK allows users to upgrade firmware with specified commands.

Type the command of `test_system_service_air_api`, then the program will run to guide user to execute related procedures.

As an example, the document shows the details of upgrading firmware in below:

Step 1. Type `test_system_service_air_api`

It will run the program that guide users to set configuration items as shown in in below:

![image_upgrade](img/system-1.png)

Step 2. Choose the item of **upgrade settings** by typing number **6**, go into the stage shown in below:

![image_ugrade](img/system-2.png)

Step 3. Choose the item of **set upgrade mode** by typing **m**, in this example, choose mode 1 that is only for upgrading.

Step 4. Set the path of dst file by typing **p**, in this case, system has already mounted host share fold to development platform. The full path of firmware is `/mnt/firmware/AmSDK.bin`.

![image_upgrade](img/system-3.png)

Step 5. Start to upgrade by typing **s**, then the system will begin to upgrade and reboot.

## DSP Mode switch and native video playback

The system allows user to playback video files with board, display on connected video output port(maybe HDMI, CVBS, or Digital (LCD)).

Please also be noted that DSP would need do a mode switch between Encode mode and Decode mode. Typically would be ENCODE mode <--> IDLE mode <--> DECODE mode.

Type the commands of `test_video_service_dyn_air_api --pause` to pause all encoding related things. DSP also back to IDLE mode.

Type the commands of `test_video_service_dyn_air_api --resume` to resume all encoding related things. DSP also resume to ENCODE mode.

When DSP is in IDLE mode, it's ready to playback video files(.mp4)

Type the commands of `test_playback_service_air_api -p [filename] -V720p --hdmi` to play file, [filename] is the video file, '-V720p' specify the VOUT mode, '--hdmi' specify the VOUT device type. user may see all available VOUT mode and VOUT device types.

During playback, there are several runtime commands:

|Press Key | Playback function |
|:-------------|:-------|
|' ' + ENTER|Pause/Resume|
|'s' + ENTER|Step play|
|'f%d' + ENTER| %d x speed fast forward|
|'b%d' + ENTER| %d x speed fast backward|
|'F%d' + ENTER| %d x speed fast forward, from begin of file|
|'B%d' + ENTER| %d x speed fast backward, from end of file|
|'g%d' + ENTER| Seek to %d ms|
|'q' + ENTER| Quit playback|
|CTRL + 'c'| Quit playback|


Typical Playback command flow:


`test_video_service_dyn_air_api --pause`

`test_playback_service_air_api -p [filename] -V720p --hdmi`

runtime playback command, quit playback

`test_video_service_dyn_air_api --resume`


## External USB camera as EFM input

The system allows user to connect an external USB camera, configure it as EFM source (secondary stream), then it has a simple dual camera solution.

USB camera may have two pixel format: YUYV(YUV422) or MJPEG, the system supports both of them, but please note that due to the USB transfer speed, YUYV USB camera may not achieve full FPS(24). MJPEG USB camera may achieve its full FPS(24). MJPEG camera would cost more CPU cycle due to SW MJPEG decoding.


To configure USB camera as EFM input in Oryx:

enable EFM in mode's limit configure file(for example, /etc/oryx/video/adv_hdr_mode_resource_limit.acs), change to 'enc_from_mem_possible    = true', as shown in below:

![image_efm_path](img/efm-limit.png)

Change EFM's buffer size to USB camera's resolution (640x480): change '/etc/oryx/video/source_buffer.acs' change efm buffer's size to 640x480, as shown in below:

![image_efm_path](img/efm-source.png)

Change secondary stream's source to EFM: change '/etc/oryx/video/stream_fmt.acs', change Stream B's 'source = 6, enable = true', as shown in below:

![image_efm_path](img/efm-stream.png)


Type the commands of `test_efm_src_service_air_api --feed-usbcam -B` to start USB camera's feeding to EFM (stream B). '-A' means stream A, '-B' means stream B.

To specify prefer format of USB camera, use '--capyuv' to select YUYV as preferred format, use '--capjpeg' to choose MJPEG as preferred format.

To specify prefer resolution of USB camera, use '-s %dx%d' to specify preferred resolution (width x height).

To specify prefer FPS of USB camera, use '-f %d' to specify preferred FPS.

For example, to specify USB camera's resolution to 640x480, 15 FPS, YUYV format, the command line is 'test_efm_src_service_air_api --feed-usbcam -B -s 640x480 -f 15 --capyuv'

Type the commands of `test_efm_src_service_air_api --end-feed` to end USB camera's feeding to EFM.

##Multi Vin

Oryx can support multi vin on s3l and s5l. Here, multi vin on s5l is introduced.

### Vin Configuration

To enable multi vin in oryx, user needs to configure the VIN number in private drivers configuration:

```C
make menuconfig
  [*] Ambarella Linux Configuration --->
    [*] Ambarella Private Drivers Configurations --->
      [*] Define Common Macros --->
        (1) Max Vin Channel Number
          Input 2 or 3 or 4 (Depends on the real channel number)
```

When firmware has been generated and burned into board, user needs to configure vins (/etc/oryx/video/vin.acs).

![image_multi_vin_path](img/multi-vin-config.png)

Items in vin.acs must be equal to the amount of real vins on board. Mode and fps of each item have to be configured correctly.


### Channel Configuration

User needs to configure channels' information by editing '/etc/oryx/video/multi_vin_chan.acs' like the following:

![image_multi_vin_path](img/multi-chan-config.png)

There are three important properties of each channel that user needs to handle carefully:

* chan_id: Identifier of current channel which can be located in other configuration file (/etc/oryx/video/multi_vin_canvas.acs);

* vsrc_id: Index of vin and it is useful when user needs to use virtual multi channel feature on s5l;

* buffer:  Each channel have five source buffers and user has to configure them carefully. (For detail info of configuring this property, please see in "example" section)

Items in multi_vin_chan.acs can be equal to or less than the max VIN Channel Number configured in private drivers.


### Canvas Configuration

Multi vin imports a new concept: canvas. It is similar to source buffer in single vin system and that means all stream's video data are coming from canvas.

User can arrange videos from different channel into one canvas in two ways:

* Stitching: combining the FOV of each source buffer output of different channel together into one canvas.

* PIP(Picture In Picture) view: Put a video with a small resolution above on a large video.

User can edit configuration file '/etc/oryx/video/multi_vin_canvas.acs' to implement stitching or PIP view like the following:

![image_multi_vin_path](img/multi-canvas-config.png)

There are also three important properties of each canvas that user needs to configure carefully:

* type: canvas can be configured as "encode" or "prev";

* size: canvas's size;

* source: each canvas can contain just one source buffer or several source buffers of different channel.


### Stream Configuration

Stream's configuration is similar to single vin system. The only difference is the source property of each stream:

* In single vin system, the source property of each stream is the source buffer id (0, 1, 2, 3).

* In multi vin system, the source property of each stream is the canvas id which configured in '/etc/oryx/video/multi_vin_canvas.acs'.

For more detailed info of configuring streams, user can jump to the "Set Streams configs" of "An example of customizing user's recording".

### Example

Let's focus on the second canvas's configuration of the above picture: it is combined by channel 0, 1 and 2's second source buffer and the size is 1360x720. 

You may be curious about the layout of this canvas, stitching or PIP view? Actually, it is decided by source buffer configuration of '/etc/oryx/video/multi_vin_chan.acs'. Here,  a general example is used to explain how to configure '/etc/oryx/video/multi_vin_chan.acs' and '/etc/oryx/video/multi_vin_canvas.acs'.

![image_multi_vin_path](img/multi-canvas-stitch.png)

Let see the above picture: size is (W0 + W1) x H0 and combined by channel 0, 1 and 2's main source buffer. Main source buffer of channel 0 is configured as W0 x H0, channel 1' is configured as W1 x H1 and channel 2's is configured as W1 x H2. (H0 = H1 + H2). 

Following is the configuration of channel's main source buffer and canvas: 

'/etc/oryx/video/multi_vin_chan.acs'

![image_multi_vin_path](img/example-stitch-chan-config.png)

'/etc/oryx/video/multi_vin_canvas.acs'

![image_multi_vin_path](img/example-stitch-canvas-config.png)

#Commands

The Oryx organizes function by created specified services, users could customized related functions by delivering commands obeyed default
syntax to related service.

##Syntax
```C
void method_call(uint32_t cmd_id, void *msg_data, int msg_data_size, am_service_result_t *result_addr, int result_max_size)
```
|Item|Description|
|:---|:----------|
|cmd_id|Command ID|
|msg_data|Input parameter data struct pointer|
|msg_data_size|Input parameter size|
|result_addr|Function call result data structure pointer|
|result_max_size|Function call result data structure|

**Result data struct**

```C
struct am_service_result_t
{
    /*!
     * method_call ret value, see AM_RESULT
     */
    int32_t ret;
    /*!
     * report service current state
     */
    int32_t state;
    /*!
     * data if used
     */
    uint8_t data[SERVICE_RESULT_DATA_MAX_LEN];
};
```

The SDK defines a group of result value to demonstrate the process of commands invoking, and also defines a group of state number to
demonstrates service state. They are shown in below:

```C
enum AM_RESULT
{
  AM_RESULT_OK                   = 0,   //!< Success
  AM_RESULT_ERR_INVALID          = -1,  //!< Invalid argument
  AM_RESULT_ERR_PERM             = -2,  //!< Operation is not permitted
  AM_RESULT_ERR_BUSY             = -3,  //!< Too busy to handle
  AM_RESULT_ERR_AGAIN            = -4,  //!< Not ready now, need to try again
  AM_RESULT_ERR_NO_ACCESS        = -5,  //!< Not allowed to access
  AM_RESULT_ERR_DSP              = -6,  //!< DSP related error
  AM_RESULT_ERR_MEM              = -7,  //!< Memory related error
  AM_RESULT_ERR_IO               = -8,  //!< I/O related error
  AM_RESULT_ERR_DATA_POINTER     = -9,  //!< Invalid data pointer
  AM_RESULT_ERR_STREAM_ID        = -10, //!< Invalid stream ID
  AM_RESULT_ERR_BUFFER_ID        = -11, //!< Invalid source buffer ID
  AM_RESULT_ERR_FILE_EXIST       = -12, //!< Directory or file does not exist
  AM_RESULT_ERR_PLATFORM_SUPPORT = -13, //!< Not supported by platform
  AM_RESULT_ERR_PLUGIN_LOAD      = -14, //!< Plugin is not loaded
  AM_RESULT_ERR_MODULE_STATE     = -15, //!< Module is not create or is disabled
  AM_RESULT_ERR_ALREADY          = -17, //!< Operation already in progress
  AM_RESULT_ERR_CONN_REFUSED     = -18, //!< Connection refused by server
  AM_RESULT_ERR_NO_CONN          = -19, //!< No connection to server
  AM_RESULT_ERR_SERVER_DOWN      = -20, //!< Server is down
  AM_RESULT_ERR_TIMEOUT          = -21, //!< Operate is timeout
  AM_RESULT_ERR_NO_IAV           = -22, //!< iav driver unloaded

  AM_RESULT_ERR_UNKNOWN          = -99, //!< Undefined error
  AM_RESULT_ERR_USER_BEGIN       = -100,
  AM_RESULT_ERR_USER_END         = -255
};
```

```
enum AM_SERVICE_STATE
{
  //!Instance created but not init
  AM_SERVICE_STATE_NOT_INIT = -1,

  //!Service process created
  AM_SERVICE_STATE_INIT_PROCESS_CREATED = 0,

  //!IPC setup for service management
  AM_SERVICE_STATE_INIT_IPC_CONNECTED = 1,

  //!Init function done
  AM_SERVICE_STATE_INIT_DONE = 2,

  //!Started and running
  AM_SERVICE_STATE_STARTED = 3,

  //!Stopped running
  AM_SERVICE_STATE_STOPPED = 4,

  //!Transition state, during this state, does not accept more cmd
  AM_SERVICE_STATE_BUSY = 5,

  //!Error state, used for debugging
  AM_SERVICE_STATE_ERROR = 6,
};

```

User delivers commands to services by default command ID with the dispatcher. The dispatcher is named api_helper, and the procedure of invoking command is shown in below:

![image_call_method](img/method_call.png)

Each service will supply related function, users could just invoking related commands by appointing command ID.

The commands could be divided into several groups, as show in below:

* [**Image Commands**](##Image Commands)
* [**Video Commands**](##Video Commands)
* [**Media Commands**](##Media Commands)
* [**Audio Commands**](##Audio Commands)
* [**Event Commands**](##Event Commands)
* [**Playback Commands**](##Playback Commands)
* [**System Commands**](#System Commands)

##Image Commands

The Image service supports commands to configure desired quality settings. The key features by Image processing commands are:

* **AAA(Auto-exposure, Auto-focus, Auto-white-balance) statistics**
* **Lens and sensor correction**
* **Color processing**
* **Noise removal**
* **Sharping**

### ID: AM_IPC_MW_CMD_IMAGE_AE_SETTING_GET

**Function**

  Use this command to get AE related parameters in current system.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](###Commands)|struct am_ae_config_s|

> For am_ae_config_s, refer to next section

### AM_IPC_MW_CMD_IMAGE_AE_SETTING_SET

**Function**

  Use this command to set AE related parameters into current system.

**Input Parameter**
```
am_ae_config_s *msg_data
```

**am_ae_config_s:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_AE_CONFIG_BIT|
|uint32_t|ae_target_ratio|Percentage of ae target to use. < 100: get darker picture; > 100: get brighter picture|
|uint32_t|sensor_shutter_min|Minimum sensor shutter time|
|uint32_t|sensor_shutter_max|Maximum sensor shutter time|
|uint32_t|sensor_gain|Image sensor analog gain, in dB uinit|
|uint32_t|sensor_shutter|Image sensor shutter time, 1/x seconds unit|
|uint32_t|luma_value[2]|luma_value[0]: RGB luma; luma_value[1]: CFA luma|
|uint8_t|ae_metering_mode|Ae meter mode, 0: spot, 1: center, 2: average, 3: custom|
|uint8_t|day_night_mode|0: day mode, colorful(default); 1: night mode, black&white|
|uint8_t|slow_shutter_mode|Slow shutter mode, 0: off; 1: auto(default)|
|uint8_t|anti_flicker_mode|Flicker mode, 0: 50Hz; 1: 60Hz|
|uint8_t|backlight_comp_enable|0: off; 1: on|
|uint8_t|auto_wdr_strength|WDR strength, 0: disable; 1~128: enable, 128 is the strongest strength|
|uint8_t|dc_iris_enable|DC iris, 0: disable; 1: enable|
|uint8_t|sensor_gain_max|Maximum sensor analog gain|
|uint8_t|ir_led_mode|IR led mode, 0: 0ff; 1: on; 2: auto|
|uint8_t|ae_enable|AE |

```
AM_AE_CONFIG_BIT

{

  AM_AE_CONFIG_AE_METERING_MODE = 0, //!< AE metering mode

  AM_AE_CONFIG_DAY_NIGHT_MODE = 1, //!< night mode

  AM_AE_CONFIG_SLOW_SHUTTER_MODE = 2, //!< slow shutter mode

  AM_AE_CONFIG_ANTI_FLICKER_MODE = 3, //!< anti-flicker mode

  AM_AE_CONFIG_AE_TARGET_RATIO = 4, //!< AE target ratio

  AM_AE_CONFIG_BACKLIGHT_COMP_ENABLE = 5, //!< back-light compensation

  AM_AE_CONFIG_AUTO_WDR_STRENGTH = 6, //!< auto WDR strength

  AM_AE_CONFIG_DC_IRIS_ENABLE = 7, //!< DC-iris

  AM_AE_CONFIG_SENSOR_GAIN_MAX = 8, //!< sensor gain max

  AM_AE_CONFIG_SENSOR_SHUTTER_MIN = 9, //!< sensor shutter minimum

  AM_AE_CONFIG_SENSOR_SHUTTER_MAX = 10, //!< sensor shutter maximum

  AM_AE_CONFIG_SENSOR_GAIN_MANUAL = 11, //!< sensor gain manual

  AM_AE_CONFIG_SENSOR_SHUTTER_MANUAL = 12, //!< sensor shutter manual

  AM_AE_CONFIG_IR_LED_MODE = 13, //!< LED mode

  AM_AE_CONFIG_AE_ENABLE = 14, //!< AE enable

```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_IMAGE_AWB_SETTING_GET

**Function**

  Use this command to get AWB related parameters in current system.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_awb_config_s|


> For am_awb_config_s, refer to next section

### AM_IPC_MW_CMD_IMAGE_AWB_SETTING_SET

**Function**

  Use this command to set AWB related parameters into current system.

**Input Parameter**

```
am_awb_config_s *msg_data
```

**am_awb_config_s:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_AWB_CONFIG_WB_MODE|
|uint32_t|wb_mode|WB mode|

```
#define AM_AWB_CONFIG_WB_MODE 0
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_IMAGE_NR_SETTING_GET

**Function**

  Use this command to get Noise Filter related parameters in current system.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_noise_filter_config_s|


> For am_noise_filter_config_s, refer to next section

### AM_IPC_MW_CMD_IMAGE_NR_SETTING_SET

**Function**

  Use this command to set Noise Filter related parameters into current system.

**Input Parameter**
```
am_noise_filter_config_s *msg_data
```

**am_noise_filter_config_s:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_NOISE_FILTER_CONFIG_BIT|
|uint32_t|mctf_strength|MCTF strength, 0: off; 1~255: different levels of mctf strength|
|uint8_t|FIR_param|Image FIR config|
|uint8_t|spatial_nr|Spatial noise reduction|
|uint8_t|chroma_nr|Chroma noise reduction|
|uint8_t|reserved|Reserved|

```
enum AM_NOISE_FILTER_CONFIG_BIT
{
  AM_NOISE_FILTER_CONFIG_MCTF_STRENGTH = 0, //!< MCTF Strength
  AM_NOISE_FILTER_CONFIG_FIR_PARAM = 1, //!< FIR parameter
  AM_NOISE_FILTER_CONFIG_SPATIAL_NR = 2, //!< SPATIAL
  AM_NOISE_FILTER_CONFIG_CHROMA_NR = 3, //!< CHROMA
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_GET

**Function**

  Use this command to get Image Style related parameters in current system.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_image_style_config_s|


> For am_image_style_config_s, refer to next section

### AM_IPC_MW_CMD_IMAGE_STYLE_SETTING_SET

**Function**

  Use this command to set Image Style related parameters into current system.

**Input Parameter**

```C
am_image_style_config_s *msg_data
```

**am_image_style_config_s:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_IMAGE_STYLE_CONFIG_BIT|
|int32_t|auto_contrast_mode|Auto contrast mode, 0: disable; 1~128: enable|
|uint8_t|quick_style_code|Code style|
|int8_t|hue|HUE|
|int8_t|saturation|Saturation, -2~2|
|int8_t|sharpness|Sharpness, -2~2|
|int8_t|brightness|Brightness, -2~2|
|int8_t|contrast|Contrast, -2~2|
|int8_t|black_level|Black level, -2~2|
|uint8_t|reserved|Reserved|

```
AM_IMAGE_STYLE_CONFIG_BIT

{

  AM_IMAGE_STYLE_CONFIG_QUICK_STYLE_CODE = 0,

  AM_IMAGE_STYLE_CONFIG_HUE = 1,

  AM_IMAGE_STYLE_CONFIG_SATURATION = 2,

  AM_IMAGE_STYLE_CONFIG_SHARPNESS = 3,

  AM_IMAGE_STYLE_CONFIG_BRIGHTNESS = 4,

  AM_IMAGE_STYLE_CONFIG_CONTRAST = 5,

  AM_IMAGE_STYLE_CONFIG_BLACK_LEVEL = 6,

  AM_IMAGE_STYLE_CONFIG_AUTO_CONTRAST_MODE = 7,

```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_IMAGE_ADJ_LOAD

**Function**

  Use this command to load bin file from specified path.

**Input Parameter**

```
AM_IQ_CONFIG *msg_data
```
```
struct AM_IQ_CONFIG
{
  void            *value;
  AM_IQ_CONFIG_KEY key;
};
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_IMAGE_AAA_START

**Function**

  Use this command to start image AAA.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_IMAGE_AAA_STOP

**Function**

  Use this command to stop image AAA.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


##Video commands

The video service provides commands for video related design and implementation.

### AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_GET

**Function**

Use this command to get video feature information.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_feature_config_t|

> For am_feature_config_t, refer to next section

### AM_IPC_MW_CMD_VIDEO_CFG_FEATURE_SET

**Function**

User this command to set video feature parameters.

**Input Parameter**

```
am_feature_config_t *msg_data
```
**am_feature_config_t:**
|Type|Field|Function|
|:---|:---------|:----------|
|uint32_t|enable_bits|Bit control to specified feature, refer to AM_FEATURE_CONFIG_BITS|
|uint32_t|version|Descripte IAV version|
|uint32_t|mode|Descripte encode mode|
|uint32_t|hdr|Descripte HDR mode|
|uint32_t|iso|Descripte ISO mode|
|uint32_t|dewarp_func|Descripte De-warp mode|
|uint32_t|dptz|Descripte DPTZ function|
|uint32_t|overlay|Descripte overlay function|
|uint32_t|video_md|Descripte motion detect function|
|uint32_t|hevc|Descripte HEVC function|

```C++
AM_FEATURE_CONFIG_BITS
{
  AM_FEATURE_CONFIG_MODE_EN_BIT        = 0,//!< Change feature mode
  AM_FEATURE_CONFIG_HDR_EN_BIT         = 1,//!< Change feature hdr
  AM_FEATURE_CONFIG_ISO_EN_BIT         = 2,//!< Change feature iso
  AM_FEATURE_CONFIG_DEWARP_EN_BIT      = 3,//!< Change feature dewarp
  AM_FEATURE_CONFIG_DPTZ_EN_BIT        = 4,//!< Change feature dptz
  AM_FEATURE_CONFIG_BITRATECTRL_EN_BIT = 5,//!< Change feature bitrate ctrl
  AM_FEATURE_CONFIG_OVERLAY_EN_BIT     = 6,//!< Change feature overlay
  AM_FEATURE_CONFIG_IAV_VERSION_EN_BIT = 7,//!< Change feature iav version
  AM_FEATURE_CONFIG_VIDEO_MD_EN_BIT    = 8,//!< Change feature video motion detect
  AM_FEATURE_CONFIG_HEVC_EN_BIT        = 9,//!< Change feature hevc
}
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_VIDEO_CFG_VIN_GET

**Function**

Use this command to get video VIN device parameters from config file.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_vin_config_t|

> For am_vin_config_t, refer to next section

### AM_IPC_MW_CMD_VIDEO_CFG_VIN_SET

**Function**

Use this command to set video VIN device parameters from config file.

**Input Parameter**

```
am_vin_config_t *msg_data
```
**am_vin_cinfig_t:**
|Type|Field|Function|
|:---|:---------|:----------|
|uint32_t|enable_bits|Bit control to specified function, refer to AM_VIN_CONFIG_BITS|
|uint32_t|vin_id|Descripte Vin ID|
|uint32_t|width|Descripte VIN width|
|uint32_t|height|Descripte VIN height|
|uint16_t|reserved0|Unused, reserved|
|uint16_t|fps|Descripte VIN frame rate|
|uint8_t|flip|Descripte VIN flip function|
|uint8_t|hdr_mode|Descripte HDR mode|
|uint8_t|bayer_pattern|Descripte bayer pattern|
|uint8_t|reserved1|Unused, reserved|


```C++
AM_VIN_CONFIG_BITS
{
  AM_VIN_CONFIG_WIDTH_HEIGHT_EN_BIT  = 0,//!< Change VIN Width and Height
  AM_VIN_CONFIG_FLIP_EN_BIT          = 1,//!< Change VIN Flip config
  AM_VIN_CONFIG_HDR_MODE_EN_BIT      = 2,//!< Change VIN HDR mode
  AM_VIN_CONFIG_FPS_EN_BIT           = 3,//!< Change VIN FPS config
  AM_VIN_CONFIG_BAYER_PATTERN_EN_BIT = 4,//!< Change VIN bayer pattern
}
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_VIDEO_DYN_VIN_PARAMETERS_GET

**Function:**

Use this command to get current VIN parameters dynamically.

**Input Parameter**

Null

**Output Parameter**


|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_vin_config_t|

> For am_vin_config_t, Please refer to previous section.

### AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_GET

**Function**

Use this command to get buffer format from config file.

**Input Parameter**

```
int32_t *msg_data  //AM_SOURCE_BUFFER_ID
```
```
enum AM_SOURCE_BUFFER_ID
{
  AM_SOURCE_BUFFER_INVALID  = -1,
  AM_SOURCE_BUFFER_MAIN     = 0,
  AM_SOURCE_BUFFER_2ND      = 1,
  AM_SOURCE_BUFFER_3RD      = 2,
  AM_SOURCE_BUFFER_4TH      = 3,
  AM_SOURCE_BUFFER_5TH      = 4,
  AM_SOURCE_BUFFER_PMN      = 5,
  AM_SOURCE_BUFFER_EFM      = 6,
  AM_SOURCE_BUFFER_MAX,

  AM_SOURCE_BUFFER_PREVIEW_A   = AM_SOURCE_BUFFER_4TH,
  AM_SOURCE_BUFFER_PREVIEW_B   = AM_SOURCE_BUFFER_3RD,
  AM_SOURCE_BUFFER_PREVIEW_C   = AM_SOURCE_BUFFER_2ND,
  AM_SOURCE_BUFFER_PREVIEW_D   = AM_SOURCE_BUFFER_5TH,
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|am_buffer_fmt_t|


> For am_buffer_fmt_t, refer next section

### AM_IPC_MW_CMD_VIDEO_CFG_BUFFER_SET

**Function**

Use this command to set buffer format to config file.

**Input Parameter**

```
am_buffer_fmt_t *msg_data
```

**am_buffer_fmt_t:**
|Type|Field|Function|
|:---|:---------|:----------|
|uint32_t|enable_bits|Bit control to specified function, refer to AM_BUFFER_FMT_BITS|
|uint32_t|buffer_id|Describe buffer ID|
|uint32_t|type|Describe buffer work type|
|uint32_t|input_crop|Describe input crop function|
|uint32_t|input_width|Describe input window width|
|uint32_t|input_height|Describe input window height|
|uint32_t|input_offset_x|Describe input window offset in x|
|uint32_t|input_offset_y|Describe input window offset in y|
|uint32_t|width|Describe buffer width|
|uint32_t|height|Describe buffer height|
|uint32_t|prewarp|Descrbe pre-warp function|
|uint32_t|cap_skip_itvl|Capture one frame every N+1 frame in yuv capture|
|uint32_t|auto_stop|Auto stop capture yuv after source buffer in full|

```C++
AM_BUFFER_FMT_BITS
{
  AM_BUFFER_FMT_TYPE_EN_BIT           = 0,
  AM_BUFFER_FMT_INPUT_CROP_EN_BIT     = 1,
  AM_BUFFER_FMT_INPUT_WIDTH_EN_BIT    = 2,
  AM_BUFFER_FMT_INPUT_HEIGHT_EN_BIT   = 3,
  AM_BUFFER_FMT_INPUT_X_EN_BIT        = 4,
  AM_BUFFER_FMT_INPUT_Y_EN_BIT        = 5,
  AM_BUFFER_FMT_WIDTH_EN_BIT          = 6,
  AM_BUFFER_FMT_HEIGHT_EN_BIT         = 7,
  AM_BUFFER_FMT_PREWARP_EN_BIT        = 8,
  AM_BUFFER_FMT_CAP_SKIP_ITVL_EN_BIT  = 9,
  AM_BUFFER_FMT_AUTO_STOP_EN_BIT      = 10,
  AM_BUFFER_FMT_SAVE_CFG_EN_BIT       = 11,
}
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET

**Function**

Use this command to get stream format in config file.

**Input Parameter**

```C++
int32_t *msg_data //AM_STREAM_ID
```

```
enum AM_STREAM_ID
{
  AM_STREAM_ID_INVALID = -1,  //for initial invalid value
  AM_STREAM_ID_0       = 0,
  AM_STREAM_ID_1,
  AM_STREAM_ID_2,
  AM_STREAM_ID_3,
  AM_STREAM_ID_4,
  AM_STREAM_ID_5,
  AM_STREAM_ID_6,
  AM_STREAM_ID_7,
  AM_STREAM_ID_8,
  AM_STREAM_ID_MAX,
};

```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_stream_fmt_t|

**am_stream_fmt_t:**
|Type|Field|Function|
|:---|:---------|:----------|
|uint32_t|enable_bits|Bit control to specified stream function, refer to AM_STREAM_FMT_BITS|
|uint32_t|stream_id|Describe stream ID|
|uint32_t|enable|Describe stream state|
|uint32_t|type|Describe stream type|
|uint32_t|source|Describe source buffer which stream used|
|uint32_t|frame_rate|Describe stream frame rate|
|uint32_t|width|Describe stream width|
|uint32_t|height|Describe stream height|
|uint32_t|offset_x|Describe stream offset x|
|uint32_t|offset_y|Describe stream offset y|
|uint32_t|hflip|Describe stream horizontal flip|
|uint32_t|vflip|Describe stream vertical flip|
|uint32_t|rotate|Describe stream rotate|

```C++
AM_STREAM_FMT_BITS
{
  AM_STREAM_FMT_ENABLE_EN_BIT       = 0,  //!< Bit0
  AM_STREAM_FMT_TYPE_EN_BIT         = 1,  //!< Bit1
  AM_STREAM_FMT_SOURCE_EN_BIT       = 2,  //!< Bit2
  AM_STREAM_FMT_FRAME_RATE_EN_BIT   = 3,  //!< Bit3
  AM_STREAM_FMT_WIDTH_EN_BIT        = 4,  //!< Bit5
  AM_STREAM_FMT_HEIGHT_EN_BIT       = 5,  //!< Bit6
  AM_STREAM_FMT_OFFSET_X_EN_BIT     = 6,  //!< Bit7
  AM_STREAM_FMT_OFFSET_Y_EN_BIT     = 7,  //!< Bit8
  AM_STREAM_FMT_HFLIP_EN_BIT        = 8,  //!< Bit9
  AM_STREAM_FMT_VFLIP_EN_BIT        = 9,  //!< Bit10
  AM_STREAM_FMT_ROTATE_EN_BIT       = 10, //!< Bit11
}
```

### AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_SET

**Function**

Use this command to set stream format to config file.

**Input Parameter**

```
am_stream_fmt_t *msg_data
```

> For am_stream_fmt_t, please refer to AM_IPC_MW_CMD_VIDEO_CFG_STREAM_FMT_GET

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|



###AM_IPC_MW_CMD_VIDEO_DYN_VOUT_HALT

**Function**

Use this command to dynamically halt vout.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|



###AM_IPC_MW_CMD_VIDEO_CFG_VOUT_GET

**Function**

Use this command to get vout config.

**Input Parameter**

```
int32_t *msg_data //AM_VOUT_ID
```

```
enum AM_VOUT_ID
{
  AM_VOUT_ID_INVALID  = -1,
  AM_VOUT_ID_HDMI     = 0,  //VOUT-B, which is usually associated with Preview-B
  AM_VOUT_ID_LCD      = 1,  //VOUT-A, which is usually associated with Preview-A
  AM_VOUT_A           = AM_VOUT_ID_LCD,
  AM_VOUT_B           = AM_VOUT_ID_HDMI,
  AM_VOUT_MAX_NUM     = 2,
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|am_vout_config_t|

struct am_vout_config_t

> For am_vout_config_t, refer to next section

###AM_IPC_MW_CMD_VIDEO_CFG_VOUT_SET

**Function**

Use this command to set vout format config file.

**Input Parameter**

```
am_vout_config_t *msg_data
```

|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_VOUT_CONFIG_BITS|
|uint32_t|vout_id|Vout ID|
|uint32_t|type|Vout type|
|uint32_t|video_type|Video output type|
|uint32_t|flip|Video flip|
|uint32_t|rotate|Video rotate|
|uint32_t|fps|Video frame rate|
|char|mode[VOUT_MAX_CHAR_NUM]|Video out mode|

```
AM_VOUT_CONFIG_BITS

{

  AM_VOUT_CONFIG_TYPE_EN_BIT          = 0,//!< Change VOUT type

  AM_VOUT_CONFIG_VIDEO_TYPE_EN_BIT    = 1,//!< Change VOUT video type

  AM_VOUT_CONFIG_MODE_EN_BIT          = 2,//!< Change VOUT mode

  AM_VOUT_CONFIG_FLIP_EN_BIT          = 3,//!< Change VOUT flip

  AM_VOUT_CONFIG_ROTATE_EN_BIT        = 4,//!< Change VOUT rotate

};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|



###AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_STATE_GET

**Function**

Use this command to get buffer state dynamically.

**Input Parameter**

```
uint32_t *msg_data //AM_SOURCE_BUFFER_ID
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_buffer_state_t|

**am_buffer_state_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|buffer_id|Buffer ID|
|uint32_t|state|Buffer state [Idle/Busy/Error/Unknow]|

###AM_IPC_MW_CMD_VIDEO_DYN_BUFFER_MAX_NUM_GET

**Function**

Use this command to get maximum number of supported buffer dynamically.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|uint32_t num|

###AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_GET

**Function**

Use this command to get stream h264 or h265 parameters from config file.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_h26x_cfg_t|

> For am_h26x_cfg_t, refer to next section

###AM_IPC_MW_CMD_VIDEO_CFG_STREAM_H26x_SET

**Function**

Use this command to set H26x format config file.

**Input Parameter**

```
am_h26x_cfg_t *msg_data
```
**am_h26x_cfg_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for relative function, refer AM_H26X_CFG_BITS|
|uint32_t|stream_id|Stream ID|
|uint32_t|bitrate_ctrl|Bitrate control method|
|uint32_t|profile|Profile level|
|uint32_t|au_type|Audio type|
|uint32_t|chroma|Chroma type|
|uint32_t|M|Number of Macro block|
|uint32_t|N|Number of frame in GOP|
|uint32_t|idr_interval|?|
|uint32_t|target_bitrate|Bitrate of target|
|uint32_t|mv_threshold|MV threshold|
|uint32_t|flat_area_improve|?|
|uint32_t|multi_ref_p|Multi-reference P frame|
|uint32_t|fast_seek_intvl|Specify fast seek P frame interval|

```
AM_H26x_CFG_BITS
{
  AM_H26x_CFG_BITRATE_CTRL_EN_BIT         = 0,

  AM_H26x_CFG_PROFILE_EN_BIT              = 1,

  AM_H26x_CFG_AU_TYPE_EN_BIT              = 2,

  AM_H26x_CFG_CHROMA_EN_BIT               = 3,

  AM_H26x_CFG_M_EN_BIT                    = 4,

  AM_H26x_CFG_N_EN_BIT                    = 5,

  AM_H26x_CFG_IDR_EN_BIT                  = 6,

  AM_H26x_CFG_BITRATE_EN_BIT              = 7,

  AM_H26x_CFG_MV_THRESHOLD_EN_BIT         = 8,

  AM_H26x_CFG_FLAT_AREA_IMPROVE_EN_BIT    = 9,

  AM_H26x_CFG_MULTI_REF_P_EN_BIT          = 10,

  AM_H26x_CFG_FAST_SEEK_INTVL_EN_BIT      = 11,

};
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_GET

**Function**

Use this command to get stream mjpeg parameters from config file.

**Input Parameter**

```
int32_t *msg_data //AM_STREAM_ID
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_mjpeg_cfg_t|

> For am_jpeg_cfg_t, refer to next section

###AM_IPC_MW_CMD_VIDEO_CFG_STREAM_MJPEG_SET

**Function**

Use this command to set stream mjpeg parameters to config file.

**Input Parameter**

```
am_mjpeg_cfg_t *msg_data
```

**am_mjpeg_cfg_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_MJPEG_CFG_BITS|
|uint32_t|stream_id|Stream ID|
|uint32_t|quality|MJPEG quality|
|uint32_t|chroma|Chroma|

```
AM_STREAM_CFG_BITS

{

  AM_STREAM_CFG_H264_EN_BIT = 0, //!< Config stream to H.264

  AM_STREAM_CFG_MJPEG_EN_BIT = 1, //!< Config stream to MJPEG

};

```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_ENCODE_START

**Function**

Use this command to start encoding.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_ENCODE_STOP

**Function**

Use this command to stop encoding.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_DYN_VCA_MODE

**Function**

Use this command to goto VCA mode.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_DYN_IAV_CURRENT_MODE

**Function**

Use this command to goto iav current mode.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_DYN_IAV_UNLOADED_MODE

**Function**

Use this command to goto iav unloaded mode.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_DYN_WORKING_MODE_GET

**Function**

Use this command to get current working mode.

**Input Parameter**

Null

**Output Parameter**:

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|enum AM_WORKING_MODE|


```
enum AM_WORKING_MODE
{
  //oryx worked in normal mode
  AM_WORKING_MODE_NORAML = 0,
  //oryx worked in vca mode
  AM_WORKING_MODE_VCA,
  //oryx worked in iav unloaded mode
  AM_WORKING_MODE_IAV_UNLOADED,

  AM_WORKING_MODE_ERROR,
};

```

###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_MAX_NUM_GET

**Function**

Use this command to get stream max number.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|uint32_t num|


###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STATUS_GET

**Function**

Use this command to get all streams status.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_stream_status_t|

**am_stream_status_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|status|Bit imply related stream with serail order, bit = 1 means stream is encoding, otherwise it's not encoding|


###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK_STATE_GET

**Function**

Use this command to check streams lock state.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|uint32_t block_bit_map|

###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_LOCK

**Function**

Use this command to test if stream is occupied and lock it.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_stream_lock_t|

**am_stream_lock_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|stream_id|Video stream ID|
|uint8_t|uuid[16]|The UUID generated by apps|
|uint8_t|operation|Lock operation|
|uint8_t|op_result|Operation result|
|uint16_t|reserved|Reserved|


###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_START

**Function**

Use this command to start a stream dynamically.

**Input Parameter**

```
int32_t *msg_data //AM_STREAM_ID
```

```
enum AM_STREAM_ID
{
  AM_STREAM_ID_INVALID = -1,  //for initial invalid value
  AM_STREAM_ID_0       = 0,
  AM_STREAM_ID_1,
  AM_STREAM_ID_2,
  AM_STREAM_ID_3,
  AM_STREAM_ID_4,
  AM_STREAM_ID_5,
  AM_STREAM_ID_6,
  AM_STREAM_ID_7,
  AM_STREAM_ID_8,
  AM_STREAM_ID_MAX,
};

```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_STOP

**Function**

Use this command to stop a stream dynamically.

**Input Parameter**

```
int32_t *msg_data //AM_STREAM_ID
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_FORCE_IDR

**Function**

Use this command to force a stream to idr.

**Input Parameter**

```
int32_t *msg_data //AM_STREAM_ID
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_GET

**Function**

Use this command to get a stream parameter dynamically.

**Input Parameter**

```
int32_t *msg_data //AM_STREAM_ID
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_stream_parameter_t|

> For am_stream_parameter_t, refer to next section

###AM_IPC_MW_CMD_VIDEO_DYN_STREAM_PARAMETERS_SET

**Function**

Use this commnad to set a stream parameter dynamically.

**Input Parameter**

```
am_stream_parameter_t *msg_data
```
**am_stream_parameter_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control of related function, refer to AM_STREAM_DYN_CTRL_BITS|
|uint32_t|stream_id|Video stream ID|
|uint32_t|source|Video source buffer|
|uint32_t|type|Video type,[None/H264/H265/MJPEG]|
|uint32_t|fps|Frame rate|
|uint32_t|bitrate_ctrl|Bit rate control mode|
|uint32_t|bitrate|Bit rate|
|uint32_t|abs_bitrate|Keep bit rate in a fixed value|
|uint32_t|width|Stream width, multiple of 16|
|uint32_t|height|Stream height, multiple of 8|
|uint32_t|offset_x|Stream offset x, multiple of 2|
|uint32_t|offset_y|Stream offset y, multiple of 2|
|uint32_t|flip|Stream flip|
|uint32_t|rotate|Stream Rotate|
|uint32_t|profile|H26x profile level|
|uint32_t|gop_n|N in gop|
|uint32_t|idr_interval|I frame interval|
|uint32_t|quality|MJPEG quality|
|uint32_t|i_frame_max_size|I frame size|

```
AM_STREAM_DYN_CTRL_BITS

{
  AM_STREAM_DYN_CTRL_SOURCE_EN_BIT       = 0,  //!< Bit0

  AM_STREAM_DYN_CTRL_TYPE_EN_BIT         = 1,  //!< Bit1

  AM_STREAM_DYN_CTRL_FRAMERATE_EN_BIT    = 2,  //!< Bit2

  AM_STREAM_DYN_CTRL_BITRATE_EN_BIT      = 3,  //!< Bit3

  AM_STREAM_DYN_CTRL_SIZE_EN_BIT         = 4,  //!< Bit4

  AM_STREAM_DYN_CTRL_OFFSET_EN_BIT       = 5,  //!< Bit5

  AM_STREAM_DYN_CTRL_FLIP_EN_BIT         = 6,  //!< Bit6

  AM_STREAM_DYN_CTRL_ROTATE_EN_BIT       = 7,  //!< Bit7

  AM_STREAM_DYN_CTRL_PROFILE_EN_BIT      = 8,  //!< Bit8

  AM_STREAM_DYN_CTRL_GOP_N_EN_BIT        = 9,  //!< Bit9

  AM_STREAM_DYN_CTRL_GOP_IDR_EN_BIT      = 10, //!< Bit10

  AM_STREAM_DYN_CTRL_ENC_QUALITY_EN_BIT  = 11, //!< Bit11

  AM_STREAM_DYN_CTRL_I_FRAME_SIZE_EN_BIT = 12, //!< Bit12

  AM_STREAM_DYN_CTRL_SAVE_CFG_EN_BIT     = 13, //!< Bit13

  AM_STREAM_DYN_CTRL_ABS_BITRATE_EN_BIT  = 14, //!< Bit14

};

```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_GET

**Function**

Use this command to get CPU clock dynamically.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|cpu_clk|

cpu_clk
```
std::map<int32_t, int32_t> cpu_clk
```


###AM_IPC_MW_CMD_VIDEO_DYN_CPU_CLK_SET

**Function**

Use this command to set CPU clock dynamically.

**Input Parameter**

```
int32_t index
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_GET

**Function**

Use this command to get DPTZ setting dynamically.

**Input Parameter**

```
int32_t *msg_data //AM_SOURCE_BUFFER_ID
```
```
enum AM_SOURCE_BUFFER_ID
{
  AM_SOURCE_BUFFER_INVALID  = -1,
  AM_SOURCE_BUFFER_MAIN     = 0,
  AM_SOURCE_BUFFER_2ND      = 1,
  AM_SOURCE_BUFFER_3RD      = 2,
  AM_SOURCE_BUFFER_4TH      = 3,
  AM_SOURCE_BUFFER_5TH      = 4,
  AM_SOURCE_BUFFER_PMN      = 5,
  AM_SOURCE_BUFFER_EFM      = 6,
  AM_SOURCE_BUFFER_MAX,

  AM_SOURCE_BUFFER_PREVIEW_A   = AM_SOURCE_BUFFER_4TH,
  AM_SOURCE_BUFFER_PREVIEW_B   = AM_SOURCE_BUFFER_3RD,
  AM_SOURCE_BUFFER_PREVIEW_C   = AM_SOURCE_BUFFER_2ND,
  AM_SOURCE_BUFFER_PREVIEW_D   = AM_SOURCE_BUFFER_5TH,
};

```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_dptz_ratio_t|


**am_dptz_ratio_t:**
|Type|Field|Description|
|:-------|:----------|:---|
|uint32_t|enable_bits|Bit control for related function, refer to AM_DPTZ_BITS|
|uint32_t|buffer_id|Specify the buffer ID|
|float|pan_ratio|Pan ratio|
|float|tilt_ratio|Tilt ratio|
|float|zoom_ratio|Zoom ratio|

```
enum AM_DPTZ_BITS {
  AM_DPTZ_PAN_RATIO_EN_BIT    = 0,
  AM_DPTZ_TILT_RATIO_EN_BIT   = 1,
  AM_DPTZ_ZOOM_RATIO_EN_BIT   = 2,
  AM_DPTZ_SIZE_X_EN_BIT       = 3,
  AM_DPTZ_SIZE_Y_EN_BIT       = 4,
  AM_DPTZ_SIZE_W_EN_BIT       = 5,
  AM_DPTZ_SIZE_H_EN_BIT       = 6
};
```

###AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_RATIO_SET

**Function**

Use this command to set DPTZ ratio dynamically.

**Input Parameter**

```
am_dptz_ratio_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_GET

**Function**

Get DPTZ Size setting dynamically.

**Input Parameter**

```
int32_t *msg_data  //AM_SOURCE_BUFFER_ID
```

```
enum AM_SOURCE_BUFFER_ID
{
  AM_SOURCE_BUFFER_INVALID  = -1,
  AM_SOURCE_BUFFER_MAIN     = 0,
  AM_SOURCE_BUFFER_2ND      = 1,
  AM_SOURCE_BUFFER_3RD      = 2,
  AM_SOURCE_BUFFER_4TH      = 3,
  AM_SOURCE_BUFFER_5TH      = 4,
  AM_SOURCE_BUFFER_PMN      = 5,
  AM_SOURCE_BUFFER_EFM      = 6,
  AM_SOURCE_BUFFER_MAX,

  AM_SOURCE_BUFFER_PREVIEW_A   = AM_SOURCE_BUFFER_4TH,
  AM_SOURCE_BUFFER_PREVIEW_B   = AM_SOURCE_BUFFER_3RD,
  AM_SOURCE_BUFFER_PREVIEW_C   = AM_SOURCE_BUFFER_2ND,
  AM_SOURCE_BUFFER_PREVIEW_D   = AM_SOURCE_BUFFER_5TH,
};

```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_dptz_ratio_t|

**am_dptz_size_t:**
|Type|Field|Description|
|:-------|:----------|:---|
|uint32_t|enable_bits|Bit control for related function, refer to AM_DPTZ_BITS|
|uint32_t|buffer_id|Buffer ID|
|uint32_t|w|Input window width|
|uint32_t|h|Input window height|
|uint32_t|x|Input window offset x|
|uint32_t|y|Input window offset y|


###AM_IPC_MW_CMD_VIDEO_DYN_DPTZ_SIZE_SET

**Function**

Use this command to set DPTZ size dynamically.

**Input Parameter**

```
am_dptz_size_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_DYN_WARP_GET

**Function**

Use this command to get Warp setting dynamically.

**Input Parameter**

```
int region_id
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_warp_t|

> For am_warp_t, refer to next section

###AM_IPC_MW_CMD_VIDEO_DYN_WARP_SET

**Function**

use this command to set Warp setting dynamically

**Input Parameter**

```
am_warp_t *msg_data
```

**am_warp_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_WARP_BITS|
|int|region_id|Fisheye correction region number|
|int|warp_mode|Warp mode, refer to AM_WARP_MODE|
|int|max_radius|Full FOV circle radius in vin|
|int|warp_region_yaw|Lens warp region yaw in degree, -90~90|
|int|warp_region_pitch|Lens warp region pitch in degree, -90~90|
|uint|warp_zoom|Warp zoom ratio, low 16bits is denominator, high 16bit is numerator|
|uint|hor_zoom|Horizontal zoom ratio parameter|
|uint|ver_zoom|Vertical zoom ratio parameter|
|float|ldc_strength|0.0~20.0, 0.0: do not lens distortion calibration, 20: maximum lens distortion calibratiom|
|float|pano_hfov_degree|For panorama, 1.0~180.0|
|int|buffer_id|Buffer ID|
|float|pan_angle|Pan angle, -90~90|
|float|tilt_angle|Tilt angle, -90~90|
|float|sub_roi_offset_x|ROI center offset x to the circle center|
|float|sub_roi_offset_y|ROI center offset y to the circle center|
|warp_region_dptz_t|warp_region_dptz[MAX_WARP_AREAS]|Specify DPTZ area input and output|

```
enum AM_WARP_BITS {
  AM_WARP_REGION_ID_EN_BIT        = 0,
  AM_WARP_LDC_STRENGTH_EN_BIT     = 1,
  AM_WARP_PANO_HFOV_DEGREE_EN_BIT = 2,
  AM_WARP_REGION_YAW_PITCH_EN_BIT = 3,
  AM_WARP_LDC_WARP_MODE_EN_BIT    = 4,
  AM_WARP_MAX_RADIUS_EN_BIT       = 5,
  AM_WARP_SAVE_CFG_EN_BIT         = 6,
  AM_WARP_HOR_VER_ZOOM_EN_BIT     = 7,
  AM_WARP_REGION_DPTZ_EN_BIT      = 8,
  AM_WARP_PAN_ANGLE_EN_BIT        = 9,
  AM_WARP_TILT_ANGLE_EN_BIT       = 10,
  AM_WARP_SUB_ROI_OFFSET_EN_BIT   = 11,
  AM_WARP_ZOOM_EN_BIT             = 12,
};

```

```
typedef struct warp_region_dptz_s {
  uint32_t input_w        = 0; //!< input window width
  uint32_t input_h        = 0; //!< input window height
  uint32_t input_x        = 0; //!< input window offset x
  uint32_t input_y        = 0; //!< input window offset y
  uint32_t output_w       = 0; //!< output size width
  uint32_t output_h       = 0; //!< output size height
  uint32_t output_x       = 0; //!< output offset x
  uint32_t output_y       = 0; //!< output offset y
  bool input_size_flag    = false; //!< input_size_flag
  bool input_offset_flag  = false; //!< input_offset_flag
  bool output_size_flag   = false; //!< output_size_flag
  bool output_offset_flag = false; //!< output_offset_flag
} warp_region_dptz_t;
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_LBR_GET

**Function**

Use this command to get LBR format dynamically.

**Input Parameter**

```
int32_t *msg_data //AM_STREAM_ID
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_encode_lbr_ctrl_t|

am_encode_lbr_ctrl_t

> For am_encode_lbr_ctrl_t, refer to next section

###AM_IPC_MW_CMD_VIDEO_DYN_LBR_SET

**Function**

Use this command to set LBR format dynamically.

**Input Parameter**

```
am_encode_lbr_ctrl_t *msg_data
```

**am_encode_lbr_ctrl_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_ENCODE_LBR_CTRL_BITS|
|uint32_t|stream_id|Stream ID|
|uint32_t|bitrate_ceiling|Could be set when auto_bitrate_seiling is false,|
|bool|enable_lbr|enable/disable|
|bool|auto_bitrate_ceilingfalse: need to set manually; true: auto bitrate ceiling|
|bool|drop_frame|Drop frame|

```
AM_ENCODE_LBR_CTRL_BITS
  AM_ENCODE_LBR_ENABLE_LBR_EN_BIT              = 0,

  AM_ENCODE_LBR_AUTO_BITRATE_CEILING_EN_BIT    = 1,

  AM_ENCODE_LBR_BITRATE_CEILING_EN_BIT         = 2,

  AM_ENCODE_LBR_DROP_FRAME_EN_BIT              = 3,

  AM_ENCODE_LBR_SAVE_CURRENT_CONFIG_EN_BIT     = 4,

};

```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_MAX_NUM_GET

**Function**

Use this command to get overlay max area number.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_overlay_limit_val_t|

**am_overlay_limit_val_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|platform_stream_num_max|Platform support max encode stream number|
|uint32_t|platform_overlay_area_num_max|Platform support max overlay area number per stream|
|uint32_t|user_def_stream_num_max|User defined max stream number for overlay|
|uint32_t|user_def_overlay_area_num_max|User defined max overlay area number per stream|

###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DESTROY

**Function**

Use this command to destroy all overlay areas.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SAVE

**Function**

Use this command to save all overlay parameters to configure file.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_INIT

**Function**

Initialize a overlay area.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_ADD

**Function**

Use this command to add a data block to area.

**Input Parameter**

```
am_overlay_data_t *msg_data
```

**am_overlay_data_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_OVERLAY_BITS|
|am_overlay_id_t|id|Overlay ID|
|uint32_t|width|Data block width|
|uint32_t|height|Data block height|
|uint32_t|offset_x|Data block offset x in the area|
|uint32_t|offset_y|Data block offset y in the area|
|uint32_t|type|Data block type|
|uint32_t|spacing|Char spacing|
|uint32_t|bg_color|Text background color|
|uint32_t|font_size|Font size|
|uint32_t|font_color|Font color|
|uint32_t|font_outline_w|Font outline width|
|uint32_t|font_outline_color|Font outline color|
|int32_t|font_hor_bold|Font horizontal bold size|
|int32_t|font_ver_bold|Font vertical bold size|
|uint32_t|font_italic|Font italic|
|uint32_t|msec_en|Whether enable msec display when type is time|
|uint32_t|time_format|Time display format|
|uint32_t|is_12h|Whether use 12H notation to display time|
|uint32_t|bmp_num|Bmp number in the animation file|
|uint32_t|interval|Frame interval to update bmp when type is animation|
|uint32_t|color_key|Color used to transparent in picture type|
|uint32_t|color_range|Color range used to transparent with color_key|
|uint32_t|line_color|Line color: 0-7|
|uint32_t|line_tn|Line thickness|
|uint32_t|p_n|Point number|
|uint32_t|p_x[OVERLAY_MAX_POINT]|Point x value for draw line|
|uint32_t|p_y[OVERLAY_MAX_POINT]|Point y value for draw line|
|char|str[OVERLAY_MAX_STRING]|String to be inserted|
|char|pre_str[OVERLAY_MAX_STRING]|Prefix string to be inserted when type in time|
|char|suf_str[OVERLAY_MAX_STRING]|Suffix string to be insert when type is time|
|char|font_type[OVERLAY_MAX_FILENAME]|Font type name|
|char|bmp[OVERLAY_MAX_FILENAME]|Bmp full path name to be inserted|

```
AM_OVERLAY_BITS
{
  AM_OVERLAY_REMOVE_EN_BIT = 0,          //!< remove a overlay area

  AM_OVERLAY_ENABLE_EN_BIT = 1,          //!< enable a overlay area

  AM_OVERLAY_DISABLE_EN_BIT = 2,         //!< disable a overlay area

  AM_OVERLAY_INIT_EN_BIT = 3,            //!< init a area

  AM_OVERLAY_DATA_ADD_EN_BIT = 4,        //!< add a data block to area

  AM_OVERLAY_DATA_UPDATE_EN_BIT = 5,     //!< update a data block for a area

  AM_OVERLAY_DATA_REMOVE_EN_BIT = 6,     //!< remove a data block for a area

  AM_OVERLAY_ROTATE_EN_BIT = 7,          //!< area whether rotated with stream

  AM_OVERLAY_BG_COLOR_EN_BIT = 8,        //!< area or text background color

  AM_OVERLAY_BUF_NUM_EN_BIT = 9,         //!< area buffer number

  AM_OVERLAY_RECT_EN_BIT = 10,           //!< area or data block size and offset

  AM_OVERLAY_DATA_TYPE_EN_BIT = 11,      //!< data block type

  AM_OVERLAY_STRING_EN_BIT = 12,         //!< string to be add or update

  AM_OVERLAY_TIME_EN_BIT = 13,           //!< prefix/suffix string or enable msec

                                         //!< display for time type data block

  AM_OVERLAY_CHAR_SPACING_EN_BIT = 14,   //!< char spacing for text

  AM_OVERLAY_FONT_TYPE_EN_BIT = 15,      //!< font type for text data type

  AM_OVERLAY_FONT_SIZE_EN_BIT = 16,      //!< font size for text data type

  AM_OVERLAY_FONT_COLOR_EN_BIT = 17,     //!< font color for text data type

  AM_OVERLAY_FONT_OUTLINE_EN_BIT = 18,   //!< font outline for text data type

  AM_OVERLAY_FONT_BOLD_EN_BIT = 19,      //!< font hor and ver bold

  AM_OVERLAY_FONT_ITALIC_EN_BIT = 20,    //!< font italic for text data type

  AM_OVERLAY_BMP_EN_BIT = 21,            //!< bmp file to be add or update

  AM_OVERLAY_BMP_COLOR_EN_BIT = 22,      //!< color key and range used for transparent

  AM_OVERLAY_ANIMATION_EN_BIT = 23,      //!< animation parameter

  AM_OVERLAY_LINE_COLOR_EN_BIT = 24,     //!< color for draw line

  AM_OVERLAY_LINE_POINTS_EN_BIT = 25,    //!< points for draw line

  AM_OVERLAY_LINE_THICKNESS_EN_BIT = 26, //!< line thickness

};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_UPDATE

**Function**

Use this command to update a area data block.

**Input Parameter**

```
am_overlay_data_t *msg_data
```

> For am_overlay_data_t, refer to previous section

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_GET

**Function**

Use this command to get area parameter.

**Input Parameter**

int32_t *msg_data //AM_STREAM_ID

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_overlay_area_t|

**am_overlay_area_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_OVERLAY_BITS|
|uint32_t|stream_id|Stream ID|
|uint32_t|enable|Area state|
|uint32_t|rotate|Area rotate attribute|
|uint32_t|bg_color|Area background color|
|uint32_t|width|Area width|
|uint32_t|height|Area Height|
|uint32_t|offset_x|Area offset x in the stream|
|uint32_t|offset_y|Area offset y in the stream|
|uint32_t|buf_num|Number of the buffer in the area|
|uint32_t|num|Number of data block in the area|

```
enum AM_OVERLAY_BITS {
  AM_OVERLAY_REMOVE_EN_BIT = 0,          //!< remove a overlay area
  AM_OVERLAY_ENABLE_EN_BIT = 1,          //!< enable a overlay area
  AM_OVERLAY_DISABLE_EN_BIT = 2,         //!< disable a overlay area
  AM_OVERLAY_INIT_EN_BIT = 3,            //!< init a area
  AM_OVERLAY_DATA_ADD_EN_BIT = 4,        //!< add a data block to area
  AM_OVERLAY_DATA_UPDATE_EN_BIT = 5,     //!< update a data block for a area
  AM_OVERLAY_DATA_REMOVE_EN_BIT = 6,     //!< remove a data block for a area
  AM_OVERLAY_ROTATE_EN_BIT = 7,          //!< area whether rotated with stream
  AM_OVERLAY_BG_COLOR_EN_BIT = 8,        //!< area or text background color
  AM_OVERLAY_BUF_NUM_EN_BIT = 9,         //!< area buffer number
  AM_OVERLAY_RECT_EN_BIT = 10,           //!< area or data block size and offset
  AM_OVERLAY_DATA_TYPE_EN_BIT = 11,      //!< data block type
  AM_OVERLAY_STRING_EN_BIT = 12,         //!< string to be add or update
  AM_OVERLAY_TIME_EN_BIT = 13,           //!< prefix/suffix string or enable msec
                                         //!< display for time type data block
  AM_OVERLAY_CHAR_SPACING_EN_BIT = 14,   //!< char spacing for text
  AM_OVERLAY_FONT_TYPE_EN_BIT = 15,      //!< font type for text data type
  AM_OVERLAY_FONT_SIZE_EN_BIT = 16,      //!< font size for text data type
  AM_OVERLAY_FONT_COLOR_EN_BIT = 17,     //!< font color for text data type
  AM_OVERLAY_FONT_OUTLINE_EN_BIT = 18,   //!< font outline for text data type
  AM_OVERLAY_FONT_BOLD_EN_BIT = 19,      //!< font hor and ver bold
  AM_OVERLAY_FONT_ITALIC_EN_BIT = 20,    //!< font italic for text data type
  AM_OVERLAY_BMP_EN_BIT = 21,            //!< bmp file to be add or update
  AM_OVERLAY_BMP_COLOR_EN_BIT = 22,      //!< color key and range used for transparent
  AM_OVERLAY_ANIMATION_EN_BIT = 23,      //!< animation parameter
  AM_OVERLAY_LINE_COLOR_EN_BIT = 24,     //!< color for draw line
  AM_OVERLAY_LINE_POINTS_EN_BIT = 25,    //!< points for draw line
  AM_OVERLAY_LINE_THICKNESS_EN_BIT = 26, //!< line thickness
};

```

###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_SET

**Function**

Use this command to set overlay.

**Input Parameter**

```
am_overlay_id_t *msg_data
```
**am_overlay_id_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_OVERLAY_BITS|
|uint32_t|stream_id|Stream ID|
|uint32_t|area_id|Area id in stream|
|uint32_t|data_index|Data block in area|

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_OVERLAY_DATA_GET

**Function**

Use this command to get area parameter.

**Input Parameter**

```
am_overlay_id_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_overlay_data_t|


###AM_IPC_MW_CMD_VIDEO_DYN_EIS_GET

**Function**

Use this command to get EIS setting.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_encode_eis_ctrl_t|

struct am_encode_eis_ctrl_t

> For am_encode_eis_ctrl_t, refer to next section

###AM_IPC_MW_CMD_VIDEO_DYN_EIS_SET

**Function**

Use this command to set EIS setting.

**Input Parameter**

```
am_encode_eis_ctrl_t *msg_data
```

**am_encode_eis_ctrl_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_ENCODE_EIS_CTRL_BITS|
|int32_t|eis_mode|EIS mode|

```
AM_ENCODE_EIS_CTRL_BITS
}
  AM_ENCODE_EIS_MODE_EN_BIT  = 0,
  AM_ENCODE_EIS_SAVE_CFG_EN_BIT = 1,

};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_CFG_ALL_LOAD

**Function**

use this command to load all modules config file

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_GET

**Function**

Use this command to get a configure item of motion detect module

**Input Parameter**

```
uit32_t *msg_data //roi_id
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_video_md_config_s|

> For am_video_md_config_t, refer to next section

###AM_IPC_MW_CMD_VIDEO_MOTION_DETECT_CONFIG_SET

**Function**

Use this command to set a configure item of motion detect module

**Input Parameter**

```
am_video_md_config_s *msg_data
```

**am_video_md_config_s:**
|Type|Field|Description|
|:-------|:----------|
|am_video_md_roi_s|roi|Refer to am_video_md_roi_s|
|am_video_md_threshold_s|threshold|Refer to am_video_md_threshold_s|
|am_video_md_level_change_delay_s|level_change_delay|Refer to am_vide_md_level_change_delay_s|
|uint32_t|buffer_id|Buffer ID|
|uint32_t|buffer_type|Buffer type|
|uint32_t|enable_bits|Bit control for related function, refer to AM_VIDEO_MD_CONFIG_BITS|
|bool|enable|true: enable motion detect; false: disable motion detect|

```
enum AM_VIDEO_MD_CONFIG_BITS
{
  /*! Config enabled */
  AM_VIDEO_MD_CONFIG_ENABLE = 0,

  /*! Config @ref am_video_md_threshold_s */
  AM_VIDEO_MD_CONFIG_THRESHOLD0 = 1,

  /*! Config @ref am_video_md_threshold_s */
  AM_VIDEO_MD_CONFIG_THRESHOLD1 = 2,

  /*! Config @ref am_video_md_level_change_delay_s */
  AM_VIDEO_MD_CONFIG_LEVEL0_CHANGE_DELAY = 3,

  /*! Config @ref am_video_md_level_change_delay_s */
  AM_VIDEO_MD_CONFIG_LEVEL1_CHANGE_DELAY = 4,

  /*! Config buffer ID */
  AM_VIDEO_MD_CONFIG_BUFFER_ID = 5,

  /*! Config buffer type */
  AM_VIDEO_MD_CONFIG_BUFFER_TYPE = 6,

  /*! Config @ref am_video_md_roi_s */
  AM_VIDEO_MD_CONFIG_ROI = 7,

  /*! Save current config */
  AM_VIDEO_MD_SAVE_CURRENT_CONFIG = 8,
};

```

```
struct am_video_md_threshold_s
{
    /*!
     * Threshold of motion level
     */
    uint32_t threshold[MOTION_LEVEL_NUM - 1];

    /*!
     * @sa AM_MD_ROI_ID
     */
    uint32_t roi_id;
};
```

```
struct am_video_md_level_change_delay_s
{
    /*!
     *  Motion level change times
     */
    uint32_t lc_delay[MOTION_LEVEL_NUM - 1];

    /*!
     * @sa AM_MD_ROI_ID
     */
    uint32_t roi_id;
};

```

```
struct am_video_md_roi_s
{
    /*!
     * @sa AM_MD_ROI_ID
     */
    uint32_t roi_id;

    /*!
     * width-offset of left border line
     */
    uint32_t left;

    /*!
     * width-offset of right border line
     */
    uint32_t right;

    /*!
     * height-offset of top border line
     */
    uint32_t top;

    /*!
     * height-offset of bottom border line
     */
    uint32_t bottom;

    /*!
     * - true: this ROI is valid
     * - false: this ROI is invalid
     */
    bool valid;
};


```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_STOP

**Function**

Use this command to dynamic stop motion detect module.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

###AM_IPC_MW_CMD_VIDEO_DYN_MOTION_DETECT_START

**Function**

Use this command to dynamic start motion detect module.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

##Media commands

The media service provides commnads for media related design and implementation, such as record, playback.

###AM_IPC_MW_CMD_MEDIA_MUXER_FILTER_START_SEND_PKT


**Function**

Use this command to make file muxer filter start to send packet to muxers.

**Input Parameter**

```
uint32_t *msg_data //video_id_bit_map
```

>Control video state by bit map

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START


**Function**

Use this command to start event recording, and it will record a event file in specified path when receives AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_START command.

**Input Parameter**

```
AMEventStruct* msg_data
```

```
struct AMEventStruct
{
    AM_EVENT_ATTR attr = AM_EVENT_NONE;
    union {
        AMEventH26x          h26x;
        AMEventMjpeg         mjpeg;
        AMEventPeriodicMjpeg periodic_mjpeg;
        AMEventStopCMD       stop_cmd;
    };
    AMEventStruct() {}
};
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


###AM_IPC_MW_CMD_MEDIA_EVENT_RECORDING_STOP

**Function**

Use this command to stop event recording.

**Input Parameter**

```
AMEventStruct *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|



### AM_IPC_MW_CMD_MEDIA_PERIODIC_JPEG_RECORDING

**Function**

 Use this command to set periodic jpeg recording,
 and it will record a sequence of jpeg in a specified file according to the parameter.

**Input Parameter**

```
AMEventStruct* msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID

**Function**

 Use this command to get playback instance id.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|int32_t player_id|

### AM_IPC_MW_CMD_MEDIA_RELEASE_PLAYBACK_INSTANCE_ID

**Function**

 Use this command to tell media service to release playback instance id
 which got by AM_IPC_MW_CMD_MEDIA_GET_PLAYBACK_INSTANCE_ID command.

**Input Parameter**

```
uint32_t *msg_data //id_bit_map
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE

**Function**

 Use this command to add audio file to media service.
 The media service will play these audio files when receives AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE command.

**Input Parameter**

```
AudioFileList *msg_data // audio_file
```

```
struct AudioFileList
{
    enum {
      MAX_FILENAME_LENGTH = 490,
      MAX_FILE_NUMBER     = 2,
    };
    AudioFileList()
    {
      memset(file_list, 0, MAX_FILE_NUMBER * MAX_FILENAME_LENGTH);
    }
    uint32_t file_number = 0;
    int32_t  playback_id_bit_map = 0;
    char file_list[MAX_FILE_NUMBER][MAX_FILENAME_LENGTH];
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_ADD_PLAYBACK_UNIX_DOMAIN

**Function**

 Use this command to add unix domain info to media service.
 The media service will playback the audio data which is received from unix domain.

**Input Parameter**

```
AMPlaybackUnixDomainUri *msg_data // unix_domain_uri
```

```
struct AMPlaybackUnixDomainUri
{
    AM_AUDIO_TYPE           audio_type;          //!<Audio type>
    uint32_t                sample_rate;         //!<Audio sample rate>
    uint32_t                channel;             //!<Audio channel>
    int32_t                 playback_id_bit_map; //!<playback id bit map>
    char                    name[64];            //!<unix domain name>
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE

**Function**

 Use this command to tell media service to start to play audio files
 which added by AM_IPC_MW_CMD_MEDIA_ADD_AUDIO_FILE command.

**Input Parameter**

```
PlaybackStartParam *msg_data
```

```
struct PlaybackStartParam
{
    uint32_t playback_id_bit_map = 0;
    bool     aec_enable = false;
    PlaybackStartParam() {}
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_PAUSE_PLAYBACK_AUDIO_FILE

**Function**

 Use this command to tell media service to pause the audio playback.
 If the media service is in a pause mode, it will continue playing the audio when receives AM_IPC_MW_CMD_MEDIA_START_PLAYBACK_AUDIO_FILE command.

**Input Parameter**

```
uint32_t *msg_data //playback_id_bit_map
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_STOP_PLAYBACK_AUDIO_FILE

**Function**

 Use this command to tell media service to stop playing audio file.

**Input Parameter**

```
uint32_t *msg_data //playback_id_bit_map
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_CREATE_PLAYBACK_INSTANCES

**Function**

 Use this command to tell media service to create playback instances.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_MEDIA_SET_PLAYBACK_INSTANCE_NUM

**Function**

 Use this command to tell media service to set playback instances number.

**Input Parameter**

```C++
AMPlaybackInstanceNum *msg_data
```

```C++
struct AMPlaybackInstanceNum
{
    int32_t playback_num        = 0;
    bool    save_to_config_file = false;
};
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_DESTROY_PLAYBACK_INSTANCES

**Function**

 Use this command to tell media service to destroy playback instances.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_MEDIA_CREATE_RECORDING_INSTANCE

**Function**

 Use this command to tell media service to create recording instance.

**Input Parameter**

```
int32_t *msg_data
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_MEDIA_DESTROY_RECORDING_INSTANCE

**Function**

 Use this command to tell media service to destroy recording instance.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_MEDIA_START_RECORDING

**Function**

 Use this command to tell media service to start media recording.

**Input Parameter**

```
uint32_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_STOP_RECORDING

**Function**

 Use this command to tell media service to stop media recording.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_START_FILE_RECORDING

**Function**

 Use this command to tell media service to start file recording.

**Input Parameter**

```
uint32_t *msg_data //muxer_id_bit_map
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_STOP_FILE_RECORDING

**Function**

 Use this command to tell media service to stop file recording.

**Input Parameter**

```
uint32_t *msg_data //muxer_id_bit_map
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_SET_RECORDING_FILE_NUM

**Function**

 Use this command to set recording file number.

**Input Parameter**

```C++
AMRecordingParam *msg_data
```

```C++
struct AMRecordingParam
{
    uint32_t muxer_id_bit_map;
    uint32_t recording_file_num;
    int32_t  recording_duration;
    int32_t  file_duration;
    bool     apply_conf_file;
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_SET_RECORDING_DURATION

**Function**

 Use this command to set recording duration.

**Input Parameter**

```C++
AMRecordingParam *param
```
> Refer to AM_IPC_MW_CMD_MEDIA_SET_RECORDING_FILE_NUM

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_MEDIA_SET_FILE_DURATION

**Function**

 Use this command to set file duration.

**Input Parameter**

```C++
AMRecordingParam *msg_data
```
> Refer to AM_IPC_MW_CMD_MEDIA_SET_RECORDING_FILE_NUM

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_SET_MUXER_PARAM

**Function**

 Use this command to set muxer parameters.

**Input Parameter**

```
AMMuxerParam *msg_data
```

```
struct AMMuxerParam
{
    uint32_t        muxer_id_bit_map  = 0;//0x00000001 << AM_MUXER_ID
    AMSettingOption file_duration_int32;
    AMSettingOption recording_file_num_u32;
    AMSettingOption recording_duration_u32;
    AMSettingOption digital_sig_enable_bool; //for av3 only
    AMSettingOption gsensor_enable_bool;
    AMSettingOption reconstruct_enable_bool;
    AMSettingOption max_file_size_u32;
    AMSettingOption write_sync_enable_bool;
    AMSettingOption hls_enable_bool;
    AMSettingOption scramble_enable_bool; //for av3 only
    AMSettingOption video_frame_rate_u32; // for time_elapse_mp4 only currently
    bool            save_to_config_file = false;
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_ENABLE_AUDIO_CODEC

**Function**

 Use this command to enable or disable audio codec.

**Input Parameter**

```
AudioCodecParam *msg_data
```

```
struct AudioCodecParam
{
    AM_AUDIO_TYPE  type;
    uint32_t       sample_rate;
    bool           enable;
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_SET_FILE_OPERATION_CALLBACK

**Function**

 Use this command to set file operation callback function.

**Input Parameter**

```
FileOperationParam *msg_data
```
```
struct FileOperationParam
{
    uint32_t type_bit_map     = 0;
    uint32_t muxer_id_bit_map = 0;
    bool     enable           = false;
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_UPDATE_3A_INFO

**Function**

 Use this command to update 3A info to media service.

**Input Parameter**

```
am_image_3A_info_s *msg_data
```
```
struct am_image_3A_info_s
{
    am_ae_config_s ae;
    am_af_config_s af;
    am_awb_config_s awb;
};
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_MEDIA_RELOAD_RECORDING_INSTANCE

**Function**

 Use this command to reload recording instance.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

##Audio APIs

The audio service provides commands for audio related design and implementation.


### AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_GET

**Function**

  Use this command to get current audio client audio sample rate.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_audio_format_t|

**am_audio_format_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|sample_rate|Sample rate|
|uint8_t|sample_bit_count|Sample bit|
|uint8_t|sample_format|Sample format, refer to PulseAudio library|
|uint8_t|channels|Sample channels, 1: mono, 2: stereo|
|uint8_t|reserved|Reserved, for alignment|

### AM_IPC_MW_CMD_AUDIO_SAMPLE_RATE_SET

**Function**

  Use this command to set current audio client audio sample rate.
  > Pulseaudio server's sample rate can't be changed.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_AUDIO_CHANNEL_GET

**Function**

  Use this command to get current audio client audio channel number.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_audio_format|

### AM_IPC_MW_CMD_AUDIO_CHANNEL_SET

**Function**

  Use this command to set current audio client audio channel number.

> Pulseaudio server's channels can't be changed.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_INDEX

**Function**

  Use this command to get audio device volume information  by audio device index number, to list all the audio device,  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.

**Input Parameter**

```
am_audio_device_t *msg_data
```

**am_audio_device_t:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|index|Device's index|
|uint8_t|type|Device's type|
|uint8_t|volume|Volume, minimum: 0; maximum: 100|
|uint8_t|alc_enable|This property is just used by mic|
|uint8_t|mute|Mute|
|char|name[AM_AUDIO_DEVICE_NAME_MAX_LEN]|Device's name|

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_INDEX

**Function**

  Use this command to set audio device volume information  by audio device index number, to list all the audio device,  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.

**Input Parameter**

```
am_audio_device_t *msg_data
```
> For am_audio_device_t, refer to previous section

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_GET_BY_NAME

**Function**

  Use this command to get audio device volume information  by audio device name string, to list all the audio device,  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_audio_device_t|

### AM_IPC_MW_CMD_AUDIO_DEVICE_VOLUME_SET_BY_NAME

**Function**

  Use this command to set audio device volume information  by audio device name string, to list all the audio device,  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.

**Input Parameter**

```C++
am_audio_device_t *msg_data
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_INDEX

**Function**

 Use this command to get audio device mute status by device index, to list all the audio device, refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_audio_device_t|


### AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_INDEX

**Function**

 Use this command to set audio device mute status by device index, to list all the audio device, refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET.

**Input Parameter**

```
am_audio_device_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_GET_BY_NAME

**Function**

 Use this command to get audio device mute status by device name string, to list all the audio device, refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET, to find related device name string, refer to AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_audio_device_t|

### AM_IPC_MW_CMD_AUDIO_DEVICE_MUTE_SET_BY_NAME

**Function**

 Use this command to set audio device mute status by device name string, to list all the audio device, refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET, to find related device name string, refer to

**Input Parameter**

```
am_audio_device_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET

**Function**

  Use this command to list all the audio devices including\n  - Sink: Audio output device  - Source: Audio input device

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_audio_device_t|

### AM_IPC_MW_CMD_AUDIO_DEVICE_NAME_GET_BY_INDEX

**Function**

  Use this command to get the audio device name string by device index.  to list all the audio devices,  refer to @ref AM_IPC_MW_CMD_AUDIO_DEVICE_INDEX_LIST_GET

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_audio_device_t|


##Event Commands

The event service provides commands for event related design and implementation.


### AM_IPC_MW_CMD_EVENT_REGISTER_MODULE_SET

**Function**

 Use this command to register an event module.

**Input Parameter**

```
am_event_module_path *msg_data
```

**am_event_module_path:**
|Type|Field|Description|
|:-------|:----------|
|char|*path_name|Path name|
|uint32_t|id|Module ID|

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_EVENT_CHECK_MODULE_REGISTER_GET

**Function**

 Use this command to check whether an event module is registered or not.

**Input Parameter**

```
uint32_t *msg_data //EVENT_MODULE_ID
```

```
enum EVENT_MODULE_ID
{
  EV_AUDIO_ALERT_DECT = 0,//!< Plugin ID of audio alert
  EV_AUDIO_ANALYSIS_DECT, //!< Plugin ID of audio analysis
  EV_FACE_DECT,           //!< Plugin ID of face detection
  EV_KEY_INPUT_DECT,      //!< Plugin ID of key press detection
  EV_ALL_MODULE_NUM,      //!< Total number of Plugins
};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_event_module_register_state|

**am_event_module_register_state:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|id|Module ID|
|bool|state|State, true: registered; false: not registered|

### AM_IPC_MW_CMD_EVENT_START_ALL_MODULE

**Function**

 Use this command to start all event modules which have been registered.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_EVENT_STOP_ALL_MODULE

**Function**

 Use this command to stop all event modules which have been registered.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_EVENT_DESTROY_ALL_MODULE

**Function**

 Use this command to destroy all event modules which have been registered.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_EVENT_START_MODULE

**Function**

 Use this command to start a specified event module which has been registered.

**Input Parameter**

```C++
uint32_t *msg_data // EVENT_MODULE_ID
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_EVENT_STOP_MODULE

**Function**

 Use this command to stop a specified event module which has been registered.

**Input Parameter**

```C++
uint32_t *msg_data // EVENT_MODULE_ID
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_EVENT_DESTROY_MODULE

**Function**

 Use this command to destroy a specified event module which has been registered.

**Input Parameter**

```C++
uint32_t *msg_data //EVENT_MODULE_ID
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_SET

**Function**

 Use this command to set a configure item of audio alert module

**Input Parameter**

```C++
am_event_audio_alert_config_s *msg_data
```

**am_event_audio_alert_config_s:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_EVENT_AUDIO_ALERT_CONFIG_BITS|
|uint32_t|channel_num|Audio channel number|
|uint32_t|sample_rate|Audio sample rate|
|uint32_t|chunk_bytes|Audio chunk bytes|
|uint32_t|sample_format|Audio sample format|
|uint32_t|sensitivity|Audio alert sensitivity|
|uint32_t|threshold|Audio alert threshold|
|uint32_t|directionAudio alert dirction|
|bool|enable|true: enable audio alert; false: disable audio alert|

```C++
AM_EVENT_AUDIO_ALERT_CONFIG_BITS

{

  AM_EVENT_AUDIO_ALERT_CONFIG_CHANNEL_NUM = 0,

  AM_EVENT_AUDIO_ALERT_CONFIG_SAMPLE_RATE = 1,

  AM_EVENT_AUDIO_ALERT_CONFIG_CHUNK_BYTES = 2,

  AM_EVENT_AUDIO_ALERT_CONFIG_SAMPLE_FORMAT = 3,

  AM_EVENT_AUDIO_ALERT_CONFIG_ENABLE = 4,

  AM_EVENT_AUDIO_ALERT_CONFIG_SENSITIVITY = 5,

  AM_EVENT_AUDIO_ALERT_CONFIG_THRESHOLD = 6,

  AM_EVENT_AUDIO_ALERT_CONFIG_DIRECTION = 7,

};
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_EVENT_AUDIO_ALERT_CONFIG_GET

**Function**

 Use this command to get a configure item of audio alert module

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_event_audio_alert_config_s|

struct am_event_audio_alert_config_s

> Refer previous section

### AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_SET

**Function**

 Use this command to set a configure item of key input module.

**Input Parameter**

```C++
am_event_key_input_config_s *msg_data
```

> For am_event_key_input_config_s, refer to next section

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_EVENT_KEY_INPUT_CONFIG_GET

**Function**

 Use this command to get a configure item of key input module

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_event_key_input_config_s|

**am_envent_key_input_config_s:**
|Type|Field|Description|
|:-------|:----------|
|uint32_t|enable_bits|Bit control for related function, refer to AM_EVENT_KEY_INPUT_CONFIG_BITS|
|uint32_t|long_press_time;

```C++
AM_EVENT_KEY_INPUT_CONFIG_BITS

{

  AM_EVENT_KEY_INPUT_CONFIG_LPT = 0,

};
```
##Playback Commands

The playback service provides Commands for playback related design and implementation.


### AM_IPC_MW_CMD_PLAYBACK_CHECK_DSP_MODE

**Function**

  Use this command to check if DSP is ready to playback.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_PLAYBACK_EXIT_DSP_PLAYBACK_MODE

**Function**

  Use this command to exit DSP playback mode.

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_START_PLAY

**Function**

  Use this command to start playback, need to specify prefer modules, output device, and initial playback speed/scan mode/direction

**Input Parameter**

```C++
am_playback_context_t *msg_data
```

**am_playback_context_t:**
|Type|Field|Description|
|:-------|:----------|
|uint16_t|playback_speed|Playback speed|
|uint8_t|instance_id|Instance ID|
|uint8_t|use_hdmi|Use HDMI output|
|uint8_t|use_digital|Use digital output|
|uint8_t|use_cvbs|Use CVBS output|
|char|prefer_demuxer[8]|Prefer demuxer module for playback, PRMP4; RTSP; FFMpeg; Auto|
|char|prefer_video_decoder[8]||Prefer audio decoder module for playback, AmbaDSP; LinuxFB, AUTO|
|char|prefer_video_output[8]|Prefer video output module for playback, LibAAC; FFMpeg; AUTO|
|char|prefer_audio_decoder[8]|Prefer audio decoder module for playback,LibAAC; FFMpeg; AUTO|
|char|prefer_audio_output[8]|Prefer audio output module for pkayback, ALSA; AUTO|
|uint8_t|scan_mode|Playback scan mode|
|uint8_t|playback_direction|Playback direction|
|char|url[AM_PLAYBACK_MAX_URL_LENGTH]|Playback url, can be local file or streaming url|

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_STOP_PLAY

**Function**

  Use this command to stop playback

**Input Parameter**

```
am_playback_instance_id_t *msg_data
```
**am_playback_instance_id_t:**
|Type|Field|Description|
|:-------|:----------|
|uint8_t|instance_id|Playback instance id|
|uint8_t|reserved0|Reserved|
|uint8_t|reserved1|Reserved|
|uint8_t|reserved2|Reserved|

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_PAUSE

**Function**

  Use this command to pause play

**Input Parameter**

```
am_playback_instance_id_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_RESUME

**Function**

  Use this command to resume play

**Input Parameter**

```C++
am_playback_instance_id_t *msg_data
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_PLAYBACK_STEP

**Function**

  Use this command to step play

**Input Parameter**

```C++
am_playback_instance_id_t *msg_data
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_QUERY_CURRENT_POSITION

**Function**

  Use this command to query playback position

**Input Parameter**

```C++
am_playback_instance_id_t *msg_data
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|unsigned long long|


### AM_IPC_MW_CMD_PLAYBACK_SEEK

**Function**

  Use this command to seek

**Input Parameter**

```C++
am_playback_instance_id_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|unsigned long long|

### AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD

**Function**

  Use this command to fast forward play

**Input Parameter**

```C++
am_playback_fast_forward_t *msg_data
```

**am_playback_fast_forward_t:**

|Type|Field|Description|
|:-------|:----------|
|uint16_t|playback_speed|Playback speed|
|uint8_t|instance_id|Playback instance id|
|uint8_t|scan_mode|Playback scan mode|

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD

**Function**

  Use this command to fast backward play

**Input Parameter**

```C++
am_playback_fast_backward_t *msg_data
```

**am_playback_fast_backward_t:**
|Type|Field|Description|
|:-------|:----------|
|uint16_t|playback_speed|Playback speed|
|uint8_t|instance_id|Playback instance id|
|uint8_t|scan_mode|Playback scan mode|

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_FAST_FORWARD_FROM_BEGIN

**Function**

  Use this command to fast forward play from begin

**Input Parameter**

```C++
am_playback_fast_forward_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_FAST_BACKWARD_FROM_END

**Function**

  Use this command to fast backward play from end

**Input Parameter**

```
am_playback_fast_backward_t *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_CONFIGURE_VOUT

**Function**

  Use this command to configure vout.

**Input Parameter**

```
am_playback_vout_t *msg_data
```

**am_playback_vout_t:**
|Type|Field|Description|
|:-------|:----------|
|uint8_t|instance_id|Instance ID|
|uint8_t|vout_id|VOUT 0 would be digital(LCD), VOUT 1 can be HDMI or CVBS|
|uint8_t|reserved0|Reserved|
|uint8_t|reserved1|Reserved|
|char|vout_mode[32]|Reserved
|char|vout_sinktype[32]|Reserved|
|char|vout_device[32]|Reserved|

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_PLAYBACK_HALT_VOUT

**Function**

  Use this command to halt vout

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


##System Commands

The system service provides Commands for system related design and implementation.


### AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_SET

**Function**

 use this command to set firmware upgrade parameters

**Input Parameter**

```C++
void *msg_data
```
**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_SYSTEM_FIRMWARE_UPGRADE_STATUS_GET

**Function**

 use this command to get firmware upgrade status

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_mw_upgrade_state|

**am_mw_upgrade_status:**
|Type|Field|Description|
|:-------|:----------|
|bool|in_progress|Whether upgrade in progress|
|am_mw_upgrade_state|state|Firmware upgrade status|

### AM_IPC_MW_CMD_SYSTEM_FIRMWARE_VERSION_GET

**Function**

 use this command to get current firmware version information

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_mw_firmware_version_all|

**am_mw_firmware_version_all:**
|Type|Field|Description|
|:-------|:----------|
|uint8_t|major|Major version number|
|uint8_t|minor|Minor version number|
|uint8_t|trivial|Revision number|

### AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_GET

**Function**

 use this command to get specified gpio LED status

**Input Parameter**

```
int32_t *msg_data //gpio_id
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_mw_led_config|

**am_mw_led_config:**
|Type|Field|Description|
|:-------|:----------|
|int32_t|gpio_id|Gpio index
|uint32_t|on_time|Used for led blink|
|uint32_t|off_time|Used for led blink|
|bool|blink_flag|Whether blink|
|bool|led_on|Led status if blink not set|

### AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_SET

**Function**

 use this command to set specified gpio LED status

**Input Parameter**

```C++
am_mw_led_config *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT

**Function**

 use this command to release specified initialized gpio led

**Input Parameter**

```
uint32_t *msg_data //gpio_id
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_SYSTEM_LED_INDICATOR_UNINIT_ALL

**Function**

 use this command to release all initialized gpio LEDs

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


### AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_GET

**Function**

 use this command to get system time

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|struct am_mw_system_settings_datetime|

**am_mw_system_settings_datetime:**
|Type|Field|Description|
|:-------|:----------|
|am_mw_date|date|Refer to am_mw_data|
|am_mw_time|time|Refer to am_mw_time|

```C++
struct am_mw_date
{
    uint16_t year; //!< year
    uint8_t  month; //!< month
    uint8_t  day; //!< day
};
```

```C++
struct am_mw_time
{
    uint8_t  hour;   //!< hour in 24-hour format
    uint8_t  minute; //!< minute
    uint8_t  second; //!< second
    /*!
     * - negative(-): west time zone
     * - positive(+): east time zone
     *
     * eg. +8 is for Beijing Time.
     */
    int8_t   timezone; //!< time zone
};

```

### AM_IPC_MW_CMD_SYSTEM_SETTINGS_DATETIME_SET

**Function**

 use this command to set system time

**Input Parameter**

```C++
am_mw_system_settings_datetime *msg_data
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|

### AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_GET

**Function**

 use this command to get ntp settings

**Input Parameter**

Null

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|string ntp_set|

### AM_IPC_MW_CMD_SYSTEM_SETTINGS_NTP_SET

**Function**

 use this command to get ntp settings

**Input Parameter**

```C++
string npt_set
```

**Output Parameter**

|Name|Type|Filed: Result value|Filed: Service state|Filed: Output Data|
|:---|:---|:-----------|:------------|:----------|
|result_addr|am_service_result_t *|[AM_RESULT](#Commands)|[AM_SERVICE_STATE](#Commands)|Null|


#Unit Test
## Video Service
The interactive test commands in Video Service could be divided into two categories:
**Static Configurations** and **Dynamic Sets**.
The Static Configurations is also same to directly edit config files. Instead, the Dynamic sets allow users to set feature in running time.

### Static Configurations: test_video_service_cfg_air_api
|Option|Usage|
|:------|:-----|
|--vin-flip [0 - 3]|VIN horizontal flip. 0: not flip, 1: flipv; 2: fliph; 3: both. Set to vin.acs.|
|--vin-fps|VIN framerate. 0: auto; Set to vin.acs.|
|-V or --vout [0 - 1]|VOUT ID. 0: VOUTB; 1: VOUTA|
|--vout-type [0 - 4], **Follow -V**|VOUT physically type. 0: none; 1: LCD; 2: HDMI; 3: CVBS; 4:YPbPr. Set to vout.acs.|
|--vout-video-type [0 - 6], **Follow -V**|VOUT video type. 0: none; 1: YUV BT601; 2: YUV BT656; 3: YUV BT1120; 4:RGB BT601; 5: RGB BT656; 6:RGB BT1120|
|--vout-mode, **Follow -V**|VOUT mode: 480i/576i/480p/576p/720p/1080p... Set to vout.acs.|
|--vout-flip [0 - 3], **Follow -V**|VOUT flip. 0: none; 1:v-flip; 2: h-flip; 3:both-flip. Set to vout.acs.|
|--vout-rotate [0 - 1], **Follow -V**|VOUT rotate. 0: none; 1:90 degree rotate. Set to vout.acs.|
|--vout-fps|VOUT frameate. 0: auto. Set to vout.acs.|
|-b or --buffer [0 - 6]|Source buffer ID. 0: Main buffer; 1: 2nd buffer; 2: 3rd buffer; 3: 4th buffer; 4: 5th buffer; 5: Pre-main buffer; 6: EFM buffer.|
|--type [OFF &#124; ENCODE &#124; PREVIEW], **Follow -b**|Source buffer type. Set to source_buffer.acs.|
|--input-crop [0 &#124; 1], **Follow -b**|Crop input. 0: use default input window; 1: use input window specify in next options. Set to source_buffer.acs.|
|--input-w, **Follow -b**|Input window width. Set to source_buffer.acs.|
|--input-h, **Follow -b**|Input window height. Set to source_buffer.acs.|
|--input-x, **Follow -b**|Input window offset X. Set to source_buffer.acs.|
|--input-y, **Follow -b**|Input window offset Y. Set to source_buffer.acs.|
|-W or --width, **Follow -b**|Source buffer width. Set to source_buffer.acs.|
|-H or --height, **Follow -b**|Source buffer height. Set to source_buffer.acs.|
|-s or --stream [0 - 3]|Stream ID|
|--enable [0 &#124; 1], **Follow -s**|Disable or enable stream. Set to stream_fmt.acs.|
|-t or --type[H264 &#124; H265 &#124; MJPEG], **Follow -s**|Stream type. Set to stream_fmt.acs.|
|--source [0 - 6], **Follow -s**|Stream source. Refer to "--buffer" option. Set to stream_fmt.acs.|
|--framerate, **Follow -s**|Stream frame rate. Set to stream_fmt.acs.|
|-W or --width, **Follow -s**|Stream width. Set to stream_fmt.acs.|
|-H or --height, **Follow -s**|Stream height. Set to stream_fmt.acs.|
|--off-x, **Follow -s**|Stream offset x. Set to stream_fmt.acs.|
|--off-y, **Follow -s**|Stream offset y. Set to stream_fmt.acs.|
|--hflip [0 &#124; 1], **Follow -s**|Stream horizontal flip. Set to stream_fmt.acs.|
|--vflip [0 &#124; 1], **Follow -s**|Stream vertical flip. Set to stream_fmt.acs.|
|--rotate [0 &#124; 1], **Follow -s**|Stream counter-clock-wise rotation 90 degrees. Set to stream_fmt.acs.|
|--b-ctrl [0 - 1], **Follow -s**|Bitrate control method. 0: CBR; 1: VBR. Set to stream_cfg.acs.|
|--profile [0 - 2], **Follow -s**|H264 or H265 Profile. 0: Baseline; 1: Main; 2: High. Set to stream_cfg.acs.|
|--au-type [0 - 3], **Follow -s**|H264 or H265 AU type. Set to stream_cfg.acs.<br>0: NO_AUD_NO_SEI; 1: AUD_BEFORE_SPS_WITH_SEI; 2: AUD_AFTER_SPS_WITH_SEI; 3: NO_AUD_WITH_SEI|
|--h-chroma [420 &#124; mono], **Follow -s**|H264 or H265 Chroma format. Set to stream_cfg.acs.|
|-m or --gop-m [1 - 3], **Follow -s**|H264 or H265 GOP M. Set to stream_cfg.acs.|
|-n or --gop-n [15 - 1800], **Follow -s**|H264 or H265 GOP N. Set to stream_cfg.acs.|
|--idr [1 - 4], **Follow -s**|H264 or H265 IDR interval. Set to stream_cfg.acs.|
|--bitrate, **Follow -s**|H264 or H265 Target bitrate. Set to stream_cfg.acs.|
|--mv [0 - 64], **Follow -s**|H264 or H265 MV threshold. Set to stream_cfg.acs.|
|--flat-area-improve [0 &#124; 1], **Follow -s**|H264 or H265 encode improve false or true. Set to stream_cfg.acs.|
|----multi-ref-p [0 &#124; 1], **Follow -s**|H264 or H265 multi ref P. Set to stream_cfg.acs.|
|--fast-seek-intvl [0 - 63], **Follow -s**|H264 or H265 fast seek interval. Set to config file. Set to stream_cfg.acs.|
|--m-quality [1 - 100], **Follow -s**|MJPEG quality. Set to stream_cfg.acs.|
|--m-chroma [420 &#124; mono], **Follow -s**|MJPEG Chroma format. Set to stream_cfg.acs.|
|--feature-mode [mode0 &#124; mode1 &#124; ... &#124; mode10]|Mode type in feature. Set to features.acs.|
|--feature-hdr [none &#124; 2x &#124; 3x &#124; 4x &#124; sensor]|HDR type in feature. Set to features.acs.|
|--feature-iso [normal &#124; plus &#124; advanced]|ISO type in feature. Set to features.acs.|
|--feature-dewarp [none &#124; ldc &#124; full &#124; eis]|Dewarp type in feature. Set to features.acs. (Not supported on S2E platform)|
|--feature-dptz [disable &#124; enable]|DPTZ in feature. Set to features.acs.|
|--feature-bitrate [none &#124; lbr]|Bitrate control type in feature. Set to features.acs.|
|--feature-overlay [disable &#124; enable]|Overlay type in feature. Set to features.acs.|
|--feature-hevc [disable &#124; enable]|HEVC type in feature. Set to features.acs.|
|--feature-iav-ver [1 &#124; 2 &#124; 3]|IAV version in feature. Set to features.acs.|

### Dynamic Sets: test_video_service_dyn_air_api
|Option|Usage|
|:------|:-----|
|--buffer-max-n|Show platform support max buffer number|
|--buffer, -b [0-3]|Source buffer ID. 0: Main buffer; 1: 2nd buffer; 2: 3rd buffer; 3: 4th buffer|
|--width, -W, **Follow -b**|Source buffer width|
|--hight, -h, **Follow -b**|Source buffer height|
|--input-x, **Follow -b**|Source buffer input window offset X|
|--input-y, **Follow -b**|Source buffer input window offset Y|
|--input-w, **Follow -b**|Source buffer input window width|
|--input-h, **Follow -b**|Source buffer input window height|
|--pan-ratio [-1.0 - 1.0], **Follow -b**|Pan ratio|
|--tilt-ratio [-1.0 - 1.0]**Follow -b**|Tilt ratio|
|--zoom-ratio [-1.0 - 1.0]**Follow -b**|Zoom ratio|
|--stream, -s [0-3], **Follow -s**|Stream ID|
|--stream-bitrate|Bitrate set|
|--strean-framerate|Framerate set|
|--stream-mjpeg-quality|MJPEG quality set|
|--stream-h26x-gop-n|H264 or H265 gop N set|
|--stream-h26x-gop-idr|H264 or H265 gop idr set|
|--stream-source, **Follow -s**|Stream source buffer set, must restart stream to apply it|
|--stream-type [H264\H265\MJPEG], **Follow -s**|Stream type set, must restart stream to apply it|
|--stream-size-w, **Follow -s**|Stream width set, must restart stream to apply it|
|--stream-size-h, **Follow -s**|Stream width set, must restart stream to apply it|
|--stream-offset-x, **Follow -s**|Stream offset x set, must restart stream to apply it|
|--stream-offset-y, **Follow -s**|Stream offset y set, must restart stream to appy it|
|--stream-flip set [0 - 3], **Follow -s**|Stream flip set, 0: none, 1: vflip, 2: hflip, 3: both flip. must restart stream to apply it|
|--stream-rotate, **Follow -s**|Stream rotate set, 0: none, else: clock-wise rotation 90 degrees. must restart stream to apply it|
|--stream-profile, **Follow -s**|Stream h26x profile set, 0: base line; 1: main line, 2: high line. H265 just support main line. must restart stream to apply it|
|--lock, **Follow -s**|Lock the stream by specifying UUID|
|--unlock, **Follow -s**|Unlock the stream by specifying UUID|
|--force-idr, **Follow -s**|Force IDR at once for current stream|
|--max-radius, **Follow -s**|Full FOV circle radius in vin, 0: use vin_width/2|
|--pan-ratio [-1.0 - 1.0], **Follow -s**|PAN ration|
|--tilt-ratio [-1.0 - 1.0], **Follow -s**|Tilt ratio|
|--dptz-x, **Follow -b**|DPTZ offset X|
|--dptz-y, **Follow -b**|DPTZ offset Y|
|--dptz-w, **Follow -b**|DPTZ width|
|--dptz-h, **Follow -b**|DPTZ height|
|--ldc-mode [0 - 8], **Follow -s**|LDC warp mode|
|--region-id [0 - 7], **Follow -s**|Region number|
|--ldc-strength [0.0 - 36.0], **Follow -s**|LDC strength|
|--pano-hfov-degree [1 - 180], **Follow -s**|Panorama HFOV degree|
|--warp-region-yaw [-90 - 90], **Follow -s**|Lens warp region yaw in degree|
|--warp-region-pitch [-90 - 90], **Follow -s**|Lens warp region pitch in degree|
|--warp-hor-zoom, **Follow -s**|Specify horizontal zoom ratio. Input: numerator/denumerator|
|--warp-ver-zoom, **Follow -s**|Specify vertical zoom ratio. Input: numerator/denumerator|

### Example Stream Configuration set: s3lm/ov4689
In this example, try to change current stream to another source buffer and make different configuration.
Set stream to 720p from source buffer 1. Step 1: start services. Step 2: start source buffer 1 and set buffer size
Step 3: set current stream to buffer 1 and re-start encoding.
```shell
s3lm # apps_launcher
s3lm # test_video_service_dyn_air_api -b 1 -W 720 -H 480 -s 0 --stream-source 1 --stream-size-w 720 --stream-size-h 480 --stream-restart
```


#### Example Panorama Lens Warp: s3lm/ov4689
It allows users to set LDC feature. So far it supports 9 kinds of Lens Dewarp in total. The related LDC mode is based on lens type.
```shell
s3lm # apps_launcher
s3lm # test_video_service_cfg_air_api --feature-mode mode5 --feature-dewarp ldc --apply
s3lm # test_video_service_dyn_air_api -s 0 --ldc-mode 2 --pano-hfov-degree 180 --ldc-strength 18
```



#Appendix
##<span id="set_machine">Set up user's building machine</span>

### PC sets

Firstly, user should make sure user install the related softwares. The related softwares are needed configuration below:

|EVK Software | Denote |
|-------------|-------:|
|AmbaUSB      |for burning firmware vis USB|
|PuTTY        |as a terminal tool for console access|
|VLC media player| for video playback of RTSP stream|

#### Setting Up the Network

1. Change the IP Address of the PC network adapted to the board to 10.0.0.1. Note that the default IP address
of the board is 10.0.0.2. The example of IP configuration On PC shows in below:

    ![image_network_path](img/network.png)

2. Use the following command to determine if the network is working.

    ```ping 10.0.0.2```

#### Connecting to a Console Window

The console window enables the users to view the system boot sequence messages and run the
demonstration applications from the Linux shell.

##### Console Window: Serial

Connect a serial cable between the serial port on the EVK and the serial port (for example, COM1) on the host
PC.
Open a terminal emulator (such as Minicom on Linux or PuTTY / HyperTerminal on Windows) on the host
PC. Ambarella recommends using PuTTY on Windows. PuTTY can be found in the EVK tools.
Configure the terminal emulator to connect to the serial port using the following data:

|Item     |Set        |
|---------|----------:|
|Speed    |115200     |
|Data Bits|8          |
|Parity   |None       |
|Stop Bits|1          |
|Flow control|None    |

**Using PuTTY as example(for Windows)**:

* From PuTTY Configuration dialog, choose Category > Connection > Serial and for Serial line to
Connect to the correct port (For example, COM1). Use the settings given below, for Speed,
Data Bits, Parity, Stop Bits, and Flow Control.

    ![putty_1_path](img/putty-1.png)

* Using the PuTTY Configuration dialog, choose Category > Session for connection type. Then,
choose Serial and set Serial Line to COM1, Speed to 115200, and save the current session (For
example, Amba Serial), so it can be reused.

    ![putty_2_path](img/putty-2.png)

* Click Open and the following console appears:

    ![putty_3_path](img/putty-3.png)

* Power on the EVK board, the console reads the boot sequence messages. After Linux boots up,
login as root.

    ![putty_4_path](img/putty-4.png)

**Using the Minicom as example(for ubuntu):**

1. Type **sudo minicom -s** in shell, choose the **Serial port setup** to config as blew:

    ![image_minicom](img/minicom.png)

2. Save and exit from current set.

3. Type **sudo minicom** to launch the default Serial port.

##### Console Window: Telnet

Connect an Ethernet cable between the EVK and the host PC.
Open a terminal emulator on the host PC.Configure the terminal emulator to the Telnet connection type using:

* host Name (or IP address): 10.0.0.2 (default)
* port:                       23

Using PuTTY as an example:

1. From PuTTY Configuration window, choose Session. In the right panel, select Telnet as
Connection type. For Host Name (or IP address) enter 10.0.0.2, for Port enter 23. Save the
session and click Open.

    ![putty_telnet_path](img/putty_telnet.png)

##### Play videos with VLC

To view the H.264 streams via VLC, configure the VLC as show in below.
Launch VLC and choose Tools > Preferences.

1. Select **All** in Show setting at the bottom left of the Preferences dialog.
2. Click **Input/Codecs** to unfold the option.
3. Single click **Demuxer**. Do not expand the folder.
4. On the right panel, select **H264 video demuxer** for the Demux module.
5. Click **Save** to quit.

    ![VLC_path](img/VLC.png)


##<span id="">Install Toolchain and AmbaUSB</span>

The related software needed will be provided by Amba Ltd, such as **AmbaUSB** and **Linaro Toolchain**.
The Linaro toochain is comprised of robust, comercial-grade tools which are optimized for Cortex-A processors. Linaro tools, software and testing procedures are include in this latest release of the processor SDK.

The version used in the latest SDK is as follows:

**Linaro-GCC :5.3    2016.02**

when downloading the Toolchain package, choose the IA64 GNU/Linux TAR file:
For example, linaro-multilib-2015.02-gcc4.9-ix64.tar.xz
Install the Toolchain
(for ubuntu linux, the commands are as follows)

```
build $ cd tools/ToolChain

build sudo chmod +x ubuntuToolChain

build sudo ./ubuntuToolChain
```


##<span id="">Prepare a Ambarella EVK board</span>

Before specified operations, users should connect cables from EVK boards to host PC firstly. As an example, the connection of Antman is shown below:

![image_ahead](img/ahead_ant.JPG)

![image_back](img/back_ant.JPG)

The middleware of Oryx can help customers quickly customize it for products.

For comprehensive showing the operations can be configured, list cmds and options below:
