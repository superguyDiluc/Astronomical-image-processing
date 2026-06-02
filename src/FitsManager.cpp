#include "FitsManager.h"
#include "FitsImageProvider.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

#include <fitsio.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <vector>

// -- property accessors -------------------------------------------------------

FitsManager::FitsManager(QObject *parent)
    : QObject(parent)
{
}

QString FitsManager::status() const { return m_status; }
QString FitsManager::errorMessage() const { return m_errorMessage; }
QString FitsManager::imageSource() const { return m_imageSource; }
QString FitsManager::tempFitsPath() const { return m_tempFitsPath; }

void FitsManager::setImageProvider(FitsImageProvider *provider)
{
    m_provider = provider;
}

void FitsManager::setStatus(const QString &status)
{
    if (m_status != status) {
        m_status = status;
        emit statusChanged();
    }
}

void FitsManager::setErrorMessage(const QString &message)
{
    if (m_errorMessage != message) {
        m_errorMessage = message;
        emit errorMessageChanged();
    }
}

void FitsManager::clearError()
{
    setErrorMessage(QString());
}

// -- FITS reading -------------------------------------------------------------

FitsManager::FitsData FitsManager::readFits2dHdu(const QString &filePath)
{
    FitsData result;
    fitsfile *fptr = nullptr;
    int fitsStatus = 0;

    if (fits_open_file(&fptr, filePath.toLocal8Bit().constData(),
                       READONLY, &fitsStatus)) {
        char errMsg[80];
        fits_get_errstatus(fitsStatus, errMsg);
        throw std::runtime_error(
            QStringLiteral("Cannot open FITS file: %1")
                .arg(QString::fromLatin1(errMsg)).toStdString());
    }

    int numHdus = 0;
    fits_get_num_hdus(fptr, &numHdus, &fitsStatus);

    bool found = false;
    for (int i = 1; i <= numHdus; ++i) {
        fits_movabs_hdu(fptr, i, nullptr, &fitsStatus);

        int naxis = 0;
        fits_get_img_dim(fptr, &naxis, &fitsStatus);
        if (fitsStatus != 0 || naxis != 2) {
            fitsStatus = 0;
            continue;
        }

        long naxes[2] = {0, 0};
        fits_get_img_size(fptr, 2, naxes, &fitsStatus);
        if (fitsStatus != 0 || naxes[0] <= 0 || naxes[1] <= 0) {
            fitsStatus = 0;
            continue;
        }

        result.width = static_cast<int>(naxes[0]);
        result.height = static_cast<int>(naxes[1]);
        long nPixels = naxes[0] * naxes[1];

        result.rawPixels.resize(static_cast<size_t>(nPixels));
        long fpixel[2] = {1, 1};
        int anyNull = 0;
        fits_read_pix(fptr, TFLOAT, fpixel, nPixels, nullptr,
                      result.rawPixels.data(), &anyNull, &fitsStatus);
        if (fitsStatus != 0) {
            char errMsg[80];
            fits_get_errstatus(fitsStatus, errMsg);
            fits_close_file(fptr, &fitsStatus);
            throw std::runtime_error(
                QStringLiteral("Cannot read pixel data from HDU %1: %2")
                    .arg(i).arg(QString::fromLatin1(errMsg)).toStdString());
        }

        found = true;
        break;
    }

    fits_close_file(fptr, &fitsStatus);

    if (!found) {
        throw std::runtime_error("No 2D image HDU found in the FITS file.");
    }

    return result;
}

// -- temp Primary-only FITS ---------------------------------------------------

QString FitsManager::writeTempPrimaryFits(const FitsData &data)
{
    QString tempDir = QStandardPaths::writableLocation(
        QStandardPaths::TempLocation);
    QString tempPath = QDir(tempDir).filePath(
        QStringLiteral("fitsviewer_primary_%1.fits")
            .arg(QDateTime::currentMSecsSinceEpoch()));

    fitsfile *fptr = nullptr;
    int fitsStatus = 0;

    // cfitsio requires ! prefix to overwrite
    QByteArray path = ("!" + tempPath).toLocal8Bit();
    if (fits_create_file(&fptr, path.constData(), &fitsStatus)) {
        char errMsg[80];
        fits_get_errstatus(fitsStatus, errMsg);
        throw std::runtime_error(
            QStringLiteral("Cannot create temp FITS: %1")
                .arg(QString::fromLatin1(errMsg)).toStdString());
    }

    long naxes[2] = {data.width, data.height};
    fits_create_img(fptr, FLOAT_IMG, 2, naxes, &fitsStatus);
    long fpixel[2] = {1, 1};
    long nPixels = static_cast<long>(data.width) * data.height;
    fits_write_pix(fptr, TFLOAT, fpixel, nPixels,
                   const_cast<float *>(data.rawPixels.data()), &fitsStatus);
    fits_close_file(fptr, &fitsStatus);

    if (fitsStatus != 0) {
        char errMsg[80];
        fits_get_errstatus(fitsStatus, errMsg);
        throw std::runtime_error(
            QStringLiteral("Failed to write temp FITS: %1")
                .arg(QString::fromLatin1(errMsg)).toStdString());
    }

    return tempPath;
}

// -- pixel to QImage ----------------------------------------------------------

QImage FitsManager::convertToQImage(const FitsData &data)
{
    if (data.rawPixels.empty())
        return {};

    // Compute min/max ignoring NaN/Inf
    float vMin = std::numeric_limits<float>::max();
    float vMax = std::numeric_limits<float>::lowest();
    for (float v : data.rawPixels) {
        if (!std::isfinite(v))
            continue;
        if (v < vMin) vMin = v;
        if (v > vMax) vMax = v;
    }

    float range = vMax - vMin;
    if (range <= 0.0f)
        range = 1.0f;

    QImage image(data.width, data.height, QImage::Format_Grayscale8);
    for (int y = 0; y < data.height; ++y) {
        uchar *scanLine = image.scanLine(y);
        for (int x = 0; x < data.width; ++x) {
            float v = data.rawPixels[static_cast<size_t>(y) * data.width + x];
            if (!std::isfinite(v))
                v = vMin;
            float normalized = (v - vMin) / range;
            scanLine[x] = static_cast<uchar>(
                std::clamp(normalized * 255.0f, 0.0f, 255.0f));
        }
    }

    return image;
}

// -- public slot --------------------------------------------------------------

void FitsManager::loadFile(const QUrl &fileUrl)
{
    clearError();
    setStatus(QStringLiteral("loading"));

    QString filePath = fileUrl.toLocalFile();
    if (filePath.isEmpty()) {
        setErrorMessage(tr("Invalid file path."));
        setStatus(QStringLiteral("error"));
        return;
    }

    try {
        FitsData data = readFits2dHdu(filePath);

        // Write temp Primary-only FITS for astrometry.net
        QString tempPath = writeTempPrimaryFits(data);
        if (m_tempFitsPath != tempPath) {
            m_tempFitsPath = tempPath;
            emit tempFitsPathChanged();
        }

        // Convert to QImage for display
        QImage qimage = convertToQImage(data);
        if (qimage.isNull()) {
            setErrorMessage(tr("Failed to convert FITS data to image."));
            setStatus(QStringLiteral("error"));
            return;
        }

        if (m_provider) {
            m_provider->updateImage(qimage);
        }

        ++m_imageVersion;
        QString newSource = QStringLiteral("image://fits/v%1").arg(m_imageVersion);
        if (m_imageSource != newSource) {
            m_imageSource = newSource;
            emit imageSourceChanged();
        }

        setStatus(QStringLiteral("ready"));

    } catch (const std::exception &e) {
        setErrorMessage(QString::fromUtf8(e.what()));
        setStatus(QStringLiteral("error"));
    }
}
