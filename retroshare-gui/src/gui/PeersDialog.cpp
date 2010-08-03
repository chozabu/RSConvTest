/****************************************************************
 *  RetroShare is distributed under the following license:
 *
 *  Copyright (C) 2006,2007 crypton
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

#include <QFile>
#include <QFileInfo>
#include <QShortcut>
#include <QWidgetAction>
#include <QScrollBar>
#include <QDesktopWidget>
#include <QColorDialog>
#include <QFontDialog>
#include <QDropEvent>
#include "common/vmessagebox.h"
#include <gui/mainpagestack.h>

#include "rshare.h"
#include "PeersDialog.h"
#include "rsiface/rsiface.h"
#include "rsiface/rspeers.h"
#include "rsiface/rsstatus.h"
#include "rsiface/rsmsgs.h"
#include "rsiface/rsnotify.h"
#include "settings/rsharesettings.h"

#include "chat/PopupChatDialog.h"
#include "msgs/MessageComposer.h"
#include "connect/ConfCertDialog.h"
#include "profile/ProfileView.h"
#include "profile/ProfileWidget.h"
#include "profile/StatusMessage.h"

#include "gui/connect/ConnectFriendWizard.h"
#include "gui/forums/CreateForum.h"
#include "gui/channels/CreateChannel.h"
#include "gui/feeds/AttachFileItem.h"
#include "RetroShareLink.h"

#include "MainWindow.h"

#include <sstream>
#include <time.h>
#include <sys/stat.h>

#include <QSound>

/* Images for context menu icons */
#define IMAGE_REMOVEFRIEND       ":/images/removefriend16.png"
#define IMAGE_EXPIORTFRIEND      ":/images/exportpeers_16x16.png"
#define IMAGE_PEERINFO           ":/images/peerdetails_16x16.png"
#define IMAGE_CHAT               ":/images/chat.png"
#define IMAGE_MSG                ":/images/message-mail.png"
#define IMAGE_CONNECT            ":/images/connect_friend.png"
/* Images for Status icons */
#define IMAGE_ONLINE             ":/images/user/identity24.png"
#define IMAGE_AWAY               ":/images/user/identity24away.png"
#define IMAGE_BUSY               ":/images/user/identity24busy.png"
#define IMAGE_INACTIVE           ":/images/user/identity24idle.png"
#define IMAGE_OFFLINE            ":/images/user/identityoffline24.png"
#define IMAGE_OFFLINE2           ":/images/user/identitylightgrey24.png"
#define IMAGE_AVAIBLE            ":/images/user/identityavaiblecyan24.png"
#define IMAGE_UNREACHABLE        ":/images/user/identityunreachable24.png"
#define IMAGE_CONNECT2           ":/images/reload24.png"
#define IMAGE_PASTELINK          ":/images/pasterslink.png"

#define COLUMN_COUNT    3
#define COLUMN_NAME     0
#define COLUMN_STATE    1
#define COLUMN_INFO     2

#define COLUMN_DATA     0 // column for storing the userdata id

#define ROLE_SORT  Qt::UserRole
#define ROLE_ID    Qt::UserRole + 1

/******
 * #define PEERS_DEBUG 1
 *****/

// quick and dirty for sorting, better use QTreeView and QSortFilterProxyModel
class MyPeerTreeWidgetItem : public QTreeWidgetItem
{
public:
    MyPeerTreeWidgetItem(QTreeWidget *pWidget, int type) : QTreeWidgetItem(type)
    {
        m_pWidget = pWidget; // can't access the member "view"
    }

    bool operator<(const QTreeWidgetItem &other) const
    {
        int role = Qt::DisplayRole;
        int column = m_pWidget ? m_pWidget->sortColumn() : 0;

        switch (column) {
        case COLUMN_STATE:
                // sort by state set in user role
                role = ROLE_SORT;

                // no break;

        case COLUMN_NAME:
            {
                const QVariant v1 = data(column, role);
                const QVariant v2 = other.data(column, role);
                return (v1.toString().compare (v2.toString(), Qt::CaseInsensitive) < 0);
            }
        }

        // let the standard do the sort
        return QTreeWidgetItem::operator<(other);
    }

private:
    QTreeWidget *m_pWidget; // the member "view" is private
};


/** Constructor */
PeersDialog::PeersDialog(QWidget *parent)
            : RsAutoUpdatePage(1500,parent),
              historyKeeper(Rshare::dataDirectory() + "/his1.xml"),
              smWidget(0)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  last_status_send_time = 0 ;


  connect( ui.peertreeWidget, SIGNAL( customContextMenuRequested( QPoint ) ), this, SLOT( peertreeWidgetCostumPopupMenu( QPoint ) ) );
  connect( ui.peertreeWidget, SIGNAL( itemDoubleClicked ( QTreeWidgetItem *, int)), this, SLOT(chatfriend(QTreeWidgetItem *)));

  connect( ui.avatartoolButton, SIGNAL(clicked()), SLOT(getAvatar()));
  connect( ui.mypersonalstatuslabel, SIGNAL(clicked()), SLOT(statusmessage()));
  connect( ui.actionSet_your_Avatar, SIGNAL(triggered()), this, SLOT(getAvatar()));
  connect( ui.actionSet_your_Personal_Message, SIGNAL(triggered()), this, SLOT(statusmessage()));
  connect( ui.addfileButton, SIGNAL(clicked() ), this , SLOT(addExtraFile()));
  connect( ui.msgText, SIGNAL(anchorClicked(const QUrl &)), SLOT(anchorClicked(const QUrl &)));

  connect(ui.action_Hide_Offline_Friends, SIGNAL(triggered()), this, SLOT(insertPeers()));
  connect(ui.action_Hide_Status_Column, SIGNAL(triggered()), this, SLOT(statusColumn()));

  ui.peertabWidget->setTabPosition(QTabWidget::North);
  ui.peertabWidget->addTab(new ProfileWidget(),QString(tr("Profile")));

  ui.peertreeWidget->setColumnCount(4);
  ui.peertreeWidget->setColumnHidden ( 3, true);
  ui.peertreeWidget->setColumnHidden ( 2, true);
  ui.peertreeWidget->sortItems( 0, Qt::AscendingOrder );

  // set header text aligment
  QTreeWidgetItem * headerItem = ui.peertreeWidget->headerItem();
  headerItem->setTextAlignment(COLUMN_NAME, Qt::AlignHCenter | Qt::AlignVCenter);
  headerItem->setTextAlignment(COLUMN_STATE, Qt::AlignLeft | Qt::AlignVCenter);
  headerItem->setTextAlignment(COLUMN_INFO, Qt::AlignHCenter | Qt::AlignVCenter);

  connect(ui.lineEdit, SIGNAL(textChanged ( ) ), this, SLOT(checkChat( ) ));
  connect(ui.Sendbtn, SIGNAL(clicked()), this, SLOT(sendMsg()));
  connect(ui.emoticonBtn, SIGNAL(clicked()), this, SLOT(smileyWidgetgroupchat()));

  ui.lineEdit->setContextMenuPolicy(Qt::CustomContextMenu) ;
  connect(ui.lineEdit,SIGNAL(customContextMenuRequested(QPoint)),this,SLOT(contextMenu(QPoint)));

  pasteLinkAct = new QAction(QIcon(":/images/pasterslink.png"), tr( "Paste retroshare Link" ), this );
  connect( pasteLinkAct , SIGNAL( triggered() ), this, SLOT( pasteLink() ) );

  connect( ui.msgText, SIGNAL(customContextMenuRequested(const QPoint&)), this, SLOT(displayInfoChatMenu(const QPoint&)));

  connect(ui.textboldChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.textunderlineChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.textitalicChatButton, SIGNAL(clicked()), this, SLOT(setFont()));
  connect(ui.fontsButton, SIGNAL(clicked()), this, SLOT(getFont()));
  connect(ui.colorChatButton, SIGNAL(clicked()), this, SLOT(setColor()));
  connect(ui.actionSave_History, SIGNAL(triggered()), this, SLOT(fileSaveAs()));

  ui.fontsButton->setIcon(QIcon(QString(":/images/fonts.png")));

  _currentColor = Qt::black;
  QPixmap pxm(16,16);
  pxm.fill(_currentColor);
  ui.colorChatButton->setIcon(pxm);

  Settings->beginGroup(QString("Chat"));
  mCurrentFont.fromString(Settings->value(QString::fromUtf8("ChatScreenFont")).toString());
  ui.lineEdit->setFont(mCurrentFont);
 
  setChatInfo(tr("Welcome to RetroShare's group chat."), QString::fromUtf8("blue"));

  if (Settings->value(QString::fromUtf8("GroupChat_History"), true).toBool())
  {
    QStringList him;
    historyKeeper.getMessages(him, "", "THIS", 8);
    foreach(QString mess, him)
    ui.msgText->append(mess);
  }  
  Settings->endGroup();

  //setChatInfo(mess,  "green");

  QMenu * grpchatmenu = new QMenu();
  grpchatmenu->addAction(ui.actionClearChat);
  grpchatmenu->addAction(ui.actionSave_History);
  //grpchatmenu->addAction(ui.actionDisable_Emoticons);
  ui.menuButton->setMenu(grpchatmenu);

  _underline = false;

  QTimer *timer = new QTimer(this);
  timer->connect(timer, SIGNAL(timeout()), this, SLOT(insertChat()));
  timer->start(500); /* half a second */
  
  QMenu *menu = new QMenu();
  menu->addAction(ui.actionAdd_Friend);
  menu->addSeparator();
  menu->addAction(ui.actionCreate_New_Forum);
  #ifndef RS_RELEASE_VERSION
  menu->addAction(ui.actionCreate_New_Channel);
  #endif
  menu->addAction(ui.actionSet_your_Avatar);
  menu->addAction(ui.actionSet_your_Personal_Message);
  
  ui.menupushButton->setMenu(menu);
  
  //ui.msgText->setOpenExternalLinks ( false );
  //ui.msgText->setOpenLinks ( false );
  
  setAcceptDrops(true);
  ui.lineEdit->setAcceptDrops(false);
  
  updateAvatar();
  loadmypersonalstatus();
  loadEmoticonsgroupchat();
  displayMenu();

  // load settings
  processSettings(true);

  // workaround for Qt bug, should be solved in next Qt release 4.7.0
  // http://bugreports.qt.nokia.com/browse/QTBUG-8270
  QShortcut *Shortcut = new QShortcut(QKeySequence (Qt::Key_Delete), ui.peertreeWidget, 0, 0, Qt::WidgetShortcut);
  connect(Shortcut, SIGNAL(activated()), this, SLOT( removefriend ()));

  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}

PeersDialog::~PeersDialog ()
{
    // save settings
    processSettings(false);

    delete smWidget;
}

void PeersDialog::processSettings(bool bLoad)
{
    QHeaderView *header = ui.peertreeWidget->header ();

    Settings->beginGroup(QString("PeersDialog"));

    if (bLoad) {
        // load settings

        // state of peer tree
        header->restoreState(Settings->value("PeerTree").toByteArray());

        // state of hideUnconnected
        ui.action_Hide_Offline_Friends->setChecked(Settings->value("hideUnconnected", false).toBool());
        
        // state of hideStatusColumn
        ui.action_Hide_Status_Column->setChecked(Settings->value("hideStatusColumn", false).toBool());

        // state of splitter
        ui.splitter->restoreState(Settings->value("Splitter").toByteArray());
    } else {
        // save settings

        // state of peer tree
        Settings->setValue("PeerTree", header->saveState());

        // state of hideUnconnected
        Settings->setValue("hideUnconnected", ui.action_Hide_Offline_Friends->isChecked());
        
        // state of hideStatusColumn
        Settings->setValue("hideStatusColumn", ui.action_Hide_Status_Column->isChecked());

        // state of splitter
        Settings->setValue("Splitter", ui.splitter->saveState());
    }

    Settings->endGroup();
}

void PeersDialog::pasteLink()
{
	ui.lineEdit->insertHtml(RSLinkClipboard::toHtml()) ;
}

void PeersDialog::contextMenu( QPoint point )
{
	if(RSLinkClipboard::empty())
		return ;

	QMenu contextMnu(this);
	contextMnu.addAction( pasteLinkAct);

	contextMnu.exec(QCursor::pos());
}

void PeersDialog::peertreeWidgetCostumPopupMenu( QPoint point )
{
    QTreeWidgetItem *c = getCurrentPeer();
    if (!c) {
        //no peer selected
        return;
    }

    QMenu contextMnu( this );

    QAction* expandAll = new QAction(tr( "Expand all" ), &contextMnu );
    connect( expandAll , SIGNAL( triggered() ), ui.peertreeWidget, SLOT (expandAll()) );

    QAction* collapseAll = new QAction(tr( "Collapse all" ), &contextMnu );
    connect( collapseAll , SIGNAL( triggered() ), ui.peertreeWidget, SLOT(collapseAll()) );

    QAction* chatAct = new QAction(QIcon(IMAGE_CHAT), tr( "Chat" ), &contextMnu );
    connect( chatAct , SIGNAL( triggered() ), this, SLOT( chatfriendproxy() ) );

    QAction* msgAct = new QAction(QIcon(IMAGE_MSG), tr( "Message Friend" ), &contextMnu );
    connect( msgAct , SIGNAL( triggered() ), this, SLOT( msgfriend() ) );

    QAction* connectfriendAct = new QAction(QIcon(IMAGE_CONNECT), tr( "Connect To Friend" ), &contextMnu );
    connect( connectfriendAct , SIGNAL( triggered() ), this, SLOT( connectfriend() ) );

    QAction* configurefriendAct = new QAction(QIcon(IMAGE_PEERINFO), tr( "Peer Details" ), &contextMnu );
    connect( configurefriendAct , SIGNAL( triggered() ), this, SLOT( configurefriend() ) );

    QAction* recommendfriendAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Recomend this Friend to..." ), &contextMnu );
    connect( recommendfriendAct , SIGNAL( triggered() ), this, SLOT( recommendfriend() ) );

    QAction* pastePersonAct = new QAction(QIcon(IMAGE_PASTELINK), tr( "Paste Person Link" ), &contextMnu );
    if(!RSLinkClipboard::empty(RetroShareLink::TYPE_PERSON)) {
        connect( pastePersonAct , SIGNAL( triggered() ), this, SLOT( pastePerson() ) );
    } else {
        pastePersonAct->setDisabled(true);
    }

    QAction* profileviewAct = new QAction(QIcon(IMAGE_PEERINFO), tr( "Profile View" ), &contextMnu );
    connect( profileviewAct , SIGNAL( triggered() ), this, SLOT( viewprofile() ) );

    QAction* exportfriendAct = new QAction(QIcon(IMAGE_EXPIORTFRIEND), tr( "Export Friend" ), &contextMnu );
    connect( exportfriendAct , SIGNAL( triggered() ), this, SLOT( exportfriend() ) );

    QAction* removefriendAct;
    if (c->type() == 0) {
        //this is a GPG key
        removefriendAct = new QAction(QIcon(IMAGE_REMOVEFRIEND), tr( "Deny Friend" ), &contextMnu );
    } else {
        removefriendAct = new QAction(QIcon(IMAGE_REMOVEFRIEND), tr( "Remove Friend Location" ), &contextMnu );
    }
    connect( removefriendAct , SIGNAL( triggered() ), this, SLOT( removefriend() ) );


    QWidget *widget = new QWidget(&contextMnu);
    widget->setStyleSheet( ".QWidget{background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1,stop:0 #FEFEFE, stop:1 #E8E8E8); border: 1px solid #CCCCCC;}");

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setMargin(0);
    hbox->setSpacing(6);
    
    iconLabel = new QLabel( this );
    iconLabel->setPixmap(QPixmap(":/images/user/friends24.png"));
    iconLabel->setMaximumSize( iconLabel->frameSize().height() + 24, 24 );
    hbox->addWidget(iconLabel);

    if (c->type() == 0) {
        //this is a GPG key
        textLabel = new QLabel( tr("<strong>GPG Key</strong>"), widget );
    } else {
        textLabel = new QLabel( tr("<strong>RetroShare instance</strong>"), widget );
    }

    hbox->addWidget(textLabel);

    spacerItem = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
    hbox->addItem(spacerItem);

    widget->setLayout( hbox );
    
    QWidgetAction *widgetAction = new QWidgetAction(this);
    widgetAction->setDefaultWidget(widget);

    contextMnu.addAction( widgetAction);
    contextMnu.addAction( chatAct);
    contextMnu.addAction( msgAct);
    contextMnu.addAction( configurefriendAct);
    //contextMnu.addAction( profileviewAct);
    if (c->type() == 0) {
        contextMnu.addAction( recommendfriendAct);
    } else {
        //this is a SSL key
        contextMnu.addAction( connectfriendAct);
    }
    contextMnu.addAction(pastePersonAct);
    contextMnu.addAction( removefriendAct);
    //contextMnu.addAction( exportfriendAct);
    contextMnu.addSeparator();
    contextMnu.addAction( expandAll);
    contextMnu.addAction( collapseAll);

    contextMnu.exec(QCursor::pos());
}

// replaced by shortcut
//void PeersDialog::keyPressEvent(QKeyEvent *e)
//{
//	if(e->key() == Qt::Key_Delete)
//	{
//		removefriend() ;
//		e->accept() ;
//	}
//	else
//		MainPage::keyPressEvent(e) ;
//}

void PeersDialog::updateDisplay()
{
        // add self nick and Avatar to Friends.
        RsPeerDetails pd ;
        if (rsPeers->getPeerDetails(rsPeers->getOwnId(),pd)) {
                QString titleStr("<span style=\"font-size:16pt; font-weight:500;"
                       "color:#32cd32;\">%1</span>");
                ui.nicklabel->setText(titleStr.arg(QString::fromStdString(pd.name) + tr(" (me)") + QString::fromStdString(pd.location))) ;
        }

        insertPeers() ;
}

/* get the list of peers from the RsIface.  */
void  PeersDialog::insertPeers()
{
    #ifdef PEERS_DEBUG
    std::cerr << "PeersDialog::insertPeers() called." << std::endl;
    #endif

    std::list<std::string> gpgFriends;
    std::list<std::string>::iterator it;

    std::list<StatusInfo> statusInfo;
    rsStatus->getStatus(statusInfo);

    if (!rsPeers) {
        /* not ready yet! */
        std::cerr << "PeersDialog::insertPeers() not ready yet : rsPeers unintialized."  << std::endl;
        return;
    }

    bool bHideUnconnected = ui.action_Hide_Offline_Friends->isChecked();

    rsPeers->getGPGAcceptedList(gpgFriends);

    //add own gpg id, if we have more than on location (ssl client)
    std::list<std::string> ownSslContacts;
    std::string ownId = rsPeers->getGPGOwnId();
    rsPeers->getSSLChildListOfGPGId(ownId, ownSslContacts);
    if (ownSslContacts.size() > 0) {
        gpgFriends.push_back(ownId);
    }

    /* get a link to the table */
    QTreeWidget *peertreeWidget = ui.peertreeWidget;

    //remove items that are not friends anymore
    int itemCount = peertreeWidget->topLevelItemCount();
    int index = 0;
    while (index < itemCount) {
        std::string gpg_widget_id = (peertreeWidget->topLevelItem(index))->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
        std::list<std::string>::iterator gpgfriendIt;
        bool found = false;
        for (gpgfriendIt = gpgFriends.begin(); gpgfriendIt != gpgFriends.end(); gpgfriendIt++) {
            if (gpg_widget_id == *gpgfriendIt) {
                found = true;
                break;
            }
        }
        if (!found) {
            delete (peertreeWidget->takeTopLevelItem(index));
            // count again
            itemCount = peertreeWidget->topLevelItemCount();
        } else {
            index++;
        }
    }

    //add the gpg friends
    for(it = gpgFriends.begin(); it != gpgFriends.end(); it++) {
        #ifdef PEERS_DEBUG
        std::cerr << "PeersDialog::insertPeers() inserting gpg_id : " << *it << std::endl;
        #endif

        /* make a widget per friend */
        QTreeWidgetItem *gpg_item = NULL;
        QTreeWidgetItem *gpg_item_loop = NULL;
        QString gpgid = QString::fromStdString(*it);
        itemCount = peertreeWidget->topLevelItemCount();
        for (int nIndex = 0; nIndex < itemCount; nIndex++) {
            gpg_item_loop = peertreeWidget->topLevelItem(nIndex);
            if (gpg_item_loop->data(COLUMN_DATA, ROLE_ID).toString() == gpgid) {
                gpg_item = gpg_item_loop;
                break;
            }
        }

        RsPeerDetails detail;
        if ((!rsPeers->getPeerDetails(*it, detail) || !detail.accept_connection)
            && detail.gpg_id != ownId) {
            //don't accept anymore connection, remove from the view
            if (gpg_item) {
                delete (peertreeWidget->takeTopLevelItem(peertreeWidget->indexOfTopLevelItem(gpg_item)));
            }
            continue;
        }

        bool bNew = false;
        if (gpg_item == NULL) {
            gpg_item = new MyPeerTreeWidgetItem(peertreeWidget, 0); //set type to 0 for custom popup menu
            gpg_item->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
            bNew = true;
        }

        gpg_item -> setText(COLUMN_NAME, QString::fromStdString(detail.name));
        gpg_item -> setSizeHint(COLUMN_NAME,  QSize( 26,26 ) );

        gpg_item -> setTextAlignment(COLUMN_NAME, Qt::AlignLeft | Qt::AlignVCenter );

        /* not displayed, used to find back the item */
        gpg_item -> setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(detail.id));

        //remove items that are not friends anymore
        int childCount = gpg_item->childCount();
        int childIndex = 0;
        while (childIndex < childCount) {
            std::string ssl_id = gpg_item->child(childIndex)->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
            if (!rsPeers->isFriend(ssl_id)) {
                delete (gpg_item->takeChild(childIndex));
                // count again
                childCount = gpg_item->childCount();
            } else {
                childIndex++;
            }
        }

        //update the childs (ssl certs)
        bool gpg_connected = false;
        bool gpg_online = false;
        std::list<std::string> sslContacts;
        rsPeers->getSSLChildListOfGPGId(detail.gpg_id, sslContacts);
        for(std::list<std::string>::iterator sslIt = sslContacts.begin(); sslIt != sslContacts.end(); sslIt++) {
            QTreeWidgetItem *sslItem = NULL;

            //find the corresponding sslItem child item of the gpg item
            bool newChild = true;
            childCount = gpg_item->childCount();
            for (int childIndex = 0; childIndex < childCount; childIndex++) {
                if (gpg_item->child(childIndex)->data(COLUMN_DATA, ROLE_ID).toString().toStdString() == *sslIt) {
                    sslItem = gpg_item->child(childIndex);
                    newChild = false;
                    break;
                }
            }

            RsPeerDetails sslDetail;
            if (!rsPeers->getPeerDetails(*sslIt, sslDetail) || !rsPeers->isFriend(*sslIt)) {
                #ifdef PEERS_DEBUG
                std::cerr << "Removing widget from the view : id : " << *sslIt << std::endl;
                #endif
                //child has disappeared, remove it from the gpg_item
                if (sslItem) {
                    gpg_item->removeChild(sslItem);
                }
                continue;
            }

            if (newChild) {
                sslItem = new MyPeerTreeWidgetItem(peertreeWidget, 1); //set type to 1 for custom popup menu
            }

            /* not displayed, used to find back the item */
            sslItem -> setData(COLUMN_DATA, ROLE_ID, QString::fromStdString(sslDetail.id));

            QString sText;
            std::string customStateString = rsMsgs->getCustomStateString(sslDetail.id);
            sText = tr("location : ") + QString::fromStdString(sslDetail.location);
            if (customStateString.empty() == false) {
                sText += tr(" - ") + QString::fromStdString(customStateString);
            }
            sslItem -> setText( COLUMN_NAME, sText);
            sslItem -> setToolTip( COLUMN_NAME, sText);

            /* not displayed, used to find back the item */
            sslItem -> setText(COLUMN_STATE, QString::fromStdString(sslDetail.autoconnect));
            // sort location
            sslItem -> setData(COLUMN_STATE, ROLE_SORT, sText);

            /* change color and icon */
            int i;
            if (sslDetail.state & RS_PEER_STATE_CONNECTED) {
                sslItem->setHidden(false);
                gpg_connected = true;

                sslItem -> setIcon(COLUMN_NAME,(QIcon(":/images/connect_established.png")));
                sslItem -> setIcon(COLUMN_STATE,(QIcon(":/images/encrypted32.png")));
                QFont font;
                font.setBold(true);
                for(i = 0; i < COLUMN_COUNT; i++) {
                    sslItem -> setTextColor(i,(Qt::darkBlue));
                    sslItem -> setFont(i,font);
                }
            } else if (sslDetail.state & RS_PEER_STATE_ONLINE) {
                sslItem->setHidden(bHideUnconnected);
                gpg_online = true;

                QFont font;
                font.setBold(true);
                for(i = 0; i < COLUMN_COUNT; i++) {
                    sslItem -> setTextColor(i,(Qt::black));
                    sslItem -> setFont(i,font);
                }
            } else {
                sslItem->setHidden(bHideUnconnected);
                if (sslDetail.autoconnect != "Offline") {
                    sslItem -> setIcon(COLUMN_NAME, (QIcon(":/images/connect_creating.png")));
                } else {
                    sslItem -> setIcon(COLUMN_NAME, (QIcon(":/images/connect_no.png")));
                }

                QFont font;
                font.setBold(false);
                for(i = 0; i < COLUMN_COUNT; i++) {
                    sslItem -> setTextColor(i,(Qt::black));
                    sslItem -> setFont(i,font);
                }
            }

            #ifdef PEERS_DEBUG
            std::cerr << "PeersDialog::insertPeers() inserting sslItem." << std::endl;
            #endif
            /* add ssl child to the list. If item is already in the list, it won't be duplicated thanks to Qt */
            if (newChild) {
                gpg_item->addChild(sslItem);
                gpg_item->setExpanded(true);
            }
        }

        int i = 0;
        if (gpg_connected) {
            gpg_item->setHidden(false);
            gpg_item -> setIcon(COLUMN_NAME,(QIcon(IMAGE_ONLINE)));
            gpg_item -> setText(COLUMN_STATE, tr("Online"));
            gpg_item -> setData(COLUMN_STATE, ROLE_SORT, BuildStateSortString(true, gpg_item->text(COLUMN_NAME), PEER_STATE_ONLINE));

            std::list<StatusInfo>::iterator it;
            for(it = statusInfo.begin(); it != statusInfo.end() ; it++) {

                std::list<std::string>::iterator cont_it;

                // don't forget the kids
                for (cont_it = sslContacts.begin(); cont_it != sslContacts.end(); cont_it++) {

                    if((it->id == *cont_it) && (rsPeers->isOnline(*cont_it))){

                        std::string status;
                        rsStatus->getStatusString(it->status, status);
                        gpg_item -> setText(COLUMN_INFO, QString::fromStdString(status));

                        QFont font;
                        font.setBold(true);

                        switch (it->status) {
                        case RS_STATUS_INACTIVE:
                            gpg_item -> setIcon(COLUMN_NAME,(QIcon(IMAGE_INACTIVE)));
                            gpg_item -> setToolTip(COLUMN_NAME, tr("Peer Idle"));
                            gpg_item -> setText(COLUMN_STATE, tr("Idle"));
                            gpg_item -> setData(COLUMN_STATE, ROLE_SORT, BuildStateSortString(true, gpg_item->text(COLUMN_NAME), PEER_STATE_INACTIVE));
                            for(i = 0; i < COLUMN_COUNT; i++) {
                                gpg_item -> setTextColor(i,(Qt::gray));
                                gpg_item -> setFont(i,font);
                            }
                            break;

                        case RS_STATUS_ONLINE:
                            gpg_item -> setIcon(COLUMN_NAME,(QIcon(IMAGE_ONLINE)));
                            gpg_item -> setToolTip(COLUMN_NAME, tr("Peer Online"));
                            gpg_item -> setText(COLUMN_STATE, tr("Online"));
                            gpg_item -> setData(COLUMN_STATE, ROLE_SORT, BuildStateSortString(true, gpg_item->text(COLUMN_NAME), PEER_STATE_ONLINE));
                            for(i = 0; i < COLUMN_COUNT; i++) {
                                gpg_item -> setTextColor(i,(Qt::darkBlue));
                                gpg_item -> setFont(i,font);
                            }
                            break;

                        case RS_STATUS_AWAY:
                            gpg_item -> setIcon(COLUMN_NAME,(QIcon(IMAGE_AWAY)));
                            gpg_item -> setToolTip(COLUMN_NAME, tr("Peer Away"));
                            gpg_item -> setText(COLUMN_STATE, tr("Away"));
                            gpg_item -> setData(COLUMN_STATE, ROLE_SORT, BuildStateSortString(true, gpg_item->text(COLUMN_NAME), PEER_STATE_AWAY));
                            for(i = 0; i < COLUMN_COUNT; i++) {
                                gpg_item -> setTextColor(i,(Qt::gray));
                                gpg_item -> setFont(i,font);
                            }
                            break;

                        case RS_STATUS_BUSY:
                            gpg_item -> setIcon(COLUMN_NAME,(QIcon(IMAGE_BUSY)));
                            gpg_item -> setToolTip(COLUMN_NAME, tr("Peer Busy"));
                            gpg_item -> setText(COLUMN_STATE, tr("Busy"));
                            gpg_item -> setData(COLUMN_STATE, ROLE_SORT, BuildStateSortString(true, gpg_item->text(COLUMN_NAME), PEER_STATE_BUSY));
                            for(i = 0; i < COLUMN_COUNT; i++) {
                                gpg_item -> setTextColor(i,(Qt::gray));
                                gpg_item -> setFont(i,font);
                            }
                            break;
                        }
                    }
                }
            }
        } else if (gpg_online) {
            gpg_item->setHidden(bHideUnconnected);
            gpg_item -> setIcon(COLUMN_NAME,(QIcon(IMAGE_AVAIBLE)));
            gpg_item -> setText(COLUMN_STATE, tr("Available"));
            gpg_item -> setData(COLUMN_STATE, ROLE_SORT, BuildStateSortString(true, gpg_item->text(COLUMN_NAME), PEER_STATE_AVAILABLE));
            QFont font;
            font.setBold(true);
            for(i = 0; i < COLUMN_COUNT; i++) {
                gpg_item -> setTextColor(i,(Qt::black));
                gpg_item -> setFont(i,font);
            }
        } else {
            gpg_item->setHidden(bHideUnconnected);
            gpg_item -> setIcon(COLUMN_NAME,(QIcon(IMAGE_OFFLINE)));
            gpg_item -> setText(COLUMN_STATE, tr("Offline"));
            gpg_item -> setData(COLUMN_STATE, ROLE_SORT, BuildStateSortString(true, gpg_item->text(COLUMN_NAME), PEER_STATE_OFFLINE));
            QFont font;
            font.setBold(false);
            for(i = 0; i < COLUMN_COUNT; i++) {
                gpg_item -> setTextColor(i,(Qt::black));
                gpg_item -> setFont(i,font);
            }
        }

        if (bNew) {
            /* add gpg item to the list */
            peertreeWidget->addTopLevelItem(gpg_item);
        }
    }
}

/* Utility Fns */
std::string getPeerRsCertId(QTreeWidgetItem *i)
{
    std::string id = i -> data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    return id;
}

/** Open a QFileDialog to browse for export a file. */
void PeersDialog::exportfriend()
{
        QTreeWidgetItem *c = getCurrentPeer();

#ifdef PEERS_DEBUG
        std::cerr << "PeersDialog::exportfriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG
                std::cerr << "PeersDialog::exportfriend() None Selected -- sorry" << std::endl;
#endif
		return;
	}

	std::string id = getPeerRsCertId(c);
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save Certificate"), "",
	                                                     tr("Certificates (*.pqi)"));

	std::string file = fileName.toStdString();
	if (file != "")
	{
#ifdef PEERS_DEBUG
                std::cerr << "PeersDialog::exportfriend() Saving to: " << file << std::endl;
#endif
		if (rsPeers)
		{
                        rsPeers->saveCertificateToFile(id, file);
		}
	}

}

void PeersDialog::chatfriendproxy()
{
    chatfriend(getCurrentPeer());
}

void PeersDialog::chatfriend(QTreeWidgetItem *pPeer)
{
    if (pPeer == NULL) {
        return;
    }

    std::string id = pPeer->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    PopupChatDialog::chatFriend(id);
}

void PeersDialog::msgfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::string id = peer->data(COLUMN_DATA, ROLE_ID).toString().toStdString();
    MessageComposer::msgFriend(id);
}

void PeersDialog::recommendfriend()
{
    QTreeWidgetItem *peer = getCurrentPeer();

    if (!peer)
        return;

    std::list <std::string> ids;
    ids.push_back(peer->data(COLUMN_DATA, ROLE_ID).toString().toStdString());
    MessageComposer::recommendFriend(ids);
}

void PeersDialog::pastePerson()
{
    RSLinkClipboard::process(RetroShareLink::TYPE_PERSON, RSLINK_PROCESS_NOTIFY_ERROR);
}

QTreeWidgetItem *PeersDialog::getCurrentPeer()
{
	/* get the current, and extract the Id */

	/* get a link to the table */
        QTreeWidget *peerWidget = ui.peertreeWidget;
        QTreeWidgetItem *item = peerWidget -> currentItem();
        if (!item)
        {
#ifdef PEERS_DEBUG
		std::cerr << "Invalid Current Item" << std::endl;
#endif
		return NULL;
	}

#ifdef PEERS_DEBUG
	/* Display the columns of this item. */
	std::ostringstream out;
        out << "CurrentPeerItem: " << std::endl;

        for(int i = 1; i < COLUMN_COUNT; i++)
	{
		QString txt = item -> text(i);
		out << "\t" << i << ":" << txt.toStdString() << std::endl;
	}
	std::cerr << out.str();
#endif
	return item;
}

/* So from the Peers Dialog we can call the following control Functions:
 * (1) Remove Current.              FriendRemove(id)
 * (2) Allow/DisAllow.              FriendStatus(id, accept)
 * (2) Connect.                     FriendConnectAttempt(id, accept)
 * (3) Set Address.                 FriendSetAddress(id, str, port)
 * (4) Set Trust.                   FriendTrustSignature(id, bool)
 * (5) Configure (GUI Only) -> 3/4
 *
 * All of these rely on the finding of the current Id.
 */


void PeersDialog::removefriend()
{
        QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG
        std::cerr << "PeersDialog::removefriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG
        	std::cerr << "PeersDialog::removefriend() None Selected -- sorry" << std::endl;
#endif
		return;
	}

	if (rsPeers)
	{
		if ((QMessageBox::question(this, tr("RetroShare"),tr("Do you want to remove this Friend?"),QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes))== QMessageBox::Yes)
		{
      rsPeers->removeFriend(getPeerRsCertId(c));
      emit friendsUpdated() ;
		}
		else
		return;
	}
}

void PeersDialog::connectfriend()
{
	QTreeWidgetItem *c = getCurrentPeer();
#ifdef PEERS_DEBUG
	std::cerr << "PeersDialog::connectfriend()" << std::endl;
#endif
	if (!c)
	{
#ifdef PEERS_DEBUG
        	std::cerr << "PeersDialog::connectfriend() Noone Selected -- sorry" << std::endl;
#endif
		return;
	}

	if (rsPeers)
	{
		rsPeers->connectAttempt(getPeerRsCertId(c));
                c -> setIcon(COLUMN_NAME,(QIcon(IMAGE_CONNECT2)));
	}
}

/* GUI stuff -> don't do anything directly with Control */
void PeersDialog::configurefriend()
{
	ConfCertDialog::show(getPeerRsCertId(getCurrentPeer()));
}

void PeersDialog::resetStatusBar() 
{
        #ifdef PEERS_DEBUG
	std::cerr << "PeersDialog: reseting status bar." << std::endl ;
        #endif

	ui.statusStringLabel->setText(QString("")) ;
}

void PeersDialog::updateStatusTyping()
{
	if(time(NULL) - last_status_send_time > 5)	// limit 'peer is typing' packets to at most every 10 sec
	{
                #ifdef PEERS_DEBUG
		std::cerr << "PeersDialog: sending group chat typing info." << std::endl ;
                #endif

		rsMsgs->sendGroupChatStatusString(rsiface->getConfig().ownName + " is typing...");
		last_status_send_time = time(NULL) ;
	}
}
// Called by libretroshare through notifyQt to display the peer's status
//
void PeersDialog::updateStatusString(const QString& status_string)
{
        #ifdef PEERS_DEBUG
	std::cerr << "PeersDialog: received group chat typing info. updating gui." << std::endl ;
        #endif

	ui.statusStringLabel->setText(status_string) ; // displays info for 5 secs.

	QTimer::singleShot(5000,this,SLOT(resetStatusBar())) ;
}

void PeersDialog::updatePeersCustomStateString(const QString& peer_id)
{
#ifdef JUST_AN_EXAMPLE
	// This is an example of how to retrieve the custom string.
	//
	std::cerr << "PeersDialog: Got notified that state string changed for peer " << peer_id.toStdString() << std::endl ;
	std::cerr << "New state string for this peer is : " << rsMsgs->getCustomStateString(peer_id.toStdString()) << std::endl ;

	QMessageBox::information(NULL,"Notification",peer_id+" has new custom string: " + QString::fromStdString(rsMsgs->getCustomStateString(peer_id.toStdString()))) ;
#endif
}

void PeersDialog::updatePeersAvatar(const QString& peer_id)
{
        #ifdef PEERS_DEBUG
	std::cerr << "PeersDialog: Got notified of new avatar for peer " << peer_id.toStdString() << std::endl ;
        #endif

        PopupChatDialog *pcd = PopupChatDialog::getPrivateChat(peer_id.toStdString(),rsPeers->getPeerName(peer_id.toStdString()), 0);
	pcd->updatePeerAvatar(peer_id.toStdString());
}

void PeersDialog::updatePeerStatusString(const QString& peer_id,const QString& status_string,bool is_private_chat)
{
	if(is_private_chat)
	{
                PopupChatDialog *pcd = PopupChatDialog::getPrivateChat(peer_id.toStdString(),rsPeers->getPeerName(peer_id.toStdString()), 0);
		pcd->updateStatusString(status_string);
	}
	else
	{
                #ifdef PEERS_DEBUG
		std::cerr << "Updating public chat msg from peer " << rsPeers->getPeerName(peer_id.toStdString()) << ": " << status_string.toStdString() << std::endl ;
                #endif

		updateStatusString(status_string) ;
	}
}

void PeersDialog::insertChat()
{
	if (!rsMsgs->chatAvailable())
	{
                #ifdef PEERS_DEBUG
		std::cerr << "no chat available." << std::endl ;
                #endif
		return;
	}

	std::list<ChatInfo> newchat;
	if (!rsMsgs->getNewChat(newchat))
	{
                #ifdef PEERS_DEBUG
		std::cerr << "could not get new chat." << std::endl ;
                #endif
		return;
	}
        #ifdef PEERS_DEBUG
        std::cerr << "got new chat." << std::endl;
        #endif
        QTextEdit *msgWidget = ui.msgText;
	std::list<ChatInfo>::iterator it;

        uint chatflags = Settings->getChatFlags();

	/* add in lines at the bottom */
	for(it = newchat.begin(); it != newchat.end(); it++)
	{
                std::string msg(it->msg.begin(), it->msg.end());
                #ifdef PEERS_DEBUG
		std::cerr << "PeersDialog::insertChat(): " << msg << std::endl;
                #endif

		/* are they private? */
		if (it->chatflags & RS_CHAT_PRIVATE)
		{
                        PopupChatDialog *pcd = PopupChatDialog::getPrivateChat(it->rsid, it->name, chatflags);
			pcd->addChatMsg(&(*it));
			playsound();
			QApplication::alert(pcd);
			continue;
		}

		std::ostringstream out;
		QString extraTxt;

                QString timestamp = QDateTime::currentDateTime().toString("hh:mm:ss");
                QString name = QString::fromStdString(it->name);
                QString line = "<span style=\"color:#C00000\">" + timestamp + "</span>" +
                                "<span style=\"color:#2D84C9\"><strong>" + " " + name + "</strong></span>";
		QString msgContents = QString::fromStdWString(it->msg);

                //std::cerr << "PeersDialog::insertChat(): 1.11\n";
		historyKeeper.addMessage(name, "THIS", msgContents);
                //std::cerr << "PeersDialog::insertChat(): 1.12\n";
                extraTxt += line;

                // notify with a systray icon msg
                if(it->rsid != rsPeers->getOwnId())
                {
                      // This is a trick to translate HTML into text.
                      QTextEdit editor ;
                      editor.setHtml(QString::fromStdWString(it->msg));
                      QString notifyMsg(QString::fromStdString(it->name)+": "+editor.toPlainText()) ;

                      if(notifyMsg.length() > 30)
                              emit notifyGroupChat(QString("New group chat"), notifyMsg.left(30)+QString("..."));
                      else
                              emit notifyGroupChat(QString("New group chat"), notifyMsg);
                }

		// create a DOM tree object from the message and embed contents with HTML tags
		QDomDocument doc;
		doc.setContent(msgContents);

		// embed links
		QDomElement body = doc.documentElement();
		RsChat::embedHtml(doc, body, defEmbedAhref);

		// embed smileys
		Settings->beginGroup("Chat");
		if (Settings->value(QString::fromUtf8("Emoteicons_GroupChat"), true).toBool())
			RsChat::embedHtml(doc, body, defEmbedImg);
		Settings->endGroup();

		msgContents = doc.toString(-1);		// -1 removes any annoying carriage return misinterpreted by QTextEdit
		extraTxt += msgContents;

                if ((msgWidget->verticalScrollBar()->maximum() - 30) < msgWidget->verticalScrollBar()->value() ) {
                msgWidget->append(extraTxt);
                } else {
                //the vertical scroll is not at the bottom, so just update the text, the scroll will stay at the current position
                int scroll = msgWidget->verticalScrollBar()->value();
                msgWidget->setHtml(msgWidget->toHtml() + extraTxt);
                msgWidget->verticalScrollBar()->setValue(scroll);
                msgWidget->update();
                }
       }
}

void PeersDialog::checkChat()
{
	/* if <return> at the end of the text -> we can send it! */
        QTextEdit *chatWidget = ui.lineEdit;
        std::string txt = chatWidget->toPlainText().toStdString();
	if ('\n' == txt[txt.length()-1])
	{
		//std::cerr << "Found <return> found at end of :" << txt << ": should send!";
		//std::cerr << std::endl;
		if (txt.length()-1 == txt.find('\n')) /* only if on first line! */
		{
			/* should remove last char ... */
			sendMsg();
		}
	}
	else
	{
		updateStatusTyping() ;

		//std::cerr << "No <return> found in :" << txt << ":";
		//std::cerr << std::endl;
	}
}

void PeersDialog::sendMsg()
{
    QTextEdit *lineWidget = ui.lineEdit;

	ChatInfo ci;
	//ci.msg = lineWidget->Text().toStdWString();
	ci.msg = lineWidget->toHtml().toStdWString();
	ci.chatflags = RS_CHAT_PUBLIC;

    //historyKeeper.addMessage("THIS", "ALL", lineWidget->toHtml() );

	std::string msg(ci.msg.begin(), ci.msg.end());
        #ifdef PEERS_DEBUG
	std::cerr << "PeersDialog::sendMsg(): " << msg << std::endl;
        #endif

	rsMsgs -> ChatSend(ci);
	ui.lineEdit->clear();
	setFont();

	/* redraw send list */
	insertSendList();

}

void  PeersDialog::insertSendList()
{
#ifdef false
	std::list<std::string> peers;
	std::list<std::string>::iterator it;

	if (!rsPeers)
	{
		/* not ready yet! */
		return;
	}

	rsPeers->getOnlineList(peers);

        /* get a link to the table */
        //QTreeWidget *sendWidget = ui.msgSendList;
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

		/* disable for the moment */
		item -> setFlags(Qt::ItemIsUserCheckable);
		item -> setCheckState(0, Qt::Checked);

		/* add to the list */
		items.append(item);
	}

        /* remove old items */
	//sendWidget->clear();
	//sendWidget->setColumnCount(1);

	/* add the items in! */
	//sendWidget->insertTopLevelItems(0, items);

	//sendWidget->update(); /* update display */
#endif
}


/* to toggle the state */


void PeersDialog::toggleSendItem( QTreeWidgetItem *item, int col )
{
        #ifdef PEERS_DEBUG
	std::cerr << "ToggleSendItem()" << std::endl;
        #endif

	/* extract id */
	std::string id = (item -> text(4)).toStdString();

	/* get state */
	bool inChat = (Qt::Checked == item -> checkState(0)); /* alway column 0 */

	/* call control fns */

	rsicontrol -> SetInChat(id, inChat);
	return;
}

//============================================================================

void PeersDialog::setColor()
{

    	bool ok;
 	QRgb color = QColorDialog::getRgba(ui.lineEdit->textColor().rgba(), &ok, this);
 	if (ok) {
 	        _currentColor = QColor(color);
 	        QPixmap pxm(16,16);
	        pxm.fill(_currentColor);
	        ui.colorChatButton->setIcon(pxm);
 	}
	setFont();
}

void PeersDialog::getFont()
{
    bool ok;
    mCurrentFont = QFontDialog::getFont(&ok, mCurrentFont, this);
    setFont();
}

void PeersDialog::setFont()
{
  mCurrentFont.setBold(ui.textboldChatButton->isChecked());
  mCurrentFont.setUnderline(ui.textunderlineChatButton->isChecked());
  mCurrentFont.setItalic(ui.textitalicChatButton->isChecked());
  ui.lineEdit->setFont(mCurrentFont);
  ui.lineEdit->setTextColor(_currentColor);
  Settings->beginGroup("Chat");
  Settings->setValue(QString::fromUtf8("ChatScreenFont"), mCurrentFont.toString());
  Settings->endGroup();


  ui.lineEdit->setFocus();

}

void PeersDialog::underline()
{
 	        _underline = !_underline;
 	        ui.lineEdit->setFontUnderline(_underline);
}


// Update Chat Info information
void PeersDialog::setChatInfo(QString info, QColor color)
{
  static unsigned int nbLines = 0;
  ++nbLines;
  // Check log size, clear it if too big
  if(nbLines > 200) {
    ui.msgText->clear();
    nbLines = 1;
  }
  ui.msgText->append(QString::fromUtf8("<font color='grey'>")+ QTime::currentTime().toString(QString::fromUtf8("hh:mm:ss")) + QString::fromUtf8("</font> - <font color='") + color.name() +QString::fromUtf8("'><i>") + info + QString::fromUtf8("</i></font>"));
}

void PeersDialog::on_actionClearChat_triggered()
{
  ui.msgText->clear();
}

void PeersDialog::displayInfoChatMenu(const QPoint& pos)
{
  // Log Menu
  QMenu myChatMenu(this);
  myChatMenu.addAction(ui.actionClearChat);
  // XXX: Why mapToGlobal() is not enough?
  myChatMenu.exec(mapToGlobal(pos)+QPoint(0,80));
}

void PeersDialog::loadEmoticonsgroupchat()
{
	QString sm_codes;
	#if defined(Q_OS_WIN32)
	QFile sm_file(QApplication::applicationDirPath() + "/emoticons/emotes.acs");
	#else
	QFile sm_file(QString(":/smileys/emotes.acs"));
	#endif
	if(!sm_file.open(QIODevice::ReadOnly))
	{
		std::cerr << "Could not open resouce file :/emoticons/emotes.acs" << std::endl ;
		return ;
	}
	sm_codes = sm_file.readAll();
	sm_file.close();
	sm_codes.remove("\n");
	sm_codes.remove("\r");
	int i = 0;
	QString smcode;
	QString smfile;
	while(sm_codes[i] != '{')
	{
		i++;

	}
	while (i < sm_codes.length()-2)
	{
		smcode = "";
		smfile = "";
		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while (sm_codes[i] != '\"')
		{
			smcode += sm_codes[i];
			i++;

		}
		i++;

		while(sm_codes[i] != '\"')
		{
			i++;
		}
		i++;
		while(sm_codes[i] != '\"' && sm_codes[i+1] != ';')
		{
			smfile += sm_codes[i];
			i++;
		}
		i++;
		if(!smcode.isEmpty() && !smfile.isEmpty())
			#if defined(Q_OS_WIN32)
		    smileys.insert(smcode, smfile);
	        #else
			smileys.insert(smcode, ":/"+smfile);
			#endif
	}

	// init <img> embedder
	defEmbedImg.InitFromAwkwardHash(smileys);
}

void PeersDialog::smileyWidgetgroupchat()
{
	qDebug("MainWindow::smileyWidget()");
	if(smWidget == 0) {
		smWidget = new QWidget(this , Qt::Popup );
		smWidget->setWindowTitle("Emoticons");
		smWidget->setWindowIcon(QIcon(QString(":/images/rstray3.png")));
		//smWidget->setFixedSize(256,256);

		smWidget->setBaseSize( 4*24, (smileys.size()/4)*24  );

		//Warning: this part of code was taken from kadu instant messenger;
		//         It was EmoticonSelector::alignTo(QWidget* w) function there
		//         comments are Polish, I dont' know how does it work...
		// oblicz pozycj� widgetu do kt�rego r�wnamy
		QWidget* w = ui.emoticonBtn;
		QPoint w_pos = w->mapToGlobal(QPoint(0,0));
		// oblicz rozmiar selektora
		QSize e_size = smWidget->sizeHint();
		// oblicz rozmiar pulpitu
		QSize s_size = QApplication::desktop()->size();
		// oblicz dystanse od widgetu do lewego brzegu i do prawego
		int l_dist = w_pos.x();
		int r_dist = s_size.width() - (w_pos.x() + w->width());
		// oblicz pozycj� w zale�no�ci od tego czy po lewej stronie
		// jest wi�cej miejsca czy po prawej
		int x;
		if (l_dist >= r_dist)
			x = w_pos.x() - e_size.width();
		else
			x = w_pos.x() + w->width();
		// oblicz pozycj� y - centrujemy w pionie
		int y = w_pos.y() + w->height()/2 - e_size.height()/2;
		// je�li wychodzi poza doln� kraw�d� to r�wnamy do niej
		if (y + e_size.height() > s_size.height())
			y = s_size.height() - e_size.height();
		// je�li wychodzi poza g�rn� kraw�d� to r�wnamy do niej
		if (y < 0)
			y = 0;
		// ustawiamy selektor na wyliczonej pozycji
		smWidget->move(x, y);

		x = 0;
		y = 0;

		QHashIterator<QString, QString> i(smileys);
		while(i.hasNext())
		{
			i.next();
			QPushButton *smButton = new QPushButton("", smWidget);
			smButton->setGeometry(x*24, y*24, 24,24);
			smButton->setIconSize(QSize(24,24));
			smButton->setIcon(QPixmap(i.value()));
			smButton->setToolTip(i.key());
			//smButton->setFixedSize(24,24);
			++x;
			if(x > 4)
			{
				x = 0;
				y++;
			}
			connect(smButton, SIGNAL(clicked()), this, SLOT(addSmileys()));
			connect(smButton, SIGNAL(clicked()), smWidget, SLOT(close()));
		}
	}

	smWidget->show();
}

void PeersDialog::addSmileys()
{
	ui.lineEdit->setText(ui.lineEdit->toHtml() + qobject_cast<QPushButton*>(sender())->toolTip().split("|").first());
}

/* GUI stuff -> don't do anything directly with Control */
void PeersDialog::viewprofile()
{
	/* display Dialog */

	QTreeWidgetItem *c = getCurrentPeer();


	static ProfileView *profileview = new ProfileView();


	if (!c)
		return;

	/* set the Id */
	std::string id = getPeerRsCertId(c);

	profileview -> setPeerId(id);
	profileview -> show();
}

void PeersDialog::updateAvatar()
{
	unsigned char *data = NULL;
	int size = 0 ;

	rsMsgs->getOwnAvatarData(data,size); 

        #ifdef PEERS_DEBUG
	std::cerr << "Image size = " << size << std::endl ;
        #endif

	if(size == 0)
	   std::cerr << "Got no image" << std::endl ;

	// set the image
	QPixmap pix ;
	pix.loadFromData(data,size,"PNG") ;
	ui.avatartoolButton->setIcon(pix); // writes image into ba in PNG format

        PopupChatDialog::updateAllAvatars();

	delete[] data ;
}

void PeersDialog::getAvatar()
{
	QString fileName = QFileDialog::getOpenFileName(this, "Load File", QDir::homePath(), "Pictures (*.png *.xpm *.jpg *.tiff *.gif)");
	if(!fileName.isEmpty())
	{
		QPixmap picture;
		picture = QPixmap(fileName).scaled(82,82, Qt::IgnoreAspectRatio,Qt::SmoothTransformation);

                #ifdef PEERS_DEBUG
		std::cerr << "Sending avatar image down the pipe" << std::endl ;
                #endif

		// send avatar down the pipe for other peers to get it.
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		picture.save(&buffer, "PNG"); // writes image into ba in PNG format

                #ifdef PEERS_DEBUG
		std::cerr << "Image size = " << ba.size() << std::endl ;
                #endif

		rsMsgs->setOwnAvatarData((unsigned char *)(ba.data()),ba.size()) ;	// last char 0 included.

		// I suppressed this because it gets called already by rsMsgs->setOwnAvatarData() through a Qt notification signal
		//updateAvatar() ;
	}
}

void PeersDialog::changeAvatarClicked() 
{

	updateAvatar();
}

void PeersDialog::on_actionAdd_Friend_activated() 
{
    ConnectFriendWizard connectwiz (this);

    connectwiz.exec ();
}

void PeersDialog::on_actionCreate_New_Forum_activated()
{
    MainWindow::activatePage (MainWindow::Forums);

    CreateForum cf (this);
    cf.exec();
    
}

void PeersDialog::on_actionCreate_New_Channel_activated()
{
#ifndef RS_RELEASE_VERSION
    MainWindow::activatePage (MainWindow::Channels);

    CreateChannel cf (this);
    cf.setWindowTitle(tr("Create a new Channel"));
    cf.ui.labelicon->setPixmap(QPixmap(":/images/add_channel64.png"));
    QString titleStr("<span style=\"font-size:24pt; font-weight:500;"
                               "color:#32CD32;\">%1</span>");
    cf.ui.textlabelcreatforums->setText( titleStr.arg( tr("New Channel") ) ) ;
    cf.exec();
#endif
}


/** Loads own personal status */
void PeersDialog::loadmypersonalstatus()
{
    ui.mypersonalstatuslabel->setText(QString::fromStdString(rsMsgs->getCustomStateString()));
}

void PeersDialog::statusmessage()
{
    StatusMessage statusmsgdialog (this);
    statusmsgdialog.exec();
}

void PeersDialog::addExtraFile()
{
	// select a file
	QString qfile = QFileDialog::getOpenFileName(this, tr("Add Extra File"), "", "", 0,
				QFileDialog::DontResolveSymlinks);
	std::string filePath = qfile.toStdString();
	if (filePath != "")
	{
	    PeersDialog::addAttachment(filePath);
	}
}

void PeersDialog::addAttachment(std::string filePath) {
	    /* add a AttachFileItem to the attachment section */
            std::cerr << "PopupChatDialog::addExtraFile() hashing file." << std::endl;

	    /* add widget in for new destination */
	    AttachFileItem *file = new AttachFileItem(filePath);
	    //file->

	    ui.verticalLayout->addWidget(file, 1, 0);

	    //when the file is local or is finished hashing, call the fileHashingFinished method to send a chat message
	    if (file->getState() == AFI_STATE_LOCAL) {
		fileHashingFinished(file);
	    } else {
		QObject::connect(file,SIGNAL(fileFinished(AttachFileItem *)), SLOT(fileHashingFinished(AttachFileItem *))) ;
	    }
}

void PeersDialog::fileHashingFinished(AttachFileItem* file) {
        std::cerr << "PeersDialog::fileHashingFinished() started." << std::endl;

	//check that the file is ok tos end
	if (file->getState() == AFI_STATE_ERROR) {
            #ifdef PEERS_DEBUG
            std::cerr << "PopupChatDialog::fileHashingFinished error file is not hashed." << std::endl;
            #endif
	    return;
	}

	ChatInfo ci;


	{
	  rsiface->lockData(); /* Lock Interface */
	  const RsConfig &conf = rsiface->getConfig();

	  ci.rsid = conf.ownId;
	  ci.name = conf.ownName;

	  rsiface->unlockData(); /* Unlock Interface */
	}

        //convert fileSize from uint_64 to string for html link
//	char fileSizeChar [100];
//	sprintf(fileSizeChar, "%lld", file->FileSize());
//	std::string fileSize = *(&fileSizeChar);

	std::string mesgString = RetroShareLink(QString::fromStdString(file->FileName()),
						file->FileSize(),
						QString::fromStdString(file->FileHash())).toHtml().toStdString() ;

//	std::string mesgString = "<a href='retroshare://file|" + (file->FileName()) + "|" + fileSize + "|" + (file->FileHash()) + "'>" 
//	+ "retroshare://file|" + (file->FileName()) + "|" + fileSize +  "|" + (file->FileHash())  + "</a>";
#ifdef PEERS_DEBUG
	std::cerr << "PeersDialog::fileHashingFinished mesgString : " << mesgString << std::endl;
#endif

	const char * messageString = mesgString.c_str ();

	//convert char massageString to w_char
	wchar_t* message;
	size_t requiredSize = mbstowcs(NULL, messageString, 0); // C4996
	/* Add one to leave room for the NULL terminator */
	message = (wchar_t *)malloc( (requiredSize + 1) * sizeof( wchar_t ));
	if (! message) {
	    std::cerr << ("Memory allocation failure.\n");
	}
	size_t size = mbstowcs( message, messageString, requiredSize + 1); // C4996
	if (size == (size_t) (-1)) {
	   printf("Couldn't convert string--invalid multibyte character.\n");
	}

	ci.msg = message;
	ci.chatflags = RS_CHAT_PUBLIC;

	rsMsgs -> ChatSend(ci);
	setFont();
}

void PeersDialog::anchorClicked (const QUrl& link ) 
{
    #ifdef PEERS_DEBUG
    std::cerr << "PeersDialog::anchorClicked link.scheme() : " << link.scheme().toStdString() << std::endl;
    #endif

    RetroShareLink::processUrl(link, NULL, RSLINK_PROCESS_NOTIFY_ALL);
}

void PeersDialog::dropEvent(QDropEvent *event)
{
	if (!(Qt::CopyAction & event->possibleActions()))
	{
                std::cerr << "PeersDialog::dropEvent() Rejecting uncopyable DropAction" << std::endl;

		/* can't do it */
		return;
	}

        std::cerr << "PeersDialog::dropEvent() Formats" << std::endl;
	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;
	for(it = formats.begin(); it != formats.end(); it++)
	{
                std::cerr << "Format: " << (*it).toStdString() << std::endl;
	}

	if (event->mimeData()->hasUrls())
	{
                std::cerr << "PeersDialog::dropEvent() Urls:" << std::endl;

		QList<QUrl> urls = event->mimeData()->urls();
		QList<QUrl>::iterator uit;
		for(uit = urls.begin(); uit != urls.end(); uit++)
		{
			std::string localpath = uit->toLocalFile().toStdString();
                        std::cerr << "Whole URL: " << uit->toString().toStdString() << std::endl;
                        std::cerr << "or As Local File: " << localpath << std::endl;

			if (localpath.size() > 0)
			{
				struct stat buf;
				//Check that the file does exist and is not a directory
				if ((-1 == stat(localpath.c_str(), &buf))) {
				    std::cerr << "PeersDialog::dropEvent() file does not exists."<< std::endl;
				    QMessageBox mb(tr("Drop file error."), tr("File not found or file name not accepted."),QMessageBox::Information,QMessageBox::Ok,0,0);
				    mb.setButtonText( QMessageBox::Ok, "OK" );
				    mb.exec();
				} else if (S_ISDIR(buf.st_mode)) {
				    std::cerr << "PeersDialog::dropEvent() directory not accepted."<< std::endl;
				    QMessageBox mb(tr("Drop file error."), tr("Directory can't be dropped, only files are accepted."),QMessageBox::Information,QMessageBox::Ok,0,0);
				    mb.setButtonText( QMessageBox::Ok, "OK" );
				    mb.exec();
				} else {
				    PeersDialog::addAttachment(localpath);
				}
			}
		}
	}

	event->setDropAction(Qt::CopyAction);
	event->accept();
}

void PeersDialog::dragEnterEvent(QDragEnterEvent *event)
{
	/* print out mimeType */
        std::cerr << "PeersDialog::dragEnterEvent() Formats" << std::endl;
	QStringList formats = event->mimeData()->formats();
	QStringList::iterator it;
	for(it = formats.begin(); it != formats.end(); it++)
	{
                std::cerr << "Format: " << (*it).toStdString() << std::endl;
	}

	if (event->mimeData()->hasUrls())
	{
                std::cerr << "PeersDialog::dragEnterEvent() Accepting Urls" << std::endl;
		event->acceptProposedAction();
	}
	else
	{
                std::cerr << "PeersDialog::dragEnterEvent() No Urls" << std::endl;
	}
}

bool PeersDialog::fileSave()
{
    if (fileName.isEmpty())
        return fileSaveAs();

    QFile file(fileName);
    if (!file.open(QFile::WriteOnly))
        return false;
    QTextStream ts(&file);
    ts.setCodec(QTextCodec::codecForName("UTF-8"));
    ts << ui.msgText->document()->toPlainText();
    ui.msgText->document()->setModified(false);
    return true;
}

bool PeersDialog::fileSaveAs()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save as..."),
                                              QString(), tr("Text File (*.txt );;All Files (*)"));
    if (fn.isEmpty())
        return false;
    setCurrentFileName(fn);
    return fileSave();    
}

void PeersDialog::setCurrentFileName(const QString &fileName)
{
    this->fileName = fileName;
    ui.msgText->document()->setModified(false);

    setWindowModified(false);
}

////play sound when recv a message
void PeersDialog::playsound(){
    Settings->beginGroup("Sound");
        Settings->beginGroup("SoundFilePath");
            QString OnlineSound = Settings->value("NewChatMessage","").toString();
        Settings->endGroup();
        Settings->beginGroup("Enable");
             bool flag = Settings->value("NewChatMessage",false).toBool();
        Settings->endGroup();
    Settings->endGroup();
    if(!OnlineSound.isEmpty()&&flag)
        if(QSound::isAvailable())
            QSound::play(OnlineSound);
}

void PeersDialog::displayMenu()
{
    QMenu *displaymenu = new QMenu();

    displaymenu->addAction(ui.action_Hide_Offline_Friends);
    displaymenu->addAction(ui.action_Hide_Status_Column);

    ui.displayButton->setMenu(displaymenu);
}

void PeersDialog::statusColumn()
{
    /* Set header resize modes and initial section sizes */
    QHeaderView * peerheader = ui.peertreeWidget->header();

    if(ui.action_Hide_Status_Column->isChecked())
    {
        ui.peertreeWidget->setColumnHidden ( 1, true);
        peerheader->resizeSection ( 0, 200 );
    }    
    else
    {
        ui.peertreeWidget->setColumnHidden ( 1, false);
        peerheader->resizeSection ( 0, 200 );
    }
    
}
