#include "trigrammanager.h"

#include <QThread>

TrigramManager::TrigramManager(QObject *parent) : QObject(parent) {}

TrigramManager::~TrigramManager() {}

TrigramManager::TrigramManager(std::set<QString> const& directories):directories(directories) {

    directory_flags = {QDir::NoDotAndDotDot, QDir::Files};
    trigrams = new std::map<QString, std::map<QString, std::set<int64_t>>>;
}

void TrigramManager::manage_trigrams() {
//    emit notification();
    for (auto directory_name: directories) {
        sizes[directory_name] = 0;
        scan_progress[directory_name] = 0;
        for (QDirIterator it(directory_name, directory_flags, QDirIterator::Subdirectories); it.hasNext(); ) {
            it.next();
            files.emplace_back(it.fileInfo().size(), std::make_pair(directory_name, it.filePath()));
            sizes[directory_name]++;
            if (QThread::currentThread()->isInterruptionRequested()) {
                return;
            }
        }
        if (sizes[directory_name] == 0) {
            emit throw_progress(100);
        }
    }

    make_worker();
}

void TrigramManager::make_worker() {
    size = 0;
    for(auto i:files)
    {
        size +=i.first;
    }
    TrigramWorker* new_worker = new TrigramWorker(nullptr,size);
    QThread* thread = new QThread();
    for (size_t i = 0;i < files.size(); ++i) {
        new_worker->files.push_back(files[i].second);
    }
    new_worker->moveToThread(thread);

    connect(this, &TrigramManager::result, new_worker, &TrigramWorker::deleteLater);
    connect(this, &TrigramManager::result, thread, &QThread::quit);
    connect(this, &TrigramManager::cancel, thread, &QThread::requestInterruption);
    connect(thread, &QThread::finished, thread, &QThread::deleteLater);
    connect(thread, &QThread::started, new_worker, &TrigramWorker::process_files);
    connect(new_worker, &TrigramWorker::files_processed, this, &TrigramManager::ready);
    connect(new_worker, &TrigramWorker::throw_error, this, &TrigramManager::catch_error);
    connect(new_worker,&TrigramWorker::update,this,&TrigramManager::progress);
    thread->start();

}

void TrigramManager::canceled() {
    emit cancel();
    emit finished();
}

void TrigramManager::catch_error(QString const& file_name) {
    emit throw_error(file_name);
}

void TrigramManager::ready(std::map<QString, std::map<QString, std::set<int64_t>>>* res) {
    for (auto i: directories) {
       (*trigrams)[i].insert((*res)[i].begin(), (*res)[i].end());
    }
    emit result(trigrams);
    emit finished();
}

void TrigramManager::progress(int value) {
    emit throw_progress(value);
}
