#include "localmusicpage.h"
#include <QFileDialog>
#include <QDirIterator>
#include <QFileInfo>
#include <QTableWidgetItem>
#include <QDebug>
#include <QMediaPlayer>
#include <QMediaMetaData>
#include <QTime>
#include <QTimer>
#include <QSettings>

bool LocalMusicPage::isAudioFile(const QString &fileName)
{
    QString suffix = QFileInfo(fileName).suffix().toLower();
    return suffix == "mp3" || suffix == "flac" || suffix == "wav" ||
           suffix == "ape" || suffix == "m4a" || suffix == "ogg";
}

LocalMusicPage::LocalMusicPage(QWidget* container, QObject *parent)
    : QObject(parent)
{
    m_selectFolderBtn = container->findChild<QPushButton*>("pb_selectFolder");
    m_playAllBtn      = container->findChild<QPushButton*>("pb_playAll");
    m_tableWidget     = container->findChild<QTableWidget*>("table_musicList");
    m_songCountLabel  = container->findChild<QLabel*>("lb_songCount");

    if (!m_selectFolderBtn || !m_tableWidget || !m_songCountLabel) {
        qWarning() << "LocalMusicPage: 未找到控件";
        return;
    }

    // 设置表格选中行为：整行选中，单选
    m_tableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableWidget->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_selectFolderBtn, &QPushButton::clicked, this, &LocalMusicPage::onSelectFolderClicked);
    if (m_playAllBtn) {
        connect(m_playAllBtn, &QPushButton::clicked, this, &LocalMusicPage::onPlayAllClicked);
    }
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked,
            this, &LocalMusicPage::onTableDoubleClicked);

    // 自动加载上次保存的文件夹
        QSettings settings("MyCompany", "MusicPlayer");  // 应用名可自定义
        QString lastFolder = settings.value("LocalMusicFolder", "").toString();
        if (!lastFolder.isEmpty() && QDir(lastFolder).exists()) {
            scanMusic(lastFolder);   // 异步扫描，不阻塞界面
        }
}

LocalMusicPage::~LocalMusicPage() {}

void LocalMusicPage::onSelectFolderClicked()
{

    QString folder = QFileDialog::getExistingDirectory(nullptr, "选择音乐文件夹", QDir::homePath());
    if (folder.isEmpty()) return;

    // 保存到 QSettings
    QSettings settings("MyCompany", "MusicPlayer");
    settings.setValue("LocalMusicFolder", folder);

    scanMusic(folder);
}

void LocalMusicPage::scanMusic(const QString &folderPath)
{
    m_tableWidget->setRowCount(0);
    m_pendingFiles.clear();
    m_allSongs.clear();

    QDirIterator it(folderPath, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        QString filePath = it.next();
        if (isAudioFile(filePath))
            m_pendingFiles.append(filePath);
    }
    m_allSongs = m_pendingFiles;   // 保存一份完整列表

    m_songCountLabel->setText(QString("共 %1 首").arg(m_pendingFiles.size()));
    processNextFile();
}

void LocalMusicPage::processNextFile()
{
    if (m_pendingFiles.isEmpty()) return;

    QString filePath = m_pendingFiles.takeFirst();
    QMediaPlayer *player = new QMediaPlayer(this);
    player->setMedia(QUrl::fromLocalFile(filePath));

    // 当媒体加载完成时（LoadedMedia），读取元数据
    connect(player, &QMediaPlayer::mediaStatusChanged, this, [=](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::LoadedMedia) {
            QString title = player->metaData(QMediaMetaData::Title).toString();
            QString artist = player->metaData(QMediaMetaData::ContributingArtist).toString();
            QString album  = player->metaData(QMediaMetaData::AlbumTitle).toString();
            qint64 durationMs = player->duration();

            if (title.isEmpty()) title = QFileInfo(filePath).baseName();
            if (artist.isEmpty()) artist = "未知歌手";
            if (album.isEmpty())  album = "未知专辑";
            QString durationStr = (durationMs > 0) ?
            QTime(0, (durationMs/60000)%60, (durationMs/1000)%60).toString("mm:ss") : "00:00";

            int row = m_tableWidget->rowCount();
            m_tableWidget->insertRow(row);

            QTableWidgetItem *titleItem = new QTableWidgetItem(title);
            titleItem->setData(Qt::UserRole, filePath);
            QTableWidgetItem *artistItem = new QTableWidgetItem(artist);
            QTableWidgetItem *albumItem  = new QTableWidgetItem(album);
            QTableWidgetItem *durationItem = new QTableWidgetItem(durationStr);

            m_tableWidget->setItem(row, 0, titleItem);
            m_tableWidget->setItem(row, 1, artistItem);
            m_tableWidget->setItem(row, 2, albumItem);
            m_tableWidget->setItem(row, 3, durationItem);

            player->deleteLater();
            processNextFile();
        }
    });

    // 错误处理
    connect(player, static_cast<void(QMediaPlayer::*)(QMediaPlayer::Error)>(&QMediaPlayer::error),
            this, [=](QMediaPlayer::Error error) {
        Q_UNUSED(error);
        // 出错则用文件名作为标题
        QString title = QFileInfo(filePath).baseName();
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);
        QTableWidgetItem *titleItem = new QTableWidgetItem(title);
        titleItem->setData(Qt::UserRole, filePath);
        m_tableWidget->setItem(row, 0, titleItem);
        m_tableWidget->setItem(row, 1, new QTableWidgetItem("未知歌手"));
        m_tableWidget->setItem(row, 2, new QTableWidgetItem("未知专辑"));
        m_tableWidget->setItem(row, 3, new QTableWidgetItem("00:00"));
        player->deleteLater();
        processNextFile();
    });

    // 超时保护（2秒内未加载完成则按文件名处理）
    QTimer::singleShot(2000, player, [=]() {
        if (player->state() == QMediaPlayer::StoppedState && !player->media().isNull()) {
            QString title = QFileInfo(filePath).baseName();
            int row = m_tableWidget->rowCount();
            m_tableWidget->insertRow(row);
            QTableWidgetItem *titleItem = new QTableWidgetItem(title);
            titleItem->setData(Qt::UserRole, filePath);
            m_tableWidget->setItem(row, 0, titleItem);
            m_tableWidget->setItem(row, 1, new QTableWidgetItem("未知歌手"));
            m_tableWidget->setItem(row, 2, new QTableWidgetItem("未知专辑"));
            m_tableWidget->setItem(row, 3, new QTableWidgetItem("00:00"));
            player->deleteLater();
            processNextFile();
        }
    });
}

void LocalMusicPage::onPlayAllClicked()
{
    //发送信号，通知主窗口执行全部播放
    emit sig_playAll();
}

void LocalMusicPage::onTableDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    QTableWidgetItem *item = m_tableWidget->item(row, 0);
    if (!item) return;
    emit sig_playMusic(item->data(Qt::UserRole).toString(), item->text());
}


bool LocalMusicPage::getSongInfo(const QString &filePath, QString &artist, QString &album, QString &duration) const
{
    // 遍历表格，查找匹配文件路径的行
    for (int row = 0; row < m_tableWidget->rowCount(); ++row) {
        QTableWidgetItem *titleItem = m_tableWidget->item(row, 0);
        if (titleItem && titleItem->data(Qt::UserRole).toString() == filePath) {
            artist = m_tableWidget->item(row, 1)->text();
            album  = m_tableWidget->item(row, 2)->text();
            duration = m_tableWidget->item(row, 3)->text();
            return true;
        }
    }
    return false;
}
