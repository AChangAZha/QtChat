#include "mainwindow.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow) {
  ui->setupUi(this);
  ui->stackedWidget->setCurrentWidget(ui->loginPage);
  m_chatClient = new ChatClient(this);
  connect(m_chatClient, &ChatClient::connected, this,
          &MainWindow::connectedServer);
  connect(m_chatClient, &ChatClient::jsonReceived, this,
          &MainWindow::jsonReceived);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_loginButton_clicked() {
  init = "login";
  m_chatClient->connectToServer(QHostAddress(ui->serverEdit->text()), 1967);
}

void MainWindow::on_sayButton_clicked() {
  if (!ui->sayLineEdit->text().isEmpty()) {
    QString tomsg = "";
    if (!to.isEmpty()) tomsg = "(私聊)";
    m_chatClient->sendMessage(ui->sayLineEdit->text() + tomsg, "message", to);
    ui->sayLineEdit->clear();
  }
}

void MainWindow::on_logoutButton_clicked() {
  m_chatClient->disconnectFromHost();
  ui->stackedWidget->setCurrentWidget(ui->loginPage);
  ui->userListWidget->clear();
}

void MainWindow::connectedServer() {
  if (init.compare("login") == 0) {
    m_chatClient->loginOrRegister(ui->usernameEdit->text(),
                                  ui->passwordEdit->text());
  } else if (init.compare("register") == 0) {
    m_chatClient->loginOrRegister(ui->username->text(), ui->password->text(),
                                  "register");
  }
}

void MainWindow::messageReceived(const QString &sender, const QString &text,
                                 const QString &time) {
  ui->roomTextEdit->append(
      QString("%1 %2\n%3").arg(sender).arg(time).arg(text));
}

void MainWindow::jsonReceived(const QJsonObject &docObj) {
  const QJsonValue typeVal = docObj.value("type");
  if (typeVal.isNull() || !typeVal.isString()) return;
  if (typeVal.toString().compare("message", Qt::CaseInsensitive) == 0) {
    const QJsonValue textVal = docObj.value("text");
    const QJsonValue senderVal = docObj.value("sender");
    const QJsonValue timeVal = docObj.value("time");
    if (textVal.isNull() || !textVal.isString()) return;
    if (senderVal.isNull() || !senderVal.isString()) return;
    if (timeVal.isNull() || !timeVal.isString()) return;
    messageReceived(senderVal.toString(), textVal.toString(),
                    timeVal.toString());
  } else if (typeVal.toString().compare("newuser", Qt::CaseInsensitive) == 0) {
    const QJsonValue usernameVal = docObj.value("username");
    if (usernameVal.isNull() || !usernameVal.isString()) return;
    userJoined(usernameVal.toString());
  } else if (typeVal.toString().compare("userdisconnected",
                                        Qt::CaseInsensitive) == 0) {
    const QJsonValue usernameVal = docObj.value("username");
    if (usernameVal.isNull() || !usernameVal.isString()) return;
    userLeft(usernameVal.toString());
  } else if (typeVal.toString().compare("userlist", Qt::CaseInsensitive) == 0) {
    ui->msg->setText("欢迎使用");
    const QJsonValue userlistVal = docObj.value("userlist");
    if (userlistVal.isNull() || !userlistVal.isArray()) return;
    userListReceived(userlistVal.toVariant().toStringList());
    ui->stackedWidget->setCurrentWidget(ui->homePage);
  } else if (typeVal.toString().compare("chatRecord", Qt::CaseInsensitive) ==
             0) {
    const QJsonValue chatRecordVal = docObj.value("chatRecord");
    if (chatRecordVal.isNull() || !chatRecordVal.isArray()) return;
    for (auto i : chatRecordVal.toArray()) {
      const QJsonObject obj = i.toObject();
      const QJsonValue textVal = obj.value("text");
      const QJsonValue senderVal = obj.value("sender");
      const QJsonValue timeVal = obj.value("time");
      if (textVal.isNull() || !textVal.isString()) continue;
      if (senderVal.isNull() || !senderVal.isString()) continue;
      if (timeVal.isNull() || !timeVal.isString()) continue;
      messageReceived(senderVal.toString(), textVal.toString(),
                      timeVal.toString());
    }
  } else if (typeVal.toString().compare("logout", Qt::CaseInsensitive) == 0) {
    on_logoutButton_clicked();
    ui->msg->setText("登录失败");
  } else if (typeVal.toString().compare("login", Qt::CaseInsensitive) == 0) {
    on_logoutButton_clicked();
    const QJsonValue msgVal = docObj.value("text");
    ui->msg->setText(msgVal.toString());
  } else if (typeVal.toString().compare("register", Qt::CaseInsensitive) == 0) {
    m_chatClient->disconnectFromHost();
    const QJsonValue msgVal = docObj.value("text");
    if (msgVal.toString() == "Register successful") {
      ui->msg->setText(msgVal.toString());
      ui->stackedWidget->setCurrentWidget(ui->loginPage);
      ui->usernameEdit->setText(ui->username->text());
    } else {
      ui->errMsg->setText(msgVal.toString());
    }
  }
}

void MainWindow::userJoined(const QString &user) {
  ui->userListWidget->addItem(user);
}

void MainWindow::userLeft(const QString &user) {
  for (auto aItem : ui->userListWidget->findItems(user, Qt::MatchExactly)) {
    ui->userListWidget->removeItemWidget(aItem);
    delete aItem;
  }
}

void MainWindow::userListReceived(const QStringList &list) {
  ui->userListWidget->clear();
  ui->userListWidget->addItems(list);
}

void MainWindow::on_userListWidget_currentTextChanged(
    const QString &currentText) {
  to = currentText;
  if (to.contains("*")) {
    to = "";
    ui->sayButton->setText("发送");
  } else
    ui->sayButton->setText("To " + currentText);
}

void MainWindow::on_registerBtn_clicked() {
  init = "register";
  m_chatClient->connectToServer(QHostAddress(ui->serverEdit->text()), 1967);
}

void MainWindow::on_goToRegister_clicked() {
  ui->stackedWidget->setCurrentWidget(ui->registerPage);
}

void MainWindow::on_goToChat_clicked() {
  ui->stackedWidget->setCurrentWidget(ui->chatPage);
}
