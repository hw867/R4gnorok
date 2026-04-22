#include "onlinemusicpage.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QMessageBox>
#include <QDebug>
#include <QTime>
#include <QPixmap>
#include <QBuffer>
#include <QImageReader>


OnlineMusicPage::OnlineMusicPage(QWidget *container, QObject *parent)
    : QObject(parent)
    , m_curPage(1)
    , m_pageSize(30)
    , m_totalPage(0)
    , m_currentPicSongName("")
{
    m_tableWidget = container->findChild<QTableWidget*>("table_onlineMusicList");
    m_prevBtn = container->findChild<QPushButton*>("pb_onlinePrevPage");
    m_nextBtn = container->findChild<QPushButton*>("pb_onlineNextPage");
    m_pageInfoLabel = container->findChild<QLabel*>("lb_onlinePageInfo");
    m_totalCountLabel = container->findChild<QLabel*>("lb_onlineTotalCount");

    if (!m_tableWidget || !m_prevBtn || !m_nextBtn) {
        qWarning() << "OnlineMusicPage: 找不到必需的UI控件";
        return;
    }

    // 连接分页按钮
    connect(m_prevBtn, &QPushButton::clicked, this, &OnlineMusicPage::onPrevPageClicked);
    connect(m_nextBtn, &QPushButton::clicked, this, &OnlineMusicPage::onNextPageClicked);
    connect(m_tableWidget, &QTableWidget::cellDoubleClicked, this, &OnlineMusicPage::onTableDoubleClicked);

    // 网络请求对象
    m_searchRq = new NetRequest(this);
    m_songUrlRq = new NetRequest(this);
    m_lyricRq = new NetRequest(this);
    m_picRq = new NetRequest(this);

    connect(m_searchRq, &NetRequest::SIG_getResult, this, &OnlineMusicPage::onSearchResult);
    connect(m_songUrlRq, &NetRequest::SIG_getResult, this, &OnlineMusicPage::onSongUrlResult);
    connect(m_lyricRq, &NetRequest::SIG_getResult, this, &OnlineMusicPage::onLyricResult);
    connect(m_picRq, &NetRequest::SIG_getResult, this, &OnlineMusicPage::onPicResult);
}

void OnlineMusicPage::search(const QString &keyword)
{
    m_searchKey = keyword.trimmed();
    if (m_searchKey.isEmpty()) return;

    m_curPage = 1;
    requestSearchPage(m_searchKey, m_curPage);
}

OnlineMusicPage::~OnlineMusicPage()
{
}

const QString serverIP = "192.168.1.105";
const QString port = "3000";

void OnlineMusicPage::requestCoverByUrl(const QString &picUrl, const QString &songName)
{
    if (picUrl.isEmpty()) return;
    m_currentPicSongName = songName;
    m_picRq->slot_sendUrlRequest(picUrl);
}

void OnlineMusicPage::onPrevPageClicked()
{
    if (m_curPage <= 1) return;
    m_curPage--;
    requestSearchPage(m_searchKey, m_curPage);
}

void OnlineMusicPage::onNextPageClicked()
{
    if (m_curPage >= m_totalPage) return;
    m_curPage++;
    requestSearchPage(m_searchKey, m_curPage);
}

void OnlineMusicPage::requestSearchPage(const QString &key, int page)
{
//    const QString serverIP = "192.168.1.105";
//    const QString port = "3000";
    int offset = (page - 1) * m_pageSize;
    QString url = QString("http://%1:%2/cloudsearch?keywords=%3&offset=%4&limit=%5")
                      .arg(serverIP).arg(port).arg(key).arg(offset).arg(m_pageSize);
    m_searchRq->slot_sendUrlRequest(url);
}

void OnlineMusicPage::onSearchResult(int code, QByteArray &bytes)
{
    if (code < 0) {
        qDebug() << "搜索失败";
        QMessageBox::warning(nullptr, "提示", "网络请求失败，请检查网络或API服务");
        return;
    }

    QJsonParseError jsonError;
    QJsonDocument json = QJsonDocument::fromJson(bytes, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        qDebug() << "JSON解析失败:" << jsonError.errorString();
        return;
    }

    if (!json.isObject()) return;
    QJsonObject obj = json.object();
    if (!obj.contains("result")) return;
    QJsonObject resultObj = obj["result"].toObject();

    int songCount = resultObj["songCount"].toInt();
    m_totalCountLabel->setText(QString("共 %1 首歌").arg(songCount));

    m_totalPage = (songCount + m_pageSize - 1) / m_pageSize;
    m_pageInfoLabel->setText(QString("第 %1 页 / 共 %2 页").arg(m_curPage).arg(m_totalPage));

    QJsonArray songs = resultObj["songs"].toArray();
    QList<MusicInfoData> musicList;
    for (const QJsonValue &val : songs) {
        QJsonObject songObj = val.toObject();
        MusicInfoData info;
        info.songName = songObj["name"].toString();
        info.songID = QString::number(songObj["id"].toInt());
        info.playTime = QTime::fromMSecsSinceStartOfDay(songObj["dt"].toInt()).toString("mm:ss");

        QJsonArray artists = songObj["ar"].toArray();
        if (!artists.isEmpty())
            info.singer = artists[0].toObject()["name"].toString();
        else
            info.singer = "未知歌手";

        QJsonObject album = songObj["al"].toObject();
        info.albumName = album["name"].toString();
        info.songPicUrl = album["picUrl"].toString();
        info.songSource = from_net;

        musicList.append(info);
    }
    showMusicList(musicList);
}

void OnlineMusicPage::showMusicList(const QList<MusicInfoData> &musicList)
{
    m_tableWidget->clearContents();
    m_tableWidget->setRowCount(0);

    for (const MusicInfoData &info : musicList) {
        int row = m_tableWidget->rowCount();
        m_tableWidget->insertRow(row);

        MyTableWidgetItem *item0 = new MyTableWidgetItem;
        item0->setMusicInfoData(info);
        item0->setText(info.songName);

        MyTableWidgetItem *item1 = new MyTableWidgetItem;
        item1->setText(info.singer);

        MyTableWidgetItem *item2 = new MyTableWidgetItem;
        item2->setText(info.albumName);

        MyTableWidgetItem *item3 = new MyTableWidgetItem;
        item3->setText(info.playTime);

        m_tableWidget->setItem(row, 0, item0);
        m_tableWidget->setItem(row, 1, item1);
        m_tableWidget->setItem(row, 2, item2);
        m_tableWidget->setItem(row, 3, item3);
    }
}

void OnlineMusicPage::onTableDoubleClicked(int row, int column)
{
    Q_UNUSED(column);
    MyTableWidgetItem *item = dynamic_cast<MyTableWidgetItem*>(m_tableWidget->item(row, 0));
    if (!item) return;

    MusicInfoData info = item->musicInfoData();
    // 发起播放请求
    requestPlaySong(info);
}

void OnlineMusicPage::requestPlaySong(const MusicInfoData &info)
{
    // 记录当前请求的歌曲ID和信息，用于回调校验
    m_currentRequestSongId = info.songID;
    m_currentRequestInfo = info;

    // 请求播放URL
//    const QString serverIP = "192.168.1.105";
//    const QString port = "3000";
    QString url = QString("http://%1:%2/song/url?id=%3").arg(serverIP).arg(port).arg(info.songID);
    m_songUrlRq->slot_sendUrlRequest(url);

}

void OnlineMusicPage::onSongUrlResult(int code, QByteArray &bytes)
{
    // 忽略过期的回调
    if (m_currentRequestSongId != m_currentRequestInfo.songID) {
        qDebug() << "忽略过期的播放URL回调，当前期望ID:" << m_currentRequestSongId
                 << "，回调ID:" << m_currentRequestInfo.songID;
        return;
    }

    if (code < 0) {
        qDebug() << "获取播放链接失败";
        QMessageBox::warning(nullptr, "提示", "获取播放链接失败，请检查网络");
        return;
    }

    QJsonParseError jsonError;
    QJsonDocument json = QJsonDocument::fromJson(bytes, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        qDebug() << "播放链接JSON解析失败";
        return;
    }

    if (!json.isObject()) return;
    QJsonObject obj = json.object();
    QJsonArray dataArray = obj["data"].toArray();
    if (dataArray.isEmpty()) return;

    QString playUrl = dataArray[0].toObject()["url"].toString();

    if (playUrl.isEmpty()|| !playUrl.startsWith("http")) {
        qDebug() << "该歌曲无音源，无法播放";
        QMessageBox::warning(nullptr, "提示", QString("歌曲《%1》暂无音源，无法播放").arg(m_currentRequestInfo.songName));
        m_currentRequestSongId.clear();
        m_currentRequestInfo = MusicInfoData();
        return;
    }else{
        // 有效，先请求歌词和封面

        QString lyricUrl = QString("http://%1:%2/lyric?id=%3").arg(serverIP).arg(port).arg(m_currentRequestInfo.songID);
        m_lyricRq->slot_sendUrlRequest(lyricUrl);
        if (!m_currentRequestInfo.songPicUrl.isEmpty()) {
            m_picRq->slot_sendUrlRequest(m_currentRequestInfo.songPicUrl);
        }
        // 然后通知播放
        emit sig_playOnlineSong(m_currentRequestInfo, playUrl);
    }
}

void OnlineMusicPage::onLyricResult(int code, QByteArray &bytes)
{
    // 忽略过期的回调
    if (m_currentRequestSongId != m_currentRequestInfo.songID) {
        qDebug() << "忽略过期的歌词回调";
        return;
    }

    if (code < 0) {
        qDebug() << "获取歌词失败";
        return;
    }

    QJsonParseError jsonError;
    QJsonDocument json = QJsonDocument::fromJson(bytes, &jsonError);
    if (jsonError.error != QJsonParseError::NoError) {
        qDebug() << "歌词JSON解析失败";
        return;
    }

    if (!json.isObject()) return;
    QJsonObject obj = json.object();
    QJsonObject lrcObj = obj["lrc"].toObject();
    QString lyricText = lrcObj["lyric"].toString();
    if (lyricText.isEmpty()) return;

    saveLyricToFile(m_currentRequestInfo.songName, lyricText);
    emit sig_lyricsReady(m_currentRequestInfo.songName);
}

void OnlineMusicPage::onPicResult(int code, QByteArray &bytes)
{
    if (code < 0) return;

       // 判断是哪个歌曲的封面请求
       QString targetSongName;
       if (!m_currentRequestInfo.songID.isEmpty()) {
           // 优先使用正在播放请求的歌曲（由 requestPlaySong 触发）
           targetSongName = m_currentRequestInfo.songName;
       } else if (!m_currentPicSongName.isEmpty()) {
           // 其次使用单独封面请求（由 requestCoverByUrl 触发）
           targetSongName = m_currentPicSongName;
       } else {
           return; // 无有效请求，忽略
       }

       QPixmap pixmap;
       if (pixmap.loadFromData(bytes)) {
           saveCoverToFile(targetSongName, bytes);
           emit sig_coverReady(targetSongName, pixmap);
       }

       // 清除单独封面请求的标记
       if (!m_currentPicSongName.isEmpty()) {
           m_currentPicSongName.clear();
       }
}

void OnlineMusicPage::saveLyricToFile(const QString &songName, const QString &lyricText)
{
    QString lrcPath = QCoreApplication::applicationDirPath() + "/lrc/";
    QDir dir(lrcPath);
    if (!dir.exists()) dir.mkpath(lrcPath);

    QString filePath = lrcPath + songName + ".lrc";
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(lyricText.toUtf8());
        file.close();
        qDebug() << "歌词保存成功:" << filePath;
    }
}

void OnlineMusicPage::saveCoverToFile(const QString &songName, const QByteArray &imageData)
{
    QBuffer buffer(const_cast<QByteArray*>(&imageData));
    if (!buffer.open(QIODevice::ReadOnly)) return;

    QByteArray format = QImageReader::imageFormat(&buffer);
    if (format.isEmpty()) format = "jpg";  // 默认 jpg

    QString extension = QString::fromUtf8(format).toLower();
    // 统一将 "jpeg" 转为 "jpg" 以保持简洁
    if (extension == "jpeg") extension = "jpg";

    QString imgPath = QCoreApplication::applicationDirPath() + "/img/";
    QDir dir(imgPath);
    if (!dir.exists()) dir.mkpath(imgPath);

    QString filePath = imgPath + songName + "." + extension;
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(imageData);
        file.close();
        qDebug() << "封面保存成功:" << filePath;
    }
}
