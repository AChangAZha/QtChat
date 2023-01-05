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
    friendlist[i] = friendName + "[离线]";
    for (ServerWorker *worker : m_clients)
    {
      if (worker->userName().compare(username) == 0)
      {
        friendlist[i] = friendName + "[在线]";
        break;
      }
    }
  }
  return friendlist;
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
    QString to = docObj.value("to").toString().trimmed();
    const QJsonValue groupVal = docObj.value("group");
    QJsonObject message;
    message["type"] = "message";
    message["text"] = text;
    message["sender"] = sender->userName();
    message["time"] =
        QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
    if (to.isEmpty())
    {
      broadcast(message, sender);
      to = "0";
    }
    else if (groupVal.isNull() || !groupVal.isString())
    {
      for (ServerWorker *worker : m_clients)
      {
        if (worker->userName().compare(to) == 0)
        {
          message["friend"] = to;
          worker->sendJson(message);
        }
      }
      to = IDatabase::getInstance().getID(to);
    }
    if (text[0] != '#')
    {
      sender->sendJson(message);
      IDatabase::getInstance().addChatRecord(sender->userID(), to, text,
                                             message["time"].toString());
    }
  }
  else if (typeVal.toString().compare("login", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue usernameVal = docObj.value("text");
    const QJsonValue passwordVal = docObj.value("pwd");
    if (usernameVal.isNull() || !usernameVal.isString() ||
        passwordVal.isNull() || !passwordVal.isString())
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
    friendList["friendList"] = getFriendList(sender->userID());
    sender->sendJson(friendList);
    // 群组列表
    QJsonObject groupList;
    groupList["type"] = "groupList";
    groupList["groupList"] = IDatabase::getInstance().getGroupList(sender->userID());
    sender->sendJson(groupList);
    // 聊天记录
    QString lasttime = IDatabase::getInstance().getLastTime(sender->userID());
    QJsonArray chatRecord2 = IDatabase::getInstance().getLastChatRecord(sender->userID(), lasttime);
    // 遍历
    for (int i = 0; i < chatRecord2.size(); i++)
    {
      sender->sendJson(chatRecord2[i].toObject());
    }
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
    search["searchList"] = IDatabase::getInstance().searchFriend(
        sender->userID(), searchVal.toString());
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
    IDatabase::getInstance().addApply(sender->userID(),
                                      IDatabase::getInstance().getID(to),
                                      addVal.toString(), time, "0");
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
        apply["message"] = sender->userName() + "(" + sender->userID() +
                           ")申请添加你为好友" + msg;
        worker->sendJson(apply);
      }
    }
  }
  // 接受申请
  else if (typeVal.toString().compare("accept", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue acceptVal = docObj.value("text");
    IDatabase::getInstance().addFriend(
        sender->userID(), IDatabase::getInstance().getID(acceptVal.toString()));
    IDatabase::getInstance().addFriend(
        IDatabase::getInstance().getID(acceptVal.toString()), sender->userID());
    IDatabase::getInstance().updateApply(
        IDatabase::getInstance().getID(acceptVal.toString()), sender->userID());
    IDatabase::getInstance().addApply(
        sender->userID(), IDatabase::getInstance().getID(acceptVal.toString()),
        "", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"), "2");
    QJsonObject apply;
    apply["type"] = "apply";
    apply["apply"] = IDatabase::getInstance().getApply(sender->userID());
    sender->sendJson(apply);
    QJsonObject friendList;
    friendList["type"] = "friendList";
    friendList["friendList"] = getFriendList(sender->userID());
    sender->sendJson(friendList);
    // 向对方发送好友列表
    for (ServerWorker *worker : m_clients)
    {
      if (worker->userName().compare(acceptVal.toString()) == 0)
      {
        QJsonObject apply;
        apply["type"] = "apply";
        apply["apply"] = IDatabase::getInstance().getApply(worker->userID());
        worker->sendJson(apply);
        QJsonObject friendList;
        friendList["type"] = "friendList";
        friendList["friendList"] = getFriendList(worker->userID());
        worker->sendJson(friendList);
      }
    }
  }
  else if (typeVal.toString().compare("createGroup", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue groupNameVal = docObj.value("text");
    if (groupNameVal.isNull() || !groupNameVal.isString())
      return;
    const QString groupName = groupNameVal.toString();
    // 随机数作为群号，种子为当前时间
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    QString groupID = QString::number(qrand() % 1000000);
    IDatabase::getInstance().addGroup(groupID, groupName);
    IDatabase::getInstance().addFriend(sender->userID(), groupID, "1");
    QJsonObject group;
    // 发送groupList
    group["type"] = "groupList";
    group["groupList"] = IDatabase::getInstance().getGroupList(sender->userID());
    sender->sendJson(group);
  }
  else if (typeVal.toString().compare("addGroup", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue groupIDVal = docObj.value("text");
    if (groupIDVal.isNull() || !groupIDVal.isString())
      return;
    const QString groupID = groupIDVal.toString();
    IDatabase::getInstance().addFriend(sender->userID(), groupID, "1");
    QJsonObject group;
    // 发送groupList
    group["type"] = "groupList";
    group["groupList"] = IDatabase::getInstance().getGroupList(sender->userID());
    sender->sendJson(group);
  }
  // getChatRecord
  else if (typeVal.toString().compare("getChatRecord", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue toVal = docObj.value("text");
    if (toVal.isNull() || !toVal.isString())
      return;
    const QString to = toVal.toString();
    QJsonObject chatRecord;
    chatRecord["type"] = "friendChatRecord";
    chatRecord["chatRecord"] = IDatabase::getInstance().getChatRecord(sender->userID(), IDatabase::getInstance().getID(to));
    sender->sendJson(chatRecord);
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
    IDatabase::getInstance().updateLastTime(userName);
    emit logMessage(userName + " disconnected");
  }
  sender->deleteLater();
}
