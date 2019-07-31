# vitabright

Vitabright is a plugin enabling you to alter the luminosity levels of your PS Vita. It thus allows you to decrease the brightness level below the minimum, and increase it above the maximum.
**This plugin is compatible with both OLED (PS Vita 1000) and LCD (PS Vita 2000) models. Tested to be functional on 3.60 Ensō, 3.65 h-encore, 3.67 h-encore and 3.68 h-encore**.

**Important note**: While most OLED screens work with vitabright, **some OLED screens** (PS Vita 1000) don't work with it. It seems that their gamma table isn't overridden properly. See issue [#13](https://github.com/devnoname120/vitabright/issues/13) for more information. 

## Installation

- Download the latest version from the [Releases section](https://github.com/devnoname120/vitabright/releases).
- Extract the files to `ur0:/tai/` on your Vita.
- Add `ur0:/tai/vitabright.skprx` below `*KERNEL` in the `ur0:/tai/config.txt` file.

**Important note**: If you have a folder `ux0:/tai` on your memory stick, it will prevent plugins in `ur0:/tai` from loading. In order to fix this, delete the folder `ux0:/tai`.

## Usage

- Once vitabright is installed and your Vita is restarted, it will automatically work. Just open the brightness settings and move the slider.
- You can customize the gamma table that vitabright uses for OLED screens (PS Vita 1000): open the file `vitabright_lut.txt` and modify it according to your needs. **Note**: for now, you cannot tweak the luminosity levels of LCD screens (PS Vita 2000) without recompiling the plugin (for advanced users).
- Note that while there is also a file named `vitabright_lut_orig.txt`, it's not used by vitabright and it's only here for informational purposes.

**Important note**: If you want to use FileZilla for transferring the file `vitabright_lut_orig.txt`, you'll first need to [change the transfer type from `Auto` to `Binary`](https://stackoverflow.com/a/555003) or it can prevent vitabright from parsing it correctly.

## OLED gamma table explanations

https://github.com/devnoname120/vitabright/wiki/What-is-the-format-of-the-OLED-gamma-table%3F

----------------------
Thanks to xyz, yifanlu, and xerpi for their help.
