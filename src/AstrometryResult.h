#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>

class AstrometryResult : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString sourceFitsPath READ sourceFitsPath NOTIFY changed)
    Q_PROPERTY(QString uploadFitsPath READ uploadFitsPath NOTIFY changed)
    Q_PROPERTY(QString subId READ subId NOTIFY changed)
    Q_PROPERTY(QString jobId READ jobId NOTIFY changed)
    Q_PROPERTY(QString status READ status NOTIFY changed)
    Q_PROPERTY(QString outputDir READ outputDir NOTIFY changed)
    Q_PROPERTY(QString wcsPath READ wcsPath NOTIFY changed)
    Q_PROPERTY(QString axyPath READ axyPath NOTIFY changed)
    Q_PROPERTY(QString rdlsPath READ rdlsPath NOTIFY changed)
    Q_PROPERTY(QString imageRadecPath READ imageRadecPath NOTIFY changed)
    Q_PROPERTY(QString createdAt READ createdAt NOTIFY changed)
    Q_PROPERTY(QString updatedAt READ updatedAt NOTIFY changed)

public:
    explicit AstrometryResult(QObject *parent = nullptr);

    QString sourceFitsPath() const;
    QString uploadFitsPath() const;
    QString subId() const;
    QString jobId() const;
    QString status() const;
    QString outputDir() const;
    QString wcsPath() const;
    QString axyPath() const;
    QString rdlsPath() const;
    QString imageRadecPath() const;
    QString createdAt() const;
    QString updatedAt() const;

    void reset();
    void setSourceFitsPath(const QString &path);
    void setUploadFitsPath(const QString &path);
    void setSubId(const QString &subId);
    void setJobId(const QString &jobId);
    void setStatus(const QString &status);
    void setOutputDir(const QString &path);
    void setWcsPath(const QString &path);
    void setAxyPath(const QString &path);
    void setRdlsPath(const QString &path);
    void setImageRadecPath(const QString &path);

    QJsonObject toJson() const;
    bool saveMetadata(QString *errorMessage = nullptr) const;

signals:
    void changed();

private:
    void touch();

    QString m_sourceFitsPath;
    QString m_uploadFitsPath;
    QString m_subId;
    QString m_jobId;
    QString m_status;
    QString m_outputDir;
    QString m_wcsPath;
    QString m_axyPath;
    QString m_rdlsPath;
    QString m_imageRadecPath;
    QString m_createdAt;
    QString m_updatedAt;
};
