/*
*  This file is part of G'MIC-Qt, a generic plug-in for raster graphics
*  editors, offering hundreds of filters thanks to the underlying G'MIC
*  image processing framework.
*
*  Copyright (C) 2020, 2021 Nicholas Hayes
*
*  Portions Copyright 2017 Sebastien Fourey
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
#include <QImage>
#include <QString>
#include <QVector>
#include <QDebug>
#include <QDataStream>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <qglobal.h>
#include <QMessageBox>
#include <QUUid>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include <list>
#include "Common.h"
#include "Host/GmicQtHost.h"
#include "ImageTools.h"
#include "GmicQt.h"
#include "gmic.h"

struct Gmic8bfLayer
{
    int32_t width;
    int32_t height;
    bool visible;
    QString name;
    cimg_library::CImg<float> imageData;
};

namespace host_8bf
{
    QString outputDir;
    QVector<Gmic8bfLayer> layers;
    int32_t activeLayerIndex;
    bool grayScale;
    bool sixteenBitsPerChannel;
    int32_t documentWidth;
    int32_t documentHeight;
    int32_t hostTileWidth;
    int32_t hostTileHeight;
}

namespace GmicQtHost
{
    const QString ApplicationName = QString("8bf Hosts");
    const char * const ApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
    const bool DarkThemeIsDefault = false;
}

namespace
{
    QString ReadUTF8String(QDataStream& dataStream)
    {
        int32_t length = 0;

        dataStream >> length;

        if (length == 0)
        {
            return QString();
        }
        else
        {
            QByteArray utf8Bytes(length, '\0');

            dataStream.readRawData(utf8Bytes.data(), length);

            return QString::fromUtf8(utf8Bytes);
        }
    }

    enum class InputFileParseStatus
    {
        Ok,
        FileOpenError,
        BadFileSignature,
        UnknownFileVersion,
        InvalidArgument,
        OutOfMemory,
        EndOfFile,
        PlatformEndianMismatch
    };

    InputFileParseStatus FillTileBuffer(
        QDataStream& dataStream,
        const size_t& requiredSize,
        char* buffer)
    {
        size_t totalBytesRead = 0;

        while (totalBytesRead < requiredSize)
        {
            int bytesToRead = static_cast<int>(std::min(requiredSize - totalBytesRead, static_cast<size_t>(INT_MAX)));

            int bytesRead = dataStream.readRawData(buffer + totalBytesRead, bytesToRead);

            if (bytesRead <= 0)
            {
                return InputFileParseStatus::EndOfFile;
            }

            totalBytesRead += bytesRead;
        }

        return InputFileParseStatus::Ok;
    }

    InputFileParseStatus CopyTileToGmicImage8Interleaved(
        const unsigned char* tileBuffer,
        size_t tileBufferStride,
        int left,
        int top,
        int right,
        int bottom,
        cimg_library::CImg<float>& out)
    {
        const int imageWidth = out.width();
        const int numberOfChannels = out.spectrum();

        if (numberOfChannels == 3)
        {
            float* rPlane = out.data(0, 0, 0, 0);
            float* gPlane = out.data(0, 0, 0, 1);
            float* bPlane = out.data(0, 0, 0, 2);

            for (int y = top; y < bottom; ++y)
            {
                const unsigned char* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);

                const size_t planeStart = (static_cast<size_t>(y) * imageWidth) + left;

                float* dstR = rPlane + planeStart;
                float* dstG = gPlane + planeStart;
                float* dstB = bPlane + planeStart;

                for (int x = left; x < right; x++)
                {
                    *dstR++ = static_cast<float>(src[0]);
                    *dstG++ = static_cast<float>(src[1]);
                    *dstB++ = static_cast<float>(src[2]);
                    src += 3;
                }
            }
        }
        else if (numberOfChannels == 4)
        {
            float* rPlane = out.data(0, 0, 0, 0);
            float* gPlane = out.data(0, 0, 0, 1);
            float* bPlane = out.data(0, 0, 0, 2);
            float* aPlane = out.data(0, 0, 0, 3);

            for (int y = top; y < bottom; ++y)
            {
                const unsigned char* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);

                const size_t planeStart = (static_cast<size_t>(y) * imageWidth) + left;

                float* dstR = rPlane + planeStart;
                float* dstG = gPlane + planeStart;
                float* dstB = bPlane + planeStart;
                float* dstA = aPlane + planeStart;

                for (int x = left; x < right; x++)
                {
                    *dstR++ = static_cast<float>(src[0]);
                    *dstG++ = static_cast<float>(src[1]);
                    *dstB++ = static_cast<float>(src[2]);
                    *dstA++ = static_cast<float>(src[3]);
                    src += 4;
                }
            }
        }
        else if (numberOfChannels == 2)
        {
            float* grayPlane = out.data(0, 0, 0, 0);
            float* alphaPlane = out.data(0, 0, 0, 1);

            for (int y = top; y < bottom; ++y)
            {
                const unsigned char* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);

                const size_t planeStart = (static_cast<size_t>(y) * imageWidth) + left;

                float* dstGray = grayPlane + planeStart;
                float* dstAlpha = alphaPlane + planeStart;

                for (int x = left; x < right; x++)
                {
                    *dstGray++ = static_cast<float>(src[0]);
                    *dstAlpha++ = static_cast<float>(src[1]);
                    src += 2;
                }
            }
        }
        else if (numberOfChannels == 1)
        {
            float* grayPlane = out.data(0, 0, 0, 0);

            for (int y = top; y < bottom; ++y)
            {
                const unsigned char* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);

                const size_t planeStart = (static_cast<size_t>(y) * imageWidth) + left;

                float* dstGray = grayPlane + planeStart;

                for (int x = left; x < right; x++)
                {
                    *dstGray++ = static_cast<float>(src[0]);
                    src++;
                }
            }
        }
        else
        {
            return InputFileParseStatus::InvalidArgument;
        }

        return InputFileParseStatus::Ok;
    }

    InputFileParseStatus CopyTileToGmicImage8Planar(
        const unsigned char* tileBuffer,
        size_t tileBufferStride,
        int left,
        int top,
        int right,
        int bottom,
        int channelIndex,
        cimg_library::CImg<float>& image)
    {
        const int imageWidth = image.width();

        float* plane = image.data(0, 0, 0, channelIndex);

        for (int y = top; y < bottom; ++y)
        {
            const unsigned char* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);
            float* dst = plane + (static_cast<size_t>(y) * static_cast<size_t>(imageWidth)) + left;

            for (int x = left; x < right; x++)
            {
                *dst++ = static_cast<float>(src[0]);

                src++;
            }
        }

        return InputFileParseStatus::Ok;
    }

    InputFileParseStatus ConvertGmic8bfInputToGmicImage8(
        QDataStream& dataStream,
        int32_t inTileWidth,
        int32_t inTileHeight,
        int32_t inNumberOfChannels,
        bool planar,
        cimg_library::CImg<float>& image)
    {
        int32_t maxTileStride = planar ? inTileWidth : inTileWidth * inNumberOfChannels;
        size_t tileBufferSize = static_cast<size_t>(maxTileStride) * inTileHeight;

        std::unique_ptr<char[]> tileBuffer(new (std::nothrow) char[tileBufferSize]);

        if (!tileBuffer)
        {
            return InputFileParseStatus::OutOfMemory;
        }

        int width = image.width();
        int height = image.height();

        if (planar)
        {
            for (int i = 0; i < inNumberOfChannels; i++)
            {
                for (int y = 0; y < height; y += inTileHeight)
                {
                    int top = y;
                    int bottom = std::min(y + inTileHeight, height);

                    size_t rowCount = static_cast<size_t>(bottom) - top;

                    for (int x = 0; x < width; x += inTileWidth)
                    {
                        int left = x;
                        int right = std::min(x + inTileWidth, width);

                        size_t tileBufferStride = static_cast<size_t>(right) - left;
                        size_t bytesToRead = tileBufferStride * rowCount;

                        InputFileParseStatus status = FillTileBuffer(dataStream, bytesToRead, tileBuffer.get());

                        if (status != InputFileParseStatus::Ok)
                        {
                            return status;
                        }

                        status = CopyTileToGmicImage8Planar(
                            reinterpret_cast<const unsigned char*>(tileBuffer.get()),
                            tileBufferStride,
                            left,
                            top,
                            right,
                            bottom,
                            i,
                            image);

                        if (status != InputFileParseStatus::Ok)
                        {
                            return status;
                        }
                    }
                }
            }
        }
        else
        {
            for (int y = 0; y < height; y += inTileHeight)
            {
                int top = y;
                int bottom = std::min(y + inTileHeight, height);

                size_t rowCount = static_cast<size_t>(bottom) - top;

                for (int x = 0; x < width; x += inTileWidth)
                {
                    int left = x;
                    int right = std::min(x + inTileWidth, width);

                    size_t columnCount = static_cast<size_t>(right) - left;
                    size_t tileBufferStride = columnCount * inNumberOfChannels;
                    size_t bytesToRead = tileBufferStride * rowCount;

                    InputFileParseStatus status = FillTileBuffer(dataStream, bytesToRead, tileBuffer.get());

                    if (status != InputFileParseStatus::Ok)
                    {
                        return status;
                    }

                    status = CopyTileToGmicImage8Interleaved(
                        reinterpret_cast<const unsigned char*>(tileBuffer.get()),
                        tileBufferStride,
                        left,
                        top,
                        right,
                        bottom,
                        image);

                    if (status != InputFileParseStatus::Ok)
                    {
                        return status;
                    }
                }
            }
        }

        return InputFileParseStatus::Ok;
    }

    InputFileParseStatus CopyTileToGmicImage16Interleaved(
        const unsigned short* tileBuffer,
        size_t tileBufferStride,
        int left,
        int top,
        int right,
        int bottom,
        cimg_library::CImg<float>& image,
        const QVector<float>& sixteenBitToEightBitLUT)
    {
        const int imageWidth = image.width();

        if (image.spectrum() == 3)
        {
            float* rPlane = image.data(0, 0, 0, 0);
            float* gPlane = image.data(0, 0, 0, 1);
            float* bPlane = image.data(0, 0, 0, 2);

            for (int y = top; y < bottom; ++y)
            {
                const unsigned short* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);

                const size_t planeStart = (static_cast<size_t>(y) * imageWidth) + left;

                float* dstR = rPlane + planeStart;
                float* dstG = gPlane + planeStart;
                float* dstB = bPlane + planeStart;

                for (int x = left; x < right; x++)
                {
                    *dstR++ = sixteenBitToEightBitLUT[src[0]];
                    *dstG++ = sixteenBitToEightBitLUT[src[1]];
                    *dstB++ = sixteenBitToEightBitLUT[src[2]];
                    src += 3;
                }
            }
        }
        else if (image.spectrum() == 4)
        {
            float* rPlane = image.data(0, 0, 0, 0);
            float* gPlane = image.data(0, 0, 0, 1);
            float* bPlane = image.data(0, 0, 0, 2);
            float* aPlane = image.data(0, 0, 0, 3);

            for (int y = top; y < bottom; ++y)
            {
                const unsigned short* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);

                const size_t planeStart = (static_cast<size_t>(y) * imageWidth) + left;

                float* dstR = rPlane + planeStart;
                float* dstG = gPlane + planeStart;
                float* dstB = bPlane + planeStart;
                float* dstA = aPlane + planeStart;

                for (int x = left; x < right; x++)
                {
                    *dstR++ = sixteenBitToEightBitLUT[src[0]];
                    *dstG++ = sixteenBitToEightBitLUT[src[1]];
                    *dstB++ = sixteenBitToEightBitLUT[src[2]];
                    *dstA++ = sixteenBitToEightBitLUT[src[3]];
                    src += 4;
                }
            }
        }
        else if (image.spectrum() == 2)
        {
            float* grayPlane = image.data(0, 0, 0, 0);
            float* alphaPlane = image.data(0, 0, 0, 1);

            for (int y = top; y < bottom; ++y)
            {
                const unsigned short* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);

                const size_t planeStart = (static_cast<size_t>(y) * imageWidth) + left;

                float* dstGray = grayPlane + planeStart;
                float* dstAlpha = alphaPlane + planeStart;

                for (int x = left; x < right; x++)
                {
                    *dstGray++ = sixteenBitToEightBitLUT[src[0]];
                    *dstAlpha++ = sixteenBitToEightBitLUT[src[1]];
                    src += 2;
                }
            }
        }
        else if (image.spectrum() == 1)
        {
            float* grayPlane = image.data(0, 0, 0, 0);

            for (int y = top; y < bottom; ++y)
            {
                const unsigned short* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);

                const size_t planeStart = (static_cast<size_t>(y) * imageWidth) + left;

                float* dstGray = grayPlane + planeStart;
                for (int x = left; x < right; x++)
                {
                    *dstGray++ = sixteenBitToEightBitLUT[src[0]];
                    src++;
                }
            }
        }
        else
        {
            return InputFileParseStatus::InvalidArgument;
        }

        return InputFileParseStatus::Ok;
    }

    InputFileParseStatus CopyTileToGmicImage16Planar(
        const unsigned short* tileBuffer,
        size_t tileBufferStride,
        int left,
        int top,
        int right,
        int bottom,
        int channelIndex,
        cimg_library::CImg<float>& image,
        const QVector<float>& sixteenBitToEightBitLUT)
    {
        const int imageWidth = image.width();
        const int imageHeight = image.height();

        float* plane = image.data(0, 0, 0, channelIndex);

        for (int y = top; y < bottom; ++y)
        {
            const unsigned short* src = tileBuffer + ((static_cast<size_t>(y) - top) * tileBufferStride);
            float* dst = plane + (static_cast<size_t>(y) * imageWidth) + left;

            for (int x = left; x < right; x++)
            {
                *dst++ = sixteenBitToEightBitLUT[src[0]];
                src++;
            }
        }

        return InputFileParseStatus::Ok;
    }

    InputFileParseStatus ConvertGmic8bfInputToGmicImage16(
        QDataStream& dataStream,
        int32_t inTileWidth,
        int32_t inTileHeight,
        int32_t inNumberOfChannels,
        bool planar,
        cimg_library::CImg<float>& image)
    {
        size_t maxTileStride = planar ? inTileWidth : static_cast<size_t>(inTileWidth) * inNumberOfChannels;
        size_t tileBufferSize = maxTileStride * inTileHeight * 2;

        std::unique_ptr<char[]> tileBuffer(new (std::nothrow) char[tileBufferSize]);

        if (!tileBuffer)
        {
            return InputFileParseStatus::OutOfMemory;
        }

        QVector<float> sixteenBitToEightBitLUT;
        sixteenBitToEightBitLUT.reserve(65536);

        for (int i = 0; i < sixteenBitToEightBitLUT.capacity(); i++)
        {
            // G'MIC expect the input image data to be a floating-point value in the range of [0, 255].
            // We use a lookup table to avoid having to repeatedly perform division on the same values.
            sixteenBitToEightBitLUT.push_back(static_cast<float>(i) / 257.0f);
        }

        int width = image.width();
        int height = image.height();

        if (planar)
        {
            for (int i = 0; i < inNumberOfChannels; i++)
            {
                for (int y = 0; y < height; y += inTileHeight)
                {
                    int top = y;
                    int bottom = std::min(y + inTileHeight, height);

                    size_t rowCount = static_cast<size_t>(bottom) - top;

                    for (int x = 0; x < width; x += inTileWidth)
                    {
                        int left = x;
                        int right = std::min(x + inTileWidth, width);

                        size_t tileBufferStride = static_cast<size_t>(right) - left;
                        size_t bytesToRead = tileBufferStride * rowCount * 2;

                        InputFileParseStatus status = FillTileBuffer(dataStream, bytesToRead, tileBuffer.get());

                        if (status != InputFileParseStatus::Ok)
                        {
                            return status;
                        }

                        status = CopyTileToGmicImage16Planar(
                            reinterpret_cast<const unsigned short*>(tileBuffer.get()),
                            tileBufferStride,
                            left,
                            top,
                            right,
                            bottom,
                            i,
                            image,
                            sixteenBitToEightBitLUT);

                        if (status != InputFileParseStatus::Ok)
                        {
                            return status;
                        }
                    }
                }
            }
        }
        else
        {
            for (int y = 0; y < height; y += inTileHeight)
            {
                int top = y;
                int bottom = std::min(y + inTileHeight, height);

                size_t rowCount = static_cast<size_t>(bottom) - top;

                for (int x = 0; x < width; x += inTileWidth)
                {
                    int left = x;
                    int right = std::min(x + inTileWidth, width);

                    size_t columnCount = static_cast<size_t>(right) - left;
                    size_t tileBufferStride = columnCount * inNumberOfChannels;
                    size_t bytesToRead = tileBufferStride * rowCount * 2;

                    InputFileParseStatus status = FillTileBuffer(dataStream, bytesToRead, tileBuffer.get());

                    if (status != InputFileParseStatus::Ok)
                    {
                        return status;
                    }

                    status = CopyTileToGmicImage16Interleaved(
                        reinterpret_cast<const unsigned short*>(tileBuffer.get()),
                        tileBufferStride,
                        left,
                        top,
                        right,
                        bottom,
                        image,
                        sixteenBitToEightBitLUT);

                    if (status != InputFileParseStatus::Ok)
                    {
                        return status;
                    }
                }
            }
        }

        return InputFileParseStatus::Ok;
    }

    InputFileParseStatus ReadGmic8bfInput(const QString& path, cimg_library::CImg<float>& image, bool isActiveLayer)
    {
        QFile file(path);

        if (!file.open(QIODevice::ReadOnly))
        {
            return InputFileParseStatus::FileOpenError;
        }

        QDataStream dataStream(&file);

        char signature[4] = {};

        dataStream.readRawData(signature, 4);

        if (strncmp(signature, "G8IM", 4) != 0)
        {
            return InputFileParseStatus::BadFileSignature;
        }

        char endian[4] = {};

        dataStream.readRawData(endian, 4);

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        if (strncmp(endian, "BEDN", 4) == 0)
        {
            dataStream.setByteOrder(QDataStream::BigEndian);
        }
#elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        if (strncmp(endian, "LEDN", 4) == 0)
        {
            dataStream.setByteOrder(QDataStream::LittleEndian);
        }
#else
#error "Unknown endianess on this platform."
#endif
        else
        {
            return InputFileParseStatus::PlatformEndianMismatch;
        }

        int32_t fileVersion = 0;

        dataStream >> fileVersion;

        if (fileVersion != 1)
        {
            return InputFileParseStatus::UnknownFileVersion;
        }

        int32_t width = 0;

        dataStream >> width;

        int32_t height = 0;

        dataStream >> height;

        int32_t numberOfChannels = 0;

        dataStream >> numberOfChannels;

        int32_t bitDepth = 0;

        dataStream >> bitDepth;

        int32_t flags = 0;

        dataStream >> flags;

        bool planar = false;

        planar = (flags & 1) != 0;

        int32_t inTileWidth = 0;

        dataStream >> inTileWidth;

        int32_t inTileHeight = 0;

        dataStream >> inTileHeight;

        if (isActiveLayer)
        {
            host_8bf::documentWidth = width;
            host_8bf::documentHeight = height;
            host_8bf::hostTileWidth = inTileWidth;
            host_8bf::hostTileHeight = inTileHeight;
        }

        image.assign(width, height, 1, numberOfChannels);

        InputFileParseStatus status = InputFileParseStatus::Ok;

        switch (bitDepth)
        {
        case 8:
            status = ConvertGmic8bfInputToGmicImage8(dataStream, inTileWidth, inTileHeight, numberOfChannels, planar, image);
            break;
        case 16:
            status = ConvertGmic8bfInputToGmicImage16(dataStream, inTileWidth, inTileHeight, numberOfChannels, planar, image);
            break;
        default:
            status = InputFileParseStatus::InvalidArgument;
            break;
        }

        return status;
    }

    InputFileParseStatus ParseInputFileIndex(const QString& indexFilePath)
    {
        QFile file(indexFilePath);

        if (!file.open(QIODevice::ReadOnly))
        {
            return InputFileParseStatus::FileOpenError;
        }

        QDataStream dataStream(&file);

        char signature[4] = {};

        dataStream.readRawData(signature, 4);

        if (strncmp(signature, "G8LI", 4) != 0)
        {
            return InputFileParseStatus::BadFileSignature;
        }

        char endian[4] = {};

        dataStream.readRawData(endian, 4);

#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        if (strncmp(endian, "BEDN", 4) == 0)
        {
            dataStream.setByteOrder(QDataStream::BigEndian);
        }
#elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        if (strncmp(endian, "LEDN", 4) == 0)
        {
            dataStream.setByteOrder(QDataStream::LittleEndian);
        }
#else
#error "Unknown endianess on this platform."
#endif
        else
        {
            return InputFileParseStatus::PlatformEndianMismatch;
        }

        int32_t fileVersion = 0;

        dataStream >> fileVersion;

        if (fileVersion != 1)
        {
            return InputFileParseStatus::UnknownFileVersion;
        }

        int32_t layerCount = 0;

        dataStream >> layerCount;

        dataStream >> host_8bf::activeLayerIndex;

        int32_t documentFlags;

        dataStream >> documentFlags;

        host_8bf::grayScale = (documentFlags & 1) != 0;
        host_8bf::sixteenBitsPerChannel = (documentFlags & 2) != 0;

        host_8bf::layers.reserve(layerCount);

        for (int32_t i = 0; i < layerCount; i++)
        {
            int32_t layerWidth = 0;

            dataStream >> layerWidth;

            int32_t layerHeight = 0;

            dataStream >> layerHeight;

            int32_t layerVisible = 0;

            dataStream >> layerVisible;

            QString layerName = ReadUTF8String(dataStream);

            QString filePath = ReadUTF8String(dataStream);

            cimg_library::CImg<float> image;

            InputFileParseStatus status = ReadGmic8bfInput(filePath, image, i == host_8bf::activeLayerIndex);

            if (status != InputFileParseStatus::Ok)
            {
                return status;
            }

            Gmic8bfLayer layer{};
            layer.width = layerWidth;
            layer.height = layerHeight;
            layer.visible = layerVisible != 0;
            layer.name = layerName;
            layer.imageData = image;

            host_8bf::layers.push_back(layer);
        }

        // Load the second input image from the alternate source, if present.
        if (layerCount == 1)
        {
            QString imagePath = ReadUTF8String(dataStream);

            if (!imagePath.isEmpty())
            {
#if defined(_MSC_VER) && defined(_DEBUG)
                auto name = imagePath.toStdWString();
#endif

                cimg_library::CImg<float> image;

                InputFileParseStatus status = ReadGmic8bfInput(imagePath, image, false);

                if (status == InputFileParseStatus::Ok)
                {
                    Gmic8bfLayer layer{};
                    layer.width = image.width();
                    layer.height = image.height();
                    layer.visible = true;
                    layer.name = "2nd Image";
                    layer.imageData = image;

                    host_8bf::layers.push_back(layer);
                }
            }
        }

        if (layerCount > 1)
        {
            // The 8bf plug-in sends layers in bottom to top order, whereas the
            // G'MIC-Qt plug-in for GIMP sends layers in top to bottom order.
            // So we reverse the layer list to match the behavior of the G'MIC-Qt
            // plug-in for GIMP.

            host_8bf::activeLayerIndex = layerCount - (1 + host_8bf::activeLayerIndex);

            // Adapted from https://stackoverflow.com/a/20652805
            for(int k = 0, s = host_8bf::layers.size(), max = (s / 2); k < max; k++)
            {
                host_8bf::layers.swapItemsAt(k, s - (1 + k));
            }
        }

        return InputFileParseStatus::Ok;
    }

    QVector<Gmic8bfLayer> FilterLayersForInputMode(GmicQt::InputMode mode)
    {
        if (host_8bf::layers.size() == 1 || mode == GmicQt::InputMode::All)
        {
            return host_8bf::layers;
        }
        else
        {
            QVector<Gmic8bfLayer> filteredLayers;

            if (mode == GmicQt::InputMode::Active)
            {
                filteredLayers.push_back(host_8bf::layers[host_8bf::activeLayerIndex]);
            }
            else if (mode == GmicQt::InputMode::ActiveAndAbove)
            {
                const QVector<Gmic8bfLayer>& layers = host_8bf::layers;

                // This case is the opposite of the GIMP plug-in because the layer order has
                // been reversed to match the top to bottom order that the GIMP plug-in uses.
                if (host_8bf::activeLayerIndex > 0)
                {
                    filteredLayers.push_back(layers[host_8bf::activeLayerIndex - 1]);
                }
                filteredLayers.push_back(layers[host_8bf::activeLayerIndex]);
            }
            else if (mode == GmicQt::InputMode::ActiveAndBelow)
            {
                const QVector<Gmic8bfLayer>& layers = host_8bf::layers;

                // This case is the opposite of the GIMP plug-in because the layer order has
                // been reversed to match the top to bottom order that the GIMP plug-in uses.
                filteredLayers.push_back(layers[host_8bf::activeLayerIndex]);
                if (host_8bf::activeLayerIndex < (layers.size() - 1))
                {
                    filteredLayers.push_back(layers[host_8bf::activeLayerIndex + 1]);
                }
            }
            else if (mode == GmicQt::InputMode::AllVisible)
            {
                const QVector<Gmic8bfLayer>& layers = host_8bf::layers;

                for (int i = 0; i < layers.size(); i++)
                {
                    const Gmic8bfLayer& layer = layers[i];
                    if (layer.visible)
                    {
                        filteredLayers.push_back(layer);
                    }
                }
            }
            else if (mode == GmicQt::InputMode::AllInvisible)
            {
                const QVector<Gmic8bfLayer>& layers = host_8bf::layers;

                for (int i = 0; i < layers.size(); i++)
                {
                    const Gmic8bfLayer& layer = layers[i];
                    if (!layer.visible)
                    {
                        filteredLayers.push_back(layer);
                    }
                }
            }

            return filteredLayers;
        }
    }

    // The following method was copied from ImageConverter.cpp.

    inline unsigned char float2uchar_bounded(const float& in)
    {
        return (in < 0.0f) ? 0 : ((in > 255.0f) ? 255 : static_cast<unsigned char>(in));
    }

    inline unsigned short float2ushort_bounded(const float& in)
    {
        // Scale the value from [0, 255] to [0, 65535].
        const float fullRangeValue = in * 257.0f;

        return (fullRangeValue < 0.0f) ? 0 : ((fullRangeValue > 65535.0f) ? 65535 : static_cast<unsigned short>(fullRangeValue));
    }

    void WriteGmic8bfImageHeader(
        QDataStream& stream,
        int width,
        int height,
        int numberOfChannels,
        int bitsPerChannel,
        bool planar,
        int tileWidth,
        int tileHeight)
    {
        const int fileVersion = 1;
        const int flags = planar ? 1 : 0;

        stream.writeRawData("G8IM", 4);
#if Q_BYTE_ORDER == Q_BIG_ENDIAN
        stream.writeRawData("BEDN", 4);
#elif Q_BYTE_ORDER == Q_LITTLE_ENDIAN
        stream.writeRawData("LEDN", 4);
#else
#error "Unknown endianess on this platform."
#endif
        stream << fileVersion;
        stream << width;
        stream << height;
        stream << numberOfChannels;
        stream << bitsPerChannel;
        stream << flags;
        stream << tileWidth;
        stream << tileHeight;
    }

    void WriteGmicOutputTile8Interleaved(
        QDataStream& dataStream,
        const cimg_library::CImg<float>& in,
        unsigned char* rowBuffer,
        int rowBufferLengthInBytes,
        int left,
        int top,
        int right,
        int bottom)
    {
        // The following code has been adapted from ImageConverter.cpp.

        if (in.spectrum() == 3)
        {
            const float* rPlane = in.data(0, 0, 0, 0);
            const float* gPlane = in.data(0, 0, 0, 1);
            const float* bPlane = in.data(0, 0, 0, 2);

            for (int y = top; y < bottom; ++y)
            {
                const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

                const float* srcR = rPlane + planeStart;
                const float* srcG = gPlane + planeStart;
                const float* srcB = bPlane + planeStart;
                unsigned char* dst = rowBuffer;

                for (int x = left; x < right; ++x)
                {
                    dst[0] = float2uchar_bounded(*srcR++);
                    dst[1] = float2uchar_bounded(*srcG++);
                    dst[2] = float2uchar_bounded(*srcB++);

                    dst += 3;
                }

                dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
            }
        }
        else if (in.spectrum() == 4)
        {
            const float* rPlane = in.data(0, 0, 0, 0);
            const float* gPlane = in.data(0, 0, 0, 1);
            const float* bPlane = in.data(0, 0, 0, 2);
            const float* aPlane = in.data(0, 0, 0, 3);

            for (int y = top; y < bottom; ++y)
            {
                const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

                const float* srcR = rPlane + planeStart;
                const float* srcG = gPlane + planeStart;
                const float* srcB = bPlane + planeStart;
                const float* srcA = aPlane + planeStart;

                unsigned char* dst = rowBuffer;

                for (int x = left; x < right; ++x)
                {
                    dst[0] = float2uchar_bounded(*srcR++);
                    dst[1] = float2uchar_bounded(*srcG++);
                    dst[2] = float2uchar_bounded(*srcB++);
                    dst[3] = float2uchar_bounded(*srcA++);

                    dst += 4;
                }

                dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
            }
        }
        else if (in.spectrum() == 2)
        {
            //
            // Gray + Alpha
            //
            const float* grayPlane = in.data(0, 0, 0, 0);
            const float* alphaPlane = in.data(0, 0, 0, 1);

            for (int y = top; y < bottom; ++y)
            {
                const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

                const float* src = grayPlane + planeStart;
                const float* srcA = alphaPlane + planeStart;

                unsigned char* dst = rowBuffer;


                for (int x = left; x < right; ++x)
                {
                    dst[0] = float2uchar_bounded(*src++);
                    dst[1] = float2uchar_bounded(*srcA++);

                    dst += 2;
                }

                dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
            }
        }
        else
        {
            //
            // 8-bits Gray levels
            //
            const float* grayPlane = in.data(0, 0, 0, 0);

            for (int y = top; y < bottom; ++y)
            {
                const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

                const float* src = grayPlane + planeStart;

                unsigned char* dst = rowBuffer;

                for (int x = left; x < right; ++x)
                {
                    dst[0] = float2uchar_bounded(*src++);

                    dst++;
                }

                dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
            }
        }
    }

    void WriteGmicOutputTile8Planar(
        QDataStream& dataStream,
        const cimg_library::CImg<float>& in,
        unsigned char* rowBuffer,
        int rowBufferLengthInBytes,
        int left,
        int top,
        int right,
        int bottom,
        int plane)
    {
        const float* srcPlane = in.data(0, 0, 0, plane);

        for (int y = top; y < bottom; ++y)
        {
            const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

            const float* src = srcPlane + planeStart;

            unsigned char* dst = rowBuffer;

            for (int x = left; x < right; ++x)
            {
                dst[0] = float2uchar_bounded(*src++);

                dst++;
            }

            dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
        }
    }

    void WriteGmicOutput8(
        const QString& outputFilePath,
        const cimg_library::CImg<float>& in,
        bool planar,
        int32_t tileWidth,
        int32_t tileHeight)
    {
        QFile file(outputFilePath);
        file.open(QFile::WriteOnly);
        QDataStream dataStream(&file);
        dataStream.setByteOrder(QDataStream::LittleEndian);

        const int width = in.width();
        const int height = in.height();
        const int numberOfChannels = in.spectrum();

        WriteGmic8bfImageHeader(dataStream, width, height, numberOfChannels, 8, planar, tileWidth, tileHeight);

        if (planar)
        {
            std::vector<unsigned char> rowBuffer(width);

            for (int i = 0; i < numberOfChannels; ++i)
            {
                for (int y = 0; y < height; y += tileHeight)
                {
                    int top = y;
                    int bottom = std::min(y + tileHeight, height);

                    for (int x = 0; x < width; x += tileWidth)
                    {
                        int left = x;
                        int right = std::min(x + tileWidth, width);

                        int rowBufferLengthInBytes = right - left;

                        WriteGmicOutputTile8Planar(
                            dataStream,
                            in,
                            rowBuffer.data(),
                            rowBufferLengthInBytes,
                            left,
                            top,
                            right,
                            bottom,
                            i);
                    }
                }
            }
        }
        else
        {
            std::vector<unsigned char> rowBuffer(static_cast<size_t>(width) * numberOfChannels);

            for (int y = 0; y < height; y += tileHeight)
            {
                int top = y;
                int bottom = std::min(y + tileHeight, height);

                for (int x = 0; x < width; x += tileWidth)
                {
                    int left = x;
                    int right = std::min(x + tileWidth, width);

                    int rowBufferLengthInBytes = (right - left) * numberOfChannels;

                    WriteGmicOutputTile8Interleaved(
                        dataStream,
                        in,
                        rowBuffer.data(),
                        rowBufferLengthInBytes,
                        left,
                        top,
                        right,
                        bottom);
                }
            }
        }
    }

    void WriteGmicOutputTile16Interleaved(
        QDataStream& dataStream,
        const cimg_library::CImg<float>& in,
        unsigned short* rowBuffer,
        int rowBufferLengthInBytes,
        int left,
        int top,
        int right,
        int bottom)
    {
        // The following code has been adapted from ImageConverter.cpp.

        if (in.spectrum() == 3)
        {
            const float* rPlane = in.data(0, 0, 0, 0);
            const float* gPlane = in.data(0, 0, 0, 1);
            const float* bPlane = in.data(0, 0, 0, 2);

            for (int y = top; y < bottom; ++y)
            {
                const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

                const float* srcR = rPlane + planeStart;
                const float* srcG = gPlane + planeStart;
                const float* srcB = bPlane + planeStart;
                unsigned short* dst = rowBuffer;

                for (int x = left; x < right; ++x)
                {
                    dst[0] = float2ushort_bounded(*srcR++);
                    dst[1] = float2ushort_bounded(*srcG++);
                    dst[2] = float2ushort_bounded(*srcB++);

                    dst += 3;
                }

                dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
            }
        }
        else if (in.spectrum() == 4)
        {
            const float* rPlane = in.data(0, 0, 0, 0);
            const float* gPlane = in.data(0, 0, 0, 1);
            const float* bPlane = in.data(0, 0, 0, 2);
            const float* aPlane = in.data(0, 0, 0, 3);

            for (int y = top; y < bottom; ++y)
            {
                const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

                const float* srcR = rPlane + planeStart;
                const float* srcG = gPlane + planeStart;
                const float* srcB = bPlane + planeStart;
                const float* srcA = aPlane + planeStart;

                unsigned short* dst = rowBuffer;

                for (int x = left; x < right; ++x)
                {
                    dst[0] = float2ushort_bounded(*srcR++);
                    dst[1] = float2ushort_bounded(*srcG++);
                    dst[2] = float2ushort_bounded(*srcB++);
                    dst[3] = float2ushort_bounded(*srcA++);

                    dst += 4;
                }

                dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
            }
        }
        else if (in.spectrum() == 2)
        {
            //
            // Gray + Alpha
            //
            const float* grayPlane = in.data(0, 0, 0, 0);
            const float* alphaPlane = in.data(0, 0, 0, 1);

            for (int y = top; y < bottom; ++y)
            {
                const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

                const float* src = grayPlane + planeStart;
                const float* srcA = alphaPlane + planeStart;

                unsigned short* dst = rowBuffer;


                for (int x = left; x < right; ++x)
                {
                    dst[0] = float2ushort_bounded(*src++);
                    dst[1] = float2ushort_bounded(*srcA++);

                    dst += 2;
                }

                dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
            }
        }
        else
        {
            //
            // 16-bits Gray levels
            //
            const float* grayPlane = in.data(0, 0, 0, 0);

            for (int y = top; y < bottom; ++y)
            {
                const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

                const float* src = grayPlane + planeStart;

                unsigned short* dst = rowBuffer;

                for (int x = left; x < right; ++x)
                {
                    dst[0] = float2ushort_bounded(*src++);

                    dst++;
                }

                dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
            }
        }
    }

    void WriteGmicOutputTile16Planar(
        QDataStream& dataStream,
        const cimg_library::CImg<float>& in,
        unsigned short* rowBuffer,
        int rowBufferLengthInBytes,
        int left,
        int top,
        int right,
        int bottom,
        int plane)
    {
        const float* srcPlane = in.data(0, 0, 0, plane);

        for (int y = top; y < bottom; ++y)
        {
            const size_t planeStart = (static_cast<size_t>(y) * in.width()) + left;

            const float* src = srcPlane + planeStart;

            unsigned short* dst = rowBuffer;

            for (int x = left; x < right; ++x)
            {
                dst[0] = float2ushort_bounded(*src++);

                dst++;
            }

            dataStream.writeRawData(reinterpret_cast<const char*>(rowBuffer), rowBufferLengthInBytes);
        }
    }

    void WriteGmicOutput16(
        const QString& outputFilePath,
        const cimg_library::CImg<float>& in,
        bool planar,
        int32_t tileWidth,
        int32_t tileHeight)
    {
        QFile file(outputFilePath);
        file.open(QFile::WriteOnly);
        QDataStream dataStream(&file);
        dataStream.setByteOrder(QDataStream::LittleEndian);

        const int width = in.width();
        const int height = in.height();
        const int numberOfChannels = in.spectrum();

        WriteGmic8bfImageHeader(dataStream, width, height, numberOfChannels, 16, planar, tileWidth, tileHeight);

        if (planar)
        {
            std::vector<unsigned short> rowBuffer(width);

            for (int i = 0; i < numberOfChannels; ++i)
            {
                for (int y = 0; y < height; y += tileHeight)
                {
                    int top = y;
                    int bottom = std::min(y + tileHeight, height);

                    for (int x = 0; x < width; x += tileWidth)
                    {
                        int left = x;
                        int right = std::min(x + tileWidth, width);

                        int columnCount = right - left;

                        int rowBufferLengthInBytes = columnCount * 2;

                        WriteGmicOutputTile16Planar(
                            dataStream,
                            in,
                            rowBuffer.data(),
                            rowBufferLengthInBytes,
                            left,
                            top,
                            right,
                            bottom,
                            i);
                    }
                }
            }
        }
        else
        {
            std::vector<unsigned short> rowBuffer(static_cast<size_t>(width) * numberOfChannels);

            for (int y = 0; y < height; y += tileHeight)
            {
                int top = y;
                int bottom = std::min(y + tileHeight, height);

                for (int x = 0; x < width; x += tileWidth)
                {
                    int left = x;
                    int right = std::min(x + tileWidth, width);

                    int rowBufferLengthInBytes = ((right - left) * numberOfChannels) * 2;

                    WriteGmicOutputTile16Interleaved(
                        dataStream,
                        in,
                        rowBuffer.data(),
                        rowBufferLengthInBytes,
                        left,
                        top,
                        right,
                        bottom);
                }
            }
        }
    }

    void EmptyOutputFolder()
    {
        QDir dir(host_8bf::outputDir);
        dir.setFilter(QDir::NoDotAndDotDot | QDir::Files);
        foreach(QString dirFile, dir.entryList())
        {
            dir.remove(dirFile);
        }
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

    if (host_8bf::layers.size() == 1)
    {
        const Gmic8bfLayer& layer = host_8bf::layers[0];

        *width = layer.width;
        *height = layer.height;
    }
    else
    {
        QVector<Gmic8bfLayer> filteredLayers = FilterLayersForInputMode(mode);

        for (int i = 0; i < filteredLayers.size(); i++)
        {
            Gmic8bfLayer& layer = filteredLayers[i];

            *width = std::max(*width, layer.width);
            *height = std::max(*height, layer.height);
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

    QVector<Gmic8bfLayer> filteredLayers = FilterLayersForInputMode(mode);

    const int layerCount = filteredLayers.size();

    images.assign(layerCount);
    imageNames.assign(layerCount);

    for (int i = 0; i < layerCount; ++i)
    {
        QByteArray layerNameBytes = filteredLayers[i].name.toUtf8();
        gmic_image<char>::string(layerNameBytes.constData()).move_to(imageNames[i]);
    }

    int maxWidth = 0;
    int maxHeight = 0;

    for (int i = 0; i < filteredLayers.size(); i++)
    {
        Gmic8bfLayer& layer = filteredLayers[i];

        maxWidth = std::max(maxWidth, layer.width);
        maxHeight = std::max(maxHeight, layer.height);
    }

    const int ix = entireImage ? 0 : static_cast<int>(std::floor(x * maxWidth));
    const int iy = entireImage ? 0 : static_cast<int>(std::floor(y * maxHeight));
    const int iw = entireImage ? maxWidth : std::min(maxWidth - ix, static_cast<int>(1 + std::ceil(width * maxWidth)));
    const int ih = entireImage ? maxHeight : std::min(maxHeight - iy, static_cast<int>(1 + std::ceil(height * maxHeight)));

    for (int i = 0; i < layerCount; i++)
    {
        if (entireImage)
        {
            images[i].assign(filteredLayers.at(i).imageData);
        }
        else
        {
            images[i].assign(filteredLayers.at(i).imageData.get_crop(ix, iy, iw, ih));
        }
    }
}

void outputImages(gmic_list<float> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode /* mode */)
{
    unused(imageNames);

    if (images.size() > 0)
    {
        // Remove any files that may be present from the last time the user clicked Apply.
        EmptyOutputFolder();

        QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd-hhmmss");
        bool haveMultipleImages = images.size() > 1;

        for (size_t i = 0; i < images.size(); ++i)
        {
            QString outputPath;

            if (haveMultipleImages)
            {
                outputPath = QString("%1/%2-%3.g8i").arg(host_8bf::outputDir).arg(timestamp).arg(i);
            }
            else
            {
                outputPath = QString("%1/%2.g8i").arg(host_8bf::outputDir).arg(timestamp);
            }

            cimg_library::CImg<float>& in = images[i];

            const int width = in.width();
            const int height = in.height();
            bool planar = false;
            int tileWidth = width;
            int tileHeight = height;

            if (host_8bf::grayScale && (in.spectrum() == 3 || in.spectrum() == 4))
            {
                // Convert the RGB image to grayscale.
                GmicQt::calibrateImage(in, in.spectrum() == 4 ? 2 : 1, false);
            }

            if (i == 0)
            {
                // Replace the active layer image with the first output image.
                // This allows users to "layer" multiple effects using the G'MIC-Qt Apply button.
                //
                // Note that only the most recently applied effect will be used by the "Last Filter"
                // or "Repeat Filter" commands.

                Gmic8bfLayer& active = host_8bf::layers[host_8bf::activeLayerIndex];

                active.width = width;
                active.height = height;
                active.imageData.assign(in);

                // If the G'MIC output is a single image that matches the host document size it will be
                // copied to the active layer when G'MIC exits.
                if (images.size() == 1 && width == host_8bf::documentWidth && height == host_8bf::documentHeight)
                {
                    // The output will be written as a tiled planar image because that is the most
                    // efficient format for the host to read.
                    planar = true;
                    tileWidth = host_8bf::hostTileWidth;
                    tileHeight = host_8bf::hostTileHeight;
                }
            }

            if (host_8bf::sixteenBitsPerChannel)
            {
                WriteGmicOutput16(outputPath, in, planar, tileWidth, tileHeight);
            }
            else
            {
                WriteGmicOutput8(outputPath, in, planar, tileWidth, tileHeight);
            }
        }
    }
}

void applyColorProfile(cimg_library::CImg<gmic_pixel_type> & images)
{
    unused(images);
}

void showMessage(const char * message)
{
    unused(message);
}


} // GmicQtHost

#if defined(_MSC_VER) && defined(_DEBUG)
#include <sstream>

// Adapted from https://stackoverflow.com/a/20387632
bool launchDebugger()
{
    // Get System directory, typically c:\windows\system32
    std::wstring systemDir(MAX_PATH + 1, '\0');
    UINT nChars = GetSystemDirectoryW(&systemDir[0], static_cast<UINT>(systemDir.length()));
    if (nChars == 0) return false; // failed to get system directory
    systemDir.resize(nChars);

    // Get process ID and create the command line
    DWORD pid = GetCurrentProcessId();
    std::wostringstream s;
    s << systemDir << L"\\vsjitdebugger.exe -p " << pid;
    std::wstring cmdLine = s.str();

    // Start debugger process
    STARTUPINFOW si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));

    if (!CreateProcessW(NULL, &cmdLine[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) return false;

    // Close debugger process handles to eliminate resource leak
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // Wait for the debugger to attach
    while (!IsDebuggerPresent()) Sleep(100);

    // Stop execution so the debugger can take over
    DebugBreak();
    return true;
}
#endif // defined(_MSC_VER) && defined(_DEBUG)

int main(int argc, char *argv[])
{
#if defined(_MSC_VER) && defined(_DEBUG)
    launchDebugger();
#endif

    QString indexFilePath;
    bool useLastParameters = false;

    if (argc >= 3)
    {
        indexFilePath = argv[1];
        host_8bf::outputDir = argv[2];
        if (argc == 4)
        {
            useLastParameters = strcmp(argv[3], "reapply") == 0;
        }
    }
    else
    {
        return 1;
    }

    if (indexFilePath.isEmpty())
    {
        return 2;
    }

    if (host_8bf::outputDir.isEmpty())
    {
        return 3;
    }

    try
    {
        InputFileParseStatus status = ParseInputFileIndex(indexFilePath);

        // The return value 5 is skipped because it is already being used to
        // indicate that the user canceled the dialog.
        switch (status)
        {
        case InputFileParseStatus::Ok:
            // No error
            break;
        case InputFileParseStatus::FileOpenError:
            return 6;
        case InputFileParseStatus::BadFileSignature:
        case InputFileParseStatus::InvalidArgument:
            return 7;
        case InputFileParseStatus::UnknownFileVersion:
            return 8;
        case InputFileParseStatus::OutOfMemory:
            return 9;
        case InputFileParseStatus::EndOfFile:
            return 10;
        case InputFileParseStatus::PlatformEndianMismatch:
            return 11;
        default:
            return 4; // Unknown error
        }
    }
    catch (const std::bad_alloc&)
    {
        return 9;
    }

    int exitCode = 0;
    std::list<GmicQt::InputMode> disabledInputModes;
    disabledInputModes.push_back(GmicQt::InputMode::NoInput);
    // disabledInputModes.push_back(GmicQt::InputMode::Active);
    // disabledInputModes.push_back(GmicQt::InputMode::All);
    // disabledInputModes.push_back(GmicQt::InputMode::ActiveAndBelow);
    // disabledInputModes.push_back(GmicQt::InputMode::ActiveAndAbove);
    // disabledInputModes.push_back(GmicQt::InputMode::AllVisible);
    // disabledInputModes.push_back(GmicQt::InputMode::AllInvisible);


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

    if (!dialogAccepted)
    {
        exitCode = 5;
    }

    return exitCode;
}
