#ifndef SCANWORKER_H
#define SCANWORKER_H

#include <QString>
#include <QObject>
#include "qcharhash.cpp"
#include <QMap>
#include <QVector>
#include <QSet>
#include <functional>
#include <QDir>
#include <cstdint>
#include <algorithm>
#include <set>
#include <map>

class ScanWorker: public QObject{

    Q_OBJECT

public:~ScanWorker();

    explicit ScanWorker(std::map<QString, std::map<QString, std::set<int64_t>>>* trigrams);

signals:
    void update(int value);
    void callNotification(QString const& message,
                          const char* window_title);
    void sendTroubles(QString const& file);
    void finished();
    void new_match(QString const& file_name, QList<int> const& coordinates);

public slots:
    void get_string(QString const& string);
    void add_dir(QList<QString> const& directories);
    void search();



private:
    void scan_directory(QString const& directory_name);
    qint64 dir_size;
    qint64 current;
    qint64 countSize(QString const &path);
    QFlags<QDir::Filter> directory_flags;
    QString substring;
    QList<QString> dirs;
    std::map<QString, std::map<QString, std::set<int64_t>>>* trigrams = nullptr;
    std::boyer_moore_horspool_searcher<QChar*, std::hash<QChar>, std::equal_to<void>>* preprocess = nullptr;
    bool substring_find(QString const& directory_name, QString const& file_name);
    void through_dirs(QString const& directory_name,std::function<bool(ScanWorker&, QString const&, QString const&)> process);
};



#endif // SCANWORKER_H
