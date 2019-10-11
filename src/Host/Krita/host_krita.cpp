/*
 * Copyright (C) 2017 Boudewijn Rempt <boud@valdyas.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include <QStandardPaths>
#include <QCommandLineParser>
#include <QCommandLineOption>
#include <QApplication>
#include <QProcess>
#include <QByteArray>
#include <QDebug>
#include <QFileDialog>
#include <QSharedMemory>
#include <QFileInfo>
#include <QDesktopWidget>
#include <QLocalServer>
#include <QLocalSocket>
#include <QDataStream>
#include <QBuffer>
#include <QUuid>

#include <ImageConverter.h>

#include <algorithm>
#include "Host/host.h"
#include "gmic_qt.h"
#include "gmic.h"

/*
 * Messages to Krita are built like this:
 *
 * command
 * mode=int
 * layer=key,imagename
 * croprect=x,y,w,h
 *
 * Messages from Krita are built like this:
 *
 * key,imagename
 *
 * After a message has been received, "ack" is sent
 *
 */

namespace GmicQt {
const QString HostApplicationName = QString("Krita");
const char *HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
const bool DarkThemeIsDefault = true;
}

static QString socketKey = "gmic-krita";
static const char ack[] = "ack";
static QVector<QSharedMemory*> sharedMemorySegments;


QByteArray sendMessageSynchronously(const QByteArray ba)
{
    QByteArray answer;

    // Send a message to Krita to ask for the images and image with the given crop and mode
    QLocalSocket socket;
    socket.connectToServer(socketKey);
    bool connected = socket.waitForConnected(1000);
    if (!connected) {
        qWarning() << "Could not connect to the Krita instance.";
        return answer;
    }

    // Send the message to Krita
    QDataStream ds(&socket);
    ds.writeBytes(ba.constData(), ba.length());
    socket.waitForBytesWritten();

    while (socket.bytesAvailable() < static_cast<int>(sizeof(quint32))) {
        if (!socket.isValid()) {
            qWarning() << "Stale request";
            return answer;
        }
        socket.waitForReadyRead(1000);
    }

    // Get the answer
    quint32 remaining;
    ds >> remaining;
    answer.resize(remaining);
    int got = 0;
    char *answerBuf = answer.data();
    do {
        got = ds.readRawData(answerBuf, remaining);
        remaining -= got;
        answerBuf += got;
    } while (remaining && got >= 0 && socket.waitForReadyRead(2000));

    if (got < 0) {
        qWarning() << "Could not receive the answer." << socket.errorString();
        return answer;
    }

    // Acknowledge receipt
    socket.write(ack, qstrlen(ack));
    socket.waitForBytesWritten(1000);
    socket.disconnectFromServer();

    return answer;
}

void gmic_qt_get_layers_extent(int *width, int *height, GmicQt::InputMode mode)
{
    *width = 0;
    *height = 0;
    QByteArray command = QString("command=gmic_qt_get_image_size\nmode=%1").arg((int)mode).toUtf8();

    QString answer = QString::fromUtf8(sendMessageSynchronously(command));
    if (answer.length() > 0) {
        QList<QString> wh = answer.split(',', QString::SkipEmptyParts);
        if (wh.length() == 2) {
            *width = wh[0].toInt();
            *height = wh[1].toInt();
        }
    }

    //qDebug() << "gmic-qt: layers extent:" << *width << *height;
}

void gmic_qt_get_cropped_images(gmic_list<float> & images,
                                gmic_list<char> & imageNames,
                                double x, double y, double width, double height,
                                GmicQt::InputMode mode)
{

    //qDebug() << "gmic-qt: get_cropped_images:" << x << y << width << height;

    const bool entireImage = x < 0 && y < 0 && width < 0 && height < 0;
    if (entireImage) {
      x = 0.0;
      y = 0.0;
      width = 1.0;
      height = 1.0;
    }

    // Create a message for Krita
    QString message = QString("command=gmic_qt_get_cropped_images\nmode=%5\ncroprect=%1,%2,%3,%4").arg(x).arg(y).arg(width).arg(height).arg(mode);
    QByteArray command = message.toUtf8();
    QString answer = QString::fromUtf8(sendMessageSynchronously(command));

    if (answer.isEmpty()) {
        qWarning() << "\tgmic-qt: empty answer!";
        return;
    }

    //qDebug() << "\tgmic-qt: " << answer;

    QStringList imagesList = answer.split("\n", QString::SkipEmptyParts);

    images.assign(imagesList.size());
    imageNames.assign(imagesList.size());

    //qDebug() << "\tgmic-qt: imagelist size" << imagesList.size();

    // Parse the answer -- there should be no new lines in layernames
    QStringList memoryKeys;
    QList<QSize> sizes;
    // Get the keys for the shared memory areas and the imageNames as prepared by Krita in G'Mic format
    for (int i = 0; i < imagesList.length(); ++i) {
        const QString &layer = imagesList[i];
        QStringList parts = layer.split(',', QString::SkipEmptyParts);
        if (parts.size() != 4) {
            qWarning() << "\tgmic-qt: Got the wrong answer!";
        }
        memoryKeys << parts[0];
        QByteArray ba = parts[1].toLatin1();
        ba = QByteArray::fromHex(ba);
        gmic_image<char>::string(ba.constData()).move_to(imageNames[i]);
        sizes << QSize(parts[2].toInt(), parts[3].toInt());
    }

    //qDebug() << "\tgmic-qt: keys" << memoryKeys;

    // Fill images from the shared memory areas
    for (int i = 0; i < memoryKeys.length(); ++i) {
        const QString &key = memoryKeys[i];
        QSharedMemory m(key);

        if (!m.attach(QSharedMemory::ReadOnly)) {
            qWarning() << "\tgmic-qt: Could not attach to shared memory area." << m.error() << m.errorString();
        }
        if (m.isAttached()) {
            if (!m.lock()) {
                qWarning() << "\tgmic-qt: Could not lock memory segment"  << m.error() << m.errorString();
            }
            //qDebug() << "Memory segment" << key << m.size() << m.constData() << m.data();

            // convert the data to the list of float
            gmic_image<float> gimg;
            gimg.assign(sizes[i].width(), sizes[i].height(), 1, 4);
            memcpy(gimg._data, m.constData(), sizes[i].width() * sizes[i].height() * 4 * sizeof(float));
            gimg.move_to(images[i]);

            if (!m.unlock()) {
                qWarning() << "\tgmic-qt: Could not unlock memory segment"  << m.error() << m.errorString();
            }
            if (!m.detach()) {
                qWarning() << "\tgmic-qt: Could not detach from memory segment"  << m.error() << m.errorString();
            }
        }
        else {
            qWarning() << "gmic-qt: Could not attach to shared memory area." << m.error() << m.errorString();
        }
    }

    sendMessageSynchronously("command=gmic_qt_detach");

    //qDebug() << "\tgmic-qt:  Images size" << images.size() << ", names size" << imageNames.size();
}

void gmic_qt_output_images( gmic_list<float> & images,
                            const gmic_list<char> & imageNames,
                            GmicQt::OutputMode mode,
                            const char * /*verboseLayersLabel*/)
{

    //qDebug() << "qmic-qt-output-images";

    Q_FOREACH(QSharedMemory *sharedMemory, sharedMemorySegments) {
        if (sharedMemory->isAttached()) {
            sharedMemory->detach();
        }
    }
    qDeleteAll(sharedMemorySegments);
    sharedMemorySegments.clear();

    //qDebug() << "\tqmic-qt: shared memory" << sharedMemorySegments.count();

    // Create qsharedmemory segments for each image
    // Create a message for Krita based on mode, the keys of the qsharedmemory segments and the imageNames
    QString message = QString("command=gmic_qt_output_images\nmode=%1\n").arg(mode);

    for (uint i = 0; i < images.size(); ++i) {

        //qDebug() << "\tgmic-qt: image number" << i;

        gmic_image<float> gimg = images.at(i);

        QSharedMemory *m = new QSharedMemory(QString("key_%1").arg(QUuid::createUuid().toString()));
        sharedMemorySegments.append(m);

        if (!m->create(gimg._width * gimg._height * gimg._spectrum * sizeof(float))) {
            qWarning() << "Could not create shared memory" << m->error() << m->errorString();
            return;
        }

        m->lock();
        memcpy(m->data(), gimg._data, gimg._width * gimg._height * gimg._spectrum * sizeof(float));
        m->unlock();

        QString layerName((const char *)imageNames[i]);

        message += "layer=" + m->key() + ","
                + layerName.toUtf8().toHex() + ","
                + QString("%1,%2,%3").arg(gimg._spectrum).arg(gimg._width).arg(gimg._height)
                + + "\n";
    }
    sendMessageSynchronously(message.toUtf8());
}

void gmic_qt_show_message(const char * )
{
    // May be left empty for Krita.
    // Only used by launchPluginHeadless(), called in the non-interactive
    // script mode of GIMP.
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> & )
{

}
#if defined Q_OS_WIN
#if defined DRMINGW
namespace
{
void tryInitDrMingw()
{
    wchar_t path[MAX_PATH];
    QString pathStr = QCoreApplication::applicationDirPath().replace(L'/', L'\\') + QStringLiteral("\\exchndl.dll");
    if (pathStr.size() > MAX_PATH - 1) {
        return;
    }
    int pathLen = pathStr.toWCharArray(path);
    path[pathLen] = L'\0'; // toWCharArray doesn't add NULL terminator
    HMODULE hMod = LoadLibraryW(path);
    if (!hMod) {
        return;
    }
    // No need to call ExcHndlInit since the crash handler is installed on DllMain
    auto myExcHndlSetLogFileNameA = reinterpret_cast<BOOL (APIENTRY *)(const char *)>(GetProcAddress(hMod, "ExcHndlSetLogFileNameA"));
    if (!myExcHndlSetLogFileNameA) {
        return;
    }
    // Set the log file path to %LocalAppData%\kritacrash.log
    QString logFile = QStandardPaths::writableLocation(QStandardPaths::DesktopLocation).replace(L'/', L'\\') + QStringLiteral("\\gmic_krita_qt_crash.log");
    myExcHndlSetLogFileNameA(logFile.toLocal8Bit());
}
} // namespace
#endif // DRMINGW
#endif // Q_OS_WIN

int main(int argc, char *argv[])
{

    bool headless = false;
    {
        QCommandLineParser parser;
        parser.setApplicationDescription("Krita G'Mic Plugin");
        parser.addHelpOption();
        parser.addPositionalArgument("socket key", "Key to find Krita's local server socket");
        QCoreApplication app(argc, argv);
        parser.process(app);
        const QStringList args = parser.positionalArguments();
        if (args.size() > 0) {
            socketKey = args[0];
        }
        if (args.size() > 1) {
            if (args[1] == "reapply") {
                headless = true;

            }
        }
#if defined Q_OS_WIN
#if defined DRMINGW
        tryInitDrMingw();
#endif
#endif
    }

    qWarning() << "gmic-qt: socket Key:" << socketKey;
    int r = 0;
    if (headless) {
        r = launchPluginHeadlessUsingLastParameters();
    }
    else {
        r = launchPlugin();
    }

    Q_FOREACH(QSharedMemory *sharedMemory, sharedMemorySegments) {
        if (sharedMemory->isAttached()) {
            sharedMemory->detach();
        }
    }

    qDeleteAll(sharedMemorySegments);
    sharedMemorySegments.clear();

    return r;
}
