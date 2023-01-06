#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QListWidgetItem>
#include <QMainWindow>

#include "chatclient.h"

QT_BEGIN_NAMESPACE
namespace Ui
{
  class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void on_loginButton_clicked();
  void on_sayButton_clicked();
  void on_logoutButton_clicked();
  void connectedServer();
  void messageReceived(const QString &sender, const QString &text,
                       const QString &time);
  void jsonReceived(const QJsonObject &docObj);
  void userJoined(const QString &user);
  void userLeft(const QString &user);
  void userListReceived(const QStringList &list);

  void on_userListWidget_currentTextChanged(const QString &currentText);

  void on_registerBtn_clicked();

  void on_goToRegister_clicked();

  void on_goToChat_clicked();

  void on_back_clicked();

  void on_addBtn_clicked();

  void on_search_clicked();

  void on_resList_itemDoubleClicked(QListWidgetItem *item);

  void on_sendBtn_clicked();

  void on_applyList_itemDoubleClicked(QListWidgetItem *item);

  void on_pushButton_2_clicked();

  void on_pushButton_3_clicked();

  void on_pushButton_clicked();

  void on_creatGroup_clicked();

  void on_friendList_itemClicked(QListWidgetItem *item);

  void on_friendSend_clicked();

  void on_pushButton_5_clicked();

  void on_friendImg_clicked();

  void on_fileBtn_clicked();

  void on_groupList_itemClicked(QListWidgetItem *item);

  void on_invite_clicked();

  void on_pushButton_4_clicked();

private:
  Ui::MainWindow *ui;
  ChatClient *m_chatClient;
  QString to = "";
  QString init = "login";
  QString img(QString text);
  void saveFile(QString text, QString sender);
  QTimer *timer;
};
#endif // MAINWINDOW_H
