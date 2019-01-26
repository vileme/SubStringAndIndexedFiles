#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "deletewindow.h"
#include "scanworker.h"
#include "trigrammanager.h"
#include "progressbar.h"
#include <memory>
#include <QMap>
#include <QPair>
#include <QTreeWidgetItem>
#include <QSet>
#include <QThread>
#include <QProgressBar>
#include <set>
#include <map>
#include <QFileSystemWatcher>
namespace Ui {
class MainWindow;
}
class main_window : public QMainWindow
{
    Q_OBJECT

public:
    explicit main_window(QWidget *parent = 0);

    Ui::MainWindow *ui;
    QTreeWidget *WidgetForDirs;
    QTreeWidget *treeWidget;
    void notification(QString const& message,
            const char* window_title);
    ~main_window();

public slots:
    void finished_process();
    void prepared(std::map<QString, std::map<QString,
                  std::set<int64_t>>>* result);
    void prepare();

    void output_match(QString const& file_name, QList<int> const& coordinates);
    void ready();

    void scan_directories();

    void setProgressValue(const int value);


    void select_directory();

    void getNotification(QString const& message,
                         const char* window_title);

    void getTroubles(QString const & file);

    void deleteDirs();

    void dir_to_delete(QTreeWidgetItem* chosenDirToDelete);

    void deletePrep();

    void catchnotification(QString const &message);

    void dir_changed(const QString  &path);

    void file_changed(const QString  &path);

    void reindex();

    void swap();

    void swapPrep();

    void firstBool();



signals:
private:
    void addToWatcher(QString const&path);
    int count = 0;
    qint64 countSize(QString const &path);

    void add_directory(QString const& dir);

    void changeButtons();

    void makeProgressBar();

    void show_about_dialog();

    void checkForMatch(QString const& absolutePath);

    void createThread();



    std::set<QString> directoriesForPrep;

    QSet<QPair<QString, QTreeWidgetItem*>> files_to_remove;

    QSet<QTreeWidgetItem *> dir_to_remove;

    std::map<QString, std::map<QString, std::set<int64_t>>>* preprocessing = nullptr;


    QThread *workerThread;

    QThread  *thread;

    TrigramManager* tm;


    ScanWorker *worker;

    QProgressBar *progress;

    QFileSystemWatcher *watcher;

    DeleteWindow *window;
    bool first = false;
    bool isScanning = false;
    bool isPrescanning = false;

};

#endif // MAINWINDOW_H
