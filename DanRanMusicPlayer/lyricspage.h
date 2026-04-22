#ifndef LYRICSPAGE_H
#define LYRICSPAGE_H

#include <QObject>
#include <QListWidget>
#include <QTimer>
#include <QList>

struct LrcNode {
    int time;       // 时间戳（单位：10ms，与 dialog.cpp 保持一致）
    QString lrcStr;
};

class LyricsPage : public QObject
{
    Q_OBJECT
public:
    explicit LyricsPage(QListWidget* lyricsDisplayWidget, QObject *parent = nullptr);
    ~LyricsPage();

public slots:
    // 外部调用：加载并显示指定歌曲的歌词（根据歌曲名查找 .lrc 文件）
    void loadAndDisplayLyrics(const QString &songName);
    // 外部调用：更新当前播放位置（用于同步高亮）
    void updatePlaybackPosition(qint64 positionMs);

private:
    void loadLrc(const QString &songName);    // 从文件加载歌词
    void analysisLyric(const QString &lyric); // 解析歌词文本
    void startTimer();                         // 启动定时器
    void stopTimer();                          // 停止定时器

private slots:
    void slot_timerTimeOut();                  // 定时器超时，高亮当前歌词

private:
    QListWidget *m_lyricsWidget;   // 指向 UI 中的歌词显示控件
    QTimer m_timer;                // 歌词同步定时器
    QList<LrcNode> m_lrcList;      // 存储解析后的歌词节点
    qint64 m_currentPositionMs;    // 当前播放位置（毫秒）
    int m_lastHighlightIndex;      // 上一次高亮的行索引
};

#endif // LYRICSPAGE_H
