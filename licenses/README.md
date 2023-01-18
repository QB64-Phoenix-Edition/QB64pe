QB64 Phoenix Edition Licensing information
==========================================

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
| MinGW-w64 C and C++ runtime | Various Permissive Licenses | license_mingw-base-runtime.txt | internal/c/c_compiler |
| libstdc++ | GPLv3 with Exception | license_libstdc++.txt | internal/c/c_compiler |

## Display Support

This is always used unless you use `$CONSOLE:ONLY`. On Mac OS the system's own GLUT implementation is used rather than `FreeGLUT`.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| FreeGLUT | MIT | license_freeglut.txt | internal/c/parts/core |

## Image Support

These libraries are pulled in if `_LOADIMAGE()` functionality is used.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| PCX image loader | Unlicense/Public Domain | license_dr_pcx.txt | internal/c/parts/video/image/dr_pcx.h |
| stb_image | MIT | license_stb_image.txt | internal/c/parts/video/image/stb_image.h |

## Font Support

These libraries are pulled in if `_LOADFONT()` functionality is used.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| FreeTYPE | FLT | license_freetype_ftl.txt | internal/c/parts/video/font/tff/ |

## Compression Support

These libraries are pulled in if `_INFLATE$()` or `_DEFLATE$()` are used.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| ZLib | ZLIB | license_zlib.txt | internal/c/c_compiler (provide by MinGW on Windows, system library otherwise) |

## Http Support

These libraries are pulled in if `_OPENCLIENT()` and `$Unstable:Http` areused:

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| libcurl | curl | license_libcurl.txt | internal/c/parts/network/http/curl/ |

## Sound Support

These libraries are pulled in when using any sound-related functionality.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| Miniaudio | MIT | license_miniaudio.txt | internal/c/parts/audio/miniaudio.h |
| libxmp-lite  | MIT | license_libxmp-lite.txt | internal/c/parts/audio/extras/libxmp-lite/ |
| RADv2 | Public Domain | license_radv2.txt | internal/c/parts/audio/extras/radv2/ |
| std_vorbis | Public Domain | license_stdvorbis.txt | internal/c/parts/audio/extras/std_vorbis.c |
| HivelyTracker | BSD 3-Clause | license_hivelytracker.txt | internal/c/parts/audio/extras/hivelytracker |

## MIDI Support

These are used if you make use of MIDI support.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| TinySoundFont | MIT | license_tinysoundfont.txt | internal/c/parts/audio/extras/tinysoundfont/tsf.h
| TinyMidiLoader | ZLIB | license_tinymidiloader.txt | internal/c/parts/audio/extras/tinysoundfont.tml.h |

## Common Dialogs Support

This is used by libqb to show alerts and also by the common dialog functions and subroutines.

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| tiny file dialogs | ZLIB | license_tinyfiledialogs.txt | internal/c/parts/gui

## Legacy OpenAL audio backend

The below licenses apply when making use of the legacy OpenAL audio backend (can be enabled in `Compiler Settings`). These replace all other sound related libraries:

| Library | License | License file | Location |
| :------ | :-----: | :----------- | :------- |
| mpg123 | LGPL 2.1 | license_mpg123.txt | internal/c/parts/audio/decode/mp3/ |
| OpenAL-soft | LGPL 2 | license_openal.txt | internal/c/parts/audio/out/ |
| Opus Tools | BSD 2-clause | license_opus.txt | internal/c/parts/audio/conversion/ |
| stb_vorbis | Public Domain | license_stdvorbis.txt | internal/c/parts/audio/decode/ogg/ |
