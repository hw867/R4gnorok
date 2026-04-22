#ifndef ONLINEMUSICPAGE_H
#define ONLINEMUSICPAGE_H

#include <QObject>
#include <QList>
#include <QString>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include "netrequest.h"
#include "mytablewidgetitem.h"

class OnlineMusicPage : public QObject
{
    Q_OBJECT
public:
    explicit OnlineMusicPage(QWidget *container, QObject *parent = nullptr);
    ~OnlineMusicPage();

    // 供外部调用：执行搜索
    void search(const QString &keyword);
    // 供外部调用：请求播放指定歌曲（如喜欢页面）
    void requestPlaySong(const MusicInfoData &info);
    void requestCoverByUrl(const QString &picUrl, const QString &songName);

signals:
    void sig_playOnlineSong(const MusicInfoData &info, const QString &playUrl);
    void sig_lyricsReady(const QString &songName);
    void sig_coverReady(const QString &songName, const QPixmap &pixmap);

private slots:
    void onPrevPageClicked();
    void onNextPageClicked();
    void onTableDoubleClicked(int row, int column);
    void onSearchResult(int code, QByteArray &bytes);
    void onSongUrlResult(int code, QByteArray &bytes);
    void onLyricResult(int code, QByteArray &bytes);
    void onPicResult(int code, QByteArray &bytes);

private:
    void requestSearchPage(const QString &key, int page);
    void showMusicList(const QList<MusicInfoData> &musicList);
    void saveLyricToFile(const QString &songName, const QString &lyricText);
    void saveCoverToFile(const QString &songName, const QByteArray &imageData);

    QTableWidget *m_tableWidget;
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QLabel *m_pageInfoLabel;
    QLabel *m_totalCountLabel;

    NetRequest *m_searchRq;
    NetRequest *m_songUrlRq;
    NetRequest *m_lyricRq;
    NetRequest *m_picRq;

    int m_curPage;
    int m_pageSize;
    int m_totalPage;
    QString m_searchKey;

    QString m_currentRequestSongId;
    MusicInfoData m_currentRequestInfo;
    QString m_currentPicSongName;   // 记录单独请求封面时的歌曲名
};

#endif // ONLINEMUSICPAGE_H
