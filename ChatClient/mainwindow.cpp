#include "mainwindow.h"

#include <QHostAddress>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QInputDialog>

#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow)
{
  ui->setupUi(this);
  ui->stackedWidget->setCurrentWidget(ui->loginPage);
  m_chatClient = new ChatClient(this);
  connect(m_chatClient, &ChatClient::connected, this,
          &MainWindow::connectedServer);
  connect(m_chatClient, &ChatClient::jsonReceived, this,
          &MainWindow::jsonReceived);
}

MainWindow::~MainWindow() { delete ui; }

void MainWindow::on_loginButton_clicked()
{
  init = "login";
  m_chatClient->connectToServer(QHostAddress(ui->serverEdit->text()), 1967);
}

void MainWindow::on_sayButton_clicked()
{
  if (!ui->sayLineEdit->text().isEmpty())
  {
    QString tomsg = "";
    if (!to.isEmpty())
      tomsg = "(私聊)";
    m_chatClient->sendMessage(ui->sayLineEdit->text() + tomsg, "message", to);
    ui->sayLineEdit->clear();
  }
}

void MainWindow::on_logoutButton_clicked()
{
  m_chatClient->disconnectFromHost();
  ui->stackedWidget->setCurrentWidget(ui->loginPage);
  ui->userListWidget->clear();
}

void MainWindow::connectedServer()
{
  if (init.compare("login") == 0)
  {
    m_chatClient->loginOrRegister(ui->usernameEdit->text(),
                                  ui->passwordEdit->text());
  }
  else if (init.compare("register") == 0)
  {
    m_chatClient->loginOrRegister(ui->username->text(), ui->password->text(),
                                  "register");
  }
}

void MainWindow::messageReceived(const QString &sender, const QString &text,
                                 const QString &time)
{
  ui->roomTextEdit->append(
      QString("%1 %2\n%3").arg(sender).arg(time).arg(text));
}

void MainWindow::jsonReceived(const QJsonObject &docObj)
{
  const QJsonValue typeVal = docObj.value("type");
  if (typeVal.isNull() || !typeVal.isString())
    return;
  if (typeVal.toString().compare("message", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue textVal = docObj.value("text");
    const QJsonValue senderVal = docObj.value("sender");
    const QJsonValue timeVal = docObj.value("time");
    if (textVal.isNull() || !textVal.isString())
      return;
    if (senderVal.isNull() || !senderVal.isString())
      return;
    if (timeVal.isNull() || !timeVal.isString())
      return;
    messageReceived(senderVal.toString(), textVal.toString(),
                    timeVal.toString());
  }
  else if (typeVal.toString().compare("newuser", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue usernameVal = docObj.value("username");
    if (usernameVal.isNull() || !usernameVal.isString())
      return;
    userJoined(usernameVal.toString());
  }
  else if (typeVal.toString().compare("userdisconnected",
                                      Qt::CaseInsensitive) == 0)
  {
    const QJsonValue usernameVal = docObj.value("username");
    if (usernameVal.isNull() || !usernameVal.isString())
      return;
    userLeft(usernameVal.toString());
  }
  else if (typeVal.toString().compare("userlist", Qt::CaseInsensitive) == 0)
  {
    ui->msg->setText("欢迎使用");
    const QJsonValue userlistVal = docObj.value("userlist");
    if (userlistVal.isNull() || !userlistVal.isArray())
      return;
    userListReceived(userlistVal.toVariant().toStringList());
    ui->stackedWidget->setCurrentWidget(ui->homePage);
  }
  else if (typeVal.toString().compare("chatRecord", Qt::CaseInsensitive) ==
           0)
  {
    ui->roomTextEdit->clear();
    const QJsonValue chatRecordVal = docObj.value("chatRecord");
    if (chatRecordVal.isNull() || !chatRecordVal.isArray())
      return;
    for (auto i : chatRecordVal.toArray())
    {
      const QJsonObject obj = i.toObject();
      const QJsonValue textVal = obj.value("text");
      const QJsonValue senderVal = obj.value("sender");
      const QJsonValue timeVal = obj.value("time");
      if (textVal.isNull() || !textVal.isString())
        continue;
      if (senderVal.isNull() || !senderVal.isString())
        continue;
      if (timeVal.isNull() || !timeVal.isString())
        continue;
      messageReceived(senderVal.toString(), textVal.toString(),
                      timeVal.toString());
    }
  }
  else if (typeVal.toString().compare("logout", Qt::CaseInsensitive) == 0)
  {
    on_logoutButton_clicked();
    ui->msg->setText("登录失败");
  }
  else if (typeVal.toString().compare("login", Qt::CaseInsensitive) == 0)
  {
    on_logoutButton_clicked();
    const QJsonValue msgVal = docObj.value("text");
    ui->msg->setText(msgVal.toString());
  }
  else if (typeVal.toString().compare("register", Qt::CaseInsensitive) == 0)
  {
    m_chatClient->disconnectFromHost();
    const QJsonValue msgVal = docObj.value("text");
    if (msgVal.toString() == "Register successful")
    {
      ui->msg->setText(msgVal.toString());
      ui->stackedWidget->setCurrentWidget(ui->loginPage);
      ui->usernameEdit->setText(ui->username->text());
    }
    else
    {
      ui->errMsg->setText(msgVal.toString());
    }
  }
  // 搜索结果
  else if (typeVal.toString().compare("search", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue searchVal = docObj.value("searchList");
    if (searchVal.isNull() || !searchVal.isArray())
      return;
    ui->resList->clear();
    ui->resList->addItems(searchVal.toVariant().toStringList());
  }
  // 申请
  else if (typeVal.toString().compare("apply", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue applyVal = docObj.value("apply");
    if (applyVal.isNull() || !applyVal.isArray())
      return;
    ui->applyList->clear();
    ui->applyList->addItems(applyVal.toVariant().toStringList());
  }
  // 新申请
  else if (typeVal.toString().compare("newApply", Qt::CaseInsensitive) == 0)
  {
    ui->addBtn->setText("新好友");
    const QJsonValue messageVal = docObj.value("message");
    if (messageVal.isNull() || !messageVal.isString())
      return;
    ui->applyList->addItem(messageVal.toString());
  }
  // 好友列表
  else if (typeVal.toString().compare("friendList", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue friendListVal = docObj.value("friendList");
    if (friendListVal.isNull() || !friendListVal.isArray())
      return;
    ui->friendList->clear();
    ui->friendList->addItems(friendListVal.toVariant().toStringList());
  }
  // 群组列表
  else if (typeVal.toString().compare("groupList", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue groupListVal = docObj.value("groupList");
    if (groupListVal.isNull() || !groupListVal.isArray())
      return;
    ui->groupList->clear();
    ui->groupList->addItems(groupListVal.toVariant().toStringList());
  }
}

void MainWindow::userJoined(const QString &user)
{
  ui->userListWidget->addItem(user);
  // 遍历好友列表
  for (int i = 0; i < ui->friendList->count(); i++)
  {
    // 提取括号前的用户名
    QString friendName = ui->friendList->item(i)->text();
    friendName = friendName.left(friendName.indexOf("("));
    // 把好友列表中的用户名和新加入的用户名比较
    if (friendName == user)
    {
      // 如果相同，就把好友列表中的用户名改成在线状态
      // 修改[离线]为[在线]
      QString friendName = ui->friendList->item(i)->text();
      friendName.replace("离线", "在线");
      ui->friendList->item(i)->setText(friendName);
    }
  }
}

void MainWindow::userLeft(const QString &user)
{
  for (auto aItem : ui->userListWidget->findItems(user, Qt::MatchExactly))
  {
    ui->userListWidget->removeItemWidget(aItem);
    delete aItem;
  }
  for (int i = 0; i < ui->friendList->count(); i++)
  {
    // 提取括号前的用户名
    QString friendName = ui->friendList->item(i)->text();
    friendName = friendName.left(friendName.indexOf("("));
    // 把好友列表中的用户名和新加入的用户名比较
    if (friendName == user)
    {
      // 如果相同，就把好友列表中的用户名改成在线状态
      // 修改[离线]为[在线]
      QString friendName = ui->friendList->item(i)->text();
      friendName.replace("在线", "离线");
      ui->friendList->item(i)->setText(friendName);
    }
  }
}

void MainWindow::userListReceived(const QStringList &list)
{
  ui->userListWidget->clear();
  ui->userListWidget->addItems(list);
}

void MainWindow::on_userListWidget_currentTextChanged(
    const QString &currentText)
{
  to = currentText;
  if (to.contains("*"))
  {
    to = "";
    ui->sayButton->setText("发送");
  }
  else
    ui->sayButton->setText("To " + currentText);
}

void MainWindow::on_registerBtn_clicked()
{
  init = "register";
  m_chatClient->connectToServer(QHostAddress(ui->serverEdit->text()), 1967);
}

void MainWindow::on_goToRegister_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->registerPage);
}

void MainWindow::on_goToChat_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->chatPage);
}

void MainWindow::on_back_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->homePage);
}

void MainWindow::on_addBtn_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->addPage);
  ui->addBtn->setText("添加好友/群组");
}

void MainWindow::on_search_clicked()
{
  QString name = ui->searchEdit->text();
  if (name == "")
  {
    return;
  }
  m_chatClient->sendMessage(name, "search");
}

void MainWindow::on_resList_itemDoubleClicked(QListWidgetItem *item)
{
  QString res = item->text();
  if (res == "")
  {
    return;
  }
  // 提取res中"("之前的内容
  int pos = res.indexOf("(");
  if (pos == -1)
  {
    return;
  }
  QString username = res.left(pos);
  // 提取res括号中的内容
  int pos2 = res.indexOf(")");
  if (pos2 == -1)
  {
    return;
  }
  QString id = res.mid(pos + 1, pos2 - pos - 1);
  // 如果id含有“群组”
  if (id.contains("群组"))
  {
    // 将id中的群组去掉
    id.replace("群组", "");
    // 发送添加群组请求
    m_chatClient->sendMessage(id, "addGroup");
    return;
  }
  // 遍历好友列表
  for (int i = 0; i < ui->friendList->count(); i++)
  {
    QString friendName = ui->friendList->item(i)->text();
    if (friendName.compare(username + "(" + id + ")[在线]") == 0 ||
        friendName.compare(username + "(" + id + ")[离线]") == 0)
    {
      // 弹出窗口
      QMessageBox::information(this, "提示", "你们已经是好友了");
      return;
    }
  }
  ui->idLabel->setText(id);
  ui->usernameLabel->setText(username);
  ui->stackedWidget->setCurrentWidget(ui->sendPage);
}

void MainWindow::on_sendBtn_clicked()
{
  QString username = ui->usernameLabel->text();
  QString send = ui->sendEdit->toPlainText();
  if (username == "")
  {
    return;
  }
  m_chatClient->sendMessage(send, "add", username);
  ui->sendEdit->clear();
  ui->stackedWidget->setCurrentWidget(ui->addPage);
}

void MainWindow::on_applyList_itemDoubleClicked(QListWidgetItem *item)
{
  QString res = item->text();
  // 如果含有“同意”则返回
  if (res.contains("同意"))
  {
    return;
  }
  if (res == "")
  {
    return;
  }
  // 提取res中"("之前的内容
  int pos = res.indexOf("(");
  if (pos == -1)
  {
    return;
  }
  QString username = res.left(pos);
  // 提取res括号中的内容
  int pos2 = res.indexOf(")");
  if (pos2 == -1)
  {
    return;
  }
  QString id = res.mid(pos + 1, pos2 - pos - 1);
  // 发送消息
  m_chatClient->sendMessage(username, "accept");
}

void MainWindow::on_pushButton_2_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->addPage);
}

void MainWindow::on_pushButton_3_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->loginPage);
}

void MainWindow::on_pushButton_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->homePage);
}

void MainWindow::on_creatGroup_clicked()
{
  // 弹出窗口，输入群名
  bool ok;
  QString text = QInputDialog::getText(this, tr("创建群组"),
                                       tr("请输入群组名:"), QLineEdit::Normal,
                                       "", &ok);
  if (ok && !text.isEmpty())
  {
    // 发送消息
    m_chatClient->sendMessage(text, "createGroup");
  }
}
