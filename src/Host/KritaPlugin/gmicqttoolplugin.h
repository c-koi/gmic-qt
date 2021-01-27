/*
 *  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
 *  editors, offering hundreds of filters thanks to the underlying G'MIC
 *  image processing framework.
 *
 *  Copyright (C) 2020 L. E. Segovia <amy@amyspark.me>
 *
 *  Description: Krita painting suite plugin for G'Mic-Qt.
 *
 *  G'MIC-Qt is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  G'MIC-Qt is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef GMICQT_TOOL_PLUGIN_H
#define GMICQT_TOOL_PLUGIN_H

#include <QObject>
#include <QSharedMemory>
#include <QtPlugin>
#include <QVector>
#include <memory>

#include "kis_qmic_interface.h"
#include "kis_qmic_plugin_interface.h"

extern QVector<KisQMicImageSP> sharedMemorySegments;
extern std::shared_ptr<KisImageInterface> iface;

class KritaGmicPlugin : public QObject, public KisQmicPluginInterface {
  Q_OBJECT
  Q_INTERFACES(KisQmicPluginInterface)

public:
  KritaGmicPlugin(QObject *parent, const QVariantList &);

  int launch(std::shared_ptr<KisImageInterface> iface,
                 bool headless = false) override;
};

#endif
