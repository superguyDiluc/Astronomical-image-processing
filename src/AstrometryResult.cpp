#include "AstrometryResult.h"

#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QJsonDocument>

namespace {
QString utcNow()
{
    return QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
}
}

AstrometryResult::AstrometryResult(QObject *parent)
    : QObject(parent)
{
    reset();
}

QString AstrometryResult::sourceFitsPath() const { return m_sourceFitsPath; }
QString AstrometryResult::uploadFitsPath() const { return m_uploadFitsPath; }
QString AstrometryResult::subId() const { return m_subId; }
QString AstrometryResult::jobId() const { return m_jobId; }
QString AstrometryResult::status() const { return m_status; }
QString AstrometryResult::outputDir() const { return m_outputDir; }
QString AstrometryResult::wcsPath() const { return m_wcsPath; }
QString AstrometryResult::axyPath() const { return m_axyPath; }
QString AstrometryResult::rdlsPath() const { return m_rdlsPath; }
QString AstrometryResult::imageRadecPath() const { return m_imageRadecPath; }
QString AstrometryResult::createdAt() const { return m_createdAt; }
QString AstrometryResult::updatedAt() const { return m_updatedAt; }

void AstrometryResult::reset()
{
    m_sourceFitsPath.clear();
    m_uploadFitsPath.clear();
    m_subId.clear();
    m_jobId.clear();
    m_status = QStringLiteral("idle");
    m_outputDir.clear();
    m_wcsPath.clear();
    m_axyPath.clear();
    m_rdlsPath.clear();
    m_imageRadecPath.clear();
    m_createdAt = utcNow();
    m_updatedAt = m_createdAt;
    emit changed();
}

void AstrometryResult::touch()
{
    m_updatedAt = utcNow();
    emit changed();
}

void AstrometryResult::setSourceFitsPath(const QString &path) { m_sourceFitsPath = path; touch(); }
void AstrometryResult::setUploadFitsPath(const QString &path) { m_uploadFitsPath = path; touch(); }
void AstrometryResult::setSubId(const QString &subId) { m_subId = subId; touch(); }
void AstrometryResult::setJobId(const QString &jobId) { m_jobId = jobId; touch(); }
void AstrometryResult::setStatus(const QString &status) { m_status = status; touch(); }
void AstrometryResult::setOutputDir(const QString &path) { m_outputDir = path; touch(); }
void AstrometryResult::setWcsPath(const QString &path) { m_wcsPath = path; touch(); }
void AstrometryResult::setAxyPath(const QString &path) { m_axyPath = path; touch(); }
void AstrometryResult::setRdlsPath(const QString &path) { m_rdlsPath = path; touch(); }
void AstrometryResult::setImageRadecPath(const QString &path) { m_imageRadecPath = path; touch(); }

QJsonObject AstrometryResult::toJson() const
{
    return {
        {QStringLiteral("sourceFitsPath"), m_sourceFitsPath},
        {QStringLiteral("uploadFitsPath"), m_uploadFitsPath},
        {QStringLiteral("subId"), m_subId},
        {QStringLiteral("jobId"), m_jobId},
        {QStringLiteral("status"), m_status},
        {QStringLiteral("outputDir"), m_outputDir},
        {QStringLiteral("wcsPath"), m_wcsPath},
        {QStringLiteral("axyPath"), m_axyPath},
        {QStringLiteral("rdlsPath"), m_rdlsPath},
        {QStringLiteral("imageRadecPath"), m_imageRadecPath},
        {QStringLiteral("createdAt"), m_createdAt},
        {QStringLiteral("updatedAt"), m_updatedAt},
    };
}

bool AstrometryResult::saveMetadata(QString *errorMessage) const
{
    if (m_outputDir.isEmpty())
        return true;

    QDir dir(m_outputDir);
    if (!dir.exists() && !dir.mkpath(QStringLiteral("."))) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot create output directory: %1").arg(m_outputDir);
        return false;
    }

    QFile file(dir.filePath(QStringLiteral("job.json")));
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        if (errorMessage)
            *errorMessage = QStringLiteral("Cannot write job metadata: %1").arg(file.errorString());
        return false;
    }

    file.write(QJsonDocument(toJson()).toJson(QJsonDocument::Indented));
    return true;
}
