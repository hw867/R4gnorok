#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QList>
#include <QLabel>
#include <QMediaPlayer>
#include <QListWidgetItem>
#include "lyricspage.h"
#include "lovemusicpage.h"
#include "onlinemusicpage.h"

class LocalMusicPage;

struct PlaylistItem {
    QString name;      // 歌曲名
    QString source;    // 本地路径或网络URL
    int type;          // 0:本地, 1:网络
    QString artist;    // 歌手
    QString album;     // 专辑
    QString duration;  // 时长 mm:ss
    QString songId;    // 在线歌曲ID（本地可为空）
    QString picUrl;    // 封面URL
};

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 轮播图
    void slot_bannerTimeout();
    void updateDotIndicator(int index);
    void on_pb_prev_clicked();
    void on_pb_next_clicked();

    // 页面跳转
    void on_pb_localMusic_clicked();
    void on_pb_loveMusic_clicked();
    void on_pb_musicPic_clicked();
    void on_pb_foundMusic_clicked();
    void on_pb_return_clicked();
    void on_pb_musicLyrics_clicked();
    void onPageChanged(int index);

    // 播放控制
    void on_pb_prevMusic_clicked();
    void on_pb_startMusic_clicked();
    void on_pb_pauseMusic_clicked();
    void on_pb_nextMusic_clicked();
    void on_pb_choseList_clicked();
    void on_pb_clear_clicked();
    void on_lw_playList_itemDoubleClicked(QListWidgetItem *item);

    // 本地音乐
    void onLocalMusicPlayRequested(QString filePath, QString songName);
    void onLocalPlayAll();

    // 喜欢音乐
    void onLoveMusicPlayRequested(const QString &source, const QString &songName, int type, const QString &songId);
    void onLovePlayAllClicked();
    void onLoveButtonClicked();
    void onFavoriteChanged(const QString &id, int type, bool isFav);

    // 在线音乐（来自 OnlineMusicPage 信号）
    void onOnlinePlayRequest(const MusicInfoData &info, const QString &playUrl);
    void onOnlineLyricsReady(const QString &songName);
    void onOnlineCoverReady(const QString &songName, const QPixmap &pixmap);

private:
    Ui::MainWindow *ui;

    // 轮播图
    QTimer *m_bannerTimer;
    QList<QLabel*> m_dotList;

    // 播放器与播放列表
    QMediaPlayer *m_player;
    QList<PlaylistItem> m_playlist;
    int m_currentIndex;
    bool is_show;                     // 播放列表显示状态
    QString m_originalDownStyle;      // 底部栏原始样式

    // 页面指针
    QWidget* m_lastPageBeforeLyrics;
    LocalMusicPage *m_localMusicPage;
    LyricsPage *m_lyricsPage;
    LoveMusicPage *m_loveMusicPage;
    OnlineMusicPage *m_onlineMusicPage;   // 在线音乐模块

    // 事件过滤器
    bool eventFilter(QObject *watched, QEvent *event) override;

    // 内部函数
    void updatePlayControls(bool isPlaying);
    void playSongByIndex(int index);
    void updatePlayListWidget();
    int findSongInPlaylist(const QString &source);
    void addSongToPlaylist(const QString &source, const QString &songName, int type, bool playNow);
    void updateLoveButtonStatus();
    void updateCover(const QPixmap &pixmap);
};

#endif // MAINWINDOW_H
