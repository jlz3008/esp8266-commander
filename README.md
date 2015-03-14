# esp8266-commander
Base project to console commands

## Purpose

The purpose of this project is to server as a basis of other projects providing a smart console
similar to uboot bootloader where execute commands for ESP8266. Also, this project provide a environment
where store key-values pair and functions to get, write, and store those pairs on non volatile memory.

The commands are specific of the/you *derivated* project.

## IDE

Files project to QtCreator is provided too, but you can not use if you want. To compile the project
you need Xtensa SDK.

## Stolen code

I stolen and adapted code from nekromant/esp8266-frankenstein , Helius/microrl , espressif/esp_iot_rtos_sdk .
Makefile come from esp8266/source-code-examples and autostolen serial driver from jlz3008/esp8266-console

Thanks to everyone.
