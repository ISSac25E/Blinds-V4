# ESP8266 RTOS SDK Installation
Many inconsistent or outdated guides. Writing this guide for personal reference.

## Windows Flexible setup

### USING https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/windows-setup.html

Download and extract the ToolChain with MSYS2 package under 'Toolchain Setup'. May take a while to extract.\
This may be extracted anywhere on the computer.\
The ToolChain + MSYS2 has the ESP32 ToolChain. It is only needed for the pre-set MSYS2 environment, a different ToolChain is required as will be shown below.

From provided document above, download and unzip the ESP8266 toolchain anywhere. (NOT ToolChain+MSYS2)\
Download the correct link depending which SDK you are planning to use.

### USING https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#get-started-get-esp-idf

Open terminal in desired location and clone desired SDK repo\
Use this command and repo as a starting example:
```
git clone --recursive https://github.com/espressif/ESP8266_RTOS_SDK.git
```

### USING https://github.com/espressif/ESP8266_RTOS_SDK/issues/785#issuecomment-748691888

Open the ".bash_profile" to update paths.\
This will be located in: ```"\msys32\home\<USER>\.bash_profile"```\
todo: this dir doesnt exist with clean setup

Add these two lines at the bottom of the file following this format.
```
export PATH=$PATH:/c/<PATH TO TOOLCHAIN>/xtensa-lx106-elf/bin
export IDF_PATH="/c/<PATH TO SDK>/ESP8266_RTOS_SDK"
```

### Continue USING https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#get-started-get-esp-idf

Before continuing to the next step, a modification must be made to the ```requirements.txt```\
This will be located in ```"\ESP8266_RTOS_SDK\requirements.txt"```

I newer version of ```pyelftools``` has a dependency that cannot be installed by python 2.7

In ```requirements.txt```, change:\
```pyelftools>=0.22``` to ```pyelftools==0.22```\
Save the file


Open the mingw32.exe terminal. (msys2.exe will not work as python is not recognized)\
This will be located in ```"\msys32\mingw32.exe"```\
Use this command to install requirements:
```
python -m pip install --user -r $IDF_PATH/requirements.txt
```

Continue using this link to build and flash the first example project:
https://docs.espressif.com/projects/esp8266-rtos-sdk/en/latest/get-started/index.html#get-started-get-esp-idf
