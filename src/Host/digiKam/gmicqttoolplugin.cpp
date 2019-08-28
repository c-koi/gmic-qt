/* ============================================================
 *
 * This file is a part of digiKam project
 * https://www.digikam.org
 *
 * Date        : 2018-07-30
 * Description : image editor plugin for GmicQt.
 *
 * Copyright (C) 2019 by Gilles Caulier <caulier dot gilles at gmail dot com>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software Foundation;
 * either version 2, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * ============================================================ */

#include "gmicqttoolplugin.h"

// Qt includes

#include <QApplication>
#include <QTranslator>
#include <QSettings>
#include <QEventLoop>
#include <QPointer>

// Local includes

#include "MainWindow.h"
#include "Widgets/LanguageSelectionWidget.h"
#include "DialogSettings.h"

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
    return QString::fromUtf8("A tool for G'MIC-Qt");
}

QString GmicQtToolPlugin::details() const
{
    return QString::fromUtf8("<p>This Image Editor tool for G'MIC-Qt.</p>");
}

QList<DPluginAuthor> GmicQtToolPlugin::authors() const
{
    return QList<DPluginAuthor>()
            << DPluginAuthor(QString::fromUtf8("Gilles Caulier"),
                             QString::fromUtf8("caulier dot gilles at gmail dot com"),
                             QString::fromUtf8("(C) 2019"))
            ;
}

void GmicQtToolPlugin::setup(QObject* const parent)
{
    DPluginAction* const ac = new DPluginAction(parent);
    ac->setIcon(icon());
    ac->setText(QString::fromUtf8("G'MIC-Qt..."));
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
    QString lang = LanguageSelectionWidget::configuredTranslator();

    if (!lang.isEmpty() && (lang != "en"))
    {
        auto translator = new QTranslator(qApp);
        translator->load(QString(":/translations/%1.qm").arg(lang));
        QApplication::installTranslator(translator);
    }

    QPointer<MainWindow> mainWindow = new MainWindow(0);

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
