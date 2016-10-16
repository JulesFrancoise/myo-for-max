# myo for max

**myo** for [Max](https://cycling74.com/products/max/) is a max external for communication with the [Myo](http://myo.com/) armband.

This object has been developed by [Jules Françoise](http://julesfrancoise.com/) at Simon Fraser University, in the context of the [MovingStories](http://movingstories.ca/) research partnership, funded by the Social Sciences and Humanities Research Council ([SSHRC](http://www.sshrc-crsh.gc.ca/)).

**Compatibility:** Mac OS X  10.8+, Max 6.1+, **64-bit only**.

It is based on the Myo c++ API by Thalmic Labs.
This object should be compiled with Max SDK version 6 or greater.

### Usage

*myo for max* is only compatible with MacOS 10.8+, with Max running in 64-bit mode. To switch Max to 64-bit:

1. In the finder, finder "Max.app" in the applications folder
2. Right click and "Get Info"
3. Uncheck "Open in 32-bit mode"
4. restart Max

### Author

This code has been initially authored by <a href="http://julesfrancoise.com">Jules Françoise</a> during his Postdoctoral Fellowship at Simon Fraser University, in the context of the [MovingStories](http://movingstories.ca/) research partnership, under the supervision of Thecla Schiphorst.

**Contact**: Jules Françoise <jfrancoi@sfu.ca>

### Copyright

Copyright (C) 2015 Jules Françoise.

### Licence

This project is released under the [Mozilla Public License 2.0](https://www.mozilla.org/en-US/MPL/2.0/) (MPL-2.0).

This Source Code Form is subject to the terms of the Mozilla Public
License, v. 2.0. If a copy of the MPL was not distributed with this
file, You can obtain one at http://mozilla.org/MPL/2.0/.

Covered Software is provided under this License on an "as is" basis, without warranty of any kind, either expressed, implied, or statutory, including, without limitation, warranties that the Covered Software is free of defects, merchantable, fit for a particular purpose or non-infringing. The entire risk as to the quality and performance of the Covered Software is with You. Should any Covered Software prove defective in any respect, You (not any Contributor) assume the cost of any necessary servicing, repair, or correction. This disclaimer of warranty constitutes an essential part of this License. No use of any Covered Software is authorized under this License except under this disclaimer.

## Compiling the external

### XCode

See the xcode project in "ide/xcode/".
You need to have
* Cycling'74 Max SDK (> 6.1)
* Myo SDK (placed in the root directory)
Then, change paths in the project setting to include the right frameworks and headers.
