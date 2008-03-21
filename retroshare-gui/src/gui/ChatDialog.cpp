/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006, crypton
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/
#include <QtGui>

#include "rshare.h"
#include "ChatDialog.h"

#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsmsgs.h"

#include "chat/PopupChatDialog.h"
#include <sstream>

#include <QTextCodec>
#include <QTextEdit>
#include <QToolBar>
#include <QTextCursor>
#include <QTextList>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>
#include <QHeaderView>

/** Constructor */
ChatDialog::ChatDialog(QWidget *parent)
: MainPage(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);

  connect(ui.lineEdit, SIGNAL(returnPressed( ) ), this, SLOT(sendMsg( ) ));
  
  connect(ui.colorChatButton, SIGNAL(clicked()), this, SLOT(setColor())); 
  
  connect( ui.msgSendList, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( msgSendListCostumPopupMenu( QPoint ) ) );

//  connect(ui.msgSendList, SIGNAL(itemChanged( QTreeWidgetItem *, int ) ), 
//  this, SLOT(toggleSendItem( QTreeWidgetItem *, int ) ));

  loadInitMsg();
  
  /* hide the Tree +/- */
  ui.msgSendList -> setRootIsDecorated( false );
  
  /* to hide the header  */
  ui.msgSendList->header()->hide();
  
  textColor = Qt::black;


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

void ChatDialog::msgSendListCostumPopupMenu( QPoint point )
{

      QMenu contextMnu( this );
      QMouseEvent *mevent = new QMouseEvent( QEvent::MouseButtonPress, point, Qt::RightButton, Qt::RightButton, Qt::NoModifier );

      privchatAct = new QAction(QIcon(), tr( "Chat" ), this );
      connect( privchatAct , SIGNAL( triggered() ), this, SLOT( privchat() ) );
      
      contextMnu.clear();
      contextMnu.addAction( privchatAct);
      contextMnu.exec( mevent->globalPos() );
}

int     ChatDialog::loadInitMsg()
{
	std::ostringstream out;

	//out << std::endl;
	//out << std::endl;
	//out << std::endl;
	out << "      Welcome to:";
	out << "<br>" << std::endl;
	out << "      Retroshare's group chat. <br>";

	QString txt = QString::fromStdString(out.str());
	ui.msgText->setHtml(txt);

	return 1;
}



void ChatDialog::insertChat()
{
	std::list<ChatInfo> newchat;
	if (!rsMsgs->getNewChat(newchat))
	{
		return;
	}

    QTextEdit *msgWidget = ui.msgText;
	std::list<ChatInfo>::iterator it;
	
	//QString color = ci.messageColor.name();
	//QString nickColor;
	//QString font  = ci.messageFont.family();
	//QString fontSize = QString::number(ci.messageFont.pointSize());

	/* add in lines at the bottom */
	for(it = newchat.begin(); it != newchat.end(); it++)
	{
		/* are they private? */
		if (it->chatflags & RS_CHAT_PRIVATE)
		{
			PopupChatDialog *pcd = getPrivateChat(it->rsid, it->name, true);
			pcd->addChatMsg(&(*it));
			continue;
		}

		std::ostringstream out;
		QString currenttxt = msgWidget->toHtml();
		QString extraTxt;

        QString timestamp = "[" + QDateTime::currentDateTime().toString("hh:mm:ss") + "]";
        QString name = QString::fromStdString(it->name);
        QString line = "<span style=\"color:#C00000\"><strong>" + timestamp + "</strong></span>" +			
            		"<span style=\"color:#2D84C9\"><strong>" + " " + name + "</strong></span> <br>";
            		
        extraTxt += line;

        extraTxt += QString::fromStdWString(it->msg);
        
        /* add it everytime */
		currenttxt += extraTxt;
		
        msgWidget->setHtml(currenttxt);
        

		QScrollBar *qsb =  msgWidget->verticalScrollBar();
		qsb -> setValue(qsb->maximum());
	}
}




void ChatDialog::sendMsg()
{
    QLineEdit *lineWidget = ui.lineEdit;
        
    QFont font = QFont("Comic Sans MS", 10);
	font.setBold(ui.textboldChatButton->isChecked());
	font.setUnderline(ui.textunderlineChatButton->isChecked());
	font.setItalic(ui.textitalicChatButton->isChecked());

	ChatInfo ci;
	ci.msg = lineWidget->text().toStdWString();
	ci.chatflags = RS_CHAT_PUBLIC;
	//ci.messageFont = font;
	//ci.messageColor = textColor;

	rsMsgs -> ChatSend(ci);
	lineWidget -> setText(QString(""));

	/* redraw send list */
	insertSendList();


}

void  ChatDialog::insertSendList()
{
	std::list<std::string> peers;
	std::list<std::string>::iterator it;

	if (!rsPeers)
	{
		/* not ready yet! */
		return;
	}

	rsPeers->getOnlineList(peers);

        /* get a link to the table */
        QTreeWidget *sendWidget = ui.msgSendList;
	QList<QTreeWidgetItem *> items;

	for(it = peers.begin(); it != peers.end(); it++)
	{

		RsPeerDetails details;
		if (!rsPeers->getPeerDetails(*it, details))
		{
			continue; /* BAD */
		}

		/* make a widget per friend */
           	QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)0);

		/* add all the labels */
		/* (0) Person */
		item -> setText(0, QString::fromStdString(details.name));

		item -> setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
		//item -> setFlags(Qt::ItemIsUserCheckable);

		item -> setCheckState(0, Qt::Checked);

		if (rsicontrol->IsInChat(*it))
		{
			item -> setCheckState(0, Qt::Checked);
		}
		else
		{
			item -> setCheckState(0, Qt::Unchecked);
		}

		/* add to the list */
		items.append(item);
	}

        /* remove old items */
	sendWidget->clear();
	sendWidget->setColumnCount(1);

	/* add the items in! */
	sendWidget->insertTopLevelItems(0, items);

	sendWidget->update(); /* update display */
}


/* to toggle the state */


void ChatDialog::toggleSendItem( QTreeWidgetItem *item, int col )
{
	std::cerr << "ToggleSendItem()" << std::endl;

	/* extract id */
	std::string id = (item -> text(4)).toStdString();

	/* get state */
	bool inChat = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

	/* call control fns */

	rsicontrol -> SetInChat(id, inChat);
	return;
}

void ChatDialog::setColor()
{
	textColor = QColorDialog::getColor(Qt::black, this);
	QPixmap pxm(24,24);
	pxm.fill(textColor);
	ui.colorChatButton->setIcon(pxm);
}


void ChatDialog::privchat()
{

}



PopupChatDialog *ChatDialog::getPrivateChat(std::string id, std::string name, bool show)
{
   /* see if it exists already */
   PopupChatDialog *popupchatdialog = NULL;

   std::map<std::string, PopupChatDialog *>::iterator it;
   if (chatDialogs.end() != (it = chatDialogs.find(id)))
   {
   	/* exists already */
   	popupchatdialog = it->second;
   }
   else 
   {
   	popupchatdialog = new PopupChatDialog(id, name);
	chatDialogs[id] = popupchatdialog;
   }

   if (show)
   {
   	popupchatdialog->show();
   }

   return popupchatdialog;

}

void ChatDialog::clearOldChats()
{
	/* nothing yet */

}


