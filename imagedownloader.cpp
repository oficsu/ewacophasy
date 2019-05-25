#include "imagedownloader.h"
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QtNetwork/QNetworkRequest>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <iostream>
#include <QMutex>

ImageDownloader::ImageDownloader(QUrl url, QString filename, QMutex& mutex, std::shared_ptr<ImageStatistics> image_statistic)
    : url(url)
    , filename(filename)
    , io_mutex(mutex)
    , image_statistic(image_statistic)
{

}

ImageDownloader::~ImageDownloader()
{
    manager->deleteLater();
    io_mutex.lock();
    bool last_image = (image_statistic->was_saved
                       + image_statistic->downloading_error_count
                       + image_statistic->count_of_already_exists)
                          == image_statistic->total_images;
    io_mutex.unlock();

    if (last_image)
    {
        ImageStatistics stat = *image_statistic;
        std::clog << "Download completed "
                  << "["
                      << stat.was_saved
                      << " / "
                      << image_statistic->total_images
                      << (image_statistic->count_of_already_exists ? ", " + std::to_string(image_statistic->count_of_already_exists) + " already exist" : "")
                      << (image_statistic->downloading_error_count ? ", " + std::to_string(image_statistic->downloading_error_count) + " errors" : "")
                  << "]: "
                  << filename.section("/",-2, -2).toStdString()
                  << std::endl;
    }
}

void ImageDownloader::run()
{
    manager = new QNetworkAccessManager();
    manager->setRedirectPolicy(QNetworkRequest::RedirectPolicy::NoLessSafeRedirectPolicy);

    if (check_exists()) return;

    auto reply = manager->get(QNetworkRequest(url));

    QFile file(filename);
    file.open(QIODevice::WriteOnly);

    QEventLoop loop;
    connect(reply, &QNetworkReply::readyRead, [&](){ file.write(reply->readAll()); });
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error()) {
        io_mutex.lock();
        ++image_statistic->downloading_error_count;
        io_mutex.unlock();

        if (file.isOpen()) file.close();

        file.remove();

        return;
    }

    ++image_statistic->was_saved;
   file.close();
}

bool ImageDownloader::check_exists()
{
    if (!QFileInfo::exists(filename)) return false;

    io_mutex.lock();
    ++image_statistic->count_of_already_exists;
    io_mutex.unlock();

    return true;
}
