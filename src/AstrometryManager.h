#pragma once

#include "AstrometryResult.h"

#include <QElapsedTimer>
#include <QNetworkAccessManager>
#include <QObject>
#include <QQueue>
#include <QTimer>

#include <functional>

class QNetworkReply;

class AstrometryManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool apiKeyConfigured READ apiKeyConfigured NOTIFY apiKeyConfiguredChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString stageText READ stageText NOTIFY stageTextChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString subId READ subId NOTIFY identifiersChanged)
    Q_PROPERTY(QString jobId READ jobId NOTIFY identifiersChanged)
    Q_PROPERTY(QString outputDir READ outputDir NOTIFY outputDirChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(int elapsedSeconds READ elapsedSeconds NOTIFY elapsedSecondsChanged)
    Q_PROPERTY(AstrometryResult *result READ result CONSTANT)

public:
    explicit AstrometryManager(QObject *parent = nullptr);

    bool apiKeyConfigured() const;
    bool running() const;
    QString status() const;
    QString stageText() const;
    QString errorMessage() const;
    QString subId() const;
    QString jobId() const;
    QString outputDir() const;
    double progress() const;
    int elapsedSeconds() const;
    AstrometryResult *result() const;

    Q_INVOKABLE QString apiKeyPreview() const;
    Q_INVOKABLE void saveApiKey(const QString &apiKey);
    Q_INVOKABLE void clearApiKey();
    Q_INVOKABLE void submitFits(const QString &fitsPath);
    Q_INVOKABLE void cancel();

signals:
    void apiKeyConfiguredChanged();
    void runningChanged();
    void statusChanged();
    void stageTextChanged();
    void errorMessageChanged();
    void identifiersChanged();
    void outputDirChanged();
    void progressChanged();
    void elapsedSecondsChanged();

private:
    struct DownloadSpec {
        QString endpoint;
        QString outputName;
    };

    void setRunning(bool running);
    void setStatus(const QString &status);
    void setStage(const QString &status, const QString &stageText, double progress);
    void setErrorMessage(const QString &message);
    void setProgress(double progress);
    void updateElapsed();

    QString apiKey() const;
    void postJson(const QString &endpoint, const QJsonObject &payload,
                  const std::function<void(const QJsonObject &)> &handler);
    void getJson(const QString &endpoint, const std::function<void(const QJsonObject &)> &handler);
    QJsonObject parseJsonReply(QNetworkReply *reply, bool *ok);

    void login();
    void upload();
    void pollSubmission();
    void pollJob();
    void prepareOutputDirectory();
    void startDownloads();
    void downloadNextFile();
    void finishSuccess();
    void fail(const QString &message);

    QNetworkAccessManager m_network;
    QTimer m_pollTimer;
    QTimer m_elapsedTimer;
    QElapsedTimer m_elapsed;
    AstrometryResult *m_result = nullptr;
    QQueue<DownloadSpec> m_downloads;

    QString m_session;
    QString m_pendingFitsPath;
    QString m_status = QStringLiteral("idle");
    QString m_stageText = QStringLiteral("Idle");
    QString m_errorMessage;
    double m_progress = 0.0;
    bool m_running = false;
    int m_elapsedSeconds = 0;
    bool m_cancelRequested = false;
};
