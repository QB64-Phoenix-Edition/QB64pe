# QB64 Phoenix Edition Licensing information

QB64-PE makes extensive use of third party libraries to provide
functionality. These third party libraries have their own licenses that you
must respect when distributing any programs compiled by QB64-PE.

As a general note, almost all third party libraries used by QB64-PE are either MIT,
Public Domain, or some other permissive license. Meeting their requirements can
be done by simply distributing the licenses in the `./licenses` folder with
your compiled program.

A few of the libraries are LGPL and require more careful handling to meet their
license requirements (either by providing source code or object files before
linking). Those are noted on this page and avoidable. Note that QB64-PE does
not give the option of using dynamic linking, all third party libraries are
statically linked.

Additionally, QB64-PE contains logic to avoid compiling in third party libraries
if they are not used by the program, those situations are noted on this page.
If a component is not compiled into your program then you do not need to meet
its license requirements.

## QB64 Phoenix Edition Runtime

This is the licensing of the provided QB64-PE runtime that compiled programs make use of.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| libqb | MIT | license_qb64.txt | internal/c/libqb.cpp, internal/c/libqb/, internal/c/qbx.cpp |

## Windows C and C++ Runtime

On Windows MinGW-w64 is used to compiled the C++ code produced by QB64-PE, and some runtime components are compiled into your code. On Linux and Mac OS this section does not apply.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| MinGW-w64 C and C++ runtime | Various Permissive Licenses | license_mingw-base-runtime.txt | internal/c/c_compiler/ |
| libstdc++ | GPLv3 with Exception | license_libstdc++.txt | internal/c/c_compiler/ |

## Display Support

This is always used unless you use `$CONSOLE:ONLY`. On Mac OS the system's own GLUT implementation is used rather than `FreeGLUT`.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| FreeGLUT | MIT | license_freeglut.txt | internal/c/parts/core/ |

## Image Support

These libraries are pulled in if `_LOADIMAGE()` or `_SAVEIMAGE()` functionality is used.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| stb_image | MIT/Public Domain | license_stb_image.txt | internal/c/parts/video/image/stb/stb_image.h |
| nanosvg | zlib | license_nanosvg.txt | internal/c/parts/video/image/nanosvg |
| dr_pcx | Unlicense/Public Domain | license_dr_pcx.txt | internal/c/parts/video/image/dr_pcx.h |
| QOI | MIT | license_qoi.txt | internal/c/parts/video/image/qoi.h |
| stb_image_write | MIT/Public Domain | license_stb_image_write.txt | internal/c/parts/video/image/stb/stb_image_write.h |
| HQx | Apache License v2 | license_hqx.txt | internal/c/parts/video/image/pixelscalers/hqx.hpp |
| MMPX | MIT | license_mmpx.txt | internal/c/parts/video/image/pixelscalers/mmpx.hpp |
| Super-xBR | MIT | license_hqx.txt | internal/c/parts/video/image/pixelscalers/sxbr.hpp |

## Font Support

These libraries are pulled in if `_LOADFONT()` functionality is used.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| FreeTYPE | FLT | license_freetype_ftl.txt | internal/c/parts/video/font/tff/ |

## Compression Support

These libraries are pulled in if `_INFLATE$()` or `_DEFLATE$()` are used.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| miniz | MIT | license_miniz.txt | internal/c/parts/data/ |

## Encoding Support

These libraries are pulled in if `_BASE64ENCODE$()` or `_BASE64DECODE$()` are used.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| MODP_B64 | MIT | license_modp_b64.txt | internal/c/parts/data/ |

## HTTP Support

These libraries are pulled in if `_OPENCLIENT()` is used:

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| libcurl | curl | license_libcurl.txt | internal/c/parts/network/http/curl/ |

## Sound Support

These libraries are pulled in when using any sound-related functionality.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| miniaudio | MIT/Public Domain | license_miniaudio.txt | internal/c/parts/audio/miniaudio.h |
| foo_midi | MIT | license_foo_midi.txt | internal/c/parts/audio/extras/foo_midi/ & internal/c/parts/audio/extras/libmidi/ |
| HivelyTracker | BSD 3-Clause | license_hivelytracker.txt | internal/c/parts/audio/extras/hivelytracker/ |
| libxmp-lite  | MIT | license_libxmp-lite.txt | internal/c/parts/audio/extras/libxmp-lite/ |
| primesynth | MIT | license_primesynth.txt | internal/c/parts/audio/extras/primesynth/ |
| QOA | MIT | license_qoa.txt | internal/c/parts/audio/extras/qoa.h |
| RADv2 | Public Domain | license_radv2.txt | internal/c/parts/audio/extras/radv2/ |
| stb_vorbis | MIT/Public Domain | license_stb_vorbis.txt | internal/c/parts/audio/extras/stb_vorbis.c |
| TinySoundFont | MIT | license_tinysoundfont.txt | internal/c/parts/audio/extras/tinysoundfont/ |
| ymfmidi | BSD-3-Clause | license_ymfmidi.txt | internal/c/parts/audio/extras/ymfmidi/ |

## Game Controller Support

This is used if you make use of game controller related functionality.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| libstem Gamepad | MIT | license_libstem_gamepad.txt | internal/c/parts/input/game_controller/libstem_gamepad |

## Common Dialogs Support

This is used by libqb to show alerts and also by the common dialog functions and subroutines.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| tiny file dialogs | ZLIB | license_tinyfiledialogs.txt | internal/c/parts/gui/ |

## Clipboard Image Support

This is used if you make use of the `_CLIPBOARDIMAGE` function or statement.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| Clip Library | MIT | license_clip.txt | internal/c/parts/os/clipboard/clip/ |
