#ifndef TRIGRAMMANAGER_H
#define TRIGRAMMANAGER_H

#include "trigramworker.h"

#include <QDir>
#include <QDirIterator>

#include <vector>


class TrigramManager : public QObject {
    Q_OBJECT

public:
    explicit TrigramManager(QObject *parent = nullptr);
    TrigramManager(std::set<QString> const& directories);
    ~TrigramManager();

signals:
    void result(std::map<QString, std::map<QString, std::set<int64_t>>>* result);
    void throw_progress(int value);
    void throw_error(QString const& file_name);
    void finished();
    void cancel();
    void notification(QString const&message);

public slots:
    void manage_trigrams();
    void canceled();

private slots:
    void ready(std::map<QString, std::map<QString, std::set<int64_t>>>* res);
    void catch_error(QString const& file_name);

private:
    void progress(int value);
    void make_worker();
    qint64 size;

    QFlags<QDirIterator::IteratorFlag> iterator_flags;
    QFlags<QDir::Filter> directory_flags;
    std::map<QString, int> sizes;
    std::map<QString, int> scan_progress;


    std::vector<std::pair<int64_t, std::pair<QString, QString>>> files;
    std::set<QString> directories;
    std::map<QString, std::map<QString, std::set<int64_t>>>* trigrams = nullptr;
    std::vector<TrigramWorker*> worker;
    size_t workers_ready = 0;
};

#endif // TRIGRAMMANAGER_H
