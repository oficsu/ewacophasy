#pragma once
#include <QObject>
#include <QRunnable>
#include <QMutex>
#include <QUrl>
#include <memory>
#include <set>

class QNetworkAccessManager;
class QNetworkReply;
class QMutex;
class QThreadPool;

class PageDownloader : public QObject, public QRunnable
{
    Q_OBJECT

    QNetworkAccessManager *manager = nullptr;
    QUrl url;
    QString save_path;
    int depth;
    int max_depth;
    QMutex & io_mutex;
    static QMutex pages_history_mutex;
    QThreadPool& pool;

    static std::set<QString> history_pages;
    static std::set<QString> const trusted_hosts;
public:
    static QByteArray user_agent;
public:
    void run() override;
    void start_next_pages(const std::vector<QUrl>& pages);
    void start_next_images(const std::vector<QUrl>& images);

    PageDownloader(const QUrl url, const QString save_path, int depth, int max_depth, QMutex &mutex, QThreadPool& pool);
    std::tuple<std::vector<QUrl>, std::vector<QUrl>> page_parser(const QByteArray & page);
    ~PageDownloader() override;
};
