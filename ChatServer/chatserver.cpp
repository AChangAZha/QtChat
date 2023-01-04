#include "chatserver.h"

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include "idatabase.h"
#include "serverworker.h"
ChatServer::ChatServer(QObject *parent) : QTcpServer(parent) {}

void ChatServer::incomingConnection(qintptr handle)
{
  ServerWorker *worker = new ServerWorker(this);
  if (!worker->setSocketDescriptor(handle))
  {
    worker->deleteLater();
    return;
  }
  connect(worker, &ServerWorker::logMessage, this, &ChatServer::logMessage);
  connect(worker, &ServerWorker::jsonReceived, this, &ChatServer::jsonReceived);
  connect(worker, &ServerWorker::disconnectedFromClient, this,
          std::bind(&ChatServer::userDisconnected, this, worker));
  m_clients.append(worker);
  emit logMessage("新的用户连接上了");
}

void ChatServer::broadcast(const QJsonObject &message, ServerWorker *exclude)
{
  for (ServerWorker *worker : m_clients)
  {
    worker->sendJson(message);
  }
}

void ChatServer::stopServer() { close(); }

void ChatServer::jsonReceived(ServerWorker *sender, const QJsonObject &docObj)
{
  const QJsonValue typeVal = docObj.value("type");
  if (typeVal.isNull() || !typeVal.isString())
    return;
  if (typeVal.toString().compare("message", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue textVal = docObj.value("text");
    if (textVal.isNull() || !textVal.isString())
      return;
    const QString text = textVal.toString().trimmed();
    if (text.isEmpty())
      return;
    const QString to = docObj.value("to").toString().trimmed();
    QJsonObject message;
    message["type"] = "message";
    message["text"] = text;
    message["sender"] = sender->userName();
    message["time"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    IDatabase::getInstance().addChatRecord(sender->userID(), "0", text, message["time"].toString());
    if (to.isEmpty())
      broadcast(message, sender);
    else
    {
      for (ServerWorker *worker : m_clients)
      {
        if (worker->userName().compare(to) == 0 || worker == sender)
          worker->sendJson(message);
      }
    }
  }
  else if (typeVal.toString().compare("login", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue usernameVal = docObj.value("text");
    const QJsonValue passwordVal = docObj.value("pwd");
    if (usernameVal.isNull() || !usernameVal.isString() || passwordVal.isNull() ||
        !passwordVal.isString())
      return;
    for (ServerWorker *worker : m_clients)
    {
      // 用户已登录
      if (worker->userName().compare(usernameVal.toString()) == 0)
      {
        QJsonObject logout;
        logout["type"] = "logout";
        sender->sendJson(logout);
        return;
      }
    }
    QString msg = IDatabase::getInstance().userLogin(usernameVal.toString(),
                                                     passwordVal.toString());
    if (msg.compare("Login successful") != 0) // 登录失败
    {
      QJsonObject login;
      login["type"] = "login";
      login["text"] = msg;
      sender->sendJson(login);
      return;
    }
    sender->setUserName(usernameVal.toString());
    sender->setID(IDatabase::getInstance().getID(usernameVal.toString()));
    QJsonObject connectedMessage;
    connectedMessage["type"] = "newuser";
    connectedMessage["username"] = usernameVal.toString();
    broadcast(connectedMessage, sender);
    QJsonObject userListMessage;
    userListMessage["type"] = "userlist";
    QJsonArray userlist;
    for (ServerWorker *worker : m_clients)
    {
      if (worker == sender)
        userlist.append(worker->userName() + "*");
      else
        userlist.append(worker->userName());
    }
    userListMessage["userlist"] = userlist;
    sender->sendJson(userListMessage);
    QJsonObject chatRecord;
    chatRecord["type"] = "chatRecord";
    chatRecord["chatRecord"] = IDatabase::getInstance().getChatRecord("", "0");
    sender->sendJson(chatRecord);
  }
  else if (typeVal.toString().compare("register", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue usernameVal = docObj.value("text");
    const QJsonValue passwordVal = docObj.value("pwd");
    if (usernameVal.isNull() || !usernameVal.isString() ||
        passwordVal.isNull() || !passwordVal.isString())
      return;
    QString msg = IDatabase::getInstance().userRegister(usernameVal.toString(),
                                                        passwordVal.toString());
    QJsonObject reg;
    reg["type"] = "register";
    reg["text"] = msg;
    sender->sendJson(reg);
  }
}

void ChatServer::userDisconnected(ServerWorker *sender)
{
  m_clients.removeAll(sender);
  const QString userName = sender->userName();
  if (!userName.isEmpty())
  {
    QJsonObject disconectedMessage;
    disconectedMessage["type"] = "userdisconnected";
    disconectedMessage["username"] = userName;
    broadcast(disconectedMessage, nullptr);
    emit logMessage(userName + " disconnected");
  }
  sender->deleteLater();
}
