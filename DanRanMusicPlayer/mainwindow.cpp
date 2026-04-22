#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "localmusicpage.h"
#include "lyricspage.h"
#include "onlinemusicpage.h"
#include <QDebug>
#include <QMediaPlayer>
#include <QTime>
#include <QFileInfo>
#include <QMessageBox>
#include <QStyle>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_lastPageBeforeLyrics(nullptr)
    , is_show(false)
{
    ui->setupUi(this);

    // 保存底部栏原始样式
    m_originalDownStyle = ui->widget_down->styleSheet();

    // 监听页面切换
    connect(ui->stackedWidget_mian, &QStackedWidget::currentChanged,
            this, &MainWindow::onPageChanged);
    m_currentIndex = -1;

    // 初始按钮可见性
    ui->pb_pauseMusic->show();
    ui->pb_startMusic->hide();
    ui->widget_list->hide();

    // ========== 1. 轮播图 ==========
    m_bannerTimer = new QTimer(this);
    m_bannerTimer->start(4000);
    connect(m_bannerTimer, &QTimer::timeout, this, &MainWindow::slot_bannerTimeout);

    m_dotList << ui->dot_1 << ui->dot_2 << ui->dot_3 << ui->dot_4
              << ui->dot_5 << ui->dot_6 << ui->dot_7 << ui->dot_8;
    updateDotIndicator(0);

    // ========== 2. 本地音乐 ==========
    m_localMusicPage = new LocalMusicPage(ui->localMusic, this);
    connect(m_localMusicPage, &LocalMusicPage::sig_playMusic,
            this, &MainWindow::onLocalMusicPlayRequested);
    connect(m_localMusicPage, &LocalMusicPage::sig_playAll,
            this, &MainWindow::onLocalPlayAll);

    // ========== 3. 播放器与歌词 ==========
    m_player = new QMediaPlayer(this);
    m_player->setVolume(60);
    m_lyricsPage = new LyricsPage(ui->lw_lrcShow, this);

    connect(m_player, &QMediaPlayer::positionChanged, [this](qint64 pos) {
        if (m_lyricsPage) m_lyricsPage->updatePlaybackPosition(pos);
    });
    connect(m_player, &QMediaPlayer::stateChanged, this, [this](QMediaPlayer::State state) {
        updatePlayControls(state == QMediaPlayer::PlayingState);
    });
    connect(m_player, &QMediaPlayer::mediaStatusChanged, this, [this](QMediaPlayer::MediaStatus status) {
        if (status == QMediaPlayer::EndOfMedia) on_pb_nextMusic_clicked();
    });

    // ========== 4. 进度条、音量等控件 ==========
    ui->horizon_progress->setRange(0, 1000);
    ui->horizon_progress->installEventFilter(this);
    ui->horizon_soundProgress->installEventFilter(this);

    connect(m_player, &QMediaPlayer::durationChanged, this, [this](qint64 duration) {
        ui->lb_totalTime->setText(duration > 0 ?
            QTime(0, duration/60000, (duration/1000)%60).toString("mm:ss") : "00:00");
    });
    connect(m_player, &QMediaPlayer::positionChanged, this, [this](qint64 pos) {
        if (m_player->duration() > 0) {
            int value = static_cast<int>(pos * 1000 / m_player->duration());
            ui->horizon_progress->setValue(value);
        } else {
            ui->horizon_progress->setValue(0);
        }
        ui->lb_curTime->setText(QTime(0, pos/60000, (pos/1000)%60).toString("mm:ss"));
    });
    connect(ui->horizon_progress, &QSlider::sliderMoved, this, [this](int value) {
        if (m_player->duration() > 0) {
            qint64 pos = static_cast<qint64>(value) * m_player->duration() / 1000;
            m_player->setPosition(pos);
        }
    });

    ui->horizon_soundProgress->setRange(0, 100);
    ui->horizon_soundProgress->setValue(m_player->volume());
    connect(ui->horizon_soundProgress, &QSlider::sliderMoved, m_player, &QMediaPlayer::setVolume);

    // 初始化播放列表显示
    updatePlayListWidget();

    // ========== 5. 喜欢音乐 ==========
    m_loveMusicPage = new LoveMusicPage(ui->loveMusicPage, this);
    connect(m_loveMusicPage, &LoveMusicPage::sig_playMusic,
            this, &MainWindow::onLoveMusicPlayRequested);
    connect(m_loveMusicPage, &LoveMusicPage::sig_favoriteChanged,
            this, &MainWindow::onFavoriteChanged);
    connect(ui->pb_playAll_2, &QPushButton::clicked,
            this, &MainWindow::onLovePlayAllClicked);
    connect(ui->pb_love, &QPushButton::clicked, this, &MainWindow::onLoveButtonClicked);
    connect(ui->pb_loveGray, &QPushButton::clicked, this, &MainWindow::onLoveButtonClicked);

    // ========== 6. 在线音乐 ==========
    m_onlineMusicPage = new OnlineMusicPage(ui->onlinePage, this);
    connect(m_onlineMusicPage, &OnlineMusicPage::sig_playOnlineSong,
            this, &MainWindow::onOnlinePlayRequest);
    connect(m_onlineMusicPage, &OnlineMusicPage::sig_lyricsReady,
            this, &MainWindow::onOnlineLyricsReady);
    connect(m_onlineMusicPage, &OnlineMusicPage::sig_coverReady,
            this, &MainWindow::onOnlineCoverReady);
    // 连接顶部栏搜索按钮
    connect(ui->pb_onlineSearch, &QPushButton::clicked, this, [this]() {
        QString keyword = ui->le_onlineSearch->text().trimmed();
        if (!keyword.isEmpty()) {
            // 跳转到在线音乐页面
            m_lastPageBeforeLyrics = ui->stackedWidget_mian->currentWidget();
            ui->stackedWidget_mian->setCurrentWidget(ui->onlinePage);
            // 执行搜索
            m_onlineMusicPage->search(keyword);
        }
    });

    // 支持回车键搜索
    connect(ui->le_onlineSearch, &QLineEdit::returnPressed, this, [this]() {
        ui->pb_onlineSearch->clicked();
    });

}

MainWindow::~MainWindow()
{
    delete ui;
}

// ================== 轮播图 ==================
void MainWindow::slot_bannerTimeout()
{
    int cur = ui->stackedWidget_banner->currentIndex();
    int total = ui->stackedWidget_banner->count();
    int next = (cur + 1) % total;
    ui->stackedWidget_banner->setCurrentIndex(next);
    updateDotIndicator(next);
}

void MainWindow::updateDotIndicator(int index)
{
    for (int i = 0; i < m_dotList.size(); ++i) {
        if (i == index) {
            m_dotList[i]->setStyleSheet(R"(
                QLabel {
                    border-radius: 6px;
                    background-color: #C20C0C;
                    min-width: 12px;
                    min-height: 12px;
                    margin: 0 5px;
                }
            )");
        } else {
            m_dotList[i]->setStyleSheet(R"(
                QLabel {
                    border-radius: 5px;
                    background-color: #cccccc;
                    min-width: 10px;
                    min-height: 10px;
                    margin: 0 5px;
                }
            )");
        }
    }
}

void MainWindow::on_pb_prev_clicked()
{
    int cur = ui->stackedWidget_banner->currentIndex();
    int total = ui->stackedWidget_banner->count();
    int prev = (cur - 1 + total) % total;
    ui->stackedWidget_banner->setCurrentIndex(prev);
    updateDotIndicator(prev);
    m_bannerTimer->start(4000);
}

void MainWindow::on_pb_next_clicked()
{
    int cur = ui->stackedWidget_banner->currentIndex();
    int total = ui->stackedWidget_banner->count();
    int next = (cur + 1) % total;
    ui->stackedWidget_banner->setCurrentIndex(next);
    updateDotIndicator(next);
    m_bannerTimer->start(4000);
}

// ================== 页面跳转 ==================
void MainWindow::onPageChanged(int index)
{
    Q_UNUSED(index);
    QWidget *cur = ui->stackedWidget_mian->currentWidget();
    if (cur == ui->lyricsPage) {
        ui->widget_down->setStyleSheet("background-color: #abc;");
        ui->lw_lrcShow->setFrameShape(QFrame::NoFrame);
    } else {
        ui->widget_down->setStyleSheet(m_originalDownStyle);
        ui->lyricsPage->setStyleSheet("");
        ui->lw_lrcShow->setFrameShape(QFrame::StyledPanel);
        ui->lw_lrcShow->setStyleSheet("");
    }
}

void MainWindow::on_pb_localMusic_clicked()
{
    m_lastPageBeforeLyrics = ui->stackedWidget_mian->currentWidget();
    ui->stackedWidget_mian->setCurrentWidget(ui->localMusic);
}

void MainWindow::on_pb_loveMusic_clicked()
{
    m_lastPageBeforeLyrics = ui->stackedWidget_mian->currentWidget();
    ui->stackedWidget_mian->setCurrentWidget(ui->loveMusicPage);
}

void MainWindow::on_pb_musicPic_clicked()
{
    if (ui->stackedWidget_mian->currentWidget() == ui->lyricsPage) return;
    m_lastPageBeforeLyrics = ui->stackedWidget_mian->currentWidget();
    ui->widget_left->hide();
    ui->widget_top->hide();
    ui->stackedWidget_mian->setCurrentWidget(ui->lyricsPage);
}

void MainWindow::on_pb_musicLyrics_clicked()
{
    if (ui->stackedWidget_mian->currentWidget() == ui->lyricsPage) return;
    m_lastPageBeforeLyrics = ui->stackedWidget_mian->currentWidget();
    ui->widget_left->hide();
    ui->widget_top->hide();
    ui->stackedWidget_mian->setCurrentWidget(ui->lyricsPage);
}

void MainWindow::on_pb_foundMusic_clicked()
{
    m_lastPageBeforeLyrics = ui->stackedWidget_mian->currentWidget();
    ui->stackedWidget_mian->setCurrentWidget(ui->homePage);
}

void MainWindow::on_pb_return_clicked()
{
    if (m_lastPageBeforeLyrics && m_lastPageBeforeLyrics != ui->lyricsPage) {
        ui->stackedWidget_mian->setCurrentWidget(m_lastPageBeforeLyrics);
        ui->widget_left->show();
        ui->widget_top->show();
    } else {
        ui->stackedWidget_mian->setCurrentWidget(ui->homePage);
        ui->widget_left->show();
        ui->widget_top->show();
    }
    m_lastPageBeforeLyrics = nullptr;
}

// ================== 本地音乐 ==================
void MainWindow::onLocalMusicPlayRequested(QString filePath, QString songName)
{
    QString artist, album, duration;
    if (m_localMusicPage->getSongInfo(filePath, artist, album, duration)) {
        PlaylistItem item;
        item.name = songName;
        item.source = filePath;
        item.type = 0;
        item.artist = artist;
        item.album = album;
        item.duration = duration;
        int idx = findSongInPlaylist(filePath);
        if (idx != -1) {
            if (idx != m_currentIndex) playSongByIndex(idx);
            return;
        }
        m_playlist.append(item);
        updatePlayListWidget();
        playSongByIndex(m_playlist.size() - 1);
    } else {
        addSongToPlaylist(filePath, songName, 0, true);
    }
}

void MainWindow::onLocalPlayAll()
{
    QStringList allSongs = m_localMusicPage->getAllSongPaths();
    if (allSongs.isEmpty()) return;
    m_playlist.clear();
    for (const QString &path : allSongs) {
        QString artist, album, duration;
        if (m_localMusicPage->getSongInfo(path, artist, album, duration)) {
            PlaylistItem item;
            item.name = QFileInfo(path).baseName();
            item.source = path;
            item.type = 0;
            item.artist = artist;
            item.album = album;
            item.duration = duration;
            m_playlist.append(item);
        } else {
            PlaylistItem item;
            item.name = QFileInfo(path).baseName();
            item.source = path;
            item.type = 0;
            item.artist = "未知歌手";
            item.album = "未知专辑";
            item.duration = "00:00";
            m_playlist.append(item);
        }
    }
    updatePlayListWidget();
    playSongByIndex(0);
}

// ================== 播放控制 ==================
void MainWindow::updatePlayControls(bool isPlaying)
{
    if (isPlaying) {
        ui->pb_startMusic->show();
        ui->pb_pauseMusic->hide();
    } else {
        ui->pb_startMusic->hide();
        ui->pb_pauseMusic->show();
    }
}

void MainWindow::playSongByIndex(int index)
{
    if (index < 0 || index >= m_playlist.size()) return;
    m_currentIndex = index;
    PlaylistItem item = m_playlist.at(index);

    // ========== 封面处理（同步更新底部栏和歌词页黑胶） ==========
    if (item.type == 0) {
        // 本地歌曲：使用默认黑胶封面
        QPixmap defaultPic(":/music_play/images/music_play/heijiaopian1.png");
        if (!defaultPic.isNull()) {
            updateCover(defaultPic);
        }
    } else {
        // 在线歌曲：尝试加载本地缓存（支持多种图片格式）
        QString basePath = QCoreApplication::applicationDirPath() + "/img/" + item.name;
        QStringList extensions = {"png", "jpg", "jpeg", "bmp"};
        QPixmap cover;
        for (const QString &ext : extensions) {
            QString coverPath = basePath + "." + ext;
            if (QFile::exists(coverPath)) {
                if (cover.load(coverPath)) {  // 尝试加载
                    break;
                }
            }
        }

        if (!cover.isNull()) {
            updateCover(cover);
            qDebug() << "从本地加载封面成功:" << item.name;
        } else if (!item.picUrl.isEmpty()) {
            qDebug() << "本地无封面，请求网络:" << item.name;
            m_onlineMusicPage->requestCoverByUrl(item.picUrl, item.name);
        } else {
            QPixmap defaultPic(":/music_play/images/music_play/heijiaopian1.png");
            if (!defaultPic.isNull()) updateCover(defaultPic);
        }
    }

    // ========== 播放音乐 ==========
    m_player->stop();
    if (item.type == 0) {
        m_player->setMedia(QUrl::fromLocalFile(item.source));
    } else {
        m_player->setMedia(QUrl(item.source));
    }
    m_player->play();

    // 更新界面信息
    ui->lb_musicPlayer->setText(QString("%1 - %2").arg(item.name).arg(item.artist));
    if (m_lyricsPage) {
        m_lyricsPage->loadAndDisplayLyrics(item.name);
    }

    // 刷新播放列表高亮和喜欢按钮状态
    updatePlayListWidget();
    updateLoveButtonStatus();
}

void MainWindow::on_pb_startMusic_clicked()
{
    m_player->pause();
    updatePlayControls(false);
}

void MainWindow::on_pb_pauseMusic_clicked()
{
    m_player->play();
    updatePlayControls(true);
}

void MainWindow::on_pb_prevMusic_clicked()
{
    if (m_playlist.isEmpty()) return;
    int prev = m_currentIndex - 1;
    if (prev < 0) prev = m_playlist.size() - 1;
    playSongByIndex(prev);
}

void MainWindow::on_pb_nextMusic_clicked()
{
    if (m_playlist.isEmpty()) return;
    int next = m_currentIndex + 1;
    if (next >= m_playlist.size()) next = 0;
    playSongByIndex(next);
}

void MainWindow::on_pb_choseList_clicked()
{
    if (!is_show) {
        ui->widget_list->show();
        is_show = true;
    } else {
        ui->widget_list->hide();
        is_show = false;
    }
}

void MainWindow::on_pb_clear_clicked()
{
    m_playlist.clear();
    m_currentIndex = -1;
    updatePlayListWidget();
    m_player->stop();
    ui->lb_musicPlayer->setText("歌名/歌手");
    ui->lb_curTime->setText("00:00");
    ui->lb_totalTime->setText("00:00");
    ui->horizon_progress->setValue(0);
    if (m_lyricsPage) {
        m_lyricsPage->loadAndDisplayLyrics("");
    }
}

void MainWindow::on_lw_playList_itemDoubleClicked(QListWidgetItem *item)
{
    int row = ui->lw_playList->row(item);
    if (row < 0 || row >= m_playlist.size()) return;   // 直接使用 row 作为索引
    playSongByIndex(row);
}

void MainWindow::updatePlayListWidget()
{
    ui->lw_playList->clear();

    QFont songFont("楷体", 14);
    for (const PlaylistItem &item : m_playlist) {
        QListWidgetItem *listItem = new QListWidgetItem(item.name);
        listItem->setFont(songFont);
        ui->lw_playList->addItem(listItem);
    }

    QLabel *countLabel = ui->centralwidget->findChild<QLabel*>("lb_totalSong");
    if (countLabel) {
        countLabel->setText(QString("共 %1 首").arg(m_playlist.size()));
    }

    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        ui->lw_playList->setCurrentRow(m_currentIndex);
    } else {
        ui->lw_playList->setCurrentRow(-1);
    }
}

int MainWindow::findSongInPlaylist(const QString &source)
{
    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist[i].source == source) return i;
    }
    return -1;
}

void MainWindow::addSongToPlaylist(const QString &source, const QString &songName, int type, bool playNow)
{
    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist[i].source == source) {
            if (playNow) playSongByIndex(i);
            return;
        }
    }
    PlaylistItem item;
    item.name = songName;
    item.source = source;
    item.type = type;
    if (type == 0) {
        QString artist, album, duration;
        if (m_localMusicPage->getSongInfo(source, artist, album, duration)) {
            item.artist = artist;
            item.album = album;
            item.duration = duration;
        } else {
            item.artist = "未知歌手";
            item.album = "未知专辑";
            item.duration = "00:00";
        }
    } else {
        // 在线歌曲（通常不会走这个分支，由 OnlineMusicPage 直接通过信号播放）
        item.artist = "在线歌手";
        item.album = "在线专辑";
        item.duration = "00:00";
    }
    m_playlist.append(item);
    updatePlayListWidget();
    if (playNow) {
        playSongByIndex(m_playlist.size() - 1);
    }
}

void MainWindow::updateCover(const QPixmap &pixmap)
{
    QPixmap scaled = pixmap.scaled(80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    ui->pb_musicPic->setIcon(QIcon(scaled));
    ui->lb_record->setPixmap(scaled);
}

// ================== 喜欢音乐 ==================
void MainWindow::updateLoveButtonStatus()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) {
        ui->pb_love->hide();
        ui->pb_loveGray->hide();
        return;
    }
    const PlaylistItem &item = m_playlist[m_currentIndex];
    QString id = (item.type == 0) ? item.source : item.songId;
    if (id.isEmpty()) {
        ui->pb_love->hide();
        ui->pb_loveGray->hide();
        return;
    }
    bool fav = m_loveMusicPage->isFavorite(id, item.type);
    ui->pb_love->setVisible(fav);
    ui->pb_loveGray->setVisible(!fav);
}

void MainWindow::onLoveButtonClicked()
{
    if (m_currentIndex < 0 || m_currentIndex >= m_playlist.size()) return;
    const PlaylistItem &item = m_playlist[m_currentIndex];
    QString id = (item.type == 0) ? item.source : item.songId;
    if (id.isEmpty()) return;
    bool isFav = m_loveMusicPage->isFavorite(id, item.type);
    if (isFav) {
        m_loveMusicPage->removeFavorite(id, item.type);
    } else {
        FavoriteItem fav;
        fav.id = id;
        fav.name = item.name;
        fav.artist = item.artist;
        fav.album = item.album;
        fav.duration = item.duration;
        fav.type = item.type;
        fav.picUrl = (item.type == 1) ? "" : ""; // 在线封面由 OnlineMusicPage 处理，此处可忽略
        m_loveMusicPage->addFavorite(fav);
    }
}

void MainWindow::onFavoriteChanged(const QString &id, int type, bool isFav)
{
    Q_UNUSED(id);
    Q_UNUSED(type);
    Q_UNUSED(isFav);
    if (m_currentIndex >= 0 && m_currentIndex < m_playlist.size()) {
        const PlaylistItem &item = m_playlist[m_currentIndex];
        if ((item.type == 0 && item.source == id) ||
            (item.type == 1 && item.songId == id)) {
            updateLoveButtonStatus();
        }
    }
}

void MainWindow::onLoveMusicPlayRequested(const QString &source, const QString &songName, int type, const QString &songId)
{
    if (type == 0) { // 本地
        addSongToPlaylist(source, songName, 0, true);
    } else { // 在线
        // 构建一个基本的 MusicInfoData，只包含 ID 和名称，请求 OnlineMusicPage 处理
        MusicInfoData info;
        info.songID = songId;
        info.songName = songName;
        // 歌手、专辑等从喜欢记录中获取？由于 FavoriteItem 里已经存储了，可以尝试从 loveMusicPage 获取完整信息
        // 但为了简化，让 OnlineMusicPage 重新请求（播放URL、歌词、封面），不影响体验
        m_onlineMusicPage->requestPlaySong(info);
    }
}

void MainWindow::onLovePlayAllClicked()
{
    QList<FavoriteItem> favorites = m_loveMusicPage->getAllFavorites();
    if (favorites.isEmpty()) return;

    m_playlist.clear();
    int firstLocalIndex = -1;
    for (const FavoriteItem &fav : favorites) {
        if (fav.type == 0) {
            PlaylistItem item;
            item.name = fav.name;
            item.source = fav.id;
            item.type = 0;
            item.artist = fav.artist;
            item.album = fav.album;
            item.duration = fav.duration;
            m_playlist.append(item);
            if (firstLocalIndex == -1) firstLocalIndex = m_playlist.size() - 1;
        }
    }
    if (m_playlist.isEmpty()) {
        QMessageBox::information(this, "提示", "喜欢列表中没有本地歌曲，无法全部播放在线歌曲。");
        return;
    }
    updatePlayListWidget();
    if (firstLocalIndex != -1) playSongByIndex(firstLocalIndex);
}

// ================== 在线音乐（来自 OnlineMusicPage 信号） ==================
void MainWindow::onOnlinePlayRequest(const MusicInfoData &info, const QString &playUrl)
{
    PlaylistItem newItem;
    newItem.name = info.songName;
    newItem.source = playUrl;
    newItem.type = 1;
    newItem.artist = info.singer;
    newItem.album = info.albumName;
    newItem.duration = info.playTime;
    newItem.songId = info.songID;
    newItem.picUrl = info.songPicUrl;

    // 检查是否已在播放列表中
    for (int i = 0; i < m_playlist.size(); ++i) {
        if (m_playlist[i].songId == info.songID && m_playlist[i].type == 1) {
            // 更新 URL 和封面
            m_playlist[i].source = playUrl;
            m_playlist[i].picUrl = info.songPicUrl;
            // 无论当前是不是这首歌，都切换到它（如果已经是当前则重新播放）
            playSongByIndex(i);
            updatePlayListWidget();
            return;
        }
    }

    // 新歌，追加到列表末尾
    m_playlist.append(newItem);
    updatePlayListWidget();
    playSongByIndex(m_playlist.size() - 1);
}

void MainWindow::onOnlineLyricsReady(const QString &songName)
{
    if (m_lyricsPage) {
        m_lyricsPage->loadAndDisplayLyrics(songName);
    }
}

void MainWindow::onOnlineCoverReady(const QString &songName, const QPixmap &pixmap)
{
    Q_UNUSED(songName);
    updateCover(pixmap);
}

// ================== 事件过滤器 ==================
bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (watched == ui->horizon_progress) {
            int value = QStyle::sliderValueFromPosition(
                        ui->horizon_progress->minimum(),
                        ui->horizon_progress->maximum(),
                        mouseEvent->pos().x(),
                        ui->horizon_progress->width());
            ui->horizon_progress->setValue(value);
            if (m_player && m_player->duration() > 0) {
                qint64 pos = static_cast<qint64>(value) * m_player->duration() / 1000;
                m_player->setPosition(pos);
            }
            return true;
        } else if (watched == ui->horizon_soundProgress) {
            int value = QStyle::sliderValueFromPosition(
                        ui->horizon_soundProgress->minimum(),
                        ui->horizon_soundProgress->maximum(),
                        mouseEvent->pos().x(),
                        ui->horizon_soundProgress->width());
            ui->horizon_soundProgress->setValue(value);
            if (m_player) m_player->setVolume(value);
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
