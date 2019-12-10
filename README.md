# Nokia5110LCDriver

<img src="https://drive.google.com/uc?export=view&id=1yljsNU_sk-49aKaAVxRM89XZSlJNzrS1" width="300" height="300">

## Installing driver
```
cd driver
make
sudo insmod main.ko
sudo mknod /dev/lcd5110 c 61 0
sudo chmod a+rw /dev/lcd5110
```
## Removing driver
```
sudo rmmod led5110
```

## Interface
```
cd ../user_interface
g++ -o int interface_main.cpp
```

Usage:
``` 
./int -[m|f] ["m -- message"|"f -- filename"]
```
