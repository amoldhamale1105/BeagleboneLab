# BeagleboneLab
Embedded Linux and device driver programming using beaglebone black  
This repo contains 3 resources for the am335xx soc based beaglebone board  
1. Compiled boot binaries  
2. uEnv.txt for various u-boot configurations  
3. Device setups and drivers. Build and test instruction sections below apply to this resource
4. Misc scripts

## Compiled boot binaries
Bootloader binaries (`MLO`, `SPL` and `u-boot.img`) required to boot Beaglebone black with Embedded Linux  
This directory also contains the default device tree binary for BBB  
`uImage` is the actual kernel image, custom built with the linux kernel source and required static modules  
All these files should be present in the BOOT partition of the SD card while booting  

## uEnv.txt
uEnv.txt is used to set environent paramters during initial booting process (u-boot) of the board  
This file along with u-boot load the kernel, dtb and initramfs (if applicable otherwise points to the device partion holding the root filesystem) into memory  
As per the suffix, different environment vars are used for different boot configurations  
While adding any of the uEnv files in the BOOT partition ensure removing the suffix from the filenames. It should be named `uEnv.txt` only

## Misc scripts
`usbnet.sh` script is used to enable internet connection on BBB  
This script accepts 2 arguments:  
1. out-interface which is the interface of the host with internet connection  
2. in-interface which is the input ethernet interface of the board  

Example exec of the script will be:
```
./usbnet.sh wlp4s0 eth0
```

## Devices and drivers
Different types of device and driver setups for managing platform devices  
Pseudo character device drivers can be tested on any host running Linux  
Cross compilation of drivers for ARM based processors has been taken care of in the Makefile (follow build and test instructions below)

### Build Instrctions
Enter the directory of driver to be tested under `Drivers/` directory  
Run the following command to build a kernel object for ARM
```
make
```
Run the following command to build a kernel object for x86_64 based machine
```
make host
```

### Test instructions
All the drivers were tested on an ARM based AM335xx SOC (Beaglebone black SBC)  
Alternatively some drivers like the pseudo character driver can be tested directly on host with appropriate build command  
The kernel module `g_ether` must be present to transfer files using ethernet over usb otherwise connect a separate ethernet cable to the board for file transfer  
Run the following command in the BBB terminal to setup a network interface over usb
```
sudo ifconfig usb0 192.168.7.2 netmask 255.255.255.0
```
Try pinging 192.168.7.1 which should be host the SBC is connected to test if the interface works  
Copy all the kernel objects including drivers and devices with the following command
```
make copy-driver
```
Device tree binaries can be transferred with the following command
```
make copy-dtb
```
Load the device or driver with the following command
```
sudo insmod drviers/<driver/device-name>.ko
```
Remove or unload the device or driver with the following command
```
sudo rmmod drviers/<driver/device-name>.ko
```
The above instructions for loading and unloading are under the assumption that your current directory is `/home/debian/`
