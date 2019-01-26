#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "scanworker.h"
#include <QCommonStyle>
#include <QDesktopWidget>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QProgressBar>
#include <QStatusBar>
#include <QtWidgets>
#include "deletewindow.h"
#include <QFileSystemWatcher>
main_window::main_window(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, size(), qApp->desktop()->availableGeometry()));

    ui->WidgetForDirs->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui->treeWidget->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui->treeWidget->header()->setSectionResizeMode(1,QHeaderView::Interactive);
    QSplitter *splitter = new QSplitter;
    splitter->addWidget(ui->WidgetForDirs);
    splitter->addWidget(ui->treeWidget);
    ui->horizontalLayout->addWidget(splitter);
    watcher = new QFileSystemWatcher(this);

    QCommonStyle style;
    setWindowTitle("File Scanning Application");

    statusBar()->showMessage("© Copyright Kozyrev Vlad 2018");


    QLineEdit* edit(new QLineEdit(this));
    edit->setMaximumWidth(150);
    ui->toolBar->addWidget(edit);
    ui->actionAdd_Directory->setIcon(QIcon(":/img/257832.png"));
    ui->action_Cancel->setIcon(QIcon(":/img/iconfinder_error_1646012.png"));
    ui->action_DeleteDir->setIcon(QIcon(":/img/remove-folder-circular-button.png"));
    ui->actionPrepare->setIcon(QIcon(":/img/icon1.png"));
    ui->action_Cancel->setDisabled(true);
    ui->actionDelete_Files->setDisabled(true);
    ui->action_DeleteDir->setDisabled(true);
    ui->actionExit->setIcon(style.standardIcon(QCommonStyle::SP_DialogCloseButton));
    ui->actionAbout->setIcon(style.standardIcon(QCommonStyle::SP_DialogHelpButton));

    connect(ui->actionAdd_Directory, &QAction::triggered, this, &main_window::select_directory);
    connect(watcher,&QFileSystemWatcher::directoryChanged,this,&main_window::dir_changed);
    connect(edit,&QLineEdit::returnPressed,this,&main_window::scan_directories);
    connect(ui->actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui->actionAbout, &QAction::triggered, this, &main_window::show_about_dialog);
    connect(ui->action_DeleteDir,&QAction::triggered,this,&main_window::deleteDirs);
    connect(ui->actionPrepare,&QAction::triggered,this,&main_window::prepare);
    connect(ui->actionDelete_Files,&QAction::triggered,this,&main_window::deletePrep);

    progress = new QProgressBar();

}
main_window::~main_window()
{
    delete ui;
}
void main_window::select_directory()
{
    QString directory = QFileDialog::getExistingDirectory(this, "Select Directory for Scanning",
        QString(), QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    add_directory(directory);
    preprocessing = nullptr;
}
void main_window::deleteDirs(){
    for(auto i = dir_to_remove.begin(); i != dir_to_remove.end();){
    QTreeWidgetItem* item = *i;
    delete item;
    i = dir_to_remove.erase(i);
}
    ui->action_DeleteDir->setEnabled(false);
}
void main_window::prepare(){
    makeProgressBar();
    ui->treeWidget->clear();
    if (ui->WidgetForDirs->topLevelItemCount() == 0) {
            notification("Please , choose directories to scan","Notification");
            return;
        }
    for(size_t i = 0; i < ui->WidgetForDirs->topLevelItemCount();i++){
        QString name = ui->WidgetForDirs->topLevelItem(i)->text(0);
        directoriesForPrep.insert(name);
    }
    if(directoriesForPrep.size()==0){
        notification("There is nothing to preprocess","Notification");
        return;
    }
    if (preprocessing != nullptr) {
        delete preprocessing;
        preprocessing = nullptr;
    }
    preprocessing = new std::map<QString, std::map<QString, std::set<int64_t>>>();
    thread = new QThread();
        tm = new TrigramManager(directoriesForPrep);
        tm->moveToThread(thread);
        connect(thread, &QThread::started, tm, &TrigramManager::manage_trigrams);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(tm, &TrigramManager::result, this, &main_window::prepared);
        connect(tm, &TrigramManager::finished, tm, &TrigramManager::deleteLater);
        connect(tm, &TrigramManager::finished, thread, &QThread::quit);
        connect(tm, &TrigramManager::finished, this, &main_window::finished_process);
        connect(tm, &TrigramManager::throw_progress, this, &main_window::setProgressValue);
        connect(tm, &TrigramManager::throw_error, this, &main_window::getTroubles);
        connect(tm,&TrigramManager::notification,this,&main_window::catchnotification);
        connect(tm,&TrigramManager::cancel,this,&main_window::deletePrep);
        connect(ui->action_Cancel, &QAction::triggered, tm, &TrigramManager::canceled);
        connect(ui->actionExit,&QAction::triggered,tm,&TrigramManager::canceled);
        connect(thread,&QThread::started,this,&main_window::swapPrep);
        thread->start();
    }

void main_window::swapPrep(){
    isPrescanning = true;
    ui->actionPrepare->setDisabled(true);
    ui->action_Cancel->setEnabled(true);
}
void main_window::catchnotification(QString const & message){
    notification(message,"test");
}
void main_window::finished_process(){
    isPrescanning = false;
    directoriesForPrep.clear();
    notification("Preparation is over","Notification");
    setProgressValue(0);
    ui->action_Cancel->setDisabled(true);
    ui->actionPrepare->setEnabled(true);
}
void main_window::prepared(std::map<QString, std::map<QString,
                              std::set<int64_t>>>* result) {
        preprocessing = result;
        ui->actionDelete_Files->setEnabled(true);
        setProgressValue(0);
}
void main_window::deletePrep(){
    isPrescanning = false;
    notification("Preprocessing is successfully deleted","Notification");
    preprocessing = nullptr;
    ui->actionDelete_Files->setDisabled(true);
}
void main_window::scan_directories(){
    QLineEdit *input = ui->toolBar->findChild<QLineEdit*>();
    input->setDisabled(true);
    QString str = input->text();
    if (str.size() == 0){
        notification("I think,you need to write a string to search for","Notification");
        input->setEnabled(true);
        return ;
    }
    if(ui->WidgetForDirs->topLevelItemCount()==0){
        notification("I think,you need to choose directories","Notification");
        input->setEnabled(true);
        return ;
    }
    ui->action_Cancel->setEnabled(true);
    count = 0;
    statusBar()->clearMessage();
    makeProgressBar();
    ui->treeWidget->clear();
    workerThread = new QThread();
    ScanWorker* scan = new ScanWorker(preprocessing);
    scan->moveToThread(workerThread);
    scan->get_string(str);
    QList<QString> directories;
    for(size_t i = 0; i < ui->WidgetForDirs->topLevelItemCount();i++){
        QString name = ui->WidgetForDirs->topLevelItem(i)->text(0);
        directories.push_back(name);
    }
    scan->add_dir(std::move(directories));
    qRegisterMetaType<QList<int>>("coordinates");
    connect(ui->actionExit,&QAction::triggered,workerThread,&QThread::requestInterruption);
    connect(workerThread,&QThread::started,scan,&ScanWorker::search);
    connect(workerThread,&QThread::started,this,&main_window::swap);
    connect(scan,&ScanWorker::finished,this,&main_window::ready);
    connect(scan,&ScanWorker::new_match,this,&main_window::output_match);
    connect(scan,&ScanWorker::sendTroubles,this,&main_window::getTroubles);
    connect(scan,&ScanWorker::finished,scan,&ScanWorker::deleteLater);
    connect(scan,SIGNAL(update(int)),this,SLOT(setProgressValue(int)));
    connect(scan,&ScanWorker::callNotification,this,&main_window::getNotification);
    connect(workerThread,&QThread::finished,workerThread,&QThread::deleteLater);
    connect(ui->action_Cancel,&QAction::triggered,workerThread,&QThread::requestInterruption);
    workerThread->start();
//    if (__cplusplus == 201703L) notification("C++17\n","Notification");
//       else if (__cplusplus == 201402L) notification("C++14\n","Notification");
//       else if (__cplusplus == 201103L) notification("C++11\n","n");
//       else if (__cplusplus == 199711L) notification("C++98\n","asd");
}
void main_window::swap(){
    isScanning = true;
}
void main_window::output_match(QString const& file_name, QList<int> const& coordinates) {
    count++;
    QTreeWidgetItem* parent = new QTreeWidgetItem(ui->treeWidget);
    parent->setText(0, file_name + ": " + QString::number(coordinates.size()));
    ui->treeWidget->insertTopLevelItem(0, parent);
      for (auto i: coordinates) {
            QTreeWidgetItem* item = new QTreeWidgetItem(parent);
            item->setText(0, QString::number(i));
        }
}
void main_window::ready(){
    isScanning = false;
    if(count == 0){
        notification("No matches found","Notification");
    }
    else {
        notification(QString("%1 matches found").arg(count),"Notification");
    }
    ui->action_Cancel->setDisabled(true);
    QLineEdit *input = ui->toolBar->findChild<QLineEdit*>();
    input->setEnabled(true);
    progress->setValue(0);
}

void main_window::makeProgressBar(){
    statusBar()->clearMessage();
    progress->setTextVisible(true);
    progress->setRange(0,100);
    if(statusBar()->findChild<QProgressBar*>()==0) statusBar()->addWidget(progress,1);

}
void main_window::setProgressValue(const int value){
    progress->setValue(value);
}

void main_window::add_directory(QString const& currentDirectory)
{
    checkForMatch(currentDirectory);
}
void main_window::checkForMatch(QString const& absolutePath)
{
    if(absolutePath==""){
        notification("I think,you need to choose directory","Notification");
        return;
    }

    int size = ui->WidgetForDirs->topLevelItemCount();

    bool check = false;

    QList<QTreeWidgetItem*> coincidecncies;

    if (size == 0) {
        QTreeWidgetItem* directoriesColumn = new QTreeWidgetItem(ui->WidgetForDirs);
        directoriesColumn->setText(0, absolutePath);
        directoriesColumn->setCheckState(0, Qt::Unchecked);
        connect(ui->WidgetForDirs, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
            this, SLOT(dir_to_delete(QTreeWidgetItem*)));
        ui->WidgetForDirs->addTopLevelItem(directoriesColumn);
        addToWatcher(absolutePath);
        return;
    }
    for (int i = 0; i < size; ++i) {
        QTreeWidgetItem* curr = ui->WidgetForDirs->topLevelItem(i);
        QString text = curr->text(0);
        if (absolutePath == text||absolutePath.contains(text)) {
            check = true;
        }
        else if (text.contains(absolutePath)) {
            coincidecncies.append(curr);
        }
    }

    if(check) notification("You've already chosen this directory or the parent of it!","Notification");

    if(coincidecncies.size() != 0) notification("You've added the parent directory of already chosen,every subdirectory is replaced.","Notification");

    if (!check || coincidecncies.size() != 0) {
        for (auto item : coincidecncies) {
            delete item;
        }
        coincidecncies.clear();
        QTreeWidgetItem* directoriesColumn = new QTreeWidgetItem(ui->WidgetForDirs);
        directoriesColumn->setText(0, absolutePath);
        directoriesColumn->setCheckState(0, Qt::Unchecked);
        connect(ui->WidgetForDirs, SIGNAL(itemChanged(QTreeWidgetItem*, int)),
            this, SLOT(dir_to_delete(QTreeWidgetItem*)));
        ui->WidgetForDirs->addTopLevelItem(directoriesColumn);
    }
    addToWatcher(absolutePath);
}
void main_window::addToWatcher(QString const &absolutePath){
    QList<QString> allDirs;
    allDirs.push_back(absolutePath);
    while (!allDirs.isEmpty()) {
        watcher->addPath(allDirs.front());
        QDir dir(allDirs.front());
        dir.setFilter(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);
        QFileInfoList list = dir.entryInfoList();
        for (QFileInfo file_info : list) {
            if (file_info.isDir()) {
                allDirs.push_back(file_info.absoluteFilePath());
            }
            else {
                watcher->addPath(file_info.absoluteFilePath());
            }
        }
        allDirs.pop_front();
    }
}
void main_window::dir_changed(const QString &path){
    if(!first){
    first = true;
    window = new DeleteWindow(this);
    window->show();
    connect(window,&DeleteWindow::ok,this,&main_window::reindex);
    connect(window,&DeleteWindow::no,this,&main_window::firstBool);
//    if(workerThread->isRunning())
    if(isScanning){
        connect(window,&DeleteWindow::ok,workerThread,&QThread::requestInterruption);
    }
    if(isPrescanning){
        connect(window,&DeleteWindow::ok,tm,&TrigramManager::canceled);
    }
    }
}
void main_window::reindex(){
    first = false;
    prepare();
}
void main_window::file_changed(const QString &path){
    notification("f","as");
    return;

}
void main_window::dir_to_delete(QTreeWidgetItem* chosenDirToDelete){
    QString dir_name = chosenDirToDelete->text(0);
    if(chosenDirToDelete->checkState(0)==Qt::Checked){
        dir_to_remove.insert(chosenDirToDelete);
    }
    else {
        dir_to_remove.erase(dir_to_remove.find(chosenDirToDelete));
    }
    if(dir_to_remove.size()>0){
        ui->action_DeleteDir->setEnabled(true);
    }
    else ui->action_DeleteDir->setDisabled(true);
}


void main_window::getNotification(QString const& message,
                                  const char* window_title)
{
    notification(message,window_title);
}

void main_window::notification(QString const& message,
    const char* window_title)
{
    QMessageBox* msgbox = new QMessageBox(QMessageBox::Information,
        QString(window_title), QString(message),
        QMessageBox::StandardButtons(QMessageBox::Ok), this);
    msgbox->open();
}

void main_window::getTroubles(QString const &file){
    QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
    item->setText(1,file);
    ui->treeWidget->addTopLevelItem(item);
}
void main_window::firstBool(){
    first = false;
}
void main_window::show_about_dialog()
{
    QMessageBox::aboutQt(this);
}

