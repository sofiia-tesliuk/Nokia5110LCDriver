# Nokia5110LCDriver

<img src="https://drive.google.com/uc?export=view&id=1yljsNU_sk-49aKaAVxRM89XZSlJNzrS1" width="300" height="300">

## Installing driver
```
cd gpio_lcd5110
make
sudo insmod gpio_lcd5110.ko
sudo mknod /dev/gpio_lcd5110 c 61 0
sudo chmod a+rw /dev/lcd5110

cd /sys/gpio_lcd5110/gpio_lcd5110
sudo chmod a+rw state_mode
sudo chmod a+rw data_mode 
```

## Usage
```
// Enable text mode
sudo echo "t" > data_mode
// Enable image mode
sudo echo "i" > data_mode

// Active
sudo echo "a" > state_mode
// Sleep
sudo echo "s" > state_mode

sudo echo "Hello!" > /dev/gpio_lcd5110
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
