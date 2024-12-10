# ESP8266 RTOS SDK Installation
Many inconsistent or outdated guides. Writing this guide for personal reference.

## Windows Flexible setup

### Using https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/windows-setup.html

Install MSYS2 using https://www.msys2.org/\
Keep MSYS2 around as a useful tool kit

Open MSYS2 and run these commands, following prompts as needed:\
use installed_packages.txt and paste into home dir
```
pacman -S --needed - < installed_packages.txt
```

```
// to update all packages. restart terminal if prompted
pacman -Syu
```
```
// to install the make package
pacman -Syu make
make --version // check installation
```
```
// to install python and pip
pacman -Syu python-pip
// check that both installed properly:
python --version
pip --version
```

From provided "[using](https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/windows-setup.html)" document above, download and unzip toolchain anywhere. (NOT ToolChain+MSYS2)\
Download the correct link depending which SDK you are planning to use

### Using https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#get-started-get-esp-idf

Open terminal in desired location and clone desired SDK repo\
Use this command and repo as a start:
```
git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
```
### Using https://github.com/espressif/ESP8266_RTOS_SDK/issues/785#issuecomment-748691888

Open the bash profile to update paths.\
This will be located in: ```"\msys64\home\<USER>\.bash_profile"```

Add these two lines at the bottom of the file following this format.\
```
export PATH=$PATH:/mnt/c/<PATH TO TOOLCHAIN>/xtensa-lx106-elf/bin
export IDF_PATH="/c/<PATH TO SDK>/ESP8266_RTOS_SDK"
```

### Continue Using https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#get-started-get-esp-idf

Open the msys terminal and use this command to install requirements:
```
python -m pip install --user -r $IDF_PATH/requirements.txt
```

## Links and Notes

https://github.com/espressif/ESP8266_RTOS_SDK/issues/785#issuecomment-748691888