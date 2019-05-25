#include "imagedownloader.h"

#include <iostream>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include <QEventLoop>
#include <QFileInfo>
#include <QFile>

#include <QMutex>

#include <QMimeDatabase>
#include <QMimeType>

#include <QDir>


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

    QFile file(filename + ".tmp");
    file.open(QIODevice::ReadWrite);

    auto reply = manager->get(QNetworkRequest(url));

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

//        TO DO: try redownload images with error
//        QRunnable * downloader = new ImageDownloader(url, filename, io_mutex, statistic);
//        pool.start(downloader, 2);

        return;
    };

    file.flush();
    file.close();

    file.rename(filename + get_extension());

    if (QFileInfo::exists(filename + ".tmp")) {
        QFile::remove(filename + ".tmp");
    }

    io_mutex.lock();
    ++image_statistic->was_saved;
    io_mutex.unlock();
}

QString ImageDownloader::get_extension()
{
    QMimeDatabase db;
    QString extension;

    if (QStringList suffixes = db.mimeTypeForFile(filename + ".tmp").suffixes(); suffixes.length()){
        extension = "." + suffixes[0];
    }

    return extension;
}

bool ImageDownloader::check_exists()
{


    if (QFileInfo::exists(filename + ".tmp")) {
        QFile file(filename + ".tmp");
        file.remove();
    }

    QDir dir(QFileInfo(filename).path());
    auto list = dir.entryList({QFileInfo(filename).fileName() + "*"},  QDir::NoDotAndDotDot|QDir::AllDirs|QDir::Files);

    if (list.size()) {
        io_mutex.lock();
        ++image_statistic->count_of_already_exists;
        io_mutex.unlock();

        return true;
    } else {
        return false;
    }
}
