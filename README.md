# [amlogic-usbdl](https://github.com/frederic/amlogic-usbdl) : unsigned code loader for Amlogic bootrom

## Disclaimer
You will be solely responsible for any damage caused to your hardware/software/warranty/data/cat/etc...

## Description
Amlogic bootrom supports booting from USB. This method of boot requires an USB host to send a signed bootloader to the bootrom via USB port.

This tool exploits a vulnerability in the USB download mode to load and run unsigned code in Secure World.

## Supported targets
* Khadas VIM3L (S905D3)
* Chromecast with Google TV (S905D3G)

## Usage
```
./amlogic-usbdl <input_file>
	input_file: payload binary to load and execute
```

## Payloads
Payloads are raw binary AArch64 executables. Some are provided in directory **payloads/**.

## License
Please see [LICENSE](/LICENSE).
