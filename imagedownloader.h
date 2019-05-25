#pragma once

#include <memory>

#include <QObject>
#include <QRunnable>
#include <QUrl>

#include "imagestatistics.h"

class QNetworkAccessManager;
class QNetworkReply;
class QMutex;

class ImageDownloader : public QObject, public QRunnable
{
    Q_OBJECT

    QNetworkAccessManager *manager = nullptr;
    QUrl url;
    QString filename;
    QMutex & io_mutex;
    std::shared_ptr<ImageStatistics> image_statistic;
    bool check_exists();
    QString get_extension();

public:
    void run() override;

    ImageDownloader(QUrl url, QString filename, QMutex &mutex, std::shared_ptr<ImageStatistics> image_statistic);
    ~ImageDownloader() override;
};
