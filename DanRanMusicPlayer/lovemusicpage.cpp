#include "loveMusicPage.h"
#include <QFile>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QHeaderView>
#include <QTabWidget>
#include <QStandardPaths>
#include <QDebug>
#include <QPushButton>

LoveMusicPage::LoveMusicPage(QWidget *container, QObject *parent)
    : QObject(parent)
{
    // 找到 UI 中的表格控件
    QTabWidget *tabWidget = container->findChild<QTabWidget*>("tabWidget");
    if (!tabWidget) {
        qWarning() << "LoveMusicPage: 未找到 tabWidget";
        return;
    }
    QWidget *songTab = tabWidget->widget(0); // 第一个标签页“歌曲”
    if (!songTab) return;

    m_tableWidget = songTab->findChild<QTableWidget*>("table_loveMusic");
    if (!m_tableWidget) {
        qWarning() << "LoveMusicPage: 未找到 table_loveMusic，请在 UI 中为 loveMusicPage 的歌曲标签页添加 QTableWidget 并命名为 table_loveMusic";
        return;
    }


    connect(m_tableWidget, &QTableWidget::cellDoubleClicked,
            this, &LoveMusicPage::onTableDoubleClicked);

    // 配置文件路径
    QString dataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir dir(dataPath);
    if (!dir.exists()) dir.mkpath(".");
    m_configPath = dataPath + "/favorites.json";

    loadFavorites();
    refreshTable();
}

LoveMusicPage::~LoveMusicPage()
{
    saveFavorites();
}

void LoveMusicPage::loadFavorites()
{
    QFile file(m_configPath);
    if (!file.open(QIODevice::ReadOnly)) return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isArray()) return;
    QJsonArray arr = doc.array();
    m_favoriteSongs.clear();
    for (const QJsonValue &val : arr) {
        QJsonObject obj = val.toObject();
        FavoriteItem item;
        item.id = obj["id"].toString();
        item.name = obj["name"].toString();
        item.artist = obj["artist"].toString();
        item.album = obj["album"].toString();
        item.duration = obj["duration"].toString();
        item.type = obj["type"].toInt();
        item.picUrl = obj["picUrl"].toString();
        m_favoriteSongs.append(item);
    }
}

void LoveMusicPage::saveFavorites()
{
    QJsonArray arr;
    for (const FavoriteItem &item : m_favoriteSongs) {
        QJsonObject obj;
        obj["id"] = item.id;
        obj["name"] = item.name;
        obj["artist"] = item.artist;
        obj["album"] = item.album;
        obj["duration"] = item.duration;
        obj["type"] = item.type;
        obj["picUrl"] = item.picUrl;
        arr.append(obj);
    }
    QJsonDocument doc(arr);
    QFile file(m_configPath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
    }
}



void LoveMusicPage::refreshTable()
{
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(0);

    for (int i = 0; i < m_favoriteSongs.size(); ++i) {
        const FavoriteItem &fav = m_favoriteSongs[i];
        m_tableWidget->insertRow(i);
        m_tableWidget->setItem(i, 0, new QTableWidgetItem(fav.name));
        m_tableWidget->setItem(i, 1, new QTableWidgetItem(fav.artist));
        m_tableWidget->setItem(i, 2, new QTableWidgetItem(fav.album));
        m_tableWidget->setItem(i, 3, new QTableWidgetItem(fav.duration));

        // 移除按钮（红色爱心）
        QPushButton *btnRemove = new QPushButton;
        btnRemove->setIcon(QIcon(":/Messageform/images/Messageform/like.png"));
        btnRemove->setFlat(true);
        btnRemove->setFixedSize(30, 30);
        btnRemove->setProperty("songId", fav.id);
        btnRemove->setProperty("songType", fav.type);
        connect(btnRemove, &QPushButton::clicked, this, &LoveMusicPage::onRemoveButtonClicked);
        m_tableWidget->setCellWidget(i, 4, btnRemove);
    }
}

void LoveMusicPage::onRemoveButtonClicked()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;
    QString id = btn->property("songId").toString();
    int type = btn->property("songType").toInt();
    removeFavorite(id, type);
}

void LoveMusicPage::onTableDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    if (row < 0 || row >= m_favoriteSongs.size()) return;
    const FavoriteItem &fav = m_favoriteSongs[row];
    QString source = (fav.type == 0) ? fav.id : "";  // 本地传路径，在线传空（主窗口会请求URL）
    emit sig_playMusic(source, fav.name, fav.type, fav.id);
}

void LoveMusicPage::addFavorite(const FavoriteItem &item)
{
    if (isFavorite(item.id, item.type)) return;
    m_favoriteSongs.append(item);
    saveFavorites();
    refreshTable();
    emit sig_favoriteChanged(item.id, item.type, true);
}

void LoveMusicPage::removeFavorite(const QString &id, int type)
{
    for (int i = 0; i < m_favoriteSongs.size(); ++i) {
        if (m_favoriteSongs[i].id == id && m_favoriteSongs[i].type == type) {
            m_favoriteSongs.removeAt(i);
            break;
        }
    }
    saveFavorites();
    refreshTable();
    emit sig_favoriteChanged(id, type, false);
}

bool LoveMusicPage::isFavorite(const QString &id, int type) const
{
    for (const FavoriteItem &item : m_favoriteSongs) {
        if (item.id == id && item.type == type) return true;
    }
    return false;
}

QList<FavoriteItem> LoveMusicPage::getAllFavorites() const
{
    return m_favoriteSongs;
}

bool LoveMusicPage::getFavoriteInfo(const QString &id, int type, FavoriteItem &outItem) const
{
    for (const FavoriteItem &item : m_favoriteSongs) {
        if (item.id == id && item.type == type) {
            outItem = item;
            return true;
        }
    }
    return false;
}
