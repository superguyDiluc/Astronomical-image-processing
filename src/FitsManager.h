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
    Q_PROPERTY(float minValue READ minValue WRITE setMinValue NOTIFY minValueChanged)
    Q_PROPERTY(float maxValue READ maxValue WRITE setMaxValue NOTIFY maxValueChanged)
    Q_PROPERTY(float pixelMin READ pixelMin NOTIFY pixelRangeChanged)
    Q_PROPERTY(float pixelMax READ pixelMax NOTIFY pixelRangeChanged)

public:
    explicit FitsManager(QObject *parent = nullptr);

    QString status() const;
    QString errorMessage() const;
    QString imageSource() const;
    QString tempFitsPath() const;
    float minValue() const;
    float maxValue() const;
    float pixelMin() const;
    float pixelMax() const;

    void setImageProvider(FitsImageProvider *provider);
    void setMinValue(float value);
    void setMaxValue(float value);

    Q_INVOKABLE void loadFile(const QUrl &fileUrl);
    Q_INVOKABLE void clearError();
    Q_INVOKABLE void applyGrayTransform();
    Q_INVOKABLE void autoAdjust();

signals:
    void statusChanged();
    void errorMessageChanged();
    void imageSourceChanged();
    void tempFitsPathChanged();
    void minValueChanged();
    void maxValueChanged();
    void pixelRangeChanged();

private:
    struct FitsData {
        std::vector<float> rawPixels;
        int width = 0;
        int height = 0;
    };

    FitsData readFits2dHdu(const QString &filePath);
    QString writeTempPrimaryFits(const FitsData &data);
    QImage convertToQImage(const FitsData &data, float displayMin, float displayMax);
    void refreshDisplay();
    void computePixelRange();

    FitsImageProvider *m_provider = nullptr;
    QString m_status = QStringLiteral("ready");
    QString m_errorMessage;
    QString m_imageSource;
    QString m_tempFitsPath;
    int m_imageVersion = 0;

    FitsData m_currentData;
    float m_minValue = 0.0f;
    float m_maxValue = 65535.0f;
    float m_pixelMin = 0.0f;
    float m_pixelMax = 65535.0f;

    void setStatus(const QString &status);
    void setErrorMessage(const QString &message);
};
