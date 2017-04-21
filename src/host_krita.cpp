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

#include <algorithm>
#include "host.h"
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
}

static QString socketKey = "gmic-krita";
static const char ack[] = "ack";

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

void gmic_qt_get_layers_extent(int * width, int * height, GmicQt::InputMode mode)
{
    *width = 0;
    *height = 0;

    QByteArray answer = sendMessageSynchronously(QString("command=gmic_qt_get_image_size\nmode=%1").arg((int)mode).toLatin1());
    if (answer.length() > 0) {
        QList<QByteArray> wh = answer.split(',');
        if (wh.length() == 2) {
            *width = wh[0].toInt();
            *height = wh[1].toInt();
        }
    }
}

void gmic_qt_get_cropped_images(gmic_list<float> & images,
                                gmic_list<char> & imageNames,
                                double x, double y, double width, double height,
                                GmicQt::InputMode mode)
{
    images.assign();
    imageNames.assign();

    // Create a message for Krita
    QString message = QString("command=gmic_qt_get_cropped_images\nmode=%5\ncroprect=%1,%2,%3,%4").arg(x).arg(y).arg(width).arg(height).arg(mode);
    QByteArray ba = message.toLatin1();
    QByteArray answer = sendMessageSynchronously(ba);

    if (answer.isEmpty()) {
        return;
    }

    QStringList imagesList = QString::fromUtf8(answer).split("\n");

    images.assign(imagesList.size());
    imageNames.assign(imagesList.size());

    // Parse the answer -- there should be no new lines in layernames
    QStringList memoryKeys;
    // Get the keys for the shared memory areas and the imageNames as prepared by Krita in G'Mic format
    for (int i = 0; i < imagesList.length(); ++i) {
        const QString &layer = imagesList[i];
        memoryKeys << layer.split(',')[0];
        gmic_image<char>::string(layer.split(',')[1].toLatin1().constData()).move_to(imageNames[i]);
    }
    // Fill images from the shared memory areas
    for (int i = 0; i < memoryKeys.length(); ++i) {
        const QString &key = memoryKeys[i];
        QSharedMemory m(key);
        if (m.isAttached()) {
            m.lock();
            // convert the data to the list of float
            gmic_image<float> img(4, width, height);
            memcpy(img, m.constData(), m.size());
            img.move_to(images[i]);
            m.unlock();
            m.detach();
        }
        else {
            qWarning() << "Could not attach to shared memory area.";
        }
    }
    Q_ASSERT(images.size() == imageNames.size());
}

void gmic_qt_output_images( gmic_list<float> & images,
                            const gmic_list<char> & imageNames,
                            GmicQt::OutputMode mode,
                            const char * /*verboseLayersLabel*/)
{
    Q_ASSERT(images.size() == imageNames.size());

    // Create qsharedmemory segments for each image
    // Create a message for Krita based on mode, the keys of the qsharedmemory segments and the imageNames
    QString message = QString("command=gmic_qt_output_images\nmode=%1\n").arg(mode);

    QMap<QString, QSharedMemory*> sharedMemoryMap;
    for (uint i = 0; i < imageNames.size(); ++i) {
        QSharedMemory *m = new QSharedMemory();
        m->create(images[i]._width * images[i]._height * 4 * sizeof(float));
        m->lock();
        memcpy(m->data(), images[i], m->size());
        m->unlock();

        if (!m->attach(QSharedMemory::ReadOnly)) {
            qWarning() << "Could not create shared memory element for layer" << imageNames[i];
        }
        sharedMemoryMap[m->key()] = m;
        message += "layer=" + m->key() + "," + QString::fromLocal8Bit((const char * const)imageNames[i]) + "\n";
    }

    QByteArray ba = message.toLatin1();

    // Create a socket
    QLocalSocket socket;
    socket.connectToServer(socketKey);
    bool connected = socket.waitForConnected(1000);
    if (!connected) return;

    // Send the message to Krita
    QDataStream ds(&socket);
    ds.writeBytes(ba.constData(), ba.length());
    bool r = socket.waitForBytesWritten();

    // Wait for the ack
    r &= socket.waitForReadyRead(); // wait for ack
    r &= (socket.read(qstrlen(ack)) == ack);
    socket.waitForDisconnected(-1);

    // The other side is done, we can release our shared memory segments.
    Q_FOREACH(QSharedMemory *m, sharedMemoryMap) {
        m->detach();
    }
    qDeleteAll(sharedMemoryMap);
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
    }
    qWarning() << "Socket Key:" << socketKey;
    if (headless) {
        return launchPluginHeadlessUsingLastParameters();
    }
    else {
        return launchPlugin();
    }
}
