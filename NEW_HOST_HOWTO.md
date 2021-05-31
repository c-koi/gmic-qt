# Contributing - New host HOWTO

This document describes the architecture of the plugin (especially its link with the host application) and provides guidelines for its adaptation to a new host application.

### Architecture of the plug-in

The plugin is intended to be a standalone program which communicates with the host application (typically digital painting or photo retouching software) to exchange image data and commands. This architecture, as opposed to a software component fully integrated in the host application process, has been designed like this for several reasons, among which :

* The plugin cannot mess up with the host application process which means in the worst case, make it crash. This would be a responsibility we don't want to endorse. If unfortunately the plugin crashes, the host application does not, and no user data are lost. As long as the host application handles properly its communication with the plugin, nothing bad should ever happen because of a plugin misbehavior.
* Being based on Qt, the plugin GUI inherits the desktop environment theme but also handles its own custom "dark theme" using style sheets in an application-wide way. Consequently, a Qt-based host application could inherit from these modifications with unexpected results (for instance, messing up the whole theme of the host GUI).
* This plugin architecture has allowed us to define our own and quite simple API made of a few functions (exactly 6) that need to be implemented for the plugin to be able to communicate with the host application. Namely, this API is the interface (in the software engineering sense) of the communication layer between the host and the plugin, from the viewpoint of the latter. Thus, any suitable implementation of this interface should be enough to make the plugin work with a given host. Note that only 4% of the whole "G'MIC-Qt for Gimp" source code is host-dependent. The next section gives more details about the work that needs to be done to have a functional plugin for a new host.
* With a plugin being a self-contained application whose host-related code is clearly identified, we can more easily build the plugin and quickly distribute new versions, thus following the rapid development cycle of G'MIC.

### New host HOWTO: What needs to be done

Consequently, in order to adapt the plug-in to a new host application cleanly, a few things need to be done:

* Provide the host application with a communication system targeted to an external application, if it does not exist yet, so that it supports passing commands and possibly large image data (preferably through memory for improved performances).
* Write a program:
  * Which is linked to an implementation of all functions declared and documented in the API header [`Host/GmicQtHost.h`](src/Host/GmicQtHost.h) (implementation usually written, at least in part, in a file named `host_HOSTNAME.cpp`). This is the only place where all the communication between the plugin and the host should occur. The plugin relies only on this interface to be implemented.
  * Which is linked to the host-agnostic code of the plugin found in this repository. (See [`GmicQt.h`](src/GmicQt.h) for more details about the services offered by this part of the plug-in's code.)
 ![Architecture](architecture.svg)
  * Which calls the `GmicQt::run()` function provided by the G'MIC-Qt code, once initialisations are done and the communication with the host is established.
  * It should be noticed that tweaking the API internals to adapt the plugin to a new host is definitely not good practice. It may break the compatibility with the plugin's future versions.
* Adapt the [qmake](gmic_qt.pro) or [CMake](CMakeLists.txt) project files and follow the [build instructions](README.md#build-instructions) from the README.

In all these steps, valuable hints may be obtained from the [implementation for the GIMP host](src/Host/Gimp), found in the [src/Host](src/Host) folder, which is where all of a given host related code should go (for the plugin side).

If you succeed in creating such a file for a new host application, you are welcome to open a [pull request.](https://github.com/c-koi/gmic-qt/pulls)

### Guidelines

First, we would like to point out that any contribution in the form of a new host adaptation is very (very) welcome.

However, we have decided that contributions will be now accepted in this (upstream) repository only if they respect the architecture described above. It ensures that the plugin executables can be built for these different hosts and that they can be easily updated to the latest G'MIC version.

Note that we do not deny the fact that some other "tweaked" versions of the G'MIC-Qt plugin (that do not follow the rules defined above) may work for other host applications. Simply, we cannot guarantee their proper functioning. We will not endorse the responsibility of any crash or data loss that may occur with such implementations. We will probably not help in updating these plugins with the newest G'MIC features either. We let the authors of these alternative implementations deal with these issues.

Anyway, we will be happy to share links to these alternative implementations when we become aware of them, [here](https://github.com/c-koi/gmic-qt/blob/master/README.md) and on the official [G'MIC website](https://gmic.eu).
