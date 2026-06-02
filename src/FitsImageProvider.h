#pragma once

#include <QImage>
#include <QMutex>
#include <QQuickImageProvider>

class FitsImageProvider : public QQuickImageProvider
{
public:
    explicit FitsImageProvider();

    QImage requestImage(const QString &id, QSize *size,
                        const QSize &requestedSize) override;

    void updateImage(const QImage &image);

private:
    QMutex m_mutex;
    QImage m_image;
};
