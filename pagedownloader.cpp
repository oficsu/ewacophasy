#include "pagedownloader.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QUrl>
#include <QtNetwork/QNetworkRequest>
#include <QEventLoop>
#include <QFile>
#include <iostream>
#include <QMutex>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QThreadPool>
#include <QDir>
#include <utility>
#include <QDebug>

#include "imagedownloader.h"
#include "imagestatistics.h"

PageDownloader::PageDownloader(const QUrl url, const QString save_path, int depth, int max_depth, QMutex &mutex, QThreadPool &pool)
    : url(url)
    , save_path(save_path)
    , depth(depth)
    , max_depth(max_depth)
    , io_mutex(mutex)
    , pool(pool)
{

}

std::tuple<std::vector<QUrl>, std::vector<QUrl>> PageDownloader::page_parser(const QByteArray &page)
{
    std::vector<QUrl> next_pages;
    std::vector<QUrl> images;

    QRegularExpression find_all_img(R"===(<img[^>]* src=\"([^\"]*)\"[^>]*>)===", QRegularExpression::PatternOption::MultilineOption);
    QRegularExpression find_all_href(R"===(<a[^>]* href=\"([^\"]*)\"[^>]*>)===", QRegularExpression::PatternOption::MultilineOption);

    auto img_it = find_all_img.globalMatch(page);
    while (img_it.hasNext()) {
        QRegularExpressionMatch match = img_it.next();
        QString src = match.captured(1);
        images.push_back(url.resolved(src));
    }

    auto href_it = find_all_href.globalMatch(page);
    while (href_it.hasNext()) {
        QRegularExpressionMatch match = href_it.next();
        QUrl href = url.resolved(match.captured(1));
        if (trusted_hosts.count(href.host())) {
            next_pages.push_back(url.resolved(href));
        }
    }

    return std::make_tuple(next_pages, images);
}
QByteArray PageDownloader::user_agent;
QMutex PageDownloader::pages_history_mutex;
std::set<QString> PageDownloader::history_pages;

std::set<QString> const PageDownloader::trusted_hosts {
    "telegra.ph",
};

PageDownloader::~PageDownloader()
{
    manager->deleteLater();
}

void PageDownloader::run()
{
    manager = new QNetworkAccessManager();
    manager->setRedirectPolicy(QNetworkRequest::RedirectPolicy::NoLessSafeRedirectPolicy);

    QByteArray page;

    pages_history_mutex.lock();
    if(history_pages.count(url.path())) {
        return;
    } else {
        history_pages.insert(url.path());
    }
    pages_history_mutex.unlock();

    if (!url.isLocalFile()) {
        QNetworkRequest request(url);

        if (!user_agent.isEmpty()) {
            request.setRawHeader("User-Agent", user_agent);
        }

        auto reply = manager->get(request);

        QEventLoop loop;


        connect(reply, &QNetworkReply::readyRead, [&]() { page.append(reply->readAll()); });
        connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
        loop.exec();

        if (reply->error()) {
            io_mutex.lock();
            std::cerr << "Download failed: " << reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt() << "\n";
            std::cerr << "      " << url.toString().toStdString() << std::endl;
            io_mutex.unlock();

            return;
        }
    } else {
        QString file_name = url.toLocalFile();
        QFile file(file_name);
        if (file.open(QIODevice::ReadOnly)) {
            page = file.readAll();
        } else {
            io_mutex.lock();
            std::cerr << "File open error: " << url.toString().toStdString() << std::endl;
            io_mutex.unlock();
            return;
        }
    }



    auto [next_pages, next_images] = page_parser(page);
    start_next_images(next_images);
    start_next_pages(next_pages);
}

void PageDownloader::start_next_pages(const std::vector<QUrl>& pages)
{
    if (depth >= max_depth) return;

    for (auto page: pages) {

        QUrl page_url = url.resolved(page);

        QRunnable * downloader = new PageDownloader(page_url, save_path, depth + 1, max_depth, io_mutex, pool);
        pool.start(downloader);
    }

}

void PageDownloader::start_next_images(const std::vector<QUrl> &images)
{
    if (!images.size()) return;

    QDir dir(save_path);
    QString save_directory = url.path().section("/", -1, -1);

    if (!dir.mkpath(save_directory)) {
        io_mutex.lock();
        std::cout << "Cannot create directory:" << save_path.toStdString() << "/" << save_directory.toStdString() << std::endl;
        io_mutex.unlock();

        return;
    }

    if (!dir.cd(save_directory)) {
        io_mutex.lock();
        std::cout << "Cd error:" << save_path.toStdString() << "/" << save_directory.toStdString() << std::endl;
        io_mutex.unlock();

        return;
    }


    auto remained_from = std::make_shared<ImageStatistics>();
    *remained_from = {images.size(), 0, 0, 0};

    for (size_t i = 0; i != images.size(); ++i) {
        QString file_name = save_path + "/" + save_directory + "/image_" + QString::number(i);

        QUrl image_url = url.resolved(images[i]);
        QRunnable * downloader = new ImageDownloader(image_url, file_name, io_mutex, remained_from);
        pool.start(downloader, 2);
    }
}
