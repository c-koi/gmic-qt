/*
*  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
*  editors, offering hundreds of filters thanks to the underlying G'MIC
*  image processing framework.
*
*  Copyright (C) 2019 Gilles Caulier <caulier dot gilles at gmail dot com>
*
*  Description: digiKam image editor plugin for GmicQt.
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

#include "gmicqttoolplugin.h"

// Qt includes

#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QEventLoop>
#include <QPointer>

// Local includes

#include "MainWindow.h"
#include "LanguageSettings.h"
#include "DialogSettings.h"
#include "gmic_qt.h"

namespace DigikamEditorGmicQtPlugin
{

GmicQtToolPlugin::GmicQtToolPlugin(QObject* const parent)
    : DPluginEditor(parent)
{
}

GmicQtToolPlugin::~GmicQtToolPlugin()
{
}

QString GmicQtToolPlugin::name() const
{
    return QString::fromUtf8("GmicQt");
}

QString GmicQtToolPlugin::iid() const
{
    return QLatin1String(DPLUGIN_IID);
}

QIcon GmicQtToolPlugin::icon() const
{
    return QIcon(":resources/gmic_hat.png");
}

QString GmicQtToolPlugin::description() const
{
    return tr("A tool for G'MIC-Qt");
}

QString GmicQtToolPlugin::details() const
{
    return tr("<p>This Image Editor tool for G'MIC-Qt.</p>"
              "<p>G'MIC-Qt is a versatile front-end to the image processing framework G'MIC</p>"
              "<p>G'MIC is a full-featured open-source framework for image processing. "
              "It provides several user interfaces to convert / manipulate / filter / "
              "visualize generic image datasets, ranging from 1D scalar signals to 3D+t sequences "
              "of multi-spectral volumetric images, hence including 2D color images.</p>"
              "<p>More details: https://gmic.eu/</p>");
}

QList<DPluginAuthor> GmicQtToolPlugin::authors() const
{
    return QList<DPluginAuthor>()
            << DPluginAuthor(QString::fromUtf8("Gilles Caulier"),
                             QString::fromUtf8("caulier dot gilles at gmail dot com"),
                             QString::fromUtf8("(C) 2019"))
            << DPluginAuthor(QString::fromUtf8("Sébastien Fourey"),
                             QString::fromUtf8("Sebastien dot Fourey at ensicaen dot fr"),
                             QString::fromUtf8("(C) 2017-2019"),
                             QString::fromUtf8("G'MIC plugin"))
            << DPluginAuthor(QString::fromUtf8("David Tschumperlé"),
                             QString::fromUtf8("David dot Tschumperle at ensicaen dot fr"),
                             QString::fromUtf8("(C) 2008-2017"),
                             QString::fromUtf8("G'MIC core"))
            ;
}

void GmicQtToolPlugin::setup(QObject* const parent)
{
    DPluginAction* const ac = new DPluginAction(parent);
    ac->setIcon(icon());
    ac->setText(tr("G'MIC-Qt..."));
    ac->setObjectName(QLatin1String("editorwindow_gmicqt"));
    ac->setActionCategory(DPluginAction::EditorEnhance);

    connect(ac, SIGNAL(triggered(bool)),
            this, SLOT(slotGmicQt()));

    addAction(ac);
}

void GmicQtToolPlugin::slotGmicQt()
{
    DialogSettings::loadSettings(GmicQt::GuiApplication);

    // Translate according to current locale or configured language
    QString lang = LanguageSettings::configuredTranslator();

    if (!lang.isEmpty() && (lang != "en"))
    {
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

    if (QSettings().value("Config/MainWindowMaximized", false).toBool())
    {
        mainWindow->showMaximized();
    }
    else
    {
        mainWindow->show();
    }

    // Wait than main widget is closed.
    QEventLoop loop;
    connect(mainWindow, SIGNAL(destroyed()),
            &loop, SLOT(quit()));
    loop.exec();
}

} // namespace DigikamEditorGmicQtPlugin
