#include "netrequest.h"
#include "qDebug"
NetRequest::NetRequest(QObject *parent) : QObject(parent)
{
    network_manager = new QNetworkAccessManager;
    network_request = new QNetworkRequest;

    connect( network_manager , SIGNAL(finished(QNetworkReply*))
             , this , SLOT( slot_replyFinished(QNetworkReply*)) );
}

void NetRequest::slot_sendUrlRequest(QString url)
{
    network_request->setUrl( QUrl(url) );
    network_manager->get( *network_request );
}

#include<QFile>
#include<QCoreApplication>

void NetRequest::slot_replyFinished(QNetworkReply *reply)
{
    if( reply->error() == QNetworkReply::NoError )
    {
//        qDebug() << __func__ <<"success";
        QByteArray bt = reply->readAll();

        Q_EMIT SIG_getResult( 200,  bt );

    }else{
//        qDebug() << "发生错误";
        QByteArray bt;
        Q_EMIT SIG_getResult( -1, bt );
    }
    reply->deleteLater();
}
