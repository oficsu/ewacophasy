#include <iostream>
#include <map>

#include <QCoreApplication>

#include <QThreadPool>
#include <QSettings>
#include <QDirIterator>
#include <QDir>
#include <QMutex>

#include "pagedownloader.h"

enum SettingsKeys {
    exported_channel_directory,
    save_directory,
    maximun_threads_count,
    recursion_depth,
    user_agent,
};

std::map<SettingsKeys, QString> key_map {
    {exported_channel_directory, "exported_channel_directory"},
    {save_directory, "save_directory"},
    {maximun_threads_count, "maximun_threads_count"},
    {recursion_depth, "recursion_depth"},
    {user_agent, "user_agent"},
};

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QThreadPool pool;



    QSettings settings("parser.ini", QSettings::IniFormat);

    bool has_settings = true;
    QString export_dir;
    QString save_dir;
    int max_threads;
    int depth;


    SettingsKeys key;

    auto empty_value = [&]() {
        return !settings.contains(key_map[key]) || settings.value(key_map[key]).toString().isEmpty();
    };

    key = exported_channel_directory;
    if (empty_value()) {
        has_settings = false;
        settings.setValue(key_map[key], "");
    } else {
        export_dir = settings.value(key_map[key]).toString();
    }

    key = save_directory;
    if (empty_value()) {
        has_settings = false;
        settings.setValue(key_map[key], "");
    } else {
        save_dir = settings.value(key_map[key]).toString();
    }

    key = maximun_threads_count;
    if (empty_value()) {
        settings.setValue(key_map[key], 2);
        max_threads = 2;
    } else {
        max_threads = settings.value(key_map[key]).toInt();
    }

    key = recursion_depth;
    if (empty_value()) {
        settings.setValue(key_map[key], 4);
        depth = 4;
    } else {
        depth = settings.value(key_map[key]).toInt();
    }

    key = user_agent;
    if (empty_value()) {
        settings.setValue(key_map[key], "");
        depth = 4;
    } else {
        PageDownloader::user_agent = settings.value(key_map[key]).toString().toLocal8Bit();
    }

    if (!has_settings) {
        std::cerr << "You need to enter parsing settings in" << "\n";
        std::cerr << "    " << settings.fileName().toStdString() << std::endl;
        return 0;
    }

    pool.setMaxThreadCount(max_threads);

    QDir exported_channel_dir(export_dir);
    QMutex io_mutex;

    QDirIterator it(exported_channel_dir.absolutePath(), QStringList("*.html"), QDir::Files, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        QString path = it.next();
        QUrl url = QUrl::fromLocalFile(path);
        PageDownloader *page_downloader = new PageDownloader(url, save_dir, 0, depth, io_mutex, pool);
        pool.start(page_downloader);
    }


    return a.exec();
}
