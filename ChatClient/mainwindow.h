#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
  void messageReceived(const QString &sender, const QString &text, const QString &time);
  void jsonReceived(const QJsonObject &docObj);
  void userJoined(const QString &user);
  void userLeft(const QString &user);
  void userListReceived(const QStringList &list);

  void on_userListWidget_currentTextChanged(const QString &currentText);

  void on_registerBtn_clicked();

  void on_goToRegister_clicked();

  void on_goToChat_clicked();

private:
  Ui::MainWindow *ui;
  ChatClient *m_chatClient;
  QString to = "";
  QString init = "login";
};
#endif // MAINWINDOW_H
