# myo for max

**myo** for [Max](https://cycling74.com/products/max/) is a max external for communication with the [Myo](http://myo.com/) armband.

This object has been developed by [Jules Françoise](http://julesfrancoise.com/) and Yves Candau at Simon Fraser University, in the context of the [MovingStories](http://movingstories.ca/) research partnership, funded by the Social Sciences and Humanities Research Council ([SSHRC](http://www.sshrc-crsh.gc.ca/)).

# Usage

### Mac

*myo for max* is only compatible with MacOS 10.8+, with Max running in 64-bit mode. To switch Max to 64-bit:

1. In the finder, finder "Max.app" in the applications folder
2. Right click and "Get Info"
3. Uncheck "Open in 32-bit mode"
4. restart Max

### Windows

Max needs dll files to load *myo for max*. You will otherwise get the following error message:

`Error 126 loading external myo`

1. Copy "lib/win/dll/myo32.dll" into the 32-bit Max support directory, most likely:

`C:\Program Files (x86)\Cycling '74\Max 7\resources\support`

2. Copy "lib/win/dll/myo64.dll" into the 64-bit Max support directory, most likely:

`C:\Program Files\Cycling '74\Max 7\resources\support`
