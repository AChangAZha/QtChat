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
  QString getLastTime(QString id);
  QJsonArray getChatRecord(QString id, QString toID);
  QJsonArray getLastChatRecord(QString id, QString time);
  QJsonArray searchFriend(QString id, QString keyword);
  void addApply(QString id, QString toID, QString message, QString time, QString status);
  QJsonArray getApply(QString id);
  void updateApply(QString id, QString toID);
  void addFriend(QString id, QString friendID, QString type = "0");
  void addGroup(QString id, QString name);
  QJsonArray getFriendList(QString id);
  QJsonArray getGroupList(QString id);
  void updateLastTime(QString userName);
  QStringList getGroupMember(QString id);
  QJsonArray getGroupChatRecord(QString id);

private:
  explicit IDatabase(QObject *parent = nullptr);
  IDatabase(IDatabase const &) = delete;
  void operator=(IDatabase const &) = delete;
  QSqlDatabase database;
  void initDatabase();
};

#endif // IDATABASE_H
