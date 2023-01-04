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

QJsonArray ChatServer::getFriendList(QString id)
{
  QJsonArray friendlist = IDatabase::getInstance().getFriendList(id);
  // 判断好友是否在线
  for (int i = 0; i < friendlist.size(); i++)
  {
    QString friendName = friendlist[i].toString();
    // 提取"("之前的内容
    QString username = friendName.left(friendName.indexOf("("));
    for (ServerWorker *worker : m_clients)
    {
      if (worker->userName().compare(username) == 0)
      {
        friendlist[i] = friendName + "[在线]";
        break;
      }
    }
    friendlist[i] = friendName + "[离线]";
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
    QJsonObject apply;
    apply["type"] = "apply";
    apply["apply"] = IDatabase::getInstance().getApply(sender->userID());
    sender->sendJson(apply);
    QJsonObject friendList;
    friendList["type"] = "friendList";
    friendList["friendList"] = IDatabase::getInstance().getFriendList(sender->userID());
    sender->sendJson(friendList);
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
  // 搜索
  else if (typeVal.toString().compare("search", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue searchVal = docObj.value("text");
    if (searchVal.isNull() || !searchVal.isString())
      return;
    QJsonObject search;
    search["type"] = "search";
    search["searchList"] = IDatabase::getInstance().searchFriend(searchVal.toString());
    sender->sendJson(search);
  }
  // 添加申请
  else if (typeVal.toString().compare("add", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue addVal = docObj.value("text");
    if (addVal.isNull() || !addVal.isString())
      return;
    const QString to = docObj.value("to").toString().trimmed();
    QString time = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    IDatabase::getInstance().addApply(sender->userID(), IDatabase::getInstance().getID(to), addVal.toString(), time, "0");
    for (ServerWorker *worker : m_clients)
    {
      if (worker->userName().compare(to) == 0)
      {
        QJsonObject apply;
        QString msg = addVal.toString();
        if (!msg.isEmpty())
        {
          msg = ": " + msg;
        }
        apply["type"] = "newApply";
        apply["message"] = sender->userName() + "(" + sender->userID() + ")申请添加你为好友：" + msg;
        worker->sendJson(apply);
      }
    }
  }
  // 接受申请
  else if (typeVal.toString().compare("accept", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue acceptVal = docObj.value("text");
    IDatabase::getInstance().addFriend(sender->userID(), IDatabase::getInstance().getID(acceptVal.toString()));
    IDatabase::getInstance().updateApply(sender->userID(), IDatabase::getInstance().getID(acceptVal.toString()));
    QJsonObject apply;
    apply["type"] = "apply";
    apply["apply"] = IDatabase::getInstance().getApply(sender->userID());
    sender->sendJson(apply);
    QJsonObject friendList;
    friendList["type"] = "friendList";
    friendList["friendList"] = getFriendList(sender->userID());
    sender->sendJson(friendList);
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
