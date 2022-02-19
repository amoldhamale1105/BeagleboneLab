# BeagleboneLab
Embedded Linux and device driver programming using beaglebone black  
Different types of device and driver setups for managing platform devices  
Pseudo character device drivers can be tested on any host running Linux  
Cross compilation of drivers for ARM based processors has been taken care of in the Makefile (follow build and test instructions below)

## Build Instrctions
Enter the directory of driver to be tested under `Drivers/` directory  
Run the following command to build a kernel object for ARM
```
make
```
Run the following command to build a kernel object for x86_64 based machine
```
make host
```

## Test instructions
All the drivers were tested on an ARM based AM335xx SOC (Beaglebone black SBC)  
Alternatively some drivers like the pseudo character driver can be tested directly on host with appropriate build command  
The kernel module `g_ether` must be present to transfer files over Ethernet
Run the following command in the BBB terminal to setup a network interface
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
