#ifndef MYTABLEWIDGETITEM_H
#define MYTABLEWIDGETITEM_H

#include <QTableWidgetItem>

struct MusicInfoData {
    QString songName;
    QString songID;
    QString singer;
    QString songPicUrl;
    QString albumName;
    QString playTime;
    int songSource;  // from_local=0, from_net=1
};

enum ENUM_SONG_SOURCE {
    from_local = 0,
    from_net
};

class MyTableWidgetItem : public QTableWidgetItem
{
public:
    MyTableWidgetItem();

    const MusicInfoData &musicInfoData() const;
    void setMusicInfoData(const MusicInfoData &newMusicInfoData);

private:
    MusicInfoData m_musicInfoData;
};

#endif // MYTABLEWIDGETITEM_H
