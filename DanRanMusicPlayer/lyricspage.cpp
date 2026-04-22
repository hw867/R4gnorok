#include "lyricspage.h"
#include <QFile>
#include <QDebug>
#include <QCoreApplication>
#include <QFont>
#include <QListWidgetItem>

LyricsPage::LyricsPage(QListWidget* lyricsDisplayWidget, QObject *parent)
    : QObject(parent)
    , m_lyricsWidget(lyricsDisplayWidget)
    , m_currentPositionMs(0)
    , m_lastHighlightIndex(-1)
{
    connect(&m_timer, &QTimer::timeout, this, &LyricsPage::slot_timerTimeOut);
}

LyricsPage::~LyricsPage()
{
    stopTimer();
}

void LyricsPage::loadAndDisplayLyrics(const QString &songName)
{
    // 停止之前的定时器，清空旧歌词
    stopTimer();
    m_lyricsWidget->clear();
    m_lrcList.clear();
    m_lastHighlightIndex = -1;

    // 加载新歌词
    loadLrc(songName);
}

void LyricsPage::updatePlaybackPosition(qint64 positionMs)
{
    m_currentPositionMs = positionMs;
    // 定时器已经在运行，会在 timeout 时根据位置更新高亮
}

void LyricsPage::loadLrc(const QString &songName)
{
    // 歌词文件路径：exe目录/lrc/歌曲名.lrc
    QString path = QCoreApplication::applicationDirPath() + "/lrc/" + songName + ".lrc";
    QFile file(path);
    if (!file.exists()) {
        qDebug() << "歌词文件不存在:" << path;
        return;
    }
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开歌词文件:" << path;
        return;
    }
    QString strLrc = QString::fromUtf8(file.readAll());
    file.close();

    analysisLyric(strLrc);
    if (!m_lrcList.isEmpty()) {
        startTimer();
    }
}

void LyricsPage::analysisLyric(const QString &lyric)
{
    QStringList strLines = lyric.split('\n');
    for (const QString& strLine : strLines) {
        QStringList strlst = strLine.split(']');
        if (strlst.size() < 2) continue;
        QString time = strlst.at(0);
        int timestamp = -1;
        if (time.contains('[')) {
            int min = time.mid(1, 2).toInt();
            int sec = time.mid(4, 2).toInt();
            int msec = time.mid(7, 2).toInt();
            timestamp = (min * 60 + sec) * 100 + msec;  // 单位：10ms
        }
        if (timestamp != -1) {
            // 添加到显示控件
            QListWidgetItem *item = new QListWidgetItem;
            item->setText(strlst.at(1));
            item->setFont(QFont("楷书", 16));
            item->setTextAlignment(Qt::AlignCenter);
            m_lyricsWidget->addItem(item);

            // 存储后台节点
            LrcNode node;
            node.time = timestamp;
            node.lrcStr = strlst.at(1);
            m_lrcList.append(node);
        }
    }
    qDebug() << "解析歌词完成，共" << m_lrcList.size() << "行";
}

void LyricsPage::startTimer()
{
    if (!m_timer.isActive()) {
        m_timer.start(100);   // 每 100ms 同步一次
    }
}

void LyricsPage::stopTimer()
{
    if (m_timer.isActive()) {
        m_timer.stop();
    }
}

void LyricsPage::slot_timerTimeOut()
{
    if (m_lrcList.isEmpty()) return;

    // 将当前播放位置（毫秒）转换为 10ms 单位（与歌词时间戳一致）
    int currentTime = m_currentPositionMs / 10;

    // 找到最后一个时间戳 ≤ 当前时间的歌词行
    int row = -1;
    for (int i = 0; i < m_lrcList.size(); ++i) {
        if (currentTime >= m_lrcList[i].time) {
            row = i;
        } else {
            break;
        }
    }
    if (row == -1 || row == m_lastHighlightIndex) return;

    // 还原上一行高亮
    if (m_lastHighlightIndex >= 0 && m_lastHighlightIndex < m_lrcList.size()) {
        QListWidgetItem* lastItem = m_lyricsWidget->item(m_lastHighlightIndex);
        if (lastItem) {
            lastItem->setFont(QFont("楷书", 16, QFont::Normal));
            lastItem->setForeground(Qt::black);
        }
    }

    // 高亮当前行
    QListWidgetItem* curItem = m_lyricsWidget->item(row);
    if (curItem) {
        curItem->setFont(QFont("楷书", 18, QFont::Bold));
        curItem->setForeground(Qt::blue);
    }
    m_lastHighlightIndex = row;

    // 自动滚动到当前行
    m_lyricsWidget->scrollToItem(curItem, QAbstractItemView::PositionAtCenter);
}
