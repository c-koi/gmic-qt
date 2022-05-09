
# G'MIC-Qt: a versatile G'MIC plugin

### Purpose

 G'MIC-Qt is a versatile front-end to the image processing framework
 [G'MIC](https://gmic.eu).  It is in fact a plugin for
 [GIMP](http://gimp.org), [Krita](https://krita.org), [Paint.NET](https://www.getpaint.net/),
 [digiKam](https://www.digikam.org) and an 8bf filter plugin for Photoshop-compatible software as well as a [standalone application](STANDALONE.md).

### Authors

  * Sébastien Fourey
  * David Tschumperlé (G'MIC lib & original GTK-based plugin)

### Contributors

 * Boudewijn Rempt <boud@valdyas.org> (Krita compatibility layer, later replaced by a native version of the plugin)
 * [amyspark](https://github.com/amyspark) (Krita native version of the plugin, work in progress)
 * [Nicholas Hayes](https://github.com/0xC0000054) (Paint.NET and 8bf filter compatibility layers, work in progress)
 * [Gilles Caulier](https://github.com/cgilles) (digiKam compatibility layer)


### Translators

 * Jan Helebrant (Czech translation)
 * Frank Tegtmeyer (German translation)
 * chroma_ghost & bazza/pixls.us (Spanish translation)
 * Sébastien Fourey (French translation)
 * Duddy Hadiwido (Indonesian translation)
 * Francesco Riosa (Italian translation)
 * iarga / pixls.us (Dutch translation)
 * Alex Mozheiko (Polish translation)
 * maxr (Portuguese translation)
 * Alex Mozheiko (Russian translation)
 * Andrex Starodubtsev (Ukrainian translation)
 * LinuxToy (https://twitter.com/linuxtoy) (Chinese translation)
 * omiya tou tokyogeometry@github (Japanese translation)

### Official (pre-release) binary packages

 * Available at [gmic.eu](https://gmic.eu)

### Travis CI last build status

 * Master branch (Linux) [![Build Status](https://api.travis-ci.org/c-koi/gmic-qt.svg?branch=master)](https://travis-ci.org/c-koi/gmic-qt)
 * Devel branch (Linux) [![Build Status](https://api.travis-ci.org/c-koi/gmic-qt.svg?branch=devel)](https://travis-ci.org/c-koi/gmic-qt)

### Build instructions

By default, the gimp integration plugin is built.

#### QMake

qmake is simple to use but only really works in an environment where bash is available.

```sh
git clone https://github.com/dtschump/gmic.git
git clone https://github.com/c-koi/gmic-qt.git
make -C gmic/src CImg.h gmic_stdlib_community.h
cd gmic-qt
qmake [HOST=none|gimp|krita|paintdotnet|8bf]
make
```

#### CMake

cmake works on all platforms. The first part is the same and requires make and wget to be available. If you don't have all dependencies, cmake will warn you which ones are missing. Note that the minimum cmake version is 3.1.

```sh
git clone https://github.com/dtschump/gmic.git
git clone https://github.com/c-koi/gmic-qt.git
make -C gmic/src CImg.h gmic_stdlib_community.h
cd gmic-qt
```

Then make a build directory:

```sh
mkdir build
cd build
```

```sh
cmake .. [-DGMIC_QT_HOST=none|gimp|krita|paintdotnet|8bf] [-DGMIC_PATH=/path/to/gmic] [-DCMAKE_BUILD_TYPE=[Debug|Release|RelwithDebInfo]
make
```

### Adapt G'MIC-Qt to new applications

Developers will find guidelines and instructions for the adaptation of the plugin to a new host application in the [NEW HOST HOWTO](https://github.com/c-koi/gmic-qt/blob/master/NEW_HOST_HOWTO.md).
