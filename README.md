Support development of enkiTS through [Github Sponsors](https://github.com/sponsors/dougbinks) or [Patreon](https://www.patreon.com/enkisoftware)

[<img src="https://img.shields.io/static/v1?logo=github&label=Github&message=Sponsor&color=#ea4aaa" width="200"/>](https://github.com/sponsors/dougbinks)    [<img src="https://c5.patreon.com/external/logo/become_a_patron_button@2x.png" alt="Become a Patron" width="150"/>](https://www.patreon.com/enkisoftware)

![enkiTS Logo](https://github.com/dougbinks/images/blob/master/enkiTS_logo_no_padding.png?raw=true)
# [enkiTS](https://github.com/dougbinks/enkiTS/) Examples

**Submodules are licensed under their own licenses, see their contents for details**

## Building

First [make sure you've cloned all submodules](https://git-scm.com/book/en/v2/Git-Tools-Submodules#Cloning-a-Project-with-Submodules). On Windows / Mac OS X / Linux with cmake installed, open a prompt in the enkiTS directory and:

1. `mkdir build`
2. `cmake ..`
3. either run `make` or on Windows with Visual Studio open `enkiTSExamples.sln`

## enki Task Scheduler Extended Samples

[enkiTS](https://github.com/dougbinks/enkiTS/) is a permissively licensed C and C++ Task Scheduler for creating parallel programs.

This repository hosts extended examples.

## [enkiTSRemoteryExample.cpp](enkiTSRemoteryExample.cpp)

This example shows how to use [Remotery](https://github.com/Celtoys/Remotery) with [enkiTS](https://github.com/dougbinks/enkiTS/).

Note that currently in release the sums might be optimized away.

![Remotery Screenshot](images/enkiTSRemoteryExample.png?raw=true)

## [enkiTSRemoteryExample.c](enkiTSRemoteryExample.c)

As above but using the C interface.

## [enkiTSMicroprofileExample.cpp](enkiTSMicroprofileExample.cpp)

This example shows how to use [microprofile](https://github.com/dougbinks/microprofile) with [enkiTS](https://github.com/dougbinks/enkiTS/).

To view context switching on Windows, run the application (or Visual Studio if launching from that) as administrator and set Options->Cswitch Trace->Enable on.

![Microprofile Screenshot](images/enkiTSMicroprofileExample.png?raw=true)

## License (zlib)

Copyright (c) 2013 Doug Binks

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgement in the product documentation would be
   appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.




