#pragma once

#include <QImage>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QUrl>

#include <vector>

class FitsImageProvider;

class FitsManager : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)
    Q_PROPERTY(QString imageSource READ imageSource NOTIFY imageSourceChanged)
    Q_PROPERTY(QString tempFitsPath READ tempFitsPath NOTIFY tempFitsPathChanged)

public:
    explicit FitsManager(QObject *parent = nullptr);

    QString status() const;
    QString errorMessage() const;
    QString imageSource() const;
    QString tempFitsPath() const;

    void setImageProvider(FitsImageProvider *provider);

    Q_INVOKABLE void loadFile(const QUrl &fileUrl);
    Q_INVOKABLE void clearError();

signals:
    void statusChanged();
    void errorMessageChanged();
    void imageSourceChanged();
    void tempFitsPathChanged();

private:
    struct FitsData {
        QImage image;
        std::vector<float> rawPixels;
        int width = 0;
        int height = 0;
    };

    FitsData readFits2dHdu(const QString &filePath);
    QString writeTempPrimaryFits(const FitsData &data);
    QImage convertToQImage(const FitsData &data);

    void setStatus(const QString &status);
    void setErrorMessage(const QString &message);

    FitsImageProvider *m_provider = nullptr;
    QString m_status = QStringLiteral("ready");
    QString m_errorMessage;
    QString m_imageSource;
    QString m_tempFitsPath;
    int m_imageVersion = 0;
};
