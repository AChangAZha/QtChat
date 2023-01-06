#include "mainwindow.h"

#include <QFile>
#include <QFileDialog>
#include <QHostAddress>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QMessageBox>
#include <QUuid>
#include <QTimer>

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
  if (ui->serverEdit->text().isEmpty() || ui->usernameEdit->text().isEmpty() ||
      ui->passwordEdit->text().isEmpty())
  {
    ui->msg->setText("请填写完整信息");
    return;
  }
  init = "login";
  m_chatClient->connectToServer(QHostAddress(ui->serverEdit->text()), 1967);
}

void MainWindow::on_sayButton_clicked()
{
  if (!ui->sayLineEdit->text().isEmpty())
  {
    QString tomsg = "";
    if (!to.isEmpty())
      tomsg = "@" + to + " ";
    m_chatClient->sendMessage(tomsg + ui->sayLineEdit->text());
    ui->sayLineEdit->clear();
  }
}

void MainWindow::on_logoutButton_clicked()
{
  m_chatClient->disconnectFromHost();
  ui->stackedWidget->setCurrentWidget(ui->loginPage);
  ui->userListWidget->clear();
  if (timer != nullptr)
  {
    timer->stop();
    delete timer;
    timer = nullptr;
    ui->addBtn->setText("添加好友/群组");
    ui->addBtn->setIcon(QIcon(":/new/prefix1/img/add.png"));
  }
  // 遍历ui->friendList
  for (int i = 0; i < ui->friendList->count(); i++)
  {
    // 获取data中的Timer
    QVariant var = ui->friendList->item(i)->data(Qt::UserRole);
    if (var.isValid())
    {
      // 停止Timer
      QTimer *timer = var.value<QTimer *>();
      timer->stop();
      delete timer;
      timer = nullptr;
    }
  }
  ui->friendList->clear();
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
  if (text[0] == '|')
  {
    ui->roomTextEdit->append(QString("%1 %2").arg(sender).arg(time));
    ui->roomTextEdit->append("<img src='" + img(text) + "'/>");
    return;
  }
  ui->roomTextEdit->append(
      QString("%1 %2\n%3").arg(sender).arg(time).arg(text));
}
QString MainWindow::img(QString text)
{
  // 生成uuid
  QUuid uuid = QUuid::createUuid();
  // 去除“{”和“}”
  QString str = uuid.toString().mid(1, 36);
  // 获取程序运行目录
  QString path = QCoreApplication::applicationDirPath();
  // 创建文件夹
  QDir dir(path + "/img");
  if (!dir.exists())
    dir.mkdir(path + "/img");
  QFile file(path + "/img/" + str);
  file.open(QIODevice::WriteOnly);
  file.write(QByteArray::fromBase64(text.mid(1).toUtf8()));
  file.close();
  return path + "/img/" + str;
}
void MainWindow::saveFile(QString text, QString sender)
{
  // 获取text中"]"之前的字符串
  QString str = text.mid(2, text.indexOf("]") - 2);
  // 获取text中"]"之后的字符串
  QString base64 = text.mid(text.indexOf("]") + 1);
  // 获取程序运行目录
  QString path = QCoreApplication::applicationDirPath();
  // 创建文件夹
  QDir dir(path + "/file");
  if (!dir.exists())
    dir.mkdir(path + "/file");
  QFile file(path + "/file/" + str);
  file.open(QIODevice::WriteOnly);
  file.write(QByteArray::fromBase64(base64.toUtf8()));
  file.close();
  QMessageBox::information(
      this, "提示",
      sender + "给你发送了一个文件，已保存到" + path + "/file/" + str);
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
    const QJsonValue friendVal = docObj.value("friend");
    if (senderVal.isNull() || !senderVal.isString())
      return;
    if (friendVal.isNull() || !friendVal.isString())
      messageReceived(senderVal.toString(), textVal.toString(),
                      timeVal.toString());
    else
    {
      if (textVal.toString()[0] == '#')
      {
        saveFile(textVal.toString(), senderVal.toString());
        return;
      }
      // 如果用户当前在friendChatPage
      if (ui->stackedWidget->currentWidget() == ui->friendChatPage)
      {
        // 如果当前聊天对象是sender
        if (senderVal.toString().compare(ui->friendName->text()) == 0 ||
            friendVal.toString().compare(ui->friendName->text()) == 0)
        {
          if (textVal.toString()[0] == '|')
          {
            ui->friendMsg->append(QString("%1 %2")
                                      .arg(senderVal.toString())
                                      .arg(timeVal.toString()));
            ui->friendMsg->append("<img src='" + img(textVal.toString()) +
                                  "'/>");
          }
          else
            ui->friendMsg->append(QString("%1 %2\n%3")
                                      .arg(senderVal.toString())
                                      .arg(timeVal.toString())
                                      .arg(textVal.toString()));
          return;
        }
      }
      // 如果当前聊天对象不是sender
      // 遍历好友列表
      for (int i = 0; i < ui->friendList->count(); i++)
      {
        QListWidgetItem *item = ui->friendList->item(i);
        // 提取括号前的内容
        QString name = item->text().split("(")[0];
        // 如果好友列表中有sender
        if (name.compare(senderVal.toString()) == 0 &&
            friendVal.toString().compare(ui->usernameEdit->text()) == 0)
        {
          // 将“线]-”后的数字加1，如果没找到，则在“线]”后加上“-1”
          QString text = item->text();
          int index = text.indexOf("]-");
          if (index != -1)
          {
            int num = text.mid(index + 2).toInt();
            text.replace(index + 2, text.length() - index - 2,
                         QString::number(num + 1));
          }
          else
          {
            text.replace("]", "]-1");
            // 将字体设置为蓝色
            // 设置Timer，让字体闪烁，把Timer绑定到item上
            QTimer *timer = new QTimer(this);
            timer->setInterval(500);
            timer->setSingleShot(false);
            timer->start();
            connect(timer, &QTimer::timeout, [=]()
                    {
            if (item->textColor() == QColor(0, 0, 255))
              item->setTextColor(QColor(0, 0, 0));
            else
              item->setTextColor(QColor(0, 0, 255)); });
            item->setData(Qt::UserRole, QVariant::fromValue(timer));
          }
          item->setText(text);
          // 置顶
          ui->friendList->takeItem(i);
          ui->friendList->insertItem(0, item);
          return;
        }
      }
      // 遍历群组列表
      for (int i = 0; i < ui->groupList->count(); i++)
      {
        QListWidgetItem *item = ui->groupList->item(i);
        // 提取括号内的内容
        QString name = item->text().split("(")[1].split(")")[0];
        if (name.compare(friendVal.toString()) == 0)
        {
          // 将“-”后的数字加1，如果没找到，则在最后加上“-1”
          QString text = item->text();
          int index = text.indexOf("-");
          if (index != -1)
          {
            int num = text.mid(index + 1).toInt();
            text.replace(index + 1, text.length() - index - 1,
                         QString::number(num + 1));
          }
          else
          {
            text.append("-1");
            // 设置Timer，让字体闪烁，把Timer绑定到item上
            QTimer *timer = new QTimer(this);
            timer->setInterval(500);
            timer->setSingleShot(false);
            timer->start();
            connect(timer, &QTimer::timeout, [=]()
                    {
            if (item->textColor() == QColor(0, 0, 255))
              item->setTextColor(QColor(0, 0, 0));
            else
              item->setTextColor(QColor(0, 0, 255)); });
            item->setData(Qt::UserRole, QVariant::fromValue(timer));
          }
          item->setText(text);
          // 置顶
          ui->groupList->takeItem(i);
          ui->groupList->insertItem(0, item);
          return;
        }
      }
    }
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
    ui->welcome->setText("欢迎你，" + ui->usernameEdit->text());
    ui->passwordEdit->setText("");
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
      ui->errMsg->setText("欢迎使用");
      ui->username->setText("");
      ui->password->setText("");
      ui->rePassword->setText("");
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
    // 按钮闪烁
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [=]()
            {
      if (ui->addBtn->text() == "新好友")
      {
        ui->addBtn->setText("");
        //隐藏icon
        ui->addBtn->setIcon(QIcon());
      }
      else
      {
        ui->addBtn->setText("新好友");
        //显示icon
        ui->addBtn->setIcon(QIcon(":/new/prefix1/img/add.png"));
      } });
    timer->start(500);
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
  // friendChatRecord
  else if (typeVal.toString().compare("friendChatRecord",
                                      Qt::CaseInsensitive) == 0)
  {
    const QJsonValue chatRecordVal = docObj.value("chatRecord");
    if (chatRecordVal.isNull() || !chatRecordVal.isArray())
      return;
    for (auto i : chatRecordVal.toArray())
    {
      const QJsonObject obj = i.toObject();
      const QJsonValue textVal = obj.value("text");
      const QJsonValue senderVal = obj.value("sender");
      const QJsonValue timeVal = obj.value("time");
      const QJsonValue toVal = obj.value("to");
      if (textVal.isNull() || !textVal.isString())
        continue;
      if (senderVal.isNull() || !senderVal.isString())
        continue;
      if (timeVal.isNull() || !timeVal.isString())
        continue;
      if (toVal.isNull() || !toVal.isString())
        continue;
      // 如果不在聊天页面
      if (ui->stackedWidget->currentWidget() != ui->friendChatPage)
      {
        return;
      }
      // 如果不是当前聊天对象
      if (ui->friendName->text().compare(toVal.toString()) != 0 &&
          ui->friendName->text().compare(senderVal.toString()) != 0)
      {
        return;
      }
      if (textVal.toString()[0] == '|')
      {
        ui->friendMsg->append(
            QString("%1 %2").arg(senderVal.toString()).arg(timeVal.toString()));
        ui->friendMsg->append("<img src='" + img(textVal.toString()) + "'/>");
      }
      else
      {
        ui->friendMsg->append(QString("%1 %2\n%3")
                                  .arg(senderVal.toString())
                                  .arg(timeVal.toString())
                                  .arg(textVal.toString()));
      }
    }
  }
  // memberList
  else if (typeVal.toString().compare("memberList", Qt::CaseInsensitive) == 0)
  {
    const QJsonValue memberListVal = docObj.value("memberList");
    if (memberListVal.isNull() || !memberListVal.isArray())
      return;
    ui->memberList->clear();
    ui->memberList->addItems(memberListVal.toVariant().toStringList());
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
      break;
    }
  }
  // 如果当前在聊天页面
  if (ui->stackedWidget->currentWidget() == ui->friendChatPage)
  {
    // 如果当前聊天对象是新加入的用户
    if (ui->friendName->text() == user)
    {
      ui->friendStatus->setText("在线");
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
      QString friendName = ui->friendList->item(i)->text();
      friendName.replace("在线", "离线");
      ui->friendList->item(i)->setText(friendName);
      break;
    }
  }
  if (ui->stackedWidget->currentWidget() == ui->friendChatPage)
  {
    if (ui->friendName->text() == user)
    {
      ui->friendStatus->setText("离线");
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
    ui->sayButton->setText("@" + currentText);
}

void MainWindow::on_registerBtn_clicked()
{
  if (ui->username->text().isEmpty() || ui->password->text().isEmpty() ||
      ui->rePassword->text().isEmpty())
  {
    ui->errMsg->setText("请填写完整信息");
    return;
  }
  if (ui->password->text().compare(ui->rePassword->text()) != 0)
  {
    ui->errMsg->setText("两次输入密码不一致");
    return;
  }
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
  if (timer != nullptr)
  {
    timer->stop();
    delete timer;
    timer = nullptr;
    ui->addBtn->setText("添加好友/群组");
    ui->addBtn->setIcon(QIcon(":/new/prefix1/img/add.png"));
  }
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
    QMessageBox::information(this, "提示",
                             "您已加入群聊" + username + "(" + id + ")");
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
  QString text = QInputDialog::getText(
      this, tr("创建群组"), tr("请输入群组名:"), QLineEdit::Normal, "", &ok);
  if (ok && !text.isEmpty())
  {
    // 发送消息
    m_chatClient->sendMessage(text, "createGroup");
  }
}

void MainWindow::on_friendList_itemClicked(QListWidgetItem *item)
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
  ui->friendName->setText(username);
  // 如果res中含有在线
  if (res.contains("在线"))
  {
    ui->friendStatus->setText("在线");
  }
  else
  {
    ui->friendStatus->setText("离线");
  }
  // 将item的样式恢复
  item->setTextColor(QColor(0, 0, 0));
  // 从item的data中获取Timer，停止计时器
  QTimer *timer = item->data(Qt::UserRole).value<QTimer *>();
  // 如果timer不为空
  if (timer != nullptr)
  {
    timer->stop();
    // 从data中删除timer
    item->setData(Qt::UserRole, QVariant());
    // 删除timer
    delete timer;
  }

  // 将“线]-”后的数字删除
  QString text = item->text();
  int index = text.indexOf("]-");
  if (index != -1)
  {
    text = text.left(index + 1);
  }
  item->setText(text);
  // 请求好友聊天记录
  m_chatClient->sendMessage(username, "getChatRecord");
  ui->friendMsg->clear();
  // 隐藏
  ui->memberList->hide();
  ui->invite->hide();
  ui->fileBtn->show();
  ui->stackedWidget->setCurrentWidget(ui->friendChatPage);
}

void MainWindow::on_friendSend_clicked()
{
  QString username = ui->friendName->text();
  QString send = ui->friendEdit->text();
  if (username == "")
  {
    return;
  }
  // 如果memberList为隐藏状态
  if (ui->memberList->isHidden())
  {
    m_chatClient->sendMessage(send, "message", username);
  }
  else
  {
    m_chatClient->sendMessage(send, "group", username);
  }
  ui->friendEdit->clear();
}

void MainWindow::on_pushButton_5_clicked()
{
  ui->stackedWidget->setCurrentWidget(ui->homePage);
}

void MainWindow::on_friendImg_clicked()
{
  QString username = ui->friendName->text();
  if (username == "")
  {
    return;
  }
  // 弹出窗口，选择图片
  QString fileName = QFileDialog::getOpenFileName(
      this, tr("选择图片"), "", tr("Images (*.png *.xpm *.jpg)"));
  if (fileName == "")
  {
    return;
  }
  // 将图片转换为base64
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly))
  {
    return;
  }
  QByteArray data = file.readAll();
  QString base64 = data.toBase64();
  base64 = "|" + base64;
  // 发送消息
  // 如果当前页是chatPage
  if (ui->stackedWidget->currentWidget() == ui->chatPage)
  {
    m_chatClient->sendMessage(base64, "message");
    return;
  }
  if (ui->memberList->isHidden())
    m_chatClient->sendMessage(base64, "message", username);
  else
    m_chatClient->sendMessage(base64, "group", username);
}

void MainWindow::on_fileBtn_clicked()
{
  // 如果好友不在线
  if (ui->friendStatus->text() == "离线")
  {
    QMessageBox::information(this, "提示", "好友不在线，无法发送文件");
    return;
  }
  QString username = ui->friendName->text();
  if (username == "")
  {
    return;
  }
  // 弹出窗口，选择文件
  QString fileName =
      QFileDialog::getOpenFileName(this, tr("选择文件"), "", tr("file (*.*)"));
  if (fileName == "")
  {
    return;
  }
  // 将文件转换为base64
  QFile file(fileName);
  if (!file.open(QIODevice::ReadOnly))
  {
    return;
  }
  // 只保留文件名，不保留路径
  int index = fileName.lastIndexOf("/");
  if (index != -1)
  {
    fileName = fileName.mid(index + 1);
  }
  QByteArray data = file.readAll();
  QString base64 = data.toBase64();
  base64 = "#[" + fileName + "]" + base64;
  // 发送消息
  m_chatClient->sendMessage(base64, "message", username);
  m_chatClient->sendMessage("[文件]" + fileName, "message", username);
}

void MainWindow::on_groupList_itemClicked(QListWidgetItem *item)
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
  QString groupname = res.left(pos);
  ui->friendStatus->setText(groupname);
  // 提取res中"("和")"之间的内容
  int end = res.indexOf(")");
  if (end == -1)
  {
    return;
  }
  ui->friendName->setText(res.mid(pos + 1, end - pos - 1));
  // 将item的样式恢复
  item->setTextColor(QColor(0, 0, 0));
  // 从item的data中获取Timer，停止计时器
  QTimer *timer = item->data(Qt::UserRole).value<QTimer *>();
  // 如果timer不为空
  if (timer != nullptr)
  {
    timer->stop();
    // 从data中删除timer
    item->setData(Qt::UserRole, QVariant());
    // 删除timer
    delete timer;
  }
  QString text = item->text();
  int index = text.indexOf("-");
  if (index != -1)
  {
    text = text.left(index);
  }
  item->setText(text);
  // 请求群聊天记录
  m_chatClient->sendMessage(res.mid(pos + 1, end - pos - 1),
                            "getGroupChatRecord");
  ui->friendMsg->clear();
  ui->memberList->show();
  ui->fileBtn->hide();
  ui->invite->show();
  ui->stackedWidget->setCurrentWidget(ui->friendChatPage);
}

void MainWindow::on_invite_clicked()
{
  // 弹出窗口，从好友列表中选择好友
  QString username = ui->friendName->text();
  if (username == "")
  {
    return;
  }
  // 弹出窗口，从Combo Box选择好友
  QStringList list;
  for (int i = 0; i < ui->friendList->count(); i++)
  {
    QListWidgetItem *item = ui->friendList->item(i);
    QString text = item->text();
    int index = text.indexOf("(");
    if (index != -1)
    {
      text = text.left(index);
    }
    list << text;
  }
  // 如果好友列表为空
  if (list.isEmpty())
  {
    QMessageBox::information(this, "提示", "好友列表为空");
    return;
  }
  bool ok;
  QString res =
      QInputDialog::getItem(this, "邀请好友", "好友列表", list, 0, false, &ok);
  if (ok && !res.isEmpty())
  {
    m_chatClient->sendMessage(res, "invite", username);
  }
}

void MainWindow::on_pushButton_4_clicked()
{
  on_friendImg_clicked();
}
