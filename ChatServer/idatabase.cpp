#include "idatabase.h"

QString IDatabase::userLogin(QString id, QString password)
{
    QSqlQuery query;
    query.prepare("SELECR * FROM users WHERE id = :id");
    query.bindValue(":id", id);
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
    return "User not found";
}

IDatabase::IDatabase(QObject *parent) : QObject(parent) { initDatabase(); }

void IDatabase::initDatabase()
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    QString aFile = "QtChat.db";
    database.setDatabaseName(aFile);
    database.open();
}
