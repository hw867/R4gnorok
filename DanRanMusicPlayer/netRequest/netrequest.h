#ifndef NETREQUEST_H
#define NETREQUEST_H

#include <QObject>

#include<QNetworkAccessManager>
#include<QNetworkRequest>
#include<QNetworkReply>

//网络请求类
//实现http/https网络请求 需要以下内容
// QT += network
// QNetworkAccessManager
// QNetworkRequest
// QNetworkReply



class NetRequest : public QObject
{
    Q_OBJECT
public:
    explicit NetRequest(QObject *parent = nullptr);

public slots:
    void slot_sendUrlRequest( QString url );

    void slot_replyFinished(QNetworkReply* reply );
signals:
    void SIG_getResult( int code , QByteArray &bt ); // 200 表示成功 -1表示错误
private:
    QNetworkAccessManager   *network_manager;
    QNetworkRequest         *network_request;
};

#endif // NETREQUEST_H
