# Dependencies
This module uses only linux headers
Check if there is one on your machine, if not install them.
For example on Ubuntu run:
``` shell
sudo apt-get install linux-headers-$(uname -r)
```
# Building
To build kernel module run:
``` shell
./build.sh
```
This script simply runs make all command
# Installing
To install builded kernel module run:
``` shell
./install.sh
```
This script checks if there is already inserted kernel module.
If not, it inserts it into kernel and checks if it was inserted correctly
# Usage
After building and installing this module you can use device mapper proxy device
To create new device mapper proxy device execute:
``` shell
sudo dmsetup create you_device_name --table "0 $size dmp /dev/mapper/dm-device"
```
If no problems occurs, you can use device.

Usage statistics for this device is stored in /sys/module/dmp/stat/volumes
``` shell
cat /sys/module/dmp/stat/volumes
```

