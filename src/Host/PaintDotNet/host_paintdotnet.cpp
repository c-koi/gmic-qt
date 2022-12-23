/*
*  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
*  editors, offering hundreds of filters thanks to the underlying G'MIC
*  image processing framework.
*
*  Copyright (C) 2018, 2019, 2020, 2022 Nicholas Hayes
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
#include "Host/GmicQtHost.h"
#include "MainWindow.h"
#include "GmicQt.h"
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

namespace GmicQtHost
{
    const QString ApplicationName = QString("Paint.NET");
    const char * const ApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
    const bool DarkThemeIsDefault = true;
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
}

namespace GmicQtHost {

void getLayersExtent(int * width, int * height, GmicQt::InputMode mode)
{
    if (mode == GmicQt::InputMode::NoInput)
    {
        *width = 0;
        *height = 0;
        return;
    }

    QString getMaxLayerSizeCommand = QString("command=gmic_qt_get_max_layer_size\nmode=%1\n").arg((int)mode);

    QString reply = QString::fromUtf8(SendMessageSynchronously(getMaxLayerSizeCommand.toUtf8()));

    if (reply.length() > 0)
    {
        QStringList items = reply.split(',', QT_SKIP_EMPTY_PARTS);

        if (items.length() == 2)
        {
            *width = items[0].toInt();
            *height = items[1].toInt();
        }
    }
}

void getCroppedImages(gmic_list<float> & images, gmic_list<char> & imageNames, double x, double y, double width, double height, GmicQt::InputMode mode)
{
    if (mode == GmicQt::InputMode::NoInput)
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

    QString getImagesCommand = QString("command=gmic_qt_get_cropped_images\nmode=%1\ncroprect=%2,%3,%4,%5\n").arg((int)mode).arg(x).arg(y).arg(width).arg(height);

    QString reply = QString::fromUtf8(SendMessageSynchronously(getImagesCommand.toUtf8()));

    QStringList layers = reply.split('\n', QT_SKIP_EMPTY_PARTS);

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
        QStringList layerData = layers[i].split(',', QT_SKIP_EMPTY_PARTS);
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
                gmic_library::gmic_image<float>& dest = images[i];

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

void outputImages(gmic_list<float> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode mode)
{
    unused(imageNames);

    if (images.size() > 0)
    {
        for (size_t i = 0; i < host_paintdotnet::sharedMemory.size(); ++i)
        {
            host_paintdotnet::sharedMemory[i].reset();
        }
        host_paintdotnet::sharedMemory.clear();

        QString outputImagesCommand = QString("command=gmic_qt_output_images\nmode=%1\n").arg((int)mode);

        for (size_t i = 0; i < images.size(); ++i)
        {
            QString mappingName = QString("pdn_%1").arg(QUuid::createUuid().toString(QUuid::StringFormat::WithoutBraces));

            const gmic_library::gmic_image<float>& out = images[i];

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
        }

        SendMessageSynchronously(outputImagesCommand.toUtf8());
    }
}

void applyColorProfile(gmic_library::gmic_image<gmic_pixel_type> & images)
{
    unused(images);
}

void showMessage(const char * message)
{
    unused(message);
}

} // namespace GmicQtHost


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

    int exitCode = 0;

    std::list<GmicQt::InputMode> disabledInputModes;
    disabledInputModes.push_back(GmicQt::InputMode::NoInput);
    // disabledInputModes.push_back(GmicQt::InputMode::Active);
    // disabledInputModes.push_back(GmicQt::InputMode::All);
    disabledInputModes.push_back(GmicQt::InputMode::ActiveAndBelow);
    disabledInputModes.push_back(GmicQt::InputMode::ActiveAndAbove);
    disabledInputModes.push_back(GmicQt::InputMode::AllVisible);
    disabledInputModes.push_back(GmicQt::InputMode::AllInvisible);


    std::list<GmicQt::OutputMode> disabledOutputModes;
    // disabledOutputModes.push_back(GmicQt::OutputMode::InPlace);
    disabledOutputModes.push_back(GmicQt::OutputMode::NewImage);
    disabledOutputModes.push_back(GmicQt::OutputMode::NewLayers);
    disabledOutputModes.push_back(GmicQt::OutputMode::NewActiveLayers);
    bool dialogAccepted = true;

    if (useLastParameters)
    {
        GmicQt::RunParameters parameters;
        parameters = GmicQt::lastAppliedFilterRunParameters(GmicQt::ReturnedRunParametersFlag::AfterFilterExecution);
        exitCode = GmicQt::run(GmicQt::UserInterfaceMode::ProgressDialog,
                               parameters,
                               disabledInputModes,
                               disabledOutputModes,
                               &dialogAccepted);
    }
    else
    {
        exitCode = GmicQt::run(GmicQt::UserInterfaceMode::Full,
                               GmicQt::RunParameters(),
                               disabledInputModes,
                               disabledOutputModes,
                               &dialogAccepted);
    }

    for (size_t i = 0; i < host_paintdotnet::sharedMemory.size(); ++i)
    {
        host_paintdotnet::sharedMemory[i].reset();
    }
    host_paintdotnet::sharedMemory.clear();

    if (dialogAccepted)
    {
        // Send the G'MIC command name to Paint.NET.
        // It will be added to the image file names when writing the G'MIC images to an external file.
        GmicQt::RunParameters parameters = GmicQt::lastAppliedFilterRunParameters(GmicQt::ReturnedRunParametersFlag::AfterFilterExecution);
        QString gmicCommandName;

        // A G'MIC command consists of a command name that can optionally be followed
        // by a space and the command arguments.
        // If a command does not take any arguments only the command name will be present.
        //
        // According to the G'MIC Language Reference command names are restricted to a
        // subset of 7-bit US-ASCII: letters, numbers and underscores.
        // These restrictions make the command name safe to use in an OS file name.
        // See https://gmic.eu/reference/adding_custom_commands.html for more information.

        size_t firstSpaceIndex = parameters.command.find_first_of(' ');

        if (firstSpaceIndex != std::string::npos)
        {
            gmicCommandName = QString::fromStdString(parameters.command.substr(0, firstSpaceIndex));
        }
        else
        {
            gmicCommandName = QString::fromStdString(parameters.command);
        }

        QString message = QString("command=gmic_qt_set_gmic_command_name\n%1\n").arg(gmicCommandName);

        SendMessageSynchronously(message.toUtf8());
    }
    else
    {
        exitCode = 3;
    }

    return exitCode;
}
