#ifndef SERVERWORKER_H
#define SERVERWORKER_H

#include <QObject>
#include <QTcpSocket>

class ServerWorker : public QObject
{
  Q_OBJECT
public:
  explicit ServerWorker(QObject *parent = nullptr);
  virtual bool setSocketDescriptor(qintptr socketDescriptor);
  QString userName();
  QString userID();
  void setUserName(QString user);
  void setID(QString id);

signals:
  void logMessage(const QString &msg);
  void jsonReceived(ServerWorker *sender, const QJsonObject &docObj);
  void disconnectedFromClient();

private:
  QTcpSocket *m_serverSocket;
  QString m_userName;
  QString m_id;
public slots:
  void onReadyRead();
  void sendMessage(const QString &text, const QString &type = "message");
  void sendJson(const QJsonObject &json);
};

#endif // SERVERWORKER_H
