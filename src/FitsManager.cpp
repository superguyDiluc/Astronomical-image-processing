#include "FitsManager.h"
#include "FitsImageProvider.h"

#include <QDateTime>
#include <QDir>
#include <QStandardPaths>

#include <fitsio.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>
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
float FitsManager::minValue() const { return m_minValue; }
float FitsManager::maxValue() const { return m_maxValue; }
float FitsManager::pixelMin() const { return m_pixelMin; }
float FitsManager::pixelMax() const { return m_pixelMax; }

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

void FitsManager::setMinValue(float value)
{
    if (!qFuzzyCompare(m_minValue, value)) {
        m_minValue = value;
        emit minValueChanged();
    }
}

void FitsManager::setMaxValue(float value)
{
    if (!qFuzzyCompare(m_maxValue, value)) {
        m_maxValue = value;
        emit maxValueChanged();
    }
}

// -- pixel range --------------------------------------------------------------

void FitsManager::computePixelRange()
{
    if (m_currentData.rawPixels.empty())
        return;

    float vMin = std::numeric_limits<float>::max();
    float vMax = std::numeric_limits<float>::lowest();
    for (float v : m_currentData.rawPixels) {
        if (!std::isfinite(v))
            continue;
        if (v < vMin) vMin = v;
        if (v > vMax) vMax = v;
    }

    m_pixelMin = vMin;
    m_pixelMax = vMax;
    emit pixelRangeChanged();
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

// -- pixel to QImage with log-stretch -----------------------------------------

QImage FitsManager::convertToQImage(const FitsData &data,
                                     float displayMin, float displayMax)
{
    if (data.rawPixels.empty())
        return {};

    float range = displayMax - displayMin;
    if (range <= 0.0f)
        range = 1.0f;

    const float logScale = 255.0f / std::log(256.0f);

    QImage image(data.width, data.height, QImage::Format_Grayscale8);
    for (int y = 0; y < data.height; ++y) {
        uchar *scanLine = image.scanLine(y);
        for (int x = 0; x < data.width; ++x) {
            float v = data.rawPixels[static_cast<size_t>(y) * data.width + x];
            if (!std::isfinite(v))
                v = displayMin;

            if (v <= displayMin) {
                scanLine[x] = 0;
            } else if (v >= displayMax) {
                scanLine[x] = 255;
            } else {
                float t = (v - displayMin) / range;
                float logVal = logScale * std::log(1.0f + 255.0f * t);
                scanLine[x] = static_cast<uchar>(
                    std::clamp(logVal, 0.0f, 255.0f));
            }
        }
    }

    return image;
}

// -- refresh display ----------------------------------------------------------

void FitsManager::refreshDisplay()
{
    if (m_currentData.rawPixels.empty() || !m_provider)
        return;

    QImage qimage = convertToQImage(m_currentData, m_minValue, m_maxValue);
    if (qimage.isNull())
        return;

    m_provider->updateImage(qimage);

    ++m_imageVersion;
    QString newSource = QStringLiteral("image://fits/v%1").arg(m_imageVersion);
    m_imageSource = newSource;
    emit imageSourceChanged();
}

// -- public slots -------------------------------------------------------------

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
        m_currentData = readFits2dHdu(filePath);

        // Write temp Primary-only FITS for astrometry.net
        QString tempPath = writeTempPrimaryFits(m_currentData);
        if (m_tempFitsPath != tempPath) {
            m_tempFitsPath = tempPath;
            emit tempFitsPathChanged();
        }

        // Compute pixel range and set initial min/max
        computePixelRange();
        setMinValue(m_pixelMin);
        setMaxValue(m_pixelMax);

        // Convert and display with linear stretch initially
        refreshDisplay();
        setStatus(QStringLiteral("ready"));

    } catch (const std::exception &e) {
        setErrorMessage(QString::fromUtf8(e.what()));
        setStatus(QStringLiteral("error"));
    }
}

void FitsManager::applyGrayTransform()
{
    if (m_currentData.rawPixels.empty()) {
        setErrorMessage(tr("No image loaded."));
        return;
    }

    clearError();
    refreshDisplay();
}

void FitsManager::autoAdjust()
{
    if (m_currentData.rawPixels.empty()) {
        setErrorMessage(tr("No image loaded."));
        return;
    }

    clearError();

    // Build histogram and find background peak via simple iterative clipping
    // (mirrors the Python sky_background_stats approach)
    std::vector<float> values;
    values.reserve(m_currentData.rawPixels.size());
    for (float v : m_currentData.rawPixels) {
        if (std::isfinite(v))
            values.push_back(v);
    }

    if (values.empty()) {
        setErrorMessage(tr("Image has no valid pixels."));
        return;
    }

    std::sort(values.begin(), values.end());

    // Iterative sigma clipping to find background median/std
    float med = values[values.size() / 2];
    float sum = 0.0f, sum2 = 0.0f;
    for (float v : values) {
        sum += v;
        sum2 += v * v;
    }
    float n = static_cast<float>(values.size());
    float stddev = std::sqrt(std::max(0.0f, sum2 / n - (sum / n) * (sum / n)));

    for (int iter = 0; iter < 8; ++iter) {
        float highCut = med + 3.0f * stddev;
        auto it = std::upper_bound(values.begin(), values.end(), highCut);
        size_t clippedSize = static_cast<size_t>(std::distance(values.begin(), it));
        if (clippedSize < 10 || clippedSize == values.size())
            break;

        float newMed = values[clippedSize / 2];
        float s = 0.0f, s2 = 0.0f;
        for (size_t i = 0; i < clippedSize; ++i) {
            s += values[i];
            s2 += values[i] * values[i];
        }
        float cn = static_cast<float>(clippedSize);
        float newStd = std::sqrt(std::max(0.0f, s2 / cn - (s / cn) * (s / cn)));

        if (std::abs(newMed - med) < 0.0001f * std::abs(med) &&
            std::abs(newStd - stddev) < 0.0001f * std::abs(stddev)) {
            med = newMed;
            stddev = newStd;
            break;
        }

        values.resize(clippedSize);
        med = newMed;
        stddev = newStd;
    }

    // Set display window: background center as min, background + reasonable range as max
    float autoMin = med;
    float autoMax = std::min(med + 12.0f * stddev, m_pixelMax);
    if (autoMax <= autoMin)
        autoMax = autoMin + 1.0f;

    setMinValue(autoMin);
    setMaxValue(autoMax);
    refreshDisplay();
}
