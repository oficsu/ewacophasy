#pragma once
#include <QObject>
#include <QRunnable>
#include <QUrl>
#include <memory>

#include "imagestatistics.h"

class QNetworkAccessManager;
class QMutex;
class QNetworkReply;

class ImageDownloader : public QObject, public QRunnable
{
    Q_OBJECT

    QNetworkAccessManager *manager = nullptr;
    QUrl url;
    QString filename;
    QMutex & io_mutex;
    std::shared_ptr<ImageStatistics> image_statistic;
public:
    void run() override;

    bool check_exists();
    ImageDownloader(QUrl url, QString filename, QMutex &mutex, std::shared_ptr<ImageStatistics> image_statistic);
    ~ImageDownloader() override;
};
