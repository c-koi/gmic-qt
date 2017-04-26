
# G'MIC-Qt: a versatile G'MIC plugin

### Purpose

 G'MIC-Qt is a versatile front-end to the image processing framework
 [G'MIC](http://gmic.eu).  It is in fact a plugin for
 [GIMP](http://gimp.org), as well as a standalone application.

### Authors

  * Sébastien Fourey
  * David Tschumperlé (G'MIC lib & original GTK-based plugin)

### Contributor

 * Boudewijn Rempt <boud@valdyas.org> (Krita compatibility layer, work in progress)

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

### Official (pre-release) binary packages

 * Available at [gmic.eu](http://gmic.eu)

### Build instructions

```sh
git clone https://github.com/dtschump/gmic.git
git clone https://github.com/c-koi/gmic-qt.git
make -C gmic/src CImg.h gmic_stdlib.h
cd gmic-qt
qmake [HOST=none|gimp]
make
```
