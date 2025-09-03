# kaleidoscope_adapter
kaleidoscope_adapter is Dygma's repository adapting the kaleidoscope framework to the higher level Dygma libraries and applications through the keyboard_api interface.

## Structure

* `fal`           framework abstraction layer  
* `kaleidoscope`  Kaleidoscope-Dygma repository  
* `keyboard_api`  Dygma keyboard interface repository  

## Mandatory App Level Definitions
Following parameters need to be specified on the application level. You can place them to
* config_kbd.h - keyboard_api configuration file which is demanded to be created on the application level.
* Makefile or CMakeList.txt

### #define KALEIDOSCOPE_ADAPTER_KEYMAP_DEFAULT
The default keymap which is placed in the program memory and is used until the custom keymap is loaded and available in the EEPROM.
