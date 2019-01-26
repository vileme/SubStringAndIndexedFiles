#include "scanworker.h"
#include <QDir>
#include <QCryptographicHash>
#include <QThread>
#include <QString>
#include <QMap>
#include <QDebug>
#include <QDirIterator>
ScanWorker::~ScanWorker(){
}
ScanWorker::ScanWorker(std::map<QString, std::map<QString, std::set<int64_t>>>* trigrams)
    :  trigrams(trigrams){
    directory_flags = {QDir::NoDotAndDotDot, QDir::Files};
}
void ScanWorker::get_string(QString const &string){
    substring = string;
    preprocess = new std::boyer_moore_horspool_searcher<QChar*, std::hash<QChar>,
                std::equal_to<void>>(substring.begin(), substring.end());
}

void ScanWorker::add_dir(QList<QString> const& directories){
    this->dirs = directories;
}
bool ScanWorker::substring_find(QString const& directory_name, QString const& file_name) {
    size_t directory_prefix = directory_name.size() - QDir(directory_name).dirName().size();
    QString relative_path = file_name.right(file_name.size() - directory_prefix);
    QFile file(file_name);
    if (!file.open(QFile::ReadOnly)) {
        return false;
    }
    const int size = substring.size();
    const int BUFFER_SIZE = 1 << 18;
    QList<int> coordinates;
    QTextStream stream(&file);
    QString buffer = stream.read(BUFFER_SIZE);
    int index = 0;
    while (buffer.size() > size - 1) {
        auto it = buffer.begin();
        while ((it = std::search(it, buffer.end(), *preprocess)) != buffer.end()) {
            coordinates.push_back(index + (it++ - buffer.begin()));

            if (QThread::currentThread()->isInterruptionRequested()) {
                break;
            }
        }
        if (QThread::currentThread()->isInterruptionRequested()) {
            break;
        }
        index += buffer.size() - size + 1;
        buffer = buffer.mid(buffer.size() - substring.size() + 1);
        buffer += stream.read(BUFFER_SIZE);
    }

    qRegisterMetaType<QList<int>>("coordinates");
    if (coordinates.size() > 0) {
        emit new_match(relative_path, coordinates);
    }
    return true;
}
void ScanWorker::scan_directory(QString const& directory_name) {
    size_t directory_prefix = directory_name.size() - QDir(directory_name).dirName().size();
    std::list<QString> files;
    for (auto i: (*trigrams)[directory_name]) {
        bool accept = true;
        int64_t needed = (((int64_t) substring[0].unicode()) << 16) +
                         (((int64_t) substring[1].unicode()) << 32);
        for (int j = 2; j < substring.size(); ++j) {
            needed = (needed >> 16) + (((int64_t) substring[j].unicode()) << 32);
            if (i.second.find(needed) == i.second.end()) {
                accept = false;
                break;
            }
        }

        if (accept) {
            files.push_back(i.first);
        }
    }
    for (auto i: files) {
        QString relative_path = i.right(i.size() - directory_prefix);
        if (!substring_find(directory_name, i)) {
            emit sendTroubles(relative_path);
        }
        if (QThread::currentThread()->isInterruptionRequested()) {
            return;
        }
        current += QFileInfo(i).size();

        double updateSize = double(current*100)/dir_size;
        emit update(updateSize);
    }
//    emit progress(directory_name, 100);
}
void ScanWorker::search(){
    dir_size = 0;
    current = 0;
    for(int i = 0; i < dirs.size();++i){
        dir_size+= countSize(dirs[i]);
    }
    if(substring.size()>=3&&trigrams!=nullptr){
        for(auto i :*trigrams){
            scan_directory(i.first);
            if(QThread::currentThread()->isInterruptionRequested()){
                break;
            }
        }

        callNotification("Warning! Search was made with the preprocess","Warning");
    }
    else {
    for(auto i:dirs){
        through_dirs(i,&ScanWorker::substring_find);
        if(QThread::currentThread()->isInterruptionRequested()){
            break;
        }
    }
    callNotification("Warning! Search was made without preprocess","Warning");
    }
    emit finished();
}

void ScanWorker::through_dirs(QString const& directory_name,
                                     std::function<bool(ScanWorker&, QString const&,
                                                        QString const&)> process) {
    size_t directory_prefix = directory_name.size() - QDir(directory_name).dirName().size();


    for (QDirIterator it(directory_name, directory_flags,QDirIterator::Subdirectories); it.hasNext(); ) {
        it.next();
        QString file_path = it.filePath();
        QString relative_path = file_path.right(file_path.size() - directory_prefix);
        if (!process(*this, directory_name, file_path)) {
            emit sendTroubles(relative_path);
        }
        current += it.fileInfo().size();
        double updateSize = double(current*100)/dir_size;
        emit update(updateSize);
        if (QThread::currentThread()->isInterruptionRequested()) {
            return;
        }
    }
}

qint64 ScanWorker::countSize(QString const &path){
    qint64 size = 0;
        QDir directory = QDir(path);
        directory.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot|QDir::NoSymLinks);
        QFileInfoList list = directory.entryInfoList();
        for(QFileInfo file_info : list){
            QString name = file_info.fileName();
            if(file_info.isDir()){
                size += countSize(path + "/" + name);
            }
            else size+=file_info.size();
        }
        return size;
}




