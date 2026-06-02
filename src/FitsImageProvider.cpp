#include "FitsImageProvider.h"

#include <QMutexLocker>

FitsImageProvider::FitsImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage FitsImageProvider::requestImage(const QString &id, QSize *size,
                                       const QSize &requestedSize)
{
    Q_UNUSED(id)
    QMutexLocker lock(&m_mutex);

    if (size)
        *size = m_image.size();

    if (requestedSize.isValid() && !m_image.isNull())
        return m_image.scaled(requestedSize, Qt::KeepAspectRatio,
                              Qt::SmoothTransformation);

    return m_image;
}

void FitsImageProvider::updateImage(const QImage &image)
{
    QMutexLocker lock(&m_mutex);
    m_image = image;
}
