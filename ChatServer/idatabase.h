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
  QString userLogin(QString id, QString password);
  QString userRegister(QString username, QString password);
  QString getUserName(QString id);

private:
  explicit IDatabase(QObject *parent = nullptr);
  IDatabase(IDatabase const &) = delete;
  void operator=(IDatabase const &) = delete;
  QSqlDatabase database;
  void initDatabase();
};

#endif // IDATABASE_H
