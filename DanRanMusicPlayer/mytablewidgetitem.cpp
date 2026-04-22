#include "mytablewidgetitem.h"

MyTableWidgetItem::MyTableWidgetItem() {}

const MusicInfoData &MyTableWidgetItem::musicInfoData() const
{
    return m_musicInfoData;
}

void MyTableWidgetItem::setMusicInfoData(const MusicInfoData &newMusicInfoData)
{
    m_musicInfoData = newMusicInfoData;
}
