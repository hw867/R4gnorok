#ifndef LOCALMUSICPAGE_H
#define LOCALMUSICPAGE_H

#include <QObject>
#include <QTableWidget>
#include <QLabel>
#include <QPushButton>
#include <QStringList>

class LocalMusicPage : public QObject
{
    Q_OBJECT
public:
    explicit LocalMusicPage(QWidget* container, QObject *parent = nullptr);
    ~LocalMusicPage();

    bool getSongInfo(const QString &filePath, QString &artist, QString &album, QString &duration) const;

signals:
    // 双击播放歌曲时发出的信号（供主窗口连接到底部播放栏）
    void sig_playMusic(QString filePath, QString songName);
    // 点击全部播放时发出信号
    void sig_playAll();

private slots:
    void onSelectFolderClicked();
    void onPlayAllClicked();
    void onTableDoubleClicked(int row, int column);

private:
    void scanMusic(const QString &folderPath);
    void processNextFile();          // 异步解析下一首
    bool isAudioFile(const QString &fileName);

    // UI 控件指针
    QTableWidget *m_tableWidget;
    QLabel       *m_songCountLabel;
    QPushButton  *m_selectFolderBtn;
    QPushButton  *m_playAllBtn;

    // 异步解析队列
    QStringList m_pendingFiles;
    QStringList m_allSongs;   // 存储所有已扫描的歌曲文件路径

public:
    QStringList getAllSongPaths() const { return m_allSongs; }

};

#endif // LOCALMUSICPAGE_H
