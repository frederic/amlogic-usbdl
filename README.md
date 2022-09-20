# [amlogic-usbdl](https://github.com/frederic/amlogic-usbdl) : unsigned code loader for Amlogic bootrom

## Disclaimer
You will be solely responsible for any damage caused to your hardware/software/warranty/data/cat/etc...

## Description
Amlogic bootrom supports booting from USB. This method of boot requires an USB host to send a signed bootloader to the bootrom via USB port.

This tool exploits a [vulnerability](https://fredericb.info/2021/02/amlogic-usbdl-unsigned-code-loader-for-amlogic-bootrom.html) in the USB download mode to load and run unsigned code in Secure World.

## Supported targets
* s905d3 : Khadas VIM3L, Chromecast with Google TV
* s905d2 : Spotify Car Thing

## Usage
```shell
$ ./amlogic-usbdl <target_name> <input_file> [<output_file>]
	target_name: s905d3 s905d2
	input_file: payload binary to load and execute (max size 65280 bytes)
	output_file: file to write data returned by payload
```

## Payloads
Payloads are raw binary AArch64 executables. Some are provided in directory **payloads/**.

## License
Please see [LICENSE](/LICENSE).
