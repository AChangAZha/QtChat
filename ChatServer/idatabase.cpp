#include "idatabase.h"

QString IDatabase::userLogin(QString username, QString password)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM users WHERE username = :username");
    query.bindValue(":username", username);
    query.exec();
    if (query.next())
    {
        if (query.value("password").toString() == password)
        {
            return "Login successful";
        }
        return "Wrong password";
    }
    return "User not found";
}

QString IDatabase::userRegister(QString username, QString password)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM users WHERE username = :username");
    query.bindValue(":username", username);
    query.exec();
    if (query.next())
    {
        return "User already exists";
    }
    query.prepare(
        "INSERT INTO users (username, password) VALUES (:username, :password)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.exec();
    return "Register successful";
}

void IDatabase::addChatRecord(QString id, QString toID, QString message,
                              QString time)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO chatrecord (id, toID, message, time) VALUES (:id, :toID, "
        ":message, :time)");
    query.bindValue(":id", id);
    query.bindValue(":toID", toID);
    query.bindValue(":message", message);
    query.bindValue(":time", time);
    query.exec();
}

QString IDatabase::getID(QString username)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM users WHERE username = :username");
    query.bindValue(":username", username);
    query.exec();
    if (query.next())
    {
        return query.value("id").toString();
    }
    return "";
}

QString IDatabase::getUserName(QString id)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM users WHERE id = :id");
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
    {
        return query.value("username").toString();
    }
    return "";
}

QString IDatabase::getLastTime(QString id)
{
    QSqlQuery query;
    query.prepare("SELECT * FROM users WHERE id = :id");
    query.bindValue(":id", id);
    query.exec();
    if (query.next())
    {
        return query.value("lasttime").toString();
    }
    return "";
}

QJsonArray IDatabase::getChatRecord(QString id, QString toID)
{
    QJsonArray chatRecord;
    QSqlQuery query;
    if (id.isEmpty())
    {
        query.prepare("SELECT * FROM chatrecord WHERE toID = :toID");
        query.bindValue(":toID", toID);
        query.exec();
    }
    else
    {
        query.prepare(
            "SELECT * FROM chatrecord WHERE (id = :id AND toID = :toID) "
            "OR (id = :toID AND toID = :id)");
        query.bindValue(":id", id);
        query.bindValue(":toID", toID);
        query.exec();
    }
    while (query.next())
    {
        QJsonObject record;
        record.insert("sender", getUserName(query.value("id").toString()));
        record.insert("toID", query.value("toID").toString());
        record.insert("to", getUserName(query.value("toID").toString()));
        record.insert("text", query.value("message").toString());
        record.insert("time", query.value("time").toString());
        chatRecord.append(record);
    }
    return chatRecord;
}

QJsonArray IDatabase::getLastChatRecord(QString id, QString time)
{
    QJsonArray chatRecord;
    QSqlQuery query;
    query.prepare(
        "SELECT * FROM chatrecord WHERE time > DATETIME(:time) AND toID = :id");
    query.bindValue(":time", time);
    query.bindValue(":id", id);
    query.exec();
    while (query.next())
    {
        QJsonObject record;
        record.insert("friend", getUserName(query.value("id").toString()));
        record.insert("sender", getUserName(query.value("id").toString()));
        record.insert("type", "message");
        chatRecord.append(record);
    }
    return chatRecord;
}

QJsonArray IDatabase::searchFriend(QString id, QString keyword)
{
    // 先通过ID查找，再通过用户名查找
    QJsonArray friendList;
    QSqlQuery query;
    query.prepare("SELECT * FROM users WHERE id = :keyword AND id != :id");
    query.bindValue(":id", id);
    query.bindValue(":keyword", keyword);
    query.exec();
    while (query.next())
    {
        friendList.append(query.value("username").toString() + "(" +
                          query.value("id").toString() + ")");
    }
    query.prepare(
        "SELECT * FROM users WHERE username LIKE :keyword AND id != :id");
    query.bindValue(":keyword", "%" + keyword + "%");
    query.bindValue(":id", id);
    query.exec();
    while (query.next())
    {
        friendList.append(query.value("username").toString() + "(" +
                          query.value("id").toString() + ")");
    }
    // 查询群组，先通过ID查找，再通过群名查找
    query.prepare("SELECT * FROM groups WHERE id = :keyword");
    query.bindValue(":keyword", keyword);
    query.exec();
    while (query.next())
    {
        friendList.append(query.value("name").toString() + "(群组" +
                          query.value("id").toString() + ")");
    }
    query.prepare("SELECT * FROM groups WHERE name LIKE :keyword");
    query.bindValue(":keyword", "%" + keyword + "%");
    query.exec();
    while (query.next())
    {
        friendList.append(query.value("name").toString() + "(群组" +
                          query.value("id").toString() + ")");
    }
    return friendList;
}

void IDatabase::addApply(QString id, QString toID, QString message,
                         QString time, QString status)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO apply (id, toID, message, time, status) VALUES (:id, :toID, "
        ":message, :time, :status)");
    query.bindValue(":id", id);
    query.bindValue(":toID", toID);
    query.bindValue(":message", message);
    query.bindValue(":time", time);
    query.bindValue(":status", status);
    query.exec();
}

QJsonArray IDatabase::getApply(QString id)
{
    QJsonArray applyList;
    QSqlQuery query;
    query.prepare("SELECT * FROM apply WHERE toID = :toID");
    query.bindValue(":toID", id);
    query.exec();
    while (query.next())
    {
        if (query.value("status").toString() == "0")
        {
            QString message = query.value("message").toString();
            if (!message.isEmpty())
            {
                message = ": " + message;
            }
            applyList.append(getUserName(query.value("id").toString()) + "(" +
                             query.value("id").toString() + ")申请添加你为好友" +
                             message);
        }
        else if (query.value("status").toString() == "1")
        {
            applyList.append("您已同意" + getUserName(query.value("id").toString()) +
                             "(" + query.value("id").toString() + ")的好友申请");
        }
        else
        {
            applyList.append(getUserName(query.value("id").toString()) + "(" +
                             query.value("id").toString() + ")已同意您的好友申请");
        }
    }
    return applyList;
}

void IDatabase::updateApply(QString id, QString toID)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE apply SET status = :status WHERE id = :id AND toID = :toID");
    query.bindValue(":status", "1");
    query.bindValue(":id", id);
    query.bindValue(":toID", toID);
    query.exec();
}

void IDatabase::addFriend(QString id, QString friendID, QString type)
{
    QSqlQuery query;
    query.prepare(
        "INSERT INTO friend (id, friendID, type) VALUES (:id, :friendID, :type)");
    query.bindValue(":id", id);
    query.bindValue(":friendID", friendID);
    query.bindValue(":type", type);
    query.exec();
}

void IDatabase::addGroup(QString id, QString name)
{
    QSqlQuery query;
    query.prepare("INSERT INTO groups (id, name) VALUES (:id, :name)");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.exec();
}

QJsonArray IDatabase::getFriendList(QString id)
{
    QJsonArray friendList;
    QSqlQuery query;
    query.prepare("SELECT * FROM friend WHERE id = :id AND type = :type");
    query.bindValue(":id", id);
    query.bindValue(":type", "0");
    query.exec();
    while (query.next())
    {
        friendList.append(getUserName(query.value("friendID").toString()) + "(" +
                          query.value("friendID").toString() + ")");
    }
    return friendList;
}

QJsonArray IDatabase::getGroupList(QString id)
{
    QJsonArray groupList;
    QSqlQuery query;
    // 查询friend表中type为1的记录，与groups联合查询
    query.prepare(
        "SELECT * FROM friend, groups WHERE friend.id = :id AND friend.type = "
        ":type AND friend.friendID = groups.id");
    query.bindValue(":id", id);
    query.bindValue(":type", "1");
    query.exec();
    while (query.next())
    {
        groupList.append(query.value("groups.name").toString() + "(" +
                         query.value("groups.id").toString() + ")");
    }
    return groupList;
}

void IDatabase::updateLastTime(QString username)
{
    QSqlQuery query;
    query.prepare(
        "UPDATE users SET lasttime = :lastTime WHERE username = :username");
    query.bindValue(":lastTime",
                    QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss"));
    query.bindValue(":username", username);
    query.exec();
}

IDatabase::IDatabase(QObject *parent) : QObject(parent) { initDatabase(); }

void IDatabase::initDatabase()
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    QString aFile = "QtChat.db";
    database.setDatabaseName(aFile);
    database.open();
}
