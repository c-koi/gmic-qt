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

#include <QApplication>
#include <QEventLoop>
#include <QPointer>
#include <QSettings>
#include <QTranslator>

#include "HeadlessProcessor.h"
#include "Logger.h"
#include "Widgets/ProgressInfoWindow.h"
#include "gmicqttoolplugin.h"
#include "DialogSettings.h"
#include "MainWindow.h"
#include "Widgets/LanguageSelectionWidget.h"
#include "gmic_qt.h"

#include "kpluginfactory.h"

K_PLUGIN_FACTORY_WITH_JSON(KritaGmicPluginFactory, "gmicqttoolplugin.json", registerPlugin<KritaGmicPlugin>();)

KritaGmicPlugin::KritaGmicPlugin(QObject *parent, const QVariantList &)
    : QObject(parent) {}

int KritaGmicPlugin::launch(std::shared_ptr<KisImageInterface> i,
                            bool headless) {
  disableInputMode(GmicQt::NoInput);
  // disableInputMode(GmicQt::Active);
  // disableInputMode(GmicQt::All);
  // disableInputMode(GmicQt::ActiveAndBelow);
  // disableInputMode(GmicQt::ActiveAndAbove);
  disableInputMode(GmicQt::AllVisible);
  disableInputMode(GmicQt::AllInvisible);

  // disableOutputMode(GmicQt::InPlace);
  disableOutputMode(GmicQt::NewImage);
  disableOutputMode(GmicQt::NewLayers);
  disableOutputMode(GmicQt::NewActiveLayers);

  int r = 0;
  iface = i;
  if (headless) {
    DialogSettings::loadSettings(GmicQt::GuiApplication);
    Logger::setMode(DialogSettings::outputMessageMode());
    // Translate according to current locale or configured language
    QString lang = LanguageSelectionWidget::configuredTranslator();
    if (!lang.isEmpty() && (lang != "en")) {
      auto translator = new QTranslator(qApp);
      translator->load(QString(":/translations/%1.qm").arg(lang));
      QCoreApplication::installTranslator(translator);
    }

    HeadlessProcessor processor;
    QPointer<ProgressInfoWindow> progressWindow = new ProgressInfoWindow(&processor);
    if (processor.command().isEmpty()) {
      return r;
    }

    processor.startProcessing();

    QEventLoop loop;
    connect(progressWindow, SIGNAL(destroyed()), &loop, SLOT(quit()));
    r = loop.exec();

  } else {
    DialogSettings::loadSettings(GmicQt::GuiApplication);

    // Translate according to current locale or configured language
    QString lang = LanguageSelectionWidget::configuredTranslator();

    if (!lang.isEmpty() && (lang != "en")) {
      auto translator = new QTranslator(qApp);
      translator->load(QString(":/translations/%1.qm").arg(lang));
      QApplication::installTranslator(translator);
    }

    disableInputMode(GmicQt::NoInput);
    // disableInputMode(GmicQt::Active);
    disableInputMode(GmicQt::All);
    disableInputMode(GmicQt::ActiveAndBelow);
    disableInputMode(GmicQt::ActiveAndAbove);
    disableInputMode(GmicQt::AllVisible);
    disableInputMode(GmicQt::AllInvisible);

    // disableOutputMode(GmicQt::InPlace);
    disableOutputMode(GmicQt::NewImage);
    disableOutputMode(GmicQt::NewLayers);
    disableOutputMode(GmicQt::NewActiveLayers);

    QPointer<MainWindow> mainWindow = new MainWindow(0);
    // We want a non modal dialog here.
    mainWindow->setWindowFlags(Qt::Tool | Qt::Dialog);
    mainWindow->setWindowModality(Qt::ApplicationModal);
    mainWindow->setAttribute(Qt::WA_DeleteOnClose); // execution loop never finishes otherwise?

    if (QSettings().value("Config/MainWindowMaximized", false).toBool()) {
      mainWindow->showMaximized();
    }
    else {
      mainWindow->show();
    }

    // Wait than main widget is closed.
    QEventLoop loop;
    connect(mainWindow, SIGNAL(destroyed()), &loop, SLOT(quit()));
    r = loop.exec();
  }

  sharedMemorySegments.clear();
  iface.reset();

  return r;
}

#include "gmicqttoolplugin.moc"
