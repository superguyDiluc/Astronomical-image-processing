#include "AstrometryManager.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHttpMultiPart>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QSettings>
#include <QUrlQuery>

#include <algorithm>

namespace {
constexpr auto ApiUrl = "https://nova.astrometry.net/api/";
constexpr auto BaseUrl = "https://nova.astrometry.net";
constexpr int PollIntervalMs = 5000;

QString settingsKey()
{
    return QStringLiteral("astrometry/apiKey");
}
}

AstrometryManager::AstrometryManager(QObject *parent)
    : QObject(parent)
    , m_result(new AstrometryResult(this))
{
    m_pollTimer.setInterval(PollIntervalMs);
    connect(&m_pollTimer, &QTimer::timeout, this, [this]() {
        if (m_status == QStringLiteral("waiting-for-job"))
            pollSubmission();
        else if (m_status == QStringLiteral("solving"))
            pollJob();
    });

    m_elapsedTimer.setInterval(1000);
    connect(&m_elapsedTimer, &QTimer::timeout, this, &AstrometryManager::updateElapsed);
}

bool AstrometryManager::apiKeyConfigured() const { return !apiKey().isEmpty(); }
bool AstrometryManager::running() const { return m_running; }
QString AstrometryManager::status() const { return m_status; }
QString AstrometryManager::stageText() const { return m_stageText; }
QString AstrometryManager::errorMessage() const { return m_errorMessage; }
QString AstrometryManager::subId() const { return m_result->subId(); }
QString AstrometryManager::jobId() const { return m_result->jobId(); }
QString AstrometryManager::outputDir() const { return m_result->outputDir(); }
double AstrometryManager::progress() const { return m_progress; }
int AstrometryManager::elapsedSeconds() const { return m_elapsedSeconds; }
AstrometryResult *AstrometryManager::result() const { return m_result; }

QString AstrometryManager::apiKey() const
{
    QString configured = QSettings().value(settingsKey()).toString().trimmed();
    if (!configured.isEmpty())
        return configured;
    return QString::fromLocal8Bit(qgetenv("ASTROMETRY_API_KEY")).trimmed();
}

QString AstrometryManager::apiKeyPreview() const
{
    QString key = apiKey();
    if (key.isEmpty())
        return QString();
    if (key.size() <= 6)
        return QStringLiteral("******");
    return QStringLiteral("***%1").arg(key.right(4));
}

void AstrometryManager::saveApiKey(const QString &apiKey)
{
    QSettings().setValue(settingsKey(), apiKey.trimmed());
    emit apiKeyConfiguredChanged();
}

void AstrometryManager::clearApiKey()
{
    QSettings().remove(settingsKey());
    emit apiKeyConfiguredChanged();
}

void AstrometryManager::setRunning(bool running)
{
    if (m_running == running)
        return;
    m_running = running;
    emit runningChanged();
}

void AstrometryManager::setStatus(const QString &status)
{
    if (m_status == status)
        return;
    m_status = status;
    m_result->setStatus(status);
    emit statusChanged();
}

void AstrometryManager::setStage(const QString &status, const QString &stageText, double progress)
{
    setStatus(status);
    if (m_stageText != stageText) {
        m_stageText = stageText;
        emit stageTextChanged();
    }
    setProgress(progress);
}

void AstrometryManager::setErrorMessage(const QString &message)
{
    if (m_errorMessage == message)
        return;
    m_errorMessage = message;
    emit errorMessageChanged();
}

void AstrometryManager::setProgress(double progress)
{
    progress = std::clamp(progress, 0.0, 1.0);
    if (qFuzzyCompare(m_progress, progress))
        return;
    m_progress = progress;
    emit progressChanged();
}

void AstrometryManager::updateElapsed()
{
    int seconds = static_cast<int>(m_elapsed.elapsed() / 1000);
    if (m_elapsedSeconds == seconds)
        return;
    m_elapsedSeconds = seconds;
    emit elapsedSecondsChanged();
}

void AstrometryManager::submitFits(const QString &fitsPath)
{
    if (m_running) {
        setErrorMessage(tr("Astrometry.net task is already running."));
        return;
    }

    if (!apiKeyConfigured()) {
        fail(tr("Astrometry.net API key is not configured."));
        return;
    }

    if (fitsPath.isEmpty() || !QFileInfo::exists(fitsPath)) {
        fail(tr("No FITS file is ready for submission."));
        return;
    }

    m_result->reset();
    m_pendingFitsPath = fitsPath;
    m_session.clear();
    m_downloads.clear();
    m_cancelRequested = false;
    m_elapsed.restart();
    m_elapsedSeconds = 0;
    emit elapsedSecondsChanged();
    setErrorMessage(QString());
    setRunning(true);
    m_elapsedTimer.start();
    m_result->setSourceFitsPath(fitsPath);
    login();
}

void AstrometryManager::cancel()
{
    if (!m_running)
        return;
    m_cancelRequested = true;
    m_pollTimer.stop();
    m_elapsedTimer.stop();
    setRunning(false);
    setStage(QStringLiteral("cancelled"), tr("Cancelled"), m_progress);
}

void AstrometryManager::postJson(const QString &endpoint, const QJsonObject &payload,
                                 const std::function<void(const QJsonObject &)> &handler)
{
    QNetworkRequest request(QUrl(QString::fromLatin1(ApiUrl) + endpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/x-www-form-urlencoded"));

    QUrlQuery query;
    query.addQueryItem(QStringLiteral("request-json"), QString::fromUtf8(QJsonDocument(payload).toJson(QJsonDocument::Compact)));
    QNetworkReply *reply = m_network.post(request, query.toString(QUrl::FullyEncoded).toUtf8());

    connect(reply, &QNetworkReply::finished, this, [this, reply, handler]() {
        reply->deleteLater();
        if (m_cancelRequested)
            return;
        bool ok = false;
        QJsonObject json = parseJsonReply(reply, &ok);
        if (ok)
            handler(json);
    });
}

void AstrometryManager::getJson(const QString &endpoint, const std::function<void(const QJsonObject &)> &handler)
{
    QNetworkReply *reply = m_network.get(QNetworkRequest(QUrl(QString::fromLatin1(ApiUrl) + endpoint)));
    connect(reply, &QNetworkReply::finished, this, [this, reply, handler]() {
        reply->deleteLater();
        if (m_cancelRequested)
            return;
        bool ok = false;
        QJsonObject json = parseJsonReply(reply, &ok);
        if (ok)
            handler(json);
    });
}

QJsonObject AstrometryManager::parseJsonReply(QNetworkReply *reply, bool *ok)
{
    *ok = false;
    if (reply->error() != QNetworkReply::NoError) {
        fail(reply->errorString());
        return {};
    }

    QJsonParseError parseError;
    QJsonDocument document = QJsonDocument::fromJson(reply->readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        fail(tr("Invalid Astrometry.net response."));
        return {};
    }

    *ok = true;
    return document.object();
}

void AstrometryManager::login()
{
    setStage(QStringLiteral("logging-in"), tr("Logging in to Astrometry.net"), 0.05);
    postJson(QStringLiteral("login"), {{QStringLiteral("apikey"), apiKey()}}, [this](const QJsonObject &json) {
        if (json.value(QStringLiteral("status")).toString() != QStringLiteral("success")) {
            fail(tr("Astrometry.net login failed."));
            return;
        }
        m_session = json.value(QStringLiteral("session")).toString();
        upload();
    });
}

void AstrometryManager::upload()
{
    setStage(QStringLiteral("uploading"), tr("Uploading FITS to Astrometry.net"), 0.2);

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QJsonObject payload{
        {QStringLiteral("session"), m_session},
        {QStringLiteral("publicly_visible"), QStringLiteral("y")},
        {QStringLiteral("allow_modifications"), QStringLiteral("d")},
        {QStringLiteral("allow_commercial_use"), QStringLiteral("d")},
    };

    QHttpPart jsonPart;
    jsonPart.setHeader(QNetworkRequest::ContentDispositionHeader, QStringLiteral("form-data; name=\"request-json\""));
    jsonPart.setBody(QJsonDocument(payload).toJson(QJsonDocument::Compact));
    multiPart->append(jsonPart);

    QFile *file = new QFile(m_pendingFitsPath);
    if (!file->open(QIODevice::ReadOnly)) {
        delete file;
        delete multiPart;
        fail(tr("Cannot open FITS for upload."));
        return;
    }
    file->setParent(multiPart);

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QStringLiteral("form-data; name=\"file\"; filename=\"upload.fits\""));
    filePart.setBodyDevice(file);
    multiPart->append(filePart);

    QNetworkReply *reply = m_network.post(QNetworkRequest(QUrl(QString::fromLatin1(ApiUrl) + QStringLiteral("upload"))), multiPart);
    multiPart->setParent(reply);

    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        if (m_cancelRequested)
            return;
        bool ok = false;
        QJsonObject json = parseJsonReply(reply, &ok);
        if (!ok)
            return;
        if (json.value(QStringLiteral("status")).toString() != QStringLiteral("success")) {
            fail(tr("Astrometry.net upload failed."));
            return;
        }

        m_result->setSubId(QString::number(json.value(QStringLiteral("subid")).toInteger()));
        emit identifiersChanged();
        setStage(QStringLiteral("waiting-for-job"), tr("Waiting for Astrometry.net job id"), 0.35);
        m_pollTimer.start();
        pollSubmission();
    });
}

void AstrometryManager::pollSubmission()
{
    getJson(QStringLiteral("submissions/%1").arg(m_result->subId()), [this](const QJsonObject &json) {
        QJsonArray jobs = json.value(QStringLiteral("jobs")).toArray();
        for (const QJsonValue &job : jobs) {
            if (!job.isNull() && !job.isUndefined()) {
                m_result->setJobId(QString::number(job.toInteger()));
                emit identifiersChanged();
                setStage(QStringLiteral("solving"), tr("Solving image on Astrometry.net"), 0.55);
                pollJob();
                return;
            }
        }
        setStage(QStringLiteral("waiting-for-job"), tr("Waiting for Astrometry.net job id"), 0.4);
    });
}

void AstrometryManager::pollJob()
{
    getJson(QStringLiteral("jobs/%1").arg(m_result->jobId()), [this](const QJsonObject &json) {
        QString status = json.value(QStringLiteral("status")).toString();
        if (status == QStringLiteral("success")) {
            m_pollTimer.stop();
            prepareOutputDirectory();
            startDownloads();
            return;
        }
        if (status == QStringLiteral("failure")) {
            fail(tr("Astrometry.net solve failed."));
            return;
        }
        setStage(QStringLiteral("solving"), tr("Solving image on Astrometry.net: %1").arg(status.isEmpty() ? tr("waiting") : status), 0.6);
    });
}

void AstrometryManager::prepareOutputDirectory()
{
    QDir root(QDir::current().filePath(QStringLiteral("astrometry")));
    if (!root.exists())
        root.mkpath(QStringLiteral("."));

    QString outputDir = root.filePath(m_result->jobId());
    QDir().mkpath(outputDir);
    m_result->setOutputDir(outputDir);
    emit outputDirChanged();

    QString uploadPath = QDir(outputDir).filePath(QStringLiteral("upload.fits"));
    QFile::remove(uploadPath);
    QFile::copy(m_pendingFitsPath, uploadPath);
    m_result->setUploadFitsPath(uploadPath);
}

void AstrometryManager::startDownloads()
{
    m_downloads.clear();
    m_downloads.enqueue({QStringLiteral("wcs_file"), QStringLiteral("result.wcs")});
    m_downloads.enqueue({QStringLiteral("axy_file"), QStringLiteral("axy.fits")});
    m_downloads.enqueue({QStringLiteral("rdls_file"), QStringLiteral("rdls.fits")});
    m_downloads.enqueue({QStringLiteral("image_rd_file"), QStringLiteral("image-radec.fits")});
    downloadNextFile();
}

void AstrometryManager::downloadNextFile()
{
    if (m_downloads.isEmpty()) {
        finishSuccess();
        return;
    }

    DownloadSpec spec = m_downloads.dequeue();
    double completed = 4.0 - m_downloads.size();
    setStage(QStringLiteral("downloading"), tr("Downloading %1").arg(spec.outputName), 0.65 + completed * 0.08);

    QUrl url(QStringLiteral("%1/%2/%3").arg(QString::fromLatin1(BaseUrl), spec.endpoint, m_result->jobId()));
    QNetworkReply *reply = m_network.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished, this, [this, reply, spec]() {
        reply->deleteLater();
        if (m_cancelRequested)
            return;
        if (reply->error() != QNetworkReply::NoError) {
            fail(reply->errorString());
            return;
        }

        QString path = QDir(m_result->outputDir()).filePath(spec.outputName);
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            fail(tr("Cannot save Astrometry.net result: %1").arg(file.errorString()));
            return;
        }
        file.write(reply->readAll());

        if (spec.outputName == QStringLiteral("result.wcs"))
            m_result->setWcsPath(path);
        else if (spec.outputName == QStringLiteral("axy.fits"))
            m_result->setAxyPath(path);
        else if (spec.outputName == QStringLiteral("rdls.fits"))
            m_result->setRdlsPath(path);
        else if (spec.outputName == QStringLiteral("image-radec.fits"))
            m_result->setImageRadecPath(path);

        downloadNextFile();
    });
}

void AstrometryManager::finishSuccess()
{
    QString metadataError;
    if (!m_result->saveMetadata(&metadataError)) {
        fail(metadataError);
        return;
    }

    m_elapsedTimer.stop();
    setRunning(false);
    setStage(QStringLiteral("success"), tr("Astrometry.net solve completed"), 1.0);
}

void AstrometryManager::fail(const QString &message)
{
    m_pollTimer.stop();
    m_elapsedTimer.stop();
    setRunning(false);
    setErrorMessage(message);
    setStage(QStringLiteral("failed"), message, m_progress);
}
