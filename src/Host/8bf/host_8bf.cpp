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
#include <qendian.h>
#include <QFile>
#include <QMessageBox>
#include <QUUid>
#include <iostream>
#include <limits>
#include <memory>
#include <new>
#include "Common.h"
#include "Host/host.h"
#include "ImageTools.h"
#include "gmic_qt.h"
#include "gmic.h"

struct Gmic8bfLayer
{
    int32_t width;
    int32_t height;
    bool visible;
    QString name;
    QImage imageData;
};

namespace host_8bf
{
    QString outputDir;
    QVector<Gmic8bfLayer> layers;
    int32_t activeLayerIndex;
    bool grayScale;
    bool sixteenBitsPerChannel;
    QVector<float> sixteenBitToEightBitLUT;
}

namespace GmicQt
{
    const QString HostApplicationName = QString("8bf Hosts");
    const char * HostApplicationShortname = GMIC_QT_XSTRINGIFY(GMIC_HOST);
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

    bool FillTileBuffer(
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
                break;
            }

            totalBytesRead += bytesRead;
        }

        return totalBytesRead == requiredSize;
    }

    bool CopyTileToQImage8Interleaved(
        char* tileBuffer,
        size_t tileBufferStride,
        int left,
        int top,
        int right,
        int bottom,
        int inNumberOfChannels,
        int outColumnStep,
        QImage& image)
    {
        for (int y = top; y < bottom; y++)
        {
            const uchar* src = reinterpret_cast<const uchar*>(tileBuffer) + ((static_cast<size_t>(y) - top) * tileBufferStride);
            uchar* dst = image.scanLine(y) + (static_cast<size_t>(left) * outColumnStep);

            for (int x = left; x < right; x++)
            {
                switch (inNumberOfChannels)
                {
                case 1:
                    dst[0] = src[0];
                    break;
                case 2:
                    dst[0] = dst[1] = dst[2] = src[0];
                    dst[3] = src[1];
                    break;
                case 3:
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    break;
                case 4:
                    dst[0] = src[0];
                    dst[1] = src[1];
                    dst[2] = src[2];
                    dst[3] = src[3];
                    break;
                default:
                    return false;
                }

                src += inNumberOfChannels;
                dst += outColumnStep;
            }
        }

        return true;
    }

    bool CopyTileToQImage8Planar(
        char* tileBuffer,
        size_t tileBufferStride,
        int left,
        int top,
        int right,
        int bottom,
        int inNumberOfChannels,
        int channelIndex,
        int outColumnStep,
        QImage& image)
    {
        for (int y = top; y < bottom; y++)
        {
            const uchar* src = reinterpret_cast<const uchar*>(tileBuffer) + ((static_cast<size_t>(y) - top) * tileBufferStride);
            uchar* dst = image.scanLine(y) + (static_cast<size_t>(left) * outColumnStep);

            for (int x = left; x < right; x++)
            {
                if (inNumberOfChannels == 2)
                {
                    // Grayscale with alpha is a special case.
                    // Qt does not have a dedicated format for it,
                    // so it is mapped to Format_RGBA8888.

                    switch (channelIndex)
                    {
                    case 0:
                        dst[0] = dst[1] = dst[2] = src[0];
                        break;
                    case 1:
                        dst[3] = src[0];
                        break;
                    default:
                        return false;
                    }
                }
                else
                {
                    dst[channelIndex] = src[0];
                }
                src++;
                dst += outColumnStep;
            }
        }

        return true;
    }

    bool ConvertGmic8bfInputToQImage8(
        QDataStream& dataStream,
        int32_t inTileWidth,
        int32_t inTileHeight,
        int32_t inNumberOfChannels,
        bool planar,
        QImage& image)
    {
        int32_t maxTileStride = planar ? inTileWidth : inTileWidth * inNumberOfChannels;
        size_t tileBufferSize = static_cast<size_t>(maxTileStride) * inTileHeight;

        std::unique_ptr<char> tileBuffer(new (std::nothrow) char[tileBufferSize]);

        if (!tileBuffer)
        {
            return false;
        }

        int width = image.width();
        int height = image.height();
        int outColumnStep;

        switch (image.format())
        {
        case QImage::Format_Grayscale8:
            outColumnStep = 1;
            break;
        case QImage::Format_RGB888:
            outColumnStep = 3;
            break;
        case QImage::Format_RGBA8888:
            outColumnStep = 4;
            break;
        default:
            return false;
        }

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

                        if (!FillTileBuffer(dataStream, bytesToRead, tileBuffer.get()))
                        {
                            return false;
                        }

                        if (!CopyTileToQImage8Planar(
                            tileBuffer.get(),
                            tileBufferStride,
                            left,
                            top,
                            right,
                            bottom,
                            inNumberOfChannels,
                            i,
                            outColumnStep,
                            image))
                        {
                            return false;
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

                    if (!FillTileBuffer(dataStream, bytesToRead, tileBuffer.get()))
                    {
                        return false;
                    }

                    if (!CopyTileToQImage8Interleaved(
                        tileBuffer.get(),
                        tileBufferStride,
                        left,
                        top,
                        right,
                        bottom,
                        inNumberOfChannels,
                        outColumnStep,
                        image))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    bool CopyTileToQImage16Interleaved(
        char* tileBuffer,
        size_t tileBufferStride,
        int left,
        int top,
        int right,
        int bottom,
        int inNumberOfChannels,
        int outColumnStep,
        QImage& image)
    {
        for (int y = top; y < bottom; y++)
        {
            const quint16* src = reinterpret_cast<const quint16*>(tileBuffer) + ((static_cast<size_t>(y) - top) * tileBufferStride);
            quint16* dst = reinterpret_cast<quint16*>(image.scanLine(y)) + (static_cast<size_t>(left) * outColumnStep);

            for (int x = left; x < right; x++)
            {
                switch (inNumberOfChannels)
                {
                case 1:
                    dst[0] = qFromLittleEndian(src[0]);
                    break;
                case 2:
                    dst[0] = dst[1] = dst[2] = qFromLittleEndian(src[0]);
                    dst[3] = qFromLittleEndian(src[1]);
                    break;
                case 3:
                    dst[0] = qFromLittleEndian(src[0]);
                    dst[1] = qFromLittleEndian(src[1]);
                    dst[2] = qFromLittleEndian(src[2]);
                    break;
                case 4:
                    dst[0] = qFromLittleEndian(src[0]);
                    dst[1] = qFromLittleEndian(src[1]);
                    dst[2] = qFromLittleEndian(src[2]);
                    dst[3] = qFromLittleEndian(src[3]);
                    break;
                default:
                    return false;
                }
                src += inNumberOfChannels;
                dst += outColumnStep;
            }
        }

        return true;
    }

    bool CopyTileToQImage16Planar(
        char* tileBuffer,
        size_t tileBufferStride,
        int left,
        int top,
        int right,
        int bottom,
        int inNumberOfChannels,
        int channelIndex,
        int outColumnStep,
        QImage& image)
    {
        for (int y = top; y < bottom; y++)
        {
            const quint16* src = reinterpret_cast<const quint16*>(tileBuffer) + ((static_cast<size_t>(y) - top) * tileBufferStride);
            quint16* dst = reinterpret_cast<quint16*>(image.scanLine(y)) + (static_cast<size_t>(left) * outColumnStep);

            for (int x = left; x < right; x++)
            {
                if (inNumberOfChannels == 2)
                {
                    // Grayscale with alpha is a special case.
                    // Qt does not have a dedicated format for it,
                    // so it is mapped to Format_RGBA64.

                    switch (channelIndex)
                    {
                    case 0:
                        dst[0] = dst[1] = dst[2] = qFromLittleEndian(src[0]);
                        break;
                    case 1:
                        dst[3] = qFromLittleEndian(src[0]);
                        break;
                    default:
                        return false;
                    }
                }
                else
                {
                    dst[channelIndex] = qFromLittleEndian(src[0]);
                }
                src++;
                dst += outColumnStep;
            }
        }

        return true;
    }

    bool ConvertGmic8bfInputToQImage16(
        QDataStream& dataStream,
        int32_t inTileWidth,
        int32_t inTileHeight,
        int32_t inNumberOfChannels,
        bool planar,
        QImage& image)
    {
        size_t maxTileStride = planar ? inTileWidth : static_cast<size_t>(inTileWidth) * inNumberOfChannels;
        size_t tileBufferSize = maxTileStride * inTileHeight * 2;

        std::unique_ptr<char> tileBuffer(new (std::nothrow) char[tileBufferSize]);

        if (!tileBuffer)
        {
            return false;
        }

        int width = image.width();
        int height = image.height();
        int outColumnStep;

        switch (image.format())
        {
        case QImage::Format_Grayscale16:
            outColumnStep = 1;
            break;
        case QImage::Format_RGBX64:
        case QImage::Format_RGBA64:
            outColumnStep = 4;
            break;
        default:
            return false;
        }

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

                        if (!FillTileBuffer(dataStream, bytesToRead, tileBuffer.get()))
                        {
                            return false;
                        }

                        if (!CopyTileToQImage16Planar(
                            tileBuffer.get(),
                            tileBufferStride,
                            left,
                            top,
                            right,
                            bottom,
                            inNumberOfChannels,
                            i,
                            outColumnStep,
                            image))
                        {
                            return false;
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

                    if (!FillTileBuffer(dataStream, bytesToRead, tileBuffer.get()))
                    {
                        return false;
                    }

                    if (!CopyTileToQImage16Interleaved(
                        tileBuffer.get(),
                        tileBufferStride,
                        left,
                        top,
                        right,
                        bottom,
                        inNumberOfChannels,
                        outColumnStep,
                        image))
                    {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    QImage ReadGmic8bfInput(const QString& path)
    {
        QFile file(path);

        if (!file.open(QIODevice::ReadOnly))
        {
            return QImage();
        }

        QDataStream dataStream(&file);
        dataStream.setByteOrder(QDataStream::LittleEndian);

        char signature[4] = {};

        dataStream.readRawData(signature, 4);

        if (strncmp(signature, "G8II", 4) != 0)
        {
            return QImage();
        }

        int32_t fileVersion = 0;

        dataStream >> fileVersion;

        if (fileVersion != 1 && fileVersion != 2)
        {
            return QImage();
        }

        int32_t width = 0;

        dataStream >> width;

        int32_t height = 0;

        dataStream >> height;

        int32_t numberOfChannels = 0;

        dataStream >> numberOfChannels;

        int32_t bitDepth = 0;

        dataStream >> bitDepth;

        bool planar = false;
        int32_t inTileWidth = width;
        int32_t inTileHeight = height;

        if (fileVersion == 2)
        {
            int32_t flags = 0;

            dataStream >> flags;

            planar = (flags & 1) != 0;

            dataStream >> inTileWidth;
            dataStream >> inTileHeight;
        }

        QImage::Format format{};

        switch (numberOfChannels)
        {
        case 1:
            format = bitDepth == 16 ? QImage::Format_Grayscale16 : QImage::Format_Grayscale8;
            break;
        case 3:
            format = bitDepth == 16 ? QImage::Format_RGBX64 : QImage::Format_RGB888;
            break;
        case 2:
        case 4:
            format = bitDepth == 16 ? QImage::Format_RGBA64 : QImage::Format_RGBA8888;
            break;
        default:
            return QImage();
        }

        QImage image(width, height, format);

        if (!image.isNull())
        {
            if (bitDepth == 16)
            {
                if (!ConvertGmic8bfInputToQImage16(dataStream, inTileWidth, inTileHeight, numberOfChannels, planar, image))
                {
                    return QImage();
                }
            }
            else
            {
                if (!ConvertGmic8bfInputToQImage8(dataStream, inTileWidth, inTileHeight, numberOfChannels, planar, image))
                {
                    return QImage();
                }
            }
        }

        return image;
    }

    bool ParseInputFileIndex(const QString& indexFilePath)
    {
        QFile file(indexFilePath);

        if (!file.open(QIODevice::ReadOnly))
        {
            return false;
        }

        QDataStream dataStream(&file);
        dataStream.setByteOrder(QDataStream::LittleEndian);

        char signature[4] = {};

        dataStream.readRawData(signature, 4);

        if (strncmp(signature, "G8IX", 4) != 0)
        {
            return false;
        }

        int32_t fileVersion = 0;

        dataStream >> fileVersion;

        if (fileVersion < 1 || fileVersion > 3)
        {
            return false;
        }

        int32_t layerCount = 0;

        dataStream >> layerCount;

        dataStream >> host_8bf::activeLayerIndex;

        host_8bf::grayScale = false;
        host_8bf::sixteenBitsPerChannel = false;

        if (fileVersion >= 2)
        {
            int32_t documentFlags;

            dataStream >> documentFlags;

            host_8bf::grayScale = (documentFlags & 1) != 0;
            host_8bf::sixteenBitsPerChannel = (documentFlags & 2) != 0;
        }

        host_8bf::layers.reserve(layerCount);

        for (size_t i = 0; i < layerCount; i++)
        {
            int32_t layerWidth = 0;

            dataStream >> layerWidth;

            int32_t layerHeight = 0;

            dataStream >> layerHeight;

            int32_t layerVisible = 0;

            dataStream >> layerVisible;

            QString layerName = ReadUTF8String(dataStream);

            QString filePath = ReadUTF8String(dataStream);

            QImage image = ReadGmic8bfInput(filePath);

            if (image.isNull())
            {
                return false;
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
        if (layerCount == 1 && fileVersion == 3)
        {
            QString imagePath = ReadUTF8String(dataStream);

            if (!imagePath.isEmpty())
            {
#if defined(_MSC_VER) && defined(_DEBUG)
                auto name = imagePath.toStdWString();
#endif

                QImage image = ReadGmic8bfInput(imagePath);

                if (!image.isNull())
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

        return true;
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

                for (int i = host_8bf::activeLayerIndex; i < layers.size(); i++)
                {
                    filteredLayers.push_back(layers[i]);
                }
            }
            else if (mode == GmicQt::InputMode::ActiveAndBelow)
            {
                const QVector<Gmic8bfLayer>& layers = host_8bf::layers;

                for (int i = 0; i <= host_8bf::activeLayerIndex; i++)
                {
                    filteredLayers.push_back(layers[i]);
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

    inline ushort float2ushort_bounded(const float& in)
    {
        // Scale the value from [0, 255] to [0, 65535].
        const float fullRangeValue = in * 257.0f;

        return (fullRangeValue < 0.0f) ? 0 : ((fullRangeValue > 65535.0f) ? 65535 : static_cast<ushort>(fullRangeValue));
    }

    void ConvertCroppedImageToGmic(const QImage& in, cimg_library::CImg<float>& out)
    {
        // The following code was copied from ImageConverter.cpp and has been adapted to support the G'MIC grayscale modes.

        Q_ASSERT_X(in.format() == QImage::Format_RGBA8888 ||
                   in.format() == QImage::Format_RGB888 ||
                   in.format() == QImage::Format_RGBA64 ||
                   in.format() == QImage::Format_RGBX64 ||
                   in.format() == QImage::Format_Grayscale8 ||
                   in.format() == QImage::Format_Grayscale16, "ConvertCroppedImageToGmic", "bad input format");

        if (in.format() == QImage::Format_RGBA8888)
        {
            const int w = in.width();
            const int h = in.height();

            if (host_8bf::grayScale)
            {
                out.assign(w, h, 1, 2);
                float* dstGray = out.data(0, 0, 0, 0);
                float* dstAlpha = out.data(0, 0, 0, 1);

                for (int y = 0; y < h; ++y)
                {
                    const unsigned char* src = in.scanLine(y);
                    int n = in.width();
                    while (n--)
                    {
                        *dstGray++ = static_cast<float>(src[0]);
                        *dstAlpha++ = static_cast<float>(src[3]);
                        src += 4;
                    }
                }
            }
            else
            {
                out.assign(w, h, 1, 4);
                float* dstR = out.data(0, 0, 0, 0);
                float* dstG = out.data(0, 0, 0, 1);
                float* dstB = out.data(0, 0, 0, 2);
                float* dstA = out.data(0, 0, 0, 3);

                for (int y = 0; y < h; ++y)
                {
                    const unsigned char* src = in.scanLine(y);
                    int n = in.width();
                    while (n--)
                    {
                        *dstR++ = static_cast<float>(src[0]);
                        *dstG++ = static_cast<float>(src[1]);
                        *dstB++ = static_cast<float>(src[2]);
                        *dstA++ = static_cast<float>(src[3]);
                        src += 4;
                    }
                }
            }
        }
        else if (in.format() == QImage::Format_RGB888)
        {
            const int w = in.width();
            const int h = in.height();

            out.assign(w, h, 1, 3);
            float* dstR = out.data(0, 0, 0, 0);
            float* dstG = out.data(0, 0, 0, 1);
            float* dstB = out.data(0, 0, 0, 2);
            for (int y = 0; y < h; ++y)
            {
                const unsigned char* src = in.scanLine(y);
                int n = in.width();
                while (n--)
                {
                    *dstR++ = static_cast<float>(src[0]);
                    *dstG++ = static_cast<float>(src[1]);
                    *dstB++ = static_cast<float>(src[2]);
                    src += 3;
                }
            }
        }
        else if (in.format() == QImage::Format_Grayscale8)
        {
            const int w = in.width();
            const int h = in.height();

            out.assign(w, h, 1, 1);
            float* dstGray = out.data(0, 0, 0, 0);
            for (int y = 0; y < h; ++y)
            {
                const unsigned char* src = in.scanLine(y);
                int n = in.width();
                while (n--)
                {
                    *dstGray++ = src[0];
                    src++;
                }
            }
        }
        else if (in.format() == QImage::Format_RGBA64)
        {
            const int w = in.width();
            const int h = in.height();

            if (host_8bf::grayScale)
            {
                out.assign(w, h, 1, 2);
                float* dstGray = out.data(0, 0, 0, 0);
                float* dstAlpha = out.data(0, 0, 0, 1);

                for (int y = 0; y < h; ++y)
                {
                    const ushort* src = reinterpret_cast<const ushort*>(in.scanLine(y));
                    int n = in.width();
                    while (n--)
                    {
                        *dstGray++ = host_8bf::sixteenBitToEightBitLUT[src[0]];
                        *dstAlpha++ = host_8bf::sixteenBitToEightBitLUT[src[3]];
                        src += 4;
                    }
                }
            }
            else
            {
                out.assign(w, h, 1, 4);
                float* dstR = out.data(0, 0, 0, 0);
                float* dstG = out.data(0, 0, 0, 1);
                float* dstB = out.data(0, 0, 0, 2);
                float* dstA = out.data(0, 0, 0, 3);

                for (int y = 0; y < h; ++y)
                {
                    const ushort* src = reinterpret_cast<const ushort*>(in.scanLine(y));
                    int n = in.width();
                    while (n--)
                    {
                        *dstR++ = host_8bf::sixteenBitToEightBitLUT[src[0]];
                        *dstG++ = host_8bf::sixteenBitToEightBitLUT[src[1]];
                        *dstB++ = host_8bf::sixteenBitToEightBitLUT[src[2]];
                        *dstA++ = host_8bf::sixteenBitToEightBitLUT[src[3]];
                        src += 4;
                    }
                }
            }
        }
        else if (in.format() == QImage::Format_RGBX64)
        {
            const int w = in.width();
            const int h = in.height();

            out.assign(w, h, 1, 3);
            float* dstR = out.data(0, 0, 0, 0);
            float* dstG = out.data(0, 0, 0, 1);
            float* dstB = out.data(0, 0, 0, 2);
            for (int y = 0; y < h; ++y)
            {
                const ushort* src = reinterpret_cast<const ushort*>(in.scanLine(y));
                int n = in.width();
                while (n--)
                {
                    *dstR++ = host_8bf::sixteenBitToEightBitLUT[src[0]];
                    *dstG++ = host_8bf::sixteenBitToEightBitLUT[src[1]];
                    *dstB++ = host_8bf::sixteenBitToEightBitLUT[src[2]];
                    src += 4;
                }
            }
        }
        else if (in.format() == QImage::Format_Grayscale16)
        {
            const int w = in.width();
            const int h = in.height();

            out.assign(w, h, 1, 1);
            float* dstGray = out.data(0, 0, 0, 0);
            for (int y = 0; y < h; ++y)
            {
                const ushort* src = reinterpret_cast<const ushort*>(in.scanLine(y));
                int n = in.width();
                while (n--)
                {
                    *dstGray++ = host_8bf::sixteenBitToEightBitLUT[src[0]];
                    src++;
                }
            }
        }
    }

    QImage ConvertGmicToOutput8(const cimg_library::CImg<float>& in)
    {
        // The following code has been adapted from ImageConverter.cpp.

        QImage out(in.width(), in.height(), QImage::Format_RGB888);

        if (in.spectrum() == 4 && out.format() != QImage::Format_RGBA8888) {
            out = out.convertToFormat(QImage::Format_RGBA8888);
        }
        else if (in.spectrum() == 3 && out.format() != QImage::Format_RGB888) {
            out = out.convertToFormat(QImage::Format_RGB888);
        }
        else if (in.spectrum() == 2 && out.format() != QImage::Format_RGBA8888) {
            out = out.convertToFormat(QImage::Format_RGBA8888);
        }
        else if (in.spectrum() == 1 && out.format() != QImage::Format_Grayscale8) {
            out = out.convertToFormat(QImage::Format_Grayscale8);
        }

        if (in.spectrum() == 3) {
            const float* srcR = in.data(0, 0, 0, 0);
            const float* srcG = in.data(0, 0, 0, 1);
            const float* srcB = in.data(0, 0, 0, 2);
            int height = out.height();
            for (int y = 0; y < height; ++y) {
                int n = in.width();
                unsigned char* dst = out.scanLine(y);
                while (n--) {
                    dst[0] = float2uchar_bounded(*srcR++);
                    dst[1] = float2uchar_bounded(*srcG++);
                    dst[2] = float2uchar_bounded(*srcB++);
                    dst += 3;
                }
            }
        }
        else if (in.spectrum() == 4) {
            const float* srcR = in.data(0, 0, 0, 0);
            const float* srcG = in.data(0, 0, 0, 1);
            const float* srcB = in.data(0, 0, 0, 2);
            const float* srcA = in.data(0, 0, 0, 3);
            int height = out.height();
            for (int y = 0; y < height; ++y) {
                int n = in.width();
                unsigned char* dst = out.scanLine(y);
                while (n--) {
                    dst[0] = float2uchar_bounded(*srcR++);
                    dst[1] = float2uchar_bounded(*srcG++);
                    dst[2] = float2uchar_bounded(*srcB++);
                    dst[3] = float2uchar_bounded(*srcA++);
                    dst += 4;
                }
            }
        }
        else if (in.spectrum() == 2) {
            //
            // Gray + Alpha
            //
            const float* src = in.data(0, 0, 0, 0);
            const float* srcA = in.data(0, 0, 0, 1);
            int height = out.height();
            for (int y = 0; y < height; ++y) {
                int n = in.width();
                unsigned char* dst = out.scanLine(y);
                while (n--) {
                    dst[2] = dst[1] = dst[0] = float2uchar_bounded(*src++);
                    dst[3] = float2uchar_bounded(*srcA++);
                    dst += 4;
                }
            }
        }
        else {
            //
            // 8-bits Gray levels
            //
            const float* src = in.data(0, 0, 0, 0);
            int height = out.height();
            for (int y = 0; y < height; ++y) {
                int n = in.width();
                unsigned char* dst = out.scanLine(y);
                while (n--) {
                    dst[0] = float2uchar_bounded(*src);
                    ++src;
                    ++dst;
                }
            }
        }

        return out;
    }

    QImage ConvertGmicToOutput16(const cimg_library::CImg<float>& in)
    {
        // The following code has been adapted from ImageConverter.cpp.

        QImage out(in.width(), in.height(), QImage::Format_RGBX64);

        if (in.spectrum() == 4 && out.format() != QImage::Format_RGBA64) {
            out = out.convertToFormat(QImage::Format_RGBA64);
        }
        else if (in.spectrum() == 3 && out.format() != QImage::Format_RGBX64) {
            out = out.convertToFormat(QImage::Format_RGBX64);
        }
        else if (in.spectrum() == 2 && out.format() != QImage::Format_RGBA64) {
            out = out.convertToFormat(QImage::Format_RGBA64);
        }
        else if (in.spectrum() == 1 && out.format() != QImage::Format_Grayscale16) {
            out = out.convertToFormat(QImage::Format_Grayscale16);
        }

        if (in.spectrum() == 3) {
            const float* srcR = in.data(0, 0, 0, 0);
            const float* srcG = in.data(0, 0, 0, 1);
            const float* srcB = in.data(0, 0, 0, 2);
            int height = out.height();
            for (int y = 0; y < height; ++y) {
                int n = in.width();
                ushort* dst = reinterpret_cast<ushort*>(out.scanLine(y));
                while (n--) {
                    dst[0] = float2ushort_bounded(*srcR++);
                    dst[1] = float2ushort_bounded(*srcG++);
                    dst[2] = float2ushort_bounded(*srcB++);
                    dst += 4;
                }
            }
        }
        else if (in.spectrum() == 4) {
            const float* srcR = in.data(0, 0, 0, 0);
            const float* srcG = in.data(0, 0, 0, 1);
            const float* srcB = in.data(0, 0, 0, 2);
            const float* srcA = in.data(0, 0, 0, 3);
            int height = out.height();
            for (int y = 0; y < height; ++y) {
                int n = in.width();
                ushort* dst = reinterpret_cast<ushort*>(out.scanLine(y));
                while (n--) {
                    dst[0] = float2ushort_bounded(*srcR++);
                    dst[1] = float2ushort_bounded(*srcG++);
                    dst[2] = float2ushort_bounded(*srcB++);
                    dst[3] = float2ushort_bounded(*srcA++);
                    dst += 4;
                }
            }
        }
        else if (in.spectrum() == 2) {
            //
            // 16-bits Gray + Alpha
            //
            const float* src = in.data(0, 0, 0, 0);
            const float* srcA = in.data(0, 0, 0, 1);
            int height = out.height();
            for (int y = 0; y < height; ++y) {
                int n = in.width();
                ushort* dst = reinterpret_cast<ushort*>(out.scanLine(y));
                while (n--) {
                    dst[0] = dst[1] = dst[2] = float2ushort_bounded(*src++);
                    dst[3] = float2ushort_bounded(*srcA++);
                    dst += 4;
                }
            }
        }
        else {
            //
            // 16-bits Gray levels
            //
            const float* src = in.data(0, 0, 0, 0);
            int height = out.height();
            for (int y = 0; y < height; ++y) {
                int n = in.width();
                ushort* dst = reinterpret_cast<ushort*>(out.scanLine(y));
                while (n--) {
                    dst[0] = float2ushort_bounded(*src);
                    ++src;
                    ++dst;
                }
            }
        }

        return out;
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

void gmic_qt_get_layers_extent(int * width, int * height, GmicQt::InputMode mode)
{
    if (mode == GmicQt::NoInput)
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
       ConvertCroppedImageToGmic(filteredLayers.at(i).imageData.copy(ix, iy, iw, ih), images[i]);
    }
}

void gmic_qt_output_images(gmic_list<float> & images, const gmic_list<char> & imageNames, GmicQt::OutputMode mode, const char * verboseLayersLabel)
{
    unused(imageNames);
    unused(verboseLayersLabel);

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
                outputPath = QString("%1/%2-%3.png").arg(host_8bf::outputDir).arg(timestamp).arg(i);
            }
            else
            {
                outputPath = QString("%1/%2.png").arg(host_8bf::outputDir).arg(timestamp);
            }

            cimg_library::CImg<float>& in = images[i];

            const int width = in.width();
            const int height = in.height();

            if (host_8bf::grayScale && (in.spectrum() == 3 || in.spectrum() == 4))
            {
                // Convert the RGB image to grayscale.
                GmicQt::calibrate_image(in, in.spectrum() == 4 ? 2 : 1, false);
            }

            QImage out;

            if (host_8bf::sixteenBitsPerChannel)
            {
                out = ConvertGmicToOutput16(in);
            }
            else
            {
                out = ConvertGmicToOutput8(in);
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
                active.imageData.swap(out);

                // The image that G'MIC passes to this method does not contain the most recent change.
                // To get the current "layered" G'MIC effects we need to save the active layer after
                // it has been updated.
                active.imageData.save(outputPath);
            }
            else
            {
                out.save(outputPath);
            }
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

    if (!ParseInputFileIndex(indexFilePath))
    {
        return 4;
    }

    if (host_8bf::sixteenBitsPerChannel)
    {
        host_8bf::sixteenBitToEightBitLUT.reserve(65536);

        for (int i = 0; i < host_8bf::sixteenBitToEightBitLUT.capacity(); i++)
        {
            // G'MIC expect the input image data to be a floating-point value in the range of [0, 255].
            // We use a lookup table to avoid having to repeatedly perform division on the same values.
            host_8bf::sixteenBitToEightBitLUT.push_back(static_cast<float>(i) / 257.0f);
        }
    }

    int exitCode = 0;
    disableInputMode(GmicQt::NoInput);
    // disableInputMode(GmicQt::Active);
    // disableInputMode(GmicQt::All);
    // disableInputMode(GmicQt::ActiveAndBelow);
    // disableInputMode(GmicQt::ActiveAndAbove);
    // disableInputMode(GmicQt::AllVisible);
    // disableInputMode(GmicQt::AllInvisible);


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

    if (!pluginDialogWasAccepted())
    {
        exitCode = 5;
    }

    return exitCode;
}
