# QB64 Phoenix Edition

![QB64-PE](source/peLogo.png)


QB64 is a modern extended BASIC+OpenGL language that retains QB4.5/QBasic compatibility and compiles native binaries for Windows (Vista and up), Linux and macOS.

The Phoenix Edition is one of the new offshoots created when the old project related pages (QB64Team/www.qb64.org) went offline, but it's still the same programming language. For the whole story visit our new [Forum](https://qb64phoenix.com/forum/showthread.php?tid=259).

# Table of Contents
1. [Installation](#Installation)
    1. [Windows](#Windows)
    2. [macOS](#macOS)
    3. [Linux](#Linux)

2.  [Usage](#Usage)
3.  [Additional Info](#Additional_Info)

# Installation <a name="Installation"></a>
Download the appropriate package for your operating system over at https://github.com/QB64-Phoenix-Edition/QB64pe/releases/latest

<a name="Windows"></a>
## Windows

Make sure to extract the package contents to a folder with full write permissions (failing to do so may result in IDE or compilation errors).

* It is advisable to whitelist the 'qb64pe' folder in your antivirus/antimalware software *

<a name="macOS"></a>
## macOS
Before using QB64-PE make sure to install the Xcode command line tools with:
```bash
xcode-select --install
```

Run ```./setup_osx.command``` to compile QB64-PE for your OS version.

<a name="Linux"></a>
## Linux
Compile QB64-PE with ```./setup_lnx.sh```.

Dependencies should be automatically installed. Required packages include OpenGL, ALSA and the GNU C++ Compiler.

<a name="Usage"></a>
# Usage
Run the ```qb64pe``` executable to launch the IDE, which you can use to edit your .BAS files. From there, hit F5 to compile and run your code.

To generate a binary without running it, hit F11.

Additionally, if you do not wish to use the integrated IDE and to only compile your program, you can use the following command-line calls:

```qb64pe -c yourfile.bas```

```qb64pe -c yourfile.bas -o outputname.exe```

Replacing `-c` with `-x` will compile without opening a separate compiler window.


<a name="Additional_Info"></a>
# Additional Information
More about QB64-PE at our wiki: https://qb64phoenix.com/qb64wiki

We have a community forum at: https://qb64phoenix.com/forum

We don't currently tweet.  Sorry.

Find us on Discord: https://discord.gg/D2M7hepTSx

Join us on Reddit: https://www.reddit.com/r/QB64pe/
