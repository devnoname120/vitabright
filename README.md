# vitabright

Vitabright is a plugin enabling you to alter the luminosity levels of your PS Vita. It thus allows you to decrease the brightness level below the minimum, and increase it above the maximum.
**This plugin is compatible with both OLED (PS Vita 1000) and LCD (PS Vita 2000) models. Tested to be functional on 3.60 Ensō, 3.65 h-encore, 3.67 h-encore and 3.68 h-encore**.

## Installation

- Download the latest version from the [Releases section](https://github.com/skgleba/vitabright/releases) to `ur0:/tai/`.
- Add `ur0:/tai/vitabright.skprx` below `*KERNEL` in the `ur0:/tai/config.txt` file.

**Important note**: If you have a folder `ux0:/tai` on your memory stick, it will prevent plugins in `ur0:/tai` from loading. In order to fix this, delete the folder `ux0:/tai`.

## Usage

- Once vitabright is installed and your Vita is restarted, it will automatically work. Just open the brightness settings and move the slider.
- You can edit the OLED gamma table with the lut editor (vpk in [releases](https://github.com/skgleba/vitabright/releases))

![](https://user-images.githubusercontent.com/2824100/102720568-760e4700-42f5-11eb-9ba2-f2cea8ce6cd6.png)

**Please share your improved `vitabright_lut.txt` file by [opening an issue](https://github.com/devnoname120/vitabright/issues/new)!**

## OLED gamma table explanations

https://github.com/devnoname120/vitabright/wiki/What-is-the-format-of-the-OLED-gamma-table%3F

----------------------
Thanks to xyz, yifanlu, and xerpi for their help.
