/*
** Copyright (c) 2011 - 10^10^10, Alexis Megas.
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from Spot-On without specific prior written permission.
**
** SPOT-ON IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** SPOT-ON, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

extern "C"
{
#include <libpq-fe.h>
}

#include <QSqlDriver>

#include "spot-on.h"
#include "spot-on-documentation.h"
#if SPOTON_GOLDBUG == 0
#include "spot-on-neighborstatistics.h"
#endif
#include "spot-on-utilities.h"
#include "ui_spot-on-private-application-credentials.h"
#if SPOTON_GOLDBUG == 0
#include "ui_spot-on-stylesheet.h"
#endif

void spoton::slotShowMainTabContextMenu(const QPoint &point)
{
  if(m_locked)
    return;

  QWidget *widget = m_ui.tab->widget(m_ui.tab->tabBar()->tabAt(point));

  if(!widget)
    return;
  else if(!widget->isEnabled())
    return;

  QMapIterator<int, QWidget *> it(m_tabWidgets);
  QString name("");

  while(it.hasNext())
    {
      it.next();

      if(it.value() == widget)
	{
	  name = m_tabWidgetsProperties[it.key()].value("name").toString();
	  break;
	}
    }

  bool enabled = true;

  if(!(name == "buzz" || name == "listeners" || name == "neighbors" ||
       name == "search" || name == "starbeam" || name == "urls"))
    enabled = false;
  else if(name.isEmpty())
    enabled = false;

  QAction *action = 0;
  QMenu menu(this);

  action = menu.addAction(tr("&Close Page"), this, SLOT(slotCloseTab(void)));
  action->setEnabled(enabled);
  action->setProperty("name", name);
  menu.exec(m_ui.tab->tabBar()->mapToGlobal(point));
}

void spoton::slotCloseTab(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QString name(action->property("name").toString());

  if(name == "buzz")
    m_ui.action_Buzz->setChecked(false);
  else if(name == "listeners")
    m_ui.action_Listeners->setChecked(false);
  else if(name == "neighbors")
    m_ui.action_Neighbors->setChecked(false);
  else if(name == "search")
    m_ui.action_Search->setChecked(false);
  else if(name == "starbeam")
    m_ui.action_StarBeam->setChecked(false);
  else if(name == "urls")
    m_ui.action_Urls->setChecked(false);
}

void spoton::slotRemoveAttachment(const QUrl &url)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QStringList list(m_ui.attachment->toPlainText().split('\n'));

  m_ui.attachment->clear();

  while(!list.isEmpty())
    {
      QString str(list.takeFirst());

      if(str != url.toString())
	m_ui.attachment->append(QString("<a href=\"%1\">%1</a>").arg(str));
    }

  QApplication::restoreOverrideCursor();
}

QByteArray spoton::poptasticNameEmail(void) const
{
  return m_settings.value("gui/poptasticNameEmail").toByteArray();
}

void spoton::slotShowNotificationsWindow(void)
{
  bool wasVisible = m_notificationsWindow->isVisible();

  m_notificationsWindow->showNormal();
  m_notificationsWindow->activateWindow();
  m_notificationsWindow->raise();

  if(!wasVisible)
    spoton_utilities::centerWidget(m_notificationsWindow, this);
}

QByteArray spoton::copyMyOpenLibraryPublicKey(void) const
{
  if(!m_crypts.value("open-library", 0) ||
     !m_crypts.value("open-library-signature", 0))
    return QByteArray();

  QByteArray name;
  QByteArray mPublicKey;
  QByteArray mSignature;
  QByteArray sPublicKey;
  QByteArray sSignature;
  bool ok = true;

  name = m_settings.value("gui/openLibraryName", "unknown").toByteArray();
  mPublicKey = m_crypts.value("open-library")->publicKey(&ok);

  if(ok)
    mSignature = m_crypts.value("open-library")->
      digitalSignature(mPublicKey, &ok);

  if(ok)
    sPublicKey = m_crypts.value("open-library-signature")->publicKey(&ok);

  if(ok)
    sSignature = m_crypts.value("open-library-signature")->
      digitalSignature(sPublicKey, &ok);

  if(ok)
    return "K" + QByteArray("open-library").toBase64() + "@" +
      name.toBase64() + "@" +
      mPublicKey.toBase64() + "@" + mSignature.toBase64() + "@" +
      sPublicKey.toBase64() + "@" + sSignature.toBase64();
  else
    return QByteArray();
}

void spoton::slotCopyMyOpenLibraryPublicKey(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString text(copyMyOpenLibraryPublicKey());

  QApplication::restoreOverrideCursor();

  if(text.length() >= 10 * 1024 * 1024)
    {
      QMessageBox::critical
	(this, tr("%1: Error").arg(SPOTON_APPLICATION_NAME),
	 tr("The open-library public key is too long (%1 bytes).").
	 arg(QLocale().toString(text.length())));
      return;
    }

  QClipboard *clipboard = QApplication::clipboard();

  if(clipboard)
    {
      m_ui.toolButtonCopyToClipboard->menu()->repaint();
      repaint();
#ifndef Q_OS_MAC
      QApplication::processEvents();
#endif
      QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
      clipboard->setText(text);
      QApplication::restoreOverrideCursor();
    }
}

void spoton::slotShareOpenLibraryPublicKey(void)
{
  if(!m_crypts.value("open-library", 0) ||
     !m_crypts.value("open-library-signature", 0))
    return;
  else if(m_kernelSocket.state() != QAbstractSocket::ConnectedState)
    return;
  else if(!m_kernelSocket.isEncrypted() &&
	  m_ui.kernelKeySize->currentText().toInt() > 0)
    return;

  if(m_ui.neighborsActionMenu->menu())
    m_ui.neighborsActionMenu->menu()->repaint();

  repaint();
#ifndef Q_OS_MAC
  QApplication::processEvents();
#endif
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QString oid("");
  int row = -1;

  if((row = m_ui.neighbors->currentRow()) >= 0)
    {
      QTableWidgetItem *item = m_ui.neighbors->item
	(row, m_ui.neighbors->columnCount() - 1); // OID

      if(item)
	oid = item->text();
    }

  if(oid.isEmpty())
    {
      QApplication::restoreOverrideCursor();
      return;
    }

  QByteArray publicKey;
  QByteArray signature;
  bool ok = true;

  publicKey = m_crypts.value("open-library")->publicKey(&ok);

  if(ok)
    signature = m_crypts.value("open-library")->digitalSignature
      (publicKey, &ok);

  QByteArray sPublicKey;
  QByteArray sSignature;

  if(ok)
    sPublicKey = m_crypts.value("open-library-signature")->publicKey(&ok);

  if(ok)
    sSignature = m_crypts.value("open-library-signature")->
      digitalSignature(sPublicKey, &ok);

  if(ok)
    {
      QByteArray message;
      QByteArray name(m_settings.value("gui/openLibraryName", "unknown").
		      toByteArray());

      if(name.isEmpty())
	name = "unknown";

      message.append("sharepublickey_");
      message.append(oid);
      message.append("_");
      message.append(QByteArray("open-library").toBase64());
      message.append("_");
      message.append(name.toBase64());
      message.append("_");
      message.append(qCompress(publicKey).toBase64());
      message.append("_");
      message.append(signature.toBase64());
      message.append("_");
      message.append(sPublicKey.toBase64());
      message.append("_");
      message.append(sSignature.toBase64());
      message.append("\n");

      if(m_kernelSocket.write(message.constData(), message.length()) !=
	 message.length())
	spoton_misc::logError
	  (QString("spoton::slotShareOpenLibraryPublicKey(): write() failure "
		   "for %1:%2.").
	   arg(m_kernelSocket.peerAddress().toString()).
	   arg(m_kernelSocket.peerPort()));
    }

  QApplication::restoreOverrideCursor();
}

void spoton::slotBuzzInvite(void)
{
  if(m_kernelSocket.state() != QAbstractSocket::ConnectedState)
    return;
  else if(!m_kernelSocket.isEncrypted() &&
	  m_ui.kernelKeySize->currentText().toInt() > 0)
    return;

  QModelIndexList list
    (m_ui.participants->selectionModel()->selectedRows(1)); // OID
  QStringList oids;

  for(int i = 0; i < list.size(); i++)
    {
      if(list.value(i).data(Qt::UserRole).toBool())
	/*
	** Ignore temporary participants.
	*/

	continue;

      else if(list.value(i).data(Qt::ItemDataRole(Qt::UserRole + 1)) != "chat")
	/*
	** Ignore non-chat participants.
	*/

	continue;
      else
	oids << list.value(i).data().toString();
    }

  if(oids.isEmpty())
    return;

  repaint();
#ifndef Q_OS_MAC
  QApplication::processEvents();
#endif

  /*
  ** Let's generate an anonymous Buzz channel.
  */

  QByteArray channel(spoton_crypt::
		     strongRandomBytes(static_cast<size_t> (m_ui.channel->
							    maxLength())).
		     toBase64().mid(0, m_ui.channel->maxLength()));
  QByteArray channelSalt(spoton_crypt::strongRandomBytes(512).toBase64());
  QByteArray channelType("aes256");
  QByteArray hashKey
    (spoton_crypt::
     strongRandomBytes(spoton_crypt::XYZ_DIGEST_OUTPUT_SIZE_IN_BYTES).
     toBase64());
  QByteArray hashType("sha512");
  QByteArray id;
  QPair<QByteArray, QByteArray> keys;
  QPointer<spoton_buzzpage> page;
  QString error("");
  unsigned long int iterationCount =
    static_cast<unsigned long int> (m_ui.buzzIterationCount->minimum());

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  keys = spoton_crypt::derivedKeys(channelType,
				   "sha1", // PBKDF2.
				   iterationCount,
				   channel + channelType + hashType,
				   channelSalt,
				   true,
				   error);
  QApplication::restoreOverrideCursor();

  if(!error.isEmpty())
    return;

  page = m_buzzPages.value(keys.first, 0);

  if(page)
    if(m_ui.buzzTab->indexOf(page) != -1)
      m_ui.buzzTab->setCurrentWidget(page);

  if(!page)
    {
      if(m_buzzIds.contains(keys.first))
	id = m_buzzIds[keys.first];
      else
	{
	  id = spoton_crypt::
	    strongRandomBytes
	    (spoton_common::BUZZ_MAXIMUM_ID_LENGTH / 2).toHex();
	  m_buzzIds[keys.first] = id;
	}

      page = new spoton_buzzpage
	(&m_kernelSocket, channel, channelSalt, channelType,
	 id, iterationCount, hashKey, hashType, keys.first, this);
      m_buzzPages[page->key()] = page;
      connect(&m_buzzStatusTimer,
	      SIGNAL(timeout(void)),
	      page,
	      SLOT(slotSendStatus(void)));
      connect(page,
	      SIGNAL(changed(void)),
	      this,
	      SLOT(slotBuzzChanged(void)));
      connect(page,
	      SIGNAL(channelSaved(void)),
	      this,
	      SLOT(slotPopulateBuzzFavorites(void)));
      connect(page,
	      SIGNAL(destroyed(QObject *)),
	      this,
	      SLOT(slotBuzzPageDestroyed(QObject *)));
      connect(page,
	      SIGNAL(unify(void)),
	      this,
	      SLOT(slotUnifyBuzz(void)));
      connect(this,
	      SIGNAL(buzzNameChanged(const QByteArray &)),
	      page,
	      SLOT(slotBuzzNameChanged(const QByteArray &)));
      connect(this,
	      SIGNAL(iconsChanged(void)),
	      page,
	      SLOT(slotSetIcons(void)));

      QMainWindow *mainWindow = new QMainWindow(0);

      mainWindow->setAttribute(Qt::WA_DeleteOnClose, true);
      mainWindow->setCentralWidget(page);
      mainWindow->setWindowIcon(windowIcon());
      mainWindow->setWindowTitle
	(QString("%1: %2").
	 arg(SPOTON_APPLICATION_NAME).
	 arg(page->channel().constData()));
      mainWindow->show();
      page->show();
      page->showUnify(true);
    }

  QByteArray message("addbuzz_");

  message.append(page->key().toBase64());
  message.append("_");
  message.append(page->channelType().toBase64());
  message.append("_");
  message.append(page->hashKey().toBase64());
  message.append("_");
  message.append(page->hashType().toBase64());
  message.append("\n");

  if(m_kernelSocket.write(message.constData(), message.length()) !=
     message.length())
    spoton_misc::logError
      (QString("spoton::slotBuzzInvite(): "
	       "write() failure for %1:%2.").
       arg(m_kernelSocket.peerAddress().toString()).
       arg(m_kernelSocket.peerPort()));

  QByteArray name(m_settings.value("gui/nodeName", "unknown").toByteArray());
  QString magnet(page->magnet());

  if(name.isEmpty())
    name = "unknown";

  for(int i = 0; i < oids.size(); i++)
    {
      QByteArray message;

      message.append("message_");
      message.append(QString("%1_").arg(oids.at(i)));
      message.append(name.toBase64());
      message.append("_");
      message.append(magnet.toLatin1().toBase64());
      message.append("_");
      message.append
	(QByteArray("1").toBase64()); // Artificial sequence number.
      message.append("_");
      message.append(QDateTime::currentDateTime().toUTC().
		     toString("MMddyyyyhhmmss").toLatin1().toBase64());
      message.append("\n");

      if(m_kernelSocket.write(message.constData(), message.length()) !=
	 message.length())
	spoton_misc::logError
	  (QString("spoton::slotBuzzInvite(): write() failure "
		   "for %1:%2.").
	   arg(m_kernelSocket.peerAddress().toString()).
	   arg(m_kernelSocket.peerPort()));
    }
}

void spoton::joinBuzzChannel(const QUrl &url)
{
  QString channel("");
  QString channelSalt("");
  QString channelType("");
  QString hashKey("");
  QString hashType("");
  QStringList list(url.toString().remove("magnet:?").split("&"));
  unsigned long int iterationCount = 0;

  while(!list.isEmpty())
    {
      QString str(list.takeFirst());

      if(str.startsWith("rn="))
	{
	  str.remove(0, 3);
	  channel = str;
	}
      else if(str.startsWith("xf="))
	{
	  str.remove(0, 3);
	  iterationCount = static_cast<unsigned long int> (qAbs(str.toInt()));
	}
      else if(str.startsWith("xs="))
	{
	  str.remove(0, 3);
	  channelSalt = str;
	}
      else if(str.startsWith("ct="))
	{
	  str.remove(0, 3);
	  channelType = str;
	}
      else if(str.startsWith("hk="))
	{
	  str.remove(0, 3);
	  hashKey = str;
	}
      else if(str.startsWith("ht="))
	{
	  str.remove(0, 3);
	  hashType = str;
	}
      else if(str.startsWith("xt="))
	{
	}
    }

  QByteArray id;
  QPair<QByteArray, QByteArray> keys;
  QPointer<spoton_buzzpage> page;
  QString error("");

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  keys = spoton_crypt::derivedKeys(channelType,
				   "sha1", // PBKDF2.
				   iterationCount,
				   channel + channelType + hashType,
				   channelSalt.toLatin1(),
				   true,
				   error);
  QApplication::restoreOverrideCursor();

  if(!error.isEmpty())
    return;

  page = m_buzzPages.value(keys.first, 0);

  if(page)
    {
      if(m_ui.buzzTab->indexOf(page) != -1)
	m_ui.buzzTab->setCurrentWidget(page);

      return;
    }

  if(m_buzzIds.contains(keys.first))
    id = m_buzzIds[keys.first];
  else
    {
      id = spoton_crypt::
	strongRandomBytes
	(spoton_common::BUZZ_MAXIMUM_ID_LENGTH / 2).toHex();
      m_buzzIds[keys.first] = id;
    }

  page = new spoton_buzzpage
    (&m_kernelSocket, channel.toLatin1(), channelSalt.toLatin1(),
     channelType.toLatin1(), id, iterationCount, hashKey.toLatin1(),
     hashType.toLatin1(), keys.first, this);
  m_buzzPages[page->key()] = page;
  connect(&m_buzzStatusTimer,
	  SIGNAL(timeout(void)),
	  page,
	  SLOT(slotSendStatus(void)));
  connect(page,
	  SIGNAL(changed(void)),
	  this,
	  SLOT(slotBuzzChanged(void)));
  connect(page,
	  SIGNAL(channelSaved(void)),
	  this,
	  SLOT(slotPopulateBuzzFavorites(void)));
  connect(page,
	  SIGNAL(destroyed(QObject *)),
	  this,
	  SLOT(slotBuzzPageDestroyed(QObject *)));
  connect(page,
	  SIGNAL(unify(void)),
	  this,
	  SLOT(slotUnifyBuzz(void)));
  connect(this,
	  SIGNAL(buzzNameChanged(const QByteArray &)),
	  page,
	  SLOT(slotBuzzNameChanged(const QByteArray &)));
  connect(this,
	  SIGNAL(iconsChanged(void)),
	  page,
	  SLOT(slotSetIcons(void)));

  QMainWindow *mainWindow = new QMainWindow(0);

  mainWindow->setAttribute(Qt::WA_DeleteOnClose, true);
  mainWindow->setCentralWidget(page);
  mainWindow->setWindowIcon(windowIcon());
  mainWindow->setWindowTitle
    (QString("%1: %2").
     arg(SPOTON_APPLICATION_NAME).
     arg(page->channel().constData()));
  mainWindow->show();
  page->show();
  page->showUnify(true);

  if(m_kernelSocket.state() == QAbstractSocket::ConnectedState)
    if(m_kernelSocket.isEncrypted() ||
       m_ui.kernelKeySize->currentText().toInt() == 0)
      {
	QByteArray message("addbuzz_");

	message.append(page->key().toBase64());
	message.append("_");
	message.append(page->channelType().toBase64());
	message.append("_");
	message.append(page->hashKey().toBase64());
	message.append("_");
	message.append(page->hashType().toBase64());
	message.append("\n");

	if(m_kernelSocket.write(message.constData(), message.length()) !=
	   message.length())
	  spoton_misc::logError
	    (QString("spoton::joinBuzzChannel(): "
		     "write() failure for %1:%2.").
	     arg(m_kernelSocket.peerAddress().toString()).
	     arg(m_kernelSocket.peerPort()));
      }
}

void spoton::notify(const QString &text)
{
  if(!m_settings.value("gui/monitorEvents", true).toBool() ||
     text.trimmed().isEmpty())
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  if(m_notificationsUi.textBrowser->toPlainText().length() > 256 * 1024)
    m_notificationsUi.textBrowser->clear();

  m_notificationsUi.textBrowser->append(text.trimmed());
  m_sb.warning->setVisible(true);
  QApplication::restoreOverrideCursor();

  if(m_optionsUi.notifications->isChecked())
    slotShowNotificationsWindow();
}

void spoton::slotPlaySounds(bool state)
{
  m_settings["gui/play_sounds"] = state;

  QSettings settings;

  settings.setValue("gui/play_sounds", state);
}

void spoton::slotShowBuzzTabContextMenu(const QPoint &point)
{
  QAction *action = 0;
  QMenu menu(this);

  action = menu.addAction(tr("&Separate..."), this,
			  SLOT(slotSeparateBuzzPage(void)));
  action->setProperty("index", m_ui.buzzTab->tabBar()->tabAt(point));
  menu.exec(m_ui.buzzTab->tabBar()->mapToGlobal(point));
}

void spoton::slotSeparateBuzzPage(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  int index = action->property("index").toInt();
  spoton_buzzpage *page = qobject_cast<spoton_buzzpage *>
    (m_ui.buzzTab->widget(index));

  if(!page)
    return;

  m_ui.buzzTab->removeTab(index);

  QMainWindow *mainWindow = new QMainWindow(0);

  mainWindow->setAttribute(Qt::WA_DeleteOnClose, true);
  mainWindow->setCentralWidget(page);
  mainWindow->setWindowIcon(windowIcon());
  mainWindow->setWindowTitle
    (QString("%1: %2").
     arg(SPOTON_APPLICATION_NAME).
     arg(page->channel().constData()));
  mainWindow->show();
  page->show();
  page->showUnify(true);
}

void spoton::slotShowBuzzDetails(bool state)
{
  m_ui.buzz_frame->setVisible(state);
}

void spoton::slotUnifyBuzz(void)
{
  spoton_buzzpage *page = qobject_cast<spoton_buzzpage *> (sender());

  if(!page)
    return;

  QMainWindow *mainWindow = qobject_cast<QMainWindow *> (page->parentWidget());

  page->setParent(this);

  if(mainWindow)
    {
      mainWindow->setCentralWidget(0);
      mainWindow->deleteLater();
    }

  page->showUnify(false);
  m_ui.buzzTab->addTab(page, QString::fromUtf8(page->channel().constData(),
					       page->channel().length()));
  m_ui.buzzTab->setCurrentIndex(m_ui.buzzTab->count() - 1);
}

void spoton::slotBuzzPageDestroyed(QObject *object)
{
  Q_UNUSED(object);

  QMutableHashIterator<QByteArray, QPointer<spoton_buzzpage> > it
    (m_buzzPages);

  while(it.hasNext())
    {
      it.next();

      if(!it.value())
	it.remove();
    }
}

void spoton::slotNewGlobalName(void)
{
  QString text("");
  bool ok = true;

  text = QInputDialog::getText
    (this, tr("%1: Global Name").arg(SPOTON_APPLICATION_NAME), tr("&Name"),
     QLineEdit::Normal, "", &ok).trimmed();

  if(!ok)
    return;

  m_rosetta.setName(text);
  m_ui.buzzName->setText(text);
  m_ui.emailNameEditable->setText(text);
  m_ui.nodeName->setText(text);
  m_ui.urlName->setText(text);
  slotSaveBuzzName();
  slotSaveEmailName();
  slotSaveNodeName();
  slotSaveUrlName();
}

void spoton::slotListenerSourceOfRandomnessChanged(int value)
{
  QSpinBox *spinBox = qobject_cast<QSpinBox *> (sender());

  if(!spinBox)
    return;

  QString connectionName("");

  {
    QSqlDatabase db = spoton_misc::database(connectionName);

    db.setDatabaseName(spoton_misc::homePath() + QDir::separator() +
		       "listeners.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("UPDATE listeners SET "
		      "source_of_randomness = ? "
		      "WHERE OID = ?");
	query.bindValue(0, value);
	query.bindValue(1, spinBox->property("oid"));
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(connectionName);
}

void spoton::updatePoptasticNameSettingsFromWidgets(spoton_crypt *crypt)
{
  if(!crypt)
    return;

  QSettings settings;
  bool ok = true;

  m_settings["gui/poptasticName"] =
    m_poptasticRetroPhoneSettingsUi.chat_primary_account->currentText().
    toLatin1();
  m_settings["gui/poptasticNameEmail"] =
    m_poptasticRetroPhoneSettingsUi.email_primary_account->currentText().
    toLatin1();
  settings.setValue
    ("gui/poptasticName",
     crypt->encryptedThenHashed(m_settings.value("gui/poptasticName").
				toByteArray(), &ok).toBase64());
  settings.setValue
    ("gui/poptasticNameEmail",
     crypt->encryptedThenHashed(m_settings.value("gui/poptasticNameEmail").
				toByteArray(), &ok).toBase64());
}

void spoton::slotShowAddParticipant(void)
{
#if SPOTON_GOLDBUG == 0
  m_addParticipantWindow->showNormal();
  m_addParticipantWindow->activateWindow();
  m_addParticipantWindow->raise();
  spoton_utilities::centerWidget(m_addParticipantWindow, this);
#endif
}

void spoton::cancelUrlQuery(void)
{
  if(m_urlDatabase.driverName() != "QPSQL")
    return;
  else if(!m_urlDatabase.driver())
    return;

  QVariant handle(m_urlDatabase.driver()->handle());

  if(handle.typeName() != QString("PGconn") || !handle.isValid())
    return;

  PGconn *connection = *static_cast<PGconn **> (handle.data());

  if(!connection)
    return;

  PGcancel *cancel = PQgetCancel(connection);

  if(!cancel)
    return;

  PQcancel(cancel, 0, 0);
  PQfreeCancel(cancel);
}

void spoton::slotNotificationsEnabled(bool state)
{
  m_settings["gui/automaticNotifications"] = state;

  QSettings settings;

  settings.setValue("gui/automaticNotifications", state);
}

void spoton::slotCopyUrlKeys(void)
{
  QClipboard *clipboard = QApplication::clipboard();

  if(!clipboard)
    return;

  spoton_crypt *crypt = m_crypts.value("chat", 0);

  if(!crypt)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QByteArray name;
  QByteArray publicKeyHash;
  QString oid("");
  int row = -1;

  if((row = m_ui.urlParticipants->currentRow()) >= 0)
    {
      QTableWidgetItem *item = m_ui.urlParticipants->
	item(row, 0); // Name

      if(item)
	name.append(item->text());

      item = m_ui.urlParticipants->item(row, 1); // OID

      if(item)
	oid = item->text();

      item = m_ui.urlParticipants->item(row, 3); // public_key_hash

      if(item)
	publicKeyHash.append(item->text());
    }

  if(oid.isEmpty() || publicKeyHash.isEmpty())
    {
      clipboard->clear();
      QApplication::restoreOverrideCursor();
      return;
    }

  if(name.isEmpty())
    name = "unknown";

  QByteArray publicKey;
  QByteArray signatureKey;
  QString connectionName("");

  {
    QSqlDatabase db = spoton_misc::database(connectionName);

    db.setDatabaseName(spoton_misc::homePath() + QDir::separator() +
		       "friends_public_keys.db");

    if(db.open())
      {
	QSqlQuery query(db);
	bool ok = true;

	query.setForwardOnly(true);
	query.prepare("SELECT public_key "
		      "FROM friends_public_keys WHERE "
		      "OID = ?");
	query.bindValue(0, oid);

	if(query.exec())
	  if(query.next())
	    publicKey = crypt->decryptedAfterAuthenticated
	      (QByteArray::fromBase64(query.value(0).
				      toByteArray()),
	       &ok);
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(connectionName);
  signatureKey = spoton_misc::signaturePublicKeyFromPublicKeyHash
    (QByteArray::fromBase64(publicKeyHash), crypt);

  if(!publicKey.isEmpty() && !signatureKey.isEmpty())
    {
      QString text("K" + QByteArray("url").toBase64() + "@" +
		   name.toBase64() + "@" +
		   publicKey.toBase64() + "@" + QByteArray().toBase64() + "@" +
		   signatureKey.toBase64() + "@" + QByteArray().toBase64());

      if(text.length() >= spoton_common::MAXIMUM_COPY_KEY_SIZES)
	{
	  QApplication::restoreOverrideCursor();
	  QMessageBox::critical
	    (this, tr("%1: Error").arg(SPOTON_APPLICATION_NAME),
	     tr("The URL keys are too long (%1 bytes).").
	     arg(QLocale().toString(text.length())));
	  return;
	}

      clipboard->setText(text);
    }
  else
    clipboard->clear();

  QApplication::restoreOverrideCursor();
}

void spoton::slotCopyPrivateApplicationMagnet(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  int row = -1;

  if(action->property("type") == "listeners")
    row = m_ui.listeners->currentRow();
  else if(action->property("type") == "neighbors")
    row = m_ui.neighbors->currentRow();
  else
    return;

  if(row < 0)
    return;

  QClipboard *clipboard = QApplication::clipboard();

  if(!clipboard)
    return;
  else
    clipboard->clear();

  QTableWidgetItem *item = 0;

  if(action->property("type") == "listeners")
    item = m_ui.listeners->item(row, 23); // private_application_credentials
  else
    item = m_ui.neighbors->item(row, 39); // private_application_credentials

  if(!item)
    return;

  clipboard->setText(item->text());
}

void spoton::slotResetPrivateApplicationInformation(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QModelIndexList list;

  if(action->property("type") == "listeners")
    list = m_ui.listeners->selectionModel()->selectedRows
      (m_ui.listeners->columnCount() - 1); // OID
  else if(action->property("type") == "neighbors")
    list = m_ui.neighbors->selectionModel()->selectedRows
      (m_ui.neighbors->columnCount() - 1); // OID

  if(list.isEmpty())
    return;

  QString connectionName("");

  {
    QSqlDatabase db = spoton_misc::database(connectionName);

    if(action->property("type") == "listeners")
      db.setDatabaseName(spoton_misc::homePath() + QDir::separator() +
			 "listeners.db");
    else
      db.setDatabaseName(spoton_misc::homePath() + QDir::separator() +
			 "neighbors.db");

    if(db.open())
      {
	QSqlQuery query(db);

	if(action->property("type") == "listeners")
	  query.prepare("UPDATE listeners SET "
			"private_application_credentials = NULL "
			"WHERE OID = ?");
	else
	  query.prepare("UPDATE neighbors SET "
			"private_application_credentials = NULL "
			"WHERE OID = ?");

	query.bindValue(0, list.at(0).data());
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(connectionName);
}

void spoton::slotSetPrivateApplicationInformation(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  spoton_crypt *crypt = m_crypts.value("chat", 0);

  if(!crypt)
    {
      QMessageBox::critical(this, tr("%1: Error").
			    arg(SPOTON_APPLICATION_NAME),
			    tr("Invalid spoton_crypt object. "
			       "This is a fatal flaw."));
      return;
    }

  QModelIndexList list;
  QString oid("");

  if(action->property("type") == "listeners")
    list = m_ui.listeners->selectionModel()->selectedRows
      (m_ui.listeners->columnCount() - 1); // OID
  else if(action->property("type") == "neighbors")
    list = m_ui.neighbors->selectionModel()->selectedRows
      (m_ui.neighbors->columnCount() - 1); // OID
  else
    return;

  if(list.isEmpty())
    {
      QMessageBox::critical(this, tr("%1: Error").
			    arg(SPOTON_APPLICATION_NAME),
			    tr("Invalid listener / neighbor OID. "
			       "Please select a listener / neighbor."));
      return;
    }
  else
    oid = list.at(0).data().toString();

  QStringList ctypes(spoton_crypt::cipherTypes());

  if(ctypes.isEmpty())
    {
      QMessageBox::critical(this, tr("%1: Error").
			    arg(SPOTON_APPLICATION_NAME),
			    tr("The method spoton_crypt::cipherTypes() has "
			       "failed. "
			       "This is a fatal flaw."));
      return;
    }

  QStringList htypes(spoton_crypt::hashTypes());

  if(htypes.isEmpty())
    {
      QMessageBox::critical(this, tr("%1: Error").
			    arg(SPOTON_APPLICATION_NAME),
			    tr("The method spoton_crypt::hashTypes() has "
			       "failed. "
			       "This is a fatal flaw."));
      return;
    }

  QDialog dialog(this);
  Ui_spoton_private_application_credentials ui;

  ui.setupUi(&dialog);
  dialog.setWindowTitle
    (tr("%1: Private Application Credentials").
     arg(SPOTON_APPLICATION_NAME));
  ui.cipher_type->addItems(ctypes);
  ui.hash_type->addItems(htypes);

  if(dialog.exec() == QDialog::Accepted)
    {
      if(ui.magnet->isChecked())
	{
	  QScopedPointer<spoton_crypt> crypt
	    (spoton_misc::parsePrivateApplicationMagnet(ui.secret->text().
							toLatin1()));

	  if(!crypt)
	    {
	      QMessageBox::critical(this, tr("%1: Error").
				    arg(SPOTON_APPLICATION_NAME),
				    tr("Invalid magnet or memory failure."));
	      return;
	    }
	}

      QString error("");
      QString magnet("");

      if(ui.magnet->isChecked())
	magnet = ui.secret->text();
      else
	{
	  QString secret(ui.secret->text().trimmed());

	  if(secret.length() < 16)
	    {
	      QMessageBox::critical(this, tr("%1: Error").
				    arg(SPOTON_APPLICATION_NAME),
				    tr("Please provide a Secret that contains "
				       "at least sixteen characters."));
	      return;
	    }

	  repaint();
#ifndef Q_OS_MAC
	  QApplication::processEvents();
#endif

	  /*
	  ** The salt will be composed of the cipher type, hash type,
	  ** and iteration count.
	  */

	  QPair<QByteArray, QByteArray> keys;

	  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	  keys = spoton_crypt::derivedKeys
	    (ui.cipher_type->currentText(),
	     ui.hash_type->currentText(),
	     static_cast<unsigned long int> (ui.iteration_count->value()),
	     secret.mid(0, 16).toUtf8(),
	     ui.cipher_type->currentText().toLatin1().toHex() +
	     ui.hash_type->currentText().toLatin1().toHex() +
	     ui.iteration_count->text().toLatin1().toHex(),
	     spoton_crypt::XYZ_DIGEST_OUTPUT_SIZE_IN_BYTES,
	     false,
	     error);
	  QApplication::restoreOverrideCursor();

	  if(error.isEmpty())
	    magnet = QString("magnet:?"
			     "ct=%1&"
			     "ht=%2&"
			     "ic=%3&"
			     "s1=%4&"
			     "s2=%5&"
			     "xt=urn:private-application-credentials").
	      arg(ui.cipher_type->currentText()).
	      arg(ui.hash_type->currentText()).
	      arg(ui.iteration_count->text()).
	      arg(keys.first.toBase64().constData()).
	      arg(keys.second.toBase64().constData());
	}

      if(error.isEmpty())
	{
	  QString connectionName("");
	  bool ok = true;

	  {
	    QSqlDatabase db = spoton_misc::database(connectionName);

	    if(action->property("type") == "listeners")
	      db.setDatabaseName(spoton_misc::homePath() + QDir::separator() +
				 "listeners.db");
	    else
	      db.setDatabaseName(spoton_misc::homePath() + QDir::separator() +
				 "neighbors.db");

	    if(db.open())
	      {
		QSqlQuery query(db);

		if(action->property("type") == "listeners")
		  query.prepare("UPDATE listeners SET "
				"private_application_credentials = ? "
				"WHERE OID = ?");
		else
		  query.prepare("UPDATE neighbors SET "
				"private_application_credentials = ? "
				"WHERE OID = ?");

		query.bindValue
		  (0, crypt->encryptedThenHashed(magnet.toLatin1(),
						 &ok).toBase64());
		query.bindValue(1, oid);

		if(ok)
		  ok = query.exec();
	      }
	    else
	      ok = false;

	    db.close();
	  }

	  QSqlDatabase::removeDatabase(connectionName);

	  if(!ok)
	    QMessageBox::critical(this, tr("%1: Error").
				  arg(SPOTON_APPLICATION_NAME),
				  tr("An error occurred while attempting "
				     "to set the private application "
				     "credentials."));
	}
      else
	QMessageBox::critical(this, tr("%1: Error").
			      arg(SPOTON_APPLICATION_NAME),
			      tr("An error (%1) occurred while deriving "
				 "private application credentials").
			      arg(error));
    }
}

void spoton::slotPrepareAndShowInstallationWizard(void)
{
  QMessageBox mb(this);

  /*
  ** Must agree with the UI settings!
  */

  m_wizardHash["accepted"] = false;
  m_wizardHash["initialize_public_keys"] = true;
  m_wizardHash["launch_kernel"] = true;
  m_wizardHash["shown"] = false;
  m_wizardHash["url_create_sqlite_db"] = false;
  m_wizardHash["url_credentials"] = true;
  m_wizardHash["url_distribution"] = false;
  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Would you like to launch the initialization wizard?"));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::WindowModal);
  mb.setWindowTitle(tr("%1: Confirmation").arg(SPOTON_APPLICATION_NAME));

  if(mb.exec() == QMessageBox::Yes)
    {
      m_wizardHash["shown"] = true;

      QDialog dialog(this);

      if(m_wizardUi)
	delete m_wizardUi;

      m_ui.setPassphrase->setVisible(false);
      m_wizardUi = new Ui_spoton_wizard;
      m_wizardUi->setupUi(&dialog);
      m_wizardUi->initialize->setVisible(false);
      m_wizardUi->previous->setDisabled(true);
      qobject_cast<QBoxLayout *> (m_wizardUi->passphrase_frame->layout())->
	insertWidget(2, m_ui.passphraseGroupBox);
      connect(m_wizardUi->cancel,
	      SIGNAL(clicked(void)),
	      &dialog,
	      SLOT(reject(void)));
      connect(m_wizardUi->initialize,
	      SIGNAL(clicked(void)),
	      &dialog,
	      SLOT(accept(void)));
      connect(m_wizardUi->next,
	      SIGNAL(clicked(void)),
	      this,
	      SLOT(slotWizardButtonClicked(void)));
      connect(m_wizardUi->prepare_sqlite_urls_db,
	      SIGNAL(clicked(void)),
	      this,
	      SLOT(slotWizardCheckClicked(void)));
      connect(m_wizardUi->previous,
	      SIGNAL(clicked(void)),
	      this,
	      SLOT(slotWizardButtonClicked(void)));
      dialog.show();
      dialog.resize(dialog.sizeHint());
      spoton_utilities::centerWidget(&dialog, this);

      if(dialog.exec() == QDialog::Accepted)
	{
	  m_wizardHash["accepted"] = true;
	  m_wizardHash["initialize_public_keys"] =
	    m_wizardUi->initialize_public_keys->isChecked();
	  m_wizardHash["launch_kernel"] =
	    m_wizardUi->launch_kernel->isChecked();
	  m_wizardHash["shown"] = false;
	  m_wizardHash["url_create_sqlite_db"] = m_wizardUi->
	    prepare_sqlite_urls_db->isChecked();
	  m_wizardHash["url_credentials"] =
	    m_wizardUi->url_credentials->isChecked();
	  m_wizardHash["url_distribution"] = m_wizardUi->
	    enable_url_distribution->isChecked();
	  repaint();
#ifndef Q_OS_MAC
	  QApplication::processEvents();
#endif
	  slotSetPassphrase();

	  if(m_wizardUi->prepare_sqlite_urls_db->isChecked())
	    slotPrepareUrlDatabases();
	}
      else
	{
	  m_ui.passphrase1->clear();
	  m_ui.passphrase2->clear();
	  m_ui.passphrase_rb->setChecked(true);
	  m_ui.username->clear();
	  m_ui.username->setFocus();
	}

      m_ui.setPassphrase->setVisible(true);
      m_ui.settingsVerticalLayout->insertWidget(1, m_ui.passphraseGroupBox);
    }

  m_wizardHash["shown"] = false;
}

void spoton::slotWizardButtonClicked(void)
{
  if(!m_wizardUi)
    return;

  if(m_wizardUi->next == sender())
    if(m_wizardUi->stackedWidget->currentIndex() == 1)
      if(!verifyInitializationPassphrase(m_wizardUi->stackedWidget->
					 parentWidget()))
	return;

  int count = m_wizardUi->stackedWidget->count();

  if(m_wizardUi->next == sender())
    m_wizardUi->stackedWidget->setCurrentIndex
      (qBound(0, m_wizardUi->stackedWidget->currentIndex() + 1, count - 1));
  else
    m_wizardUi->stackedWidget->setCurrentIndex
      (qBound(0, m_wizardUi->stackedWidget->currentIndex() - 1, count - 1));

  switch(m_wizardUi->stackedWidget->currentIndex())
    {
    case 0:
      {
	m_wizardUi->next->setEnabled(true);
	m_wizardUi->previous->setEnabled(false);
#if defined(Q_OS_WIN)
	QByteArray tmp(qgetenv("USERNAME").mid(0, 256).trimmed());

	if(!tmp.isEmpty())
	  m_ui.username->setText(tmp);
#endif
	break;
      }
    case 1:
      {
	m_ui.username->setFocus();
	m_ui.username->selectAll();
	m_wizardUi->next->setEnabled(true);
	m_wizardUi->previous->setEnabled(true);
	break;
      }
    case 2: case 3:
      {
	m_wizardUi->initialize->setVisible(false);
	m_wizardUi->next->setEnabled(true);
	m_wizardUi->next->setVisible(true);
	m_wizardUi->previous->setEnabled(true);
	break;
      }
    case 4:
      {
	m_wizardUi->initialize->setVisible(true);
	m_wizardUi->next->setEnabled(true);
	m_wizardUi->next->setVisible(false);
	m_wizardUi->previous->setEnabled(true);
	break;
      }
    default:
      break;
    }
}

bool spoton::verifyInitializationPassphrase(QWidget *parent)
{
  QString str1(m_ui.passphrase1->text());
  QString str2(m_ui.passphrase2->text());
  QString str3(m_ui.username->text());

  if(str3.trimmed().isEmpty())
    {
      str3 = "unknown";
      m_ui.username->setText(str3);
    }
  else
    m_ui.username->setText(str3.trimmed());

  if(!m_ui.passphrase_rb->isChecked())
    {
      str1 = m_ui.question->text();
      str2 = m_ui.answer->text();
    }

  if(str1.length() < 8 || str2.length() < 8)
    {
      if(m_ui.passphrase_rb->isChecked())
	QMessageBox::critical(parent, tr("%1: Error").
			      arg(SPOTON_APPLICATION_NAME),
			      tr("The passphrases must contain at least "
				 "eight characters each."));
      else
	QMessageBox::critical(parent, tr("%1: Error").
			      arg(SPOTON_APPLICATION_NAME),
			      tr("The answer and question must contain "
				 "at least eight characters each."));

      if(m_ui.passphrase_rb->isChecked())
	{
	  m_ui.passphrase1->selectAll();
	  m_ui.passphrase1->setFocus();
	}
      else
	{
	  m_ui.question->selectAll();
	  m_ui.question->setFocus();
	}

      return false;
    }

  if(m_ui.passphrase_rb->isChecked() && str1 != str2)
    {
      QMessageBox::critical(parent, tr("%1: Error").
			    arg(SPOTON_APPLICATION_NAME),
			    tr("The passphrases are not identical."));
      m_ui.passphrase1->selectAll();
      m_ui.passphrase1->setFocus();
      return false;
    }

  if(str3.isEmpty())
    {
      QMessageBox::critical(parent, tr("%1: Error").
			    arg(SPOTON_APPLICATION_NAME),
			    tr("Please provide a name."));
      m_ui.username->selectAll();
      m_ui.username->setFocus();
      return false;
    }

  return true;
}

void spoton::slotNeighborSilenceTimeChanged(int value)
{
  QSpinBox *spinBox = qobject_cast<QSpinBox *> (sender());

  if(!spinBox)
    return;

  QString connectionName("");

  {
    QSqlDatabase db = spoton_misc::database(connectionName);

    db.setDatabaseName(spoton_misc::homePath() + QDir::separator() +
		       "neighbors.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("UPDATE neighbors SET "
		      "silence_time = ? "
		      "WHERE OID = ?");
	query.bindValue(0, value);
	query.bindValue(1, spinBox->property("oid"));
	query.exec();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(connectionName);
}

void spoton::slotShowDocumentation(void)
{
  m_documentation->showNormal();
  m_documentation->activateWindow();
  m_documentation->raise();
  spoton_utilities::centerWidget(m_documentation, this);
}

void spoton::slotAfterFirstShow(void)
{
  repaint();
#ifndef Q_OS_MAC
  QApplication::processEvents();
#endif
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_sb.status->setText(tr("Preparing databases. Please be patient."));
  m_sb.status->repaint();
  spoton_misc::prepareDatabases();
  spoton_misc::prepareUrlDistillersDatabase();
  spoton_misc::prepareUrlKeysDatabase();
  m_sb.status->clear();
  QApplication::restoreOverrideCursor();
}

void spoton::slotSetWidgetStyleSheet(const QPoint &point)
{
#if SPOTON_GOLDBUG == 0
  QWidget *widget = qobject_cast<QWidget *> (sender());

  if(!widget)
    return;

  QAction *action = 0;
  QMenu menu(this);

  action = menu.addAction(tr("&Copy Style Sheet"),
			  this,
			  SLOT(slotCopyStyleSheet(void)));
  action->setProperty("widget_name", widget->objectName());
  action = menu.addAction(tr("&Reset Widget Style Sheet"),
			  this,
			  SLOT(slotResetStyleSheet(void)));
  action->setProperty("widget_name", widget->objectName());
  action = menu.addAction(tr("Set Widget &Style Sheet..."),
			  this,
			  SLOT(slotSetStyleSheet(void)));
  action->setProperty("widget_name", widget->objectName());
  action->setProperty("widget_stylesheet", widget->styleSheet());
  menu.addSeparator();
  menu.addAction(tr("Reset &All Widget Style Sheets"),
		 this,
		 SLOT(slotResetAllStyleSheets(void)));
  menu.exec(widget->mapToGlobal(point));
#else
  Q_UNUSED(point);
#endif
}

void spoton::slotSetStyleSheet(void)
{
#if SPOTON_GOLDBUG == 0
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QWidget *widget =
    findChild<QWidget *> (action->property("widget_name").toString());

  if(!widget)
    return;

  QDialog dialog(this);
  QString str(widget->styleSheet());
  Ui_spoton_stylesheet ui;

  ui.setupUi(&dialog);
  ui.preview->setProperty("widget_name", widget->objectName());
  ui.textEdit->setText(action->property("widget_stylesheet").toString());
  dialog.setWindowTitle
    (tr("Spot-On: Widget Style Sheet (%1)").arg(widget->objectName()));
  connect(ui.preview,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotPreviewStyleSheet(void)));

  if(dialog.exec() == QDialog::Accepted)
    {
      QString str(ui.textEdit->toPlainText().trimmed());

      widget->setStyleSheet(str);

      QSettings settings;

      m_settings[QString("gui/widget_stylesheet_%1").
		 arg(widget->objectName())] = str;
      settings.setValue
	(QString("gui/widget_stylesheet_%1").arg(widget->objectName()), str);
    }
  else
    widget->setStyleSheet(str);

#endif
}

void spoton::slotCopyStyleSheet(void)
{
#if SPOTON_GOLDBUG == 0
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QWidget *widget = findChild<QWidget *> (action->property("widget_name").
					  toString());

  if(!widget)
    return;

  QClipboard *clipboard = QApplication::clipboard();

  if(!clipboard)
    return;

  clipboard->setText(widget->styleSheet());
#endif
}

void spoton::slotPreviewStyleSheet(void)
{
#if SPOTON_GOLDBUG == 0
  QPushButton *pushButton = qobject_cast<QPushButton *> (sender());

  if(!pushButton)
    return;

  QWidget *widget = findChild<QWidget *> (pushButton->property("widget_name").
					  toString());

  if(!widget)
    return;

  QWidget *parent = pushButton->parentWidget();

  if(!parent)
    return;

  do
    {
      if(qobject_cast<QDialog *> (parent))
	break;

      if(parent)
	parent = parent->parentWidget();
    }
  while(parent != 0);

  if(!parent)
    return;

  QTextEdit *textEdit = parent->findChild<QTextEdit *> ();

  if(!textEdit)
    return;

  widget->setStyleSheet(textEdit->toPlainText());
#endif
}

void spoton::slotResetStyleSheet(void)
{
#if SPOTON_GOLDBUG == 0
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QWidget *widget = findChild<QWidget *> (action->property("widget_name").
					  toString());

  if(!widget)
    return;

  widget->setStyleSheet(widget->property("original_style_sheet").toString());

  QSettings settings;
  QString str(widget->styleSheet().trimmed());

  m_settings[QString("gui/widget_stylesheet_%1").arg(widget->objectName())] =
    str;
  settings.setValue
    (QString("gui/widget_stylesheet_%1").arg(widget->objectName()), str);
#endif
}

void spoton::slotShowNeighborStatistics(void)
{
#if SPOTON_GOLDBUG == 0
  QModelIndexList list;

  list = m_ui.neighbors->selectionModel()->selectedRows
    (m_ui.neighbors->columnCount() - 1); // OID

  if(list.isEmpty())
    return;

  qint64 oid = list.at(0).data().toLongLong();
  spoton_neighborstatistics *s = findChild<spoton_neighborstatistics *>
    (QString::number(oid));

  if(!s)
    {
      s = new spoton_neighborstatistics(this);
      s->setObjectName(QString::number(oid));
      connect(QCoreApplication::instance(),
	      SIGNAL(aboutToQuit(void)),
	      s,
	      SLOT(deleteLater(void)));
    }

  s->show(); // Custom.
  s->showNormal();
  s->activateWindow();
  s->raise();
  spoton_utilities::centerWidget(s, this);
#endif
}

void spoton::slotResetAllStyleSheets(void)
{
#if SPOTON_GOLDBUG == 0
  QMessageBox mb(this);

  mb.setIcon(QMessageBox::Question);
  mb.setStandardButtons(QMessageBox::No | QMessageBox::Yes);
  mb.setText(tr("Are you sure that you wish to reset all custom widget "
		"style sheets?"));
  mb.setWindowIcon(windowIcon());
  mb.setWindowModality(Qt::WindowModal);
  mb.setWindowTitle(tr("%1: Confirmation").arg(SPOTON_APPLICATION_NAME));

  if(mb.exec() != QMessageBox::Yes)
    return;

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QSettings settings;

  foreach(QWidget *widget, findChildren<QWidget *> ())
    if(widget->property("original_style_sheet").isValid())
      {
	widget->setStyleSheet
	  (widget->property("original_style_sheet").toString());

	QString str(widget->styleSheet().trimmed());

	m_settings[QString("gui/widget_stylesheet_%1").
		   arg(widget->objectName())] = str;
	settings.setValue
	  (QString("gui/widget_stylesheet_%1").arg(widget->objectName()), str);
      }

  QApplication::restoreOverrideCursor();
#endif
}

void spoton::slotGenerateOneYearListenerCertificate(void)
{
  QHostAddress address;
  QString oid("");
  int keySize = 0;
  int row = m_ui.listeners->currentRow();

  if(row < 0)
    return;

  QTableWidgetItem *item = 0;

  item = m_ui.listeners->item(row, 2); // Bluetooth Flags / SSL Key Size

  if(item)
    keySize = item->text().toInt();
  else
    return;

  if(keySize <= 0)
    return;

  /*
  ** It's impossible to determine if the user wishes to bundle the listener's
  ** external address into the new certificate.
  ** We'll assume that they do.
  */

  item = m_ui.listeners->item(row, 7); // External Address

  if(!item)
    return;
  else
    address = QHostAddress(item->text());

  item = m_ui.listeners->item(row, 15); // Transport

  if(!(item && (item->text().toLower().trimmed() == "tcp"
#if (QT_VERSION >= QT_VERSION_CHECK(5, 12, 0))
		|| item->text().toLower().trimmed() == "udp")))
#else
    )))
#endif
    return;

  item = m_ui.listeners->item(row, m_ui.listeners->columnCount() - 1); // OID

  if(item)
    oid = item->text();
  else
    return;

  spoton_crypt *crypt = m_crypts.value("chat", 0);

  if(!crypt)
    {
      QMessageBox::critical
	(this, tr("%1: Error").arg(SPOTON_APPLICATION_NAME),
	 tr("Invalid spoton_crypt object. This is a fatal flaw."));
      return;
    }

  QByteArray certificate;
  QByteArray privateKey;
  QByteArray publicKey;
  QString error("");

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_sb.status->setText
    (tr("Generating %1-bit SSL/TLS data. Please be patient.").
     arg(m_ui.listenerKeySize->currentText()));
  m_sb.status->repaint();
  spoton_crypt::generateSslKeys
    (keySize,
     certificate,
     privateKey,
     publicKey,
     address,
     60L * 60L * 24L * 365L,
     error);
  m_sb.status->clear();
  QApplication::restoreOverrideCursor();

  if(!error.isEmpty())
    {
      QMessageBox::critical(this, tr("%1: Error").
			    arg(SPOTON_APPLICATION_NAME),
			    tr("An error (%1) occurred while attempting "
			       "to generate a new certificate.").
			    arg(error));
      return;
    }

  QString connectionName("");
  bool ok = true;

  {
    QSqlDatabase db = spoton_misc::database(connectionName);

    db.setDatabaseName(spoton_misc::homePath() + QDir::separator() +
		       "listeners.db");

    if(db.open())
      {
	QSqlQuery query(db);

	query.prepare("UPDATE listeners SET certificate = ?, "
		      "private_key = ?, "
		      "public_key = ? "
		      "WHERE OID = ?");
	query.bindValue
	  (0, crypt->encryptedThenHashed(certificate, &ok).
	   toBase64());

	if(ok)
	  query.bindValue
	    (1, crypt->encryptedThenHashed(privateKey, &ok).
	     toBase64());

	if(ok)
	  query.bindValue
	    (2, crypt->encryptedThenHashed(publicKey, &ok).
	     toBase64());

	query.bindValue(3, oid);

	if(ok)
	  ok = query.exec();

	if(query.lastError().isValid())
	  error = query.lastError().text().trimmed();
      }
    else
      {
	ok = false;

	if(db.lastError().isValid())
	  error = db.lastError().text();
      }

    db.close();
  }

  QSqlDatabase::removeDatabase(connectionName);

  if(ok)
    {
    }
  else if(error.isEmpty())
    QMessageBox::critical(this, tr("%1: Error").
			  arg(SPOTON_APPLICATION_NAME),
			  tr("The generated data could not be recorded. "
			     "Please enable logging via the Log Viewer "
			     "and try again."));
  else
    QMessageBox::critical(this, tr("%1: Error").
			  arg(SPOTON_APPLICATION_NAME),
			  tr("An error (%1) occurred while attempting "
			     "to record the generated data.").arg(error));
}

void spoton::slotShowSMPWindow(void)
{
  menuBar()->repaint();
  repaint();
#ifndef Q_OS_MAC
  QApplication::processEvents();
#endif
  m_smpWindow.show(this);
  spoton_utilities::centerWidget(&m_smpWindow, this);
}

void spoton::slotAboutToShowEmailSecretsMenu(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.emailSecrets->menu()->clear();

  QMapIterator<QString, QByteArray> it
    (m_smpWindow.streams(QStringList() << "e-mail"
			               << "poptastic"));

  while(it.hasNext())
    {
      it.next();

      QAction *action = m_ui.emailSecrets->menu()->addAction
	(it.key(),
	 this,
	 SLOT(slotEmailSecretsActionSelected(void)));

      action->setProperty("stream", it.value());
    }

  if(m_ui.emailSecrets->menu()->actions().isEmpty())
    {
      /*
      ** Please do not translate Empty.
      */

      QAction *action = m_ui.emailSecrets->menu()->addAction("Empty");

      action->setEnabled(false);
    }

  QApplication::restoreOverrideCursor();
}

void spoton::slotEmailSecretsActionSelected(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  m_ui.goldbug->setText(action->property("stream").toString());
}

void spoton::slotAboutToShowChatSecretsMenu(void)
{
  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
  m_ui.chatSecrets->menu()->clear();

  QMapIterator<QString, QByteArray> it
    (m_smpWindow.streams(QStringList() << "chat"
			               << "poptastic"));

  while(it.hasNext())
    {
      it.next();

      QAction *action = m_ui.chatSecrets->menu()->addAction
	(it.key(),
	 this,
	 SLOT(slotChatSecretsActionSelected(void)));

      action->setProperty("stream", it.value());
    }

  if(m_ui.chatSecrets->menu()->actions().isEmpty())
    {
      /*
      ** Please do not translate Empty.
      */

      QAction *action = m_ui.chatSecrets->menu()->addAction("Empty");

      action->setEnabled(false);
    }

  QApplication::restoreOverrideCursor();
}

void spoton::slotChatSecretsActionSelected(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  int row = m_ui.participants->currentRow();

  if(row < 0)
    return;

  QTableWidgetItem *item1 = m_ui.participants->item(row, 1); // OID
  QTableWidgetItem *item2 = m_ui.participants->item
    (row, 6); // Gemini Encryption Key
  QTableWidgetItem *item3 = m_ui.participants->item
    (row, 7); // Gemini Hash Key

  if(!item1 || !item2 || !item3)
    return;
  else if(item1->data(Qt::UserRole).toBool()) // Temporary friend?
    return; // Temporary!

  QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

  QPair<QByteArray, QByteArray> gemini;

  gemini.first = action->property("stream").toString().mid
    (0, static_cast<int> (spoton_crypt::cipherKeyLength("aes256"))).toLatin1();
  gemini.second = action->property("stream").toString().mid
    (gemini.first.length(), spoton_crypt::XYZ_DIGEST_OUTPUT_SIZE_IN_BYTES).
    toLatin1();

  if(saveGemini(gemini, item1->text()))
    {
      disconnect(m_ui.participants,
		 SIGNAL(itemChanged(QTableWidgetItem *)),
		 this,
		 SLOT(slotGeminiChanged(QTableWidgetItem *)));
      item2->setText(gemini.first);
      item3->setText(gemini.second);
      connect(m_ui.participants,
	      SIGNAL(itemChanged(QTableWidgetItem *)),
	      this,
	      SLOT(slotGeminiChanged(QTableWidgetItem *)));
    }

  QApplication::restoreOverrideCursor();
}

void spoton::slotGoldBugDialogActionSelected(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(!action)
    return;

  QLineEdit *lineEdit = qobject_cast<QLineEdit *>
    (action->property("pointer").value<QWidget *> ());

  if(!lineEdit)
    return;

  lineEdit->setText(action->property("stream").toString());
}

void spoton::slotWizardCheckClicked(void)
{
  QCheckBox *checkBox = qobject_cast<QCheckBox *> (sender());

  if(checkBox == m_wizardUi->prepare_sqlite_urls_db)
    {
      if(checkBox->isChecked())
	{
	  m_wizardUi->launch_kernel->setChecked(false);
	  m_wizardUi->launch_kernel->setEnabled(false);
	}
      else
	m_wizardUi->launch_kernel->setEnabled(true);
    }
}
