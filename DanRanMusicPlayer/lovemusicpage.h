#ifndef LOVEMUSICPAGE_H
#define LOVEMUSICPAGE_H

#include <QObject>
#include <QTableWidget>
#include <QList>

struct FavoriteItem {
    QString id;         // 唯一标识：本地完整路径 或 在线歌曲ID
    QString name;       // 歌曲名
    QString artist;     // 歌手
    QString album;      // 专辑
    QString duration;   // 时长 mm:ss
    int type;           // 0:本地, 1:在线
    QString picUrl;     // 封面URL（在线歌曲可用）
};

class LoveMusicPage : public QObject
{
    Q_OBJECT
public:
    explicit LoveMusicPage(QWidget *container, QObject *parent = nullptr);
    ~LoveMusicPage();

    // 对外接口
    void addFavorite(const FavoriteItem &item);
    void removeFavorite(const QString &id, int type);
    bool isFavorite(const QString &id, int type) const;
    QList<FavoriteItem> getAllFavorites() const;  // 用于全部播放
    bool getFavoriteInfo(const QString &id, int type, FavoriteItem &outItem) const;

signals:
    // 通知主窗口播放歌曲：source为播放源（本地路径或在线URL），songName歌名，type类型，songId唯一标识
    void sig_playMusic(const QString &source, const QString &songName, int type, const QString &songId);
    // 喜欢状态变化（用于全局同步UI）
    void sig_favoriteChanged(const QString &id, int type, bool isFav);

private slots:
    void onTableDoubleClicked(int row, int column);
    void onRemoveButtonClicked();

private:
    void loadFavorites();
    void saveFavorites();
    void refreshTable();

    QTableWidget *m_tableWidget;
    QList<FavoriteItem> m_favoriteSongs;
    QString m_configPath;
};

#endif // LOVEMUSICPAGE_H
