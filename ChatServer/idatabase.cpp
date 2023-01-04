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
    query.prepare("INSERT INTO users (username, password) VALUES (:username, :password)");
    query.bindValue(":username", username);
    query.bindValue(":password", password);
    query.exec();
    return "Register successful";
}

void IDatabase::addChatRecord(QString id, QString toID, QString message, QString time)
{
    QSqlQuery query;
    query.prepare("INSERT INTO chatrecord (id, toID, message, time) VALUES (:id, :toID, :message, :time)");
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
        query.prepare("SELECT * FROM chatrecord WHERE id = :id AND toID = :toID");
        query.bindValue(":id", id);
        query.bindValue(":toID", toID);
        query.exec();
    }
    while (query.next())
    {
        QJsonObject record;
        record.insert("sender", getUserName(query.value("id").toString()));
        record.insert("toID", query.value("toID").toString());
        record.insert("text", query.value("message").toString());
        record.insert("time", query.value("time").toString());
        chatRecord.append(record);
    }
    return chatRecord;
}

IDatabase::IDatabase(QObject *parent) : QObject(parent) { initDatabase(); }

void IDatabase::initDatabase()
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    QString aFile = "QtChat.db";
    database.setDatabaseName(aFile);
    database.open();
}
