cbtt
====

CBTT Github Repository

Currently Tested Platforms (Direct by DreadWingKnight):
* Linux x86 and x86-64
* Linux ARM (Raspberry Pi Raspbian)
* Windows 32/64 Bit. (64-bit runs in 32-bit address space, so be careful if you see it starting to use a lot of ram. You WERE warned.)

Other platforms reported to work
* *BSD including openbsd and freebsd
* MacOSX

Build instructions:
* Have zlib installed
* Have pthreads installed
* (Optional) Have the MySQL Development libraries and headers installed
* run "make bnbt" or "make bnbtmysql" depending on if you need MySQL support or not (gmake on *BSD)

Optional make functions
* C++=<C++/G++ compiler>
** This is useful if you have a multi-platform distcc compile cluster.
** For example, you can use this to specify "distcc <host compiler>" and only builders with a compatible gcc toolchain installed will attempt to build (so that you use x86_64-pc-linux-gnu only for your 64-bit systems or arm-linux-gnueabihf for your Raspberry Pi/Banana Pi/Hummingboard/etc)