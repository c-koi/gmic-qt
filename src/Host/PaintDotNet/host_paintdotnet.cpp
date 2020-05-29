/*
*  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
*  editors, offering hundreds of filters thanks to the underlying G'MIC
*  image processing framework.
*
*  Copyright (C) 2018, 2019, 2020 Nicholas Hayes
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
#include <QString>
#include <QDebug>
#include <QDataStream>
#include <QMessageBox>
#include <QUUid>
#include <iostream>
#include <limits>
#include <memory>
#include <vector>
#include "Common.h"
#include "Host/host.h"
#include "MainWindow.h"
#include "gmic_qt.h"
#include "gmic.h"
#include <Windows.h>

struct KernelHandleCloser
{
    void operator()(void* pointer)
    {
        if (pointer)
        {
            CloseHandle(pointer);
        }
    }
};

struct MappedFileViewCloser
{
    void operator()(void* pointer)
    {
        if (pointer)
        {
            UnmapViewOfFile(pointer);
        }
    }
};

typedef std::unique_ptr<void, KernelHandleCloser> ScopedFileMapping;
typedef std::unique_ptr<void, MappedFileViewCloser> ScopedFileMappingView;

namespace host_paintdotnet
{
    QString pipeName;
    std::vector<ScopedFileMapping> sharedMemory;
}

namespace GmicQt
{
    const QString HostApplicationName = QString("Paint.NET");
    const char * HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
    const bool DarkThemeIsDefault = false;
}

namespace
{
    class ScopedCreateFile : public std::unique_ptr<void, KernelHandleCloser>
    {
    public:
        ScopedCreateFile(HANDLE handle) : std::unique_ptr<void, KernelHandleCloser>(handle == INVALID_HANDLE_VALUE ? nullptr : handle)
        {
        }
    };

    BOOL ReadFileBlocking(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPOVERLAPPED lpOverlapped)
    {
        BYTE* bufferStart = static_cast<BYTE*>(lpBuffer);
        DWORD totalBytesRead = 0;
        do
        {
            if (!ReadFile(hFile, bufferStart + totalBytesRead, nNumberOfBytesToRead - totalBytesRead, nullptr, lpOverlapped))
            {
                DWORD error = GetLastError();
                switch (error)
                {
                case ERROR_SUCCESS:
                case ERROR_IO_PENDING:
                    break;
                default:
                    return FALSE;
                }
            }

            DWORD bytesRead = 0;

            if (GetOverlappedResult(hFile, lpOverlapped, &bytesRead, TRUE))
            {
                ResetEvent(lpOverlapped->hEvent);
            }
            else
            {
                return FALSE;
            }

            totalBytesRead += bytesRead;
        } while (totalBytesRead < nNumberOfBytesToRead);

        return TRUE;
    }

    BOOL WriteFileBlocking(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPOVERLAPPED lpOverlapped)
    {
        const BYTE* bufferStart = static_cast<const BYTE*>(lpBuffer);
        DWORD totalBytesWritten = 0;
        do
        {
            if (!WriteFile(hFile, bufferStart + totalBytesWritten, nNumberOfBytesToWrite - totalBytesWritten, nullptr, lpOverlapped))
            {
                DWORD error = GetLastError();
                switch (error)
                {
                case ERROR_SUCCESS:
                case ERROR_IO_PENDING:
                    break;
                default:
                    return FALSE;
                }
            }

            DWORD bytesWritten = 0;

            if (GetOverlappedResult(hFile, lpOverlapped, &bytesWritten, TRUE))
            {
                ResetEvent(lpOverlapped->hEvent);
            }
            else
            {
                return FALSE;
            }

            totalBytesWritten += bytesWritten;
        } while (totalBytesWritten < nNumberOfBytesToWrite);

        return TRUE;
    }

    QByteArray SendMessageSynchronously(QByteArray message)
    {
        QByteArray reply;

        ScopedCreateFile handle(CreateFileW(reinterpret_cast<LPCWSTR>(host_paintdotnet::pipeName.utf16()),
                                            GENERIC_READ | GENERIC_WRITE,
                                            0,
                                            nullptr,
                                            OPEN_EXISTING,
                                            FILE_FLAG_OVERLAPPED,
                                            nullptr));

        if (!handle)
        {
            qWarning() << "Could not open the Paint.NET pipe handle. GetLastError=" << GetLastError();

            return reply;
        }

        OVERLAPPED overlapped = {};
        overlapped.hEvent = CreateEventW(nullptr, TRUE, FALSE, nullptr);

        if (!overlapped.hEvent)
        {
            qWarning() << "Could not create the overlapped.hEvent handle. GetLastError=" << GetLastError();

            return reply;
        }

        // Write the message.

        const int messageLength = message.length();

        if (!WriteFileBlocking(handle.get(), &messageLength, sizeof(int), &overlapped))
        {
            qWarning() << "WriteFileBlocking(1) failed GetLastError=" << GetLastError();

            CloseHandle(overlapped.hEvent);

            return reply;
        }

        if (!WriteFileBlocking(handle.get(), message.data(), static_cast<DWORD>(messageLength), &overlapped))
        {
            qWarning() << "WriteFileBlocking(2) failed GetLastError=" << GetLastError();

            CloseHandle(overlapped.hEvent);

            return reply;
        }

        // Read the reply.

        int bytesRemaining = 0;

        if (!ReadFileBlocking(handle.get(), &bytesRemaining, sizeof(int), &overlapped))
        {
            qWarning() << "ReadFileBlocking(1) failed GetLastError=" << GetLastError();

            CloseHandle(overlapped.hEvent);

            return reply;
        }

        if (bytesRemaining > 0)
        {
            reply.resize(bytesRemaining);

            if (!ReadFileBlocking(handle.get(), reply.data(), static_cast<DWORD>(bytesRemaining), &overlapped))
            {
                qWarning() << "ReadFileBlocking(2) failed GetLastError=" << GetLastError();

                CloseHandle(overlapped.hEvent);

                return reply;
            }
        }

        // Send a message to Paint.NET that all data has been read.

        static const char done[] = "done";
        constexpr DWORD doneLength = sizeof(done) / sizeof(done[0]);

        WriteFileBlocking(handle.get(), done, doneLength, &overlapped);

        CloseHandle(overlapped.hEvent);

        return reply;
    }

    inline quint8 Float2Uint8Clamped(const float& value)
    {
        return value < 0.0f ? 0 : value > 255.0f ? 255 : static_cast<quint8>(value);
    }
    
    QWidget* GetParentWindow()
    {
        foreach(QWidget* widget, qApp->topLevelWidgets())
        {
            if (qobject_cast<MainWindow*>(widget) != nullptr && widget->isVisible())
            {
                return widget;
            }
        }

        return nullptr;
    }

    void ShowWarningMessage(QString message)
    {
        QWidget* parent = GetParentWindow();
        QString title = QString("G'MIC-Qt for Paint.NET");

        QMessageBox::warning(parent, title, message);
    }
}

void gmic_qt_get_layers_extent(int * width, int * height, GmicQt::InputMode mode)
{
    if (mode == GmicQt::NoInput)
    {
        *width = 0;
        *height = 0;
        return;
    }

    QString getMaxLayerSizeCommand = QString("command=gmic_qt_get_max_layer_size\nmode=%1\n").arg(mode);

    QString reply = QString::fromUtf8(SendMessageSynchronously(getMaxLayerSizeCommand.toUtf8()));

    if (reply.length() > 0)
    {
        QStringList items = reply.split(',', QString::SkipEmptyParts);

        if (items.length() == 2)
        {
            *width = items[0].toInt();
            *height = items[1].toInt();
        }
    }
}

void gmic_qt_get_cropped_images(gmic_list<float> & images, gmic_list<char> & imageNames, double x, double y, double width, double height, GmicQt::InputMode mode)
{
    if (mode == GmicQt::NoInput)
    {
        images.assign();
        imageNames.assign();
        return;
    }

    const bool entireImage = x < 0 && y < 0 && width < 0 && height < 0;
    if (entireImage)
    {
        x = 0.0;
        y = 0.0;
        width = 1.0;
        height = 1.0;
    }

    QString getImagesCommand = QString("command=gmic_qt_get_cropped_images\nmode=%1\ncroprect=%2,%3,%4,%5\n").arg(mode).arg(x).arg(y).arg(width).arg(height);

    QString reply = QString::fromUtf8(SendMessageSynchronously(getImagesCommand.toUtf8()));

    QStringList layers = reply.split('\n', QString::SkipEmptyParts);

    const int layerCount = layers.length();

    images.assign(layerCount);
    imageNames.assign(layerCount);

    for (int i = 0; i < layerCount; ++i)
    {
        QString layerName = QString("layer%1").arg(i);
        QByteArray layerNameBytes = layerName.toUtf8();
        gmic_image<char>::string(layerNameBytes.constData()).move_to(imageNames[i]);
    }

    for (int i = 0; i < layerCount; ++i)
    {
        QStringList layerData = layers[i].split(',', QString::SkipEmptyParts);
        if (layerData.length() != 4)
        {
            return;
        }

        LPCWSTR fileMappingName = reinterpret_cast<LPCWSTR>(layerData[0].utf16());
        const qint32 width = layerData[1].toInt();
        const qint32 height = layerData[2].toInt();
        const qint32 stride = layerData[3].toInt();

        ScopedFileMapping fileMappingObject(OpenFileMappingW(FILE_MAP_READ, FALSE, fileMappingName));

        if (fileMappingObject)
        {
            const size_t imageDataSize = static_cast<size_t>(stride) * static_cast<size_t>(height);

            ScopedFileMappingView mappedData(MapViewOfFile(fileMappingObject.get(), FILE_MAP_READ, 0, 0, imageDataSize));

            if (mappedData)
            {
                const quint8* scan0 = static_cast<const quint8*>(mappedData.get());
                cimg_library::CImg<float>& dest = images[i];

                dest.assign(width, height, 1, 4);
                float* dstR = dest.data(0, 0, 0, 0);
                float* dstG = dest.data(0, 0, 0, 1);
                float* dstB = dest.data(0, 0, 0, 2);
                float* dstA = dest.data(0, 0, 0, 3);

                for (qint32 y = 0; y < height; ++y)
                {
                    const quint8* src = scan0 + (y * stride);

                    for (qint32 x = 0; x < width; ++x)
                    {
                        *dstB++ = static_cast<float>(src[0]);
                        *dstG++ = static_cast<float>(src[1]);
                        *dstR++ = static_cast<float>(src[2]);
                        *dstA++ = static_cast<float>(src[3]);

                        src += 4;
                    }
                }
            }
            else
            {
                qWarning() << "MapViewOfFile failed GetLastError=" << GetLastError();
                return;
            }
        }
        else
        {
            qWarning() << "OpenFileMappingW failed GetLastError=" << GetLastError();
            return;
        }
    }

    SendMessageSynchronously("command=gmic_qt_release_shared_memory");
}

void gmic_qt_output_images(gmic_list<float> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode mode, const char * verboseLayersLabel)
{
    unused(imageNames);
    unused(verboseLayersLabel);

    if (images.size() > 0)
    {
        for (size_t i = 0; i < host_paintdotnet::sharedMemory.size(); ++i)
        {
            host_paintdotnet::sharedMemory[i].reset();
        }
        host_paintdotnet::sharedMemory.clear();

        QString outputImagesCommand = QString("command=gmic_qt_output_images\nmode=%1\n").arg(mode);

        QString mappingName = QString("pdn_%1").arg(QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces));


        const cimg_library::CImg<float>& out = images[0];

        const int width = out.width();
        const int height = out.height();

        const qint64 imageSizeInBytes = static_cast<qint64>(width) * static_cast<qint64>(height) * 4;

        const DWORD capacityHigh = static_cast<DWORD>((imageSizeInBytes >> 32) & 0xFFFFFFFF);
        const DWORD capacityLow = static_cast<DWORD>(imageSizeInBytes & 0x00000000FFFFFFFF);

        ScopedFileMapping fileMappingObject(CreateFileMappingW(INVALID_HANDLE_VALUE,
                                                               nullptr,
                                                               PAGE_READWRITE,
                                                               capacityHigh,
                                                               capacityLow,
                                                               reinterpret_cast<LPCWSTR>(mappingName.utf16())));
        if (fileMappingObject)
        {
            ScopedFileMappingView mappedData(MapViewOfFile(fileMappingObject.get(), FILE_MAP_ALL_ACCESS, 0, 0, static_cast<size_t>(imageSizeInBytes)));

            if (mappedData)
            {
                quint8* scan0 = static_cast<quint8*>(mappedData.get());
                const int stride = width * 4;

                if (out.spectrum() == 3)
                {
                    const float* srcR = out.data(0, 0, 0, 0);
                    const float* srcG = out.data(0, 0, 0, 1);
                    const float* srcB = out.data(0, 0, 0, 2);

                    for (int y = 0; y < height; ++y)
                    {
                        quint8* dst = scan0 + (y * stride);

                        for (int x = 0; x < width; ++x)
                        {
                            dst[0] = Float2Uint8Clamped(*srcB++);
                            dst[1] = Float2Uint8Clamped(*srcG++);
                            dst[2] = Float2Uint8Clamped(*srcR++);
                            dst[3] = 255;

                            dst += 4;
                        }
                    }
                }
                else if (out.spectrum() == 4)
                {
                    const float* srcR = out.data(0, 0, 0, 0);
                    const float* srcG = out.data(0, 0, 0, 1);
                    const float* srcB = out.data(0, 0, 0, 2);
                    const float* srcA = out.data(0, 0, 0, 3);

                    for (int y = 0; y < height; ++y)
                    {
                        quint8* dst = scan0 + (y * stride);

                        for (int x = 0; x < width; ++x)
                        {
                            dst[0] = Float2Uint8Clamped(*srcB++);
                            dst[1] = Float2Uint8Clamped(*srcG++);
                            dst[2] = Float2Uint8Clamped(*srcR++);
                            dst[3] = Float2Uint8Clamped(*srcA++);

                            dst += 4;
                        }
                    }
                }
                else if (out.spectrum() == 2)
                {
                    const float* srcGray = out.data(0, 0, 0, 0);
                    const float* srcAlpha = out.data(0, 0, 0, 1);

                    for (int y = 0; y < height; ++y)
                    {
                        quint8* dst = scan0 + (y * stride);

                        for (int x = 0; x < width; ++x)
                        {
                            dst[0] = dst[1] = dst[2] = Float2Uint8Clamped(*srcGray++);
                            dst[3] = Float2Uint8Clamped(*srcAlpha++);

                            dst += 4;
                        }
                    }
                }
                else if (out.spectrum() == 1)
                {
                    const float* srcGray = out.data(0, 0, 0, 0);

                    for (int y = 0; y < height; ++y)
                    {
                        quint8* dst = scan0 + (y * stride);

                        for (int x = 0; x < width; ++x)
                        {
                            dst[0] = dst[1] = dst[2] = Float2Uint8Clamped(*srcGray++);
                            dst[3] = 255;

                            dst += 4;
                        }
                    }
                }
                else
                {
                    qWarning() << "The image must have between 1 and 4 channels. Actual value=" << out.spectrum();
                    return;
                }

                // Manually release the mapped data to ensue it is committed before the parent file mapping handle
                // is moved into the host_paintdotnet::sharedMemory vector (which invalidates the previous handle).

                mappedData.reset();

                outputImagesCommand += "layer=" + mappingName + ","
                    + QString::number(width) + ","
                    + QString::number(height) + ","
                    + QString::number(stride) + "\n";


                host_paintdotnet::sharedMemory.push_back(std::move(fileMappingObject));
            }
            else
            {
                qWarning() << "MapViewOfFile failed GetLastError=" << GetLastError();
                return;
            }
        }
        else
        {
            qWarning() << "CreateFileMappingW failed GetLastError=" << GetLastError();
            return;
        }

        QByteArray reply = SendMessageSynchronously(outputImagesCommand.toUtf8());

        if (reply.length() > 0)
        {
            ShowWarningMessage(QString::fromUtf8(reply));
        }
    }
}

void gmic_qt_apply_color_profile(cimg_library::CImg<gmic_pixel_type> & images)
{
    unused(images);
}

void gmic_qt_show_message(const char * message)
{
    unused(message);
}

int main(int argc, char *argv[])
{
    bool useLastParameters = false;

    if (argc >= 3 && strcmp(argv[1], ".PDN") == 0)
    {
        host_paintdotnet::pipeName = argv[2];
        if (argc == 4)
        {
            useLastParameters = strcmp(argv[3], "reapply") == 0;
        }
    }
    else
    {
        return 1;
    }

    if (host_paintdotnet::pipeName.isEmpty())
    {
        return 2;
    }

    // Check that the layer data length is within the limit for the size_t type on 32-bit builds.
    // This prevents an overflow when calculating the total image size if the image is larger than 4GB.
#if defined(_M_IX86) || defined(__i386__)
    quint64 maxDataLength = 0;

    QString message = QString("command=gmic_qt_get_max_layer_data_length");
    QDataStream stream(SendMessageSynchronously(message.toUtf8()));
    stream.setByteOrder(QDataStream::ByteOrder::LittleEndian);
    stream >> maxDataLength;

    if (maxDataLength > std::numeric_limits<size_t>::max())
    {
        return 3;
    }
#endif

    int exitCode = 0;

    disableInputMode(GmicQt::NoInput);
    // disableInputMode(GmicQt::Active);
    // disableInputMode(GmicQt::All);
    disableInputMode(GmicQt::ActiveAndBelow);
    disableInputMode(GmicQt::ActiveAndAbove);
    disableInputMode(GmicQt::AllVisible);
    disableInputMode(GmicQt::AllInvisible);

    // disableOutputMode(GmicQt::InPlace);
    disableOutputMode(GmicQt::NewImage);
    disableOutputMode(GmicQt::NewLayers);
    disableOutputMode(GmicQt::NewActiveLayers);
	
	// disablePreviewMode(GmicQt::FirstOutput);
    disablePreviewMode(GmicQt::SecondOutput);
    disablePreviewMode(GmicQt::ThirdOutput);
    disablePreviewMode(GmicQt::FourthOutput);
    disablePreviewMode(GmicQt::First2SecondOutput);
    disablePreviewMode(GmicQt::First2ThirdOutput);
    disablePreviewMode(GmicQt::First2FourthOutput);
    disablePreviewMode(GmicQt::AllOutputs);

    if (useLastParameters)
    {
        exitCode = launchPluginHeadlessUsingLastParameters();
    }
    else
    {
        exitCode = launchPlugin();
    }

    for (size_t i = 0; i < host_paintdotnet::sharedMemory.size(); ++i)
    {
        host_paintdotnet::sharedMemory[i].reset();
    }
    host_paintdotnet::sharedMemory.clear();

    return exitCode;
}
