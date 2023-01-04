#ifndef IDATABASE_H
#define IDATABASE_H

#include <QObject>
#include <QSqlDatabase>
#include <QtSql>

class IDatabase : public QObject
{
  Q_OBJECT
public:
  static IDatabase &getInstance()
  {
    // 单例模式
    static IDatabase instance;
    return instance;
  }
  QString userLogin(QString username, QString password);
  QString userRegister(QString username, QString password);
  void addChatRecord(QString id, QString toID, QString message, QString time);
  QString getID(QString username);
  QString getUserName(QString id);
  QJsonArray getChatRecord(QString id, QString toID);
  QJsonArray searchFriend(QString keyword);
  void addApply(QString id, QString toID, QString message, QString time, QString status);
  QJsonArray getApply(QString id);
  void updateApply(QString id, QString toID);
  void addFriend(QString id, QString friendID);
  QJsonArray getFriendList(QString id);

private:
  explicit IDatabase(QObject *parent = nullptr);
  IDatabase(IDatabase const &) = delete;
  void operator=(IDatabase const &) = delete;
  QSqlDatabase database;
  void initDatabase();
};

#endif // IDATABASE_H
