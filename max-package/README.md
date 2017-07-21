# myo for max

**myo** for [Max](https://cycling74.com/products/max/) is a max external for communication with the [Myo](http://myo.com/) armband.

This object has been developed by [Jules Françoise](http://julesfrancoise.com/) and Yves Candau at Simon Fraser University, in the context of the [MovingStories](http://movingstories.ca/) research partnership, funded by the Social Sciences and Humanities Research Council ([SSHRC](http://www.sshrc-crsh.gc.ca/)).

# Usage

### General

Install the myo package by copying the [max-package/](./) folder to one of the two possible Max package folders:

* `Max 7/Packages` in your Documents (Mac) or My Documents (Windows) folder, or
* `Max 7/Packages` in your `/Users/Shared` (Mac) or `C:\ProgramData` (Windows) folder

More information on Max packages is available [here](https://docs.cycling74.com/max7/vignettes/packages).

### Mac

*myo for max* is only compatible with MacOS 10.8+, with Max running in 64-bit mode. To switch Max to 64-bit:

1. In the finder, finder "Max.app" in the applications folder
2. Right click and "Get Info"
3. Uncheck "Open in 32-bit mode"
4. restart Max

### Windows

Max needs dll files to load *myo for max*. You will otherwise get the following error message:

`Error 126 loading external myo`

Install the package as previously indicated to make the dll files in the package available to Max.
