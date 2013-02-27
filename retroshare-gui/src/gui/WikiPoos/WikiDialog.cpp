/*
 * Retroshare Wiki Plugin.
 *
 * Copyright 2012-2012 by Robert Fernie.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License Version 2.1 as published by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 *
 * Please report all bugs and problems to "retroshare@lunamutt.com".
 *
 */

#include <QMenu>
#include <QFile>
#include <QFileInfo>

#include "WikiDialog.h"
#include "gui/WikiPoos/WikiAddDialog.h"
#include "gui/WikiPoos/WikiEditDialog.h"

#include "gui/gxs/WikiGroupDialog.h"

#include <retroshare/rswiki.h>

// These should be in retroshare/ folder.
#include "gxs/rsgxsflags.h"


#include <iostream>
#include <sstream>

#include <QTimer>

#define USE_PEGMMD_RENDERER     1

#ifdef USE_PEGMMD_RENDERER
#include "markdown_lib.h"
#endif

/******
 * #define WIKI_DEBUG 1
 *****/

#define WIKI_DEBUG 1

#define WIKIDIALOG_LISTING_GROUPMETA		2
#define WIKIDIALOG_LISTING_PAGES		5
#define WIKIDIALOG_MOD_LIST			6
#define WIKIDIALOG_MOD_PAGES			7
#define WIKIDIALOG_WIKI_PAGE			8

#define WIKIDIALOG_EDITTREE_DATA		9



/* Images for TreeWidget (Copied from GxsForums.cpp) */
#define IMAGE_FOLDER         ":/images/folder16.png"
#define IMAGE_FOLDERGREEN    ":/images/folder_green.png"
#define IMAGE_FOLDERRED      ":/images/folder_red.png"
#define IMAGE_FOLDERYELLOW   ":/images/folder_yellow.png"
#define IMAGE_FORUM          ":/images/konversation.png"
#define IMAGE_SUBSCRIBE      ":/images/edit_add24.png"
#define IMAGE_UNSUBSCRIBE    ":/images/cancel.png"
#define IMAGE_INFO           ":/images/info16.png"
#define IMAGE_NEWFORUM       ":/images/new_forum16.png"
#define IMAGE_FORUMAUTHD     ":/images/konv_message2.png"
#define IMAGE_COPYLINK       ":/images/copyrslink.png"


/** Constructor */
WikiDialog::WikiDialog(QWidget *parent)
: MainPage(parent)
{
	/* Invoke the Qt Designer generated object setup routine */
	ui.setupUi(this);

	mAddPageDialog = NULL;
	mAddGroupDialog = NULL;
	mEditDialog = NULL;

	connect( ui.toolButton_NewGroup, SIGNAL(clicked()), this, SLOT(OpenOrShowAddGroupDialog()));
	connect( ui.toolButton_NewPage, SIGNAL(clicked()), this, SLOT(OpenOrShowAddPageDialog()));
	connect( ui.toolButton_Edit, SIGNAL(clicked()), this, SLOT(OpenOrShowEditDialog()));
	connect( ui.toolButton_Republish, SIGNAL(clicked()), this, SLOT(OpenOrShowRepublishDialog()));

	// Usurped until Refresh works normally
	connect( ui.toolButton_Delete, SIGNAL(clicked()), this, SLOT(insertWikiGroups()));

	connect( ui.treeWidget_Pages, SIGNAL(itemSelectionChanged()), this, SLOT(groupTreeChanged()));


	// GroupTreeWidget.
        connect(ui.groupTreeWidget, SIGNAL(treeCustomContextMenuRequested(QPoint)), this, SLOT(groupListCustomPopupMenu(QPoint)));
        connect(ui.groupTreeWidget, SIGNAL(treeItemActivated(QString)), this, SLOT(wikiGroupChanged(QString)));





	QTimer *timer = new QTimer(this);
	timer->connect(timer, SIGNAL(timeout()), this, SLOT(checkUpdate()));
	timer->start(1000);

	/* setup TokenQueue */
        mWikiQueue = new TokenQueue(rsWiki->getTokenService(), this);


	/* Setup Group Tree */
        mYourGroups = ui.groupTreeWidget->addCategoryItem(tr("My Groups"), QIcon(IMAGE_FOLDER), true);
        mSubscribedGroups = ui.groupTreeWidget->addCategoryItem(tr("Subscribed Groups"), QIcon(IMAGE_FOLDERRED), true);
        mPopularGroups = ui.groupTreeWidget->addCategoryItem(tr("Popular Groups"), QIcon(IMAGE_FOLDERGREEN), false);
        mOtherGroups = ui.groupTreeWidget->addCategoryItem(tr("Other Groups"), QIcon(IMAGE_FOLDERYELLOW), false);


}

void WikiDialog::checkUpdate()
{
	/* update */
	if (!rsWiki)
		return;

	if (rsWiki->updated())
	{
		insertWikiGroups();
	}

	return;
}


void WikiDialog::OpenOrShowAddPageDialog()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No Group selected";
		std::cerr << std::endl;
		return;
	}

	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

	std::cerr << "WikiDialog::OpenOrShowAddPageDialog() GroupId: " << groupId;
	std::cerr << std::endl;

	mEditDialog->setupData(groupId, "");
	mEditDialog->setNewPage();

	mEditDialog->show();
}



void WikiDialog::OpenOrShowAddGroupDialog()
{
	newGroup();
}

/*********************** **** **** **** ***********************/
/** New / Edit Groups          ********************************/
/*********************** **** **** **** ***********************/

void WikiDialog::newGroup()
{
        WikiGroupDialog cf(mWikiQueue, this);

        cf.exec ();
}

void WikiDialog::showGroupDetails()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::showGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}
}

void WikiDialog::editGroupDetails()
{
	std::string groupId = getSelectedGroup();
	if (groupId == "")
	{
		std::cerr << "WikiDialog::editGroupDetails() No Group selected";
		std::cerr << std::endl;
		return;
	}


        //WikiGroupDialog cf (this);
        //cf.existingGroup(groupId,  GXS_GROUP_DIALOG_EDIT_MODE);

        //cf.exec ();
}



void WikiDialog::OpenOrShowEditDialog()
{
	std::string groupId;
	std::string pageId;
	std::string origPageId;

	if (!getSelectedPage(groupId, pageId, origPageId))
	{
		std::cerr << "WikiDialog::OpenOrShowAddPageDialog() No Group or PageId selected";
		std::cerr << std::endl;
		return;
	}

	std::cerr << "WikiDialog::OpenOrShowAddPageDialog()";
	std::cerr << std::endl;

	if (!mEditDialog)
	{
		mEditDialog = new WikiEditDialog(NULL);
	}

	mEditDialog->setupData(groupId, pageId);
	mEditDialog->show();
}

void WikiDialog::OpenOrShowRepublishDialog()
{
	OpenOrShowEditDialog();

	std::string groupId;
	std::string pageId;
	std::string origPageId;

	if (!getSelectedPage(groupId, pageId, origPageId))
	{
		std::cerr << "WikiDialog::OpenOrShowAddRepublishDialog() No Group or PageId selected";
		std::cerr << std::endl;
		if (mEditDialog)
		{
			mEditDialog->hide();
		}
		return;
	}

	mEditDialog->setRepublishMode(origPageId);
}


void WikiDialog::groupTreeChanged()
{
	/* */
	std::string groupId;
	std::string pageId;
	std::string origPageId;

	getSelectedPage(groupId, pageId, origPageId);
	if (pageId == mPageSelected)
	{
		return; /* nothing changed */
	}

	if (pageId == "")
	{
		/* clear Mods */
		clearGroupTree();
		return;
	}

        RsGxsGrpMsgIdPair origPagePair = std::make_pair(groupId, origPageId);
        RsGxsGrpMsgIdPair pagepair = std::make_pair(groupId, pageId);
	requestWikiPage(pagepair);
}

void WikiDialog::updateWikiPage(const RsWikiSnapshot &page)
{
#ifdef USE_PEGMMD_RENDERER
	/* render as HTML */
	int extensions = 0;
	char *answer = markdown_to_string((char *) page.mPage.c_str(), extensions, HTML_FORMAT);

	QString renderedText = QString::fromUtf8(answer);
	ui.textBrowser->setHtml(renderedText);

	// free answer.
	free(answer);
#else
	/* render as HTML */
	QString renderedText = "IN (dummy) RENDERED TEXT MODE:\n";
	renderedText += QString::fromStdString(page.mPage);
	ui.textBrowser->setPlainText(renderedText);
#endif
}


void WikiDialog::clearWikiPage()
{
	ui.textBrowser->setPlainText("");
}


void 	WikiDialog::clearGroupTree()
{
	ui.treeWidget_Pages->clear();
}


#define WIKI_GROUP_COL_GROUPNAME	0
#define WIKI_GROUP_COL_GROUPID		1

#define WIKI_GROUP_COL_PAGENAME		0
#define WIKI_GROUP_COL_PAGEID		1
#define WIKI_GROUP_COL_ORIGPAGEID	2


bool WikiDialog::getSelectedPage(std::string &groupId, std::string &pageId, std::string &origPageId)
{
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedPage()" << std::endl;
#endif

	/* get current item */
	QTreeWidgetItem *item = ui.treeWidget_Pages->currentItem();

	if (!item)
	{
		/* leave current list */
#ifdef WIKI_DEBUG 
		std::cerr << "WikiDialog::getSelectedPage() Nothing selected" << std::endl;
#endif
		return false;
	}

	/* check if it has changed */
	groupId = getSelectedGroup();
	if (groupId == "")
	{
		return false;
	}

	pageId = item->text(WIKI_GROUP_COL_PAGEID).toStdString();
	origPageId = item->text(WIKI_GROUP_COL_ORIGPAGEID).toStdString();

#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedPage() PageId: " << pageId << std::endl;
#endif
	return true;
}


std::string WikiDialog::getSelectedGroup()
{
#ifdef WIKI_DEBUG 
	std::cerr << "WikiDialog::getSelectedGroup(): " << mGroupId << std::endl;
#endif
	return mGroupId;
}


/************************** Request / Response *************************/
/*** Loading Main Index ***/

void WikiDialog::insertWikiGroups()
{
	requestGroupMeta();
}


void WikiDialog::requestGroupMeta()
{
	std::cerr << "WikiDialog::requestGroupMeta()";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_META;

	uint32_t token;
	mWikiQueue->requestGroupInfo(token,  RS_TOKREQ_ANSTYPE_SUMMARY, opts, WIKIDIALOG_LISTING_GROUPMETA);
}


void WikiDialog::loadGroupMeta(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadGroupMeta()";
	std::cerr << std::endl;

	std::list<RsGroupMetaData> groupMeta;

        if (!rsWiki->getGroupSummary(token, groupMeta))
        {
                std::cerr << "WikiDialog::loadGroupMeta() Error getting GroupMeta";
                std::cerr << std::endl;
                return;
        }

        if (groupMeta.size() > 0)
        {
                insertGroupsData(groupMeta);
        }
        else
        {
                std::cerr << "WikiDialog::loadGroupMeta() ERROR No Groups...";
                std::cerr << std::endl;
        }
}


void WikiDialog::requestPages(const std::list<RsGxsGroupId> &groupIds)
{
	std::cerr << "WikiDialog::requestPages()";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;
	opts.mOptions = (RS_TOKREQOPT_MSG_LATEST | RS_TOKREQOPT_MSG_THREAD); // We want latest version of Thread Heads.
	uint32_t token;
	mWikiQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, groupIds, WIKIDIALOG_LISTING_PAGES);
}


void WikiDialog::loadPages(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadPages()";
	std::cerr << std::endl;

	clearGroupTree();

	QTreeWidgetItem *groupItem = NULL;

	std::vector<RsWikiSnapshot> snapshots;
	std::vector<RsWikiSnapshot>::iterator vit;
	if (!rsWiki->getSnapshots(token, snapshots))
	{
		// ERROR
		return;
	}

	for(vit = snapshots.begin(); vit != snapshots.end(); vit++)
	{
		RsWikiSnapshot page = *vit;

		std::cerr << "WikiDialog::loadPages() PageId: " << page.mMeta.mMsgId;
		std::cerr << " Page: " << page.mMeta.mMsgName;
		std::cerr << std::endl;

		QTreeWidgetItem *pageItem = new QTreeWidgetItem();
		pageItem->setText(WIKI_GROUP_COL_PAGENAME, QString::fromStdString(page.mMeta.mMsgName));
		pageItem->setText(WIKI_GROUP_COL_PAGEID, QString::fromStdString(page.mMeta.mMsgId));
		pageItem->setText(WIKI_GROUP_COL_ORIGPAGEID, QString::fromStdString(page.mMeta.mOrigMsgId));

		ui.treeWidget_Pages->addTopLevelItem(pageItem);
	}
}

/***** Wiki *****/

void WikiDialog::requestWikiPage(const RsGxsGrpMsgIdPair &msgId)
{
	std::cerr << "WikiDialog::requestWikiPage(" << msgId.first << "," << msgId.second << ")";
	std::cerr << std::endl;

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_MSG_DATA;

	uint32_t token;

	GxsMsgReq msgIds;
	std::vector<RsGxsMessageId> &vect_msgIds = msgIds[msgId.first];
	vect_msgIds.push_back(msgId.second);

	mWikiQueue->requestMsgInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, msgIds, WIKIDIALOG_WIKI_PAGE);
}


void WikiDialog::loadWikiPage(const uint32_t &token)
{
	std::cerr << "WikiDialog::loadWikiPage()";
	std::cerr << std::endl;

	// Should only have one WikiPage....
	std::vector<RsWikiSnapshot> snapshots;
	if (!rsWiki->getSnapshots(token, snapshots))
	{
		std::cerr << "WikiDialog::loadWikiPage() ERROR";
		std::cerr << std::endl;

		// ERROR
		return;
	}

	if (snapshots.size() != 1) 
	{
		std::cerr << "WikiDialog::loadWikiPage() SIZE ERROR";
		std::cerr << std::endl;

		// ERROR
		return;
	}


	RsWikiSnapshot page = snapshots[0];
	
	std::cerr << "WikiDialog::loadWikiPage() PageId: " << page.mMeta.mMsgId;
	std::cerr << " Page: " << page.mMeta.mMsgName;
	std::cerr << std::endl;

	updateWikiPage(page);
}



void WikiDialog::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	std::cerr << "WikiDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
	
	if (queue == mWikiQueue)
	{
		/* now switch on req */
		switch(req.mUserType)
		{
			case WIKIDIALOG_LISTING_GROUPMETA:
				loadGroupMeta(req.mToken);
				break;

			case WIKIDIALOG_LISTING_PAGES:
				loadPages(req.mToken);
				break;

			case WIKIDIALOG_WIKI_PAGE:
				loadWikiPage(req.mToken);
				break;

#define GXSGROUP_NEWGROUPID	     1
			case GXSGROUP_NEWGROUPID:
				insertWikiGroups();
				break;
			default:
				std::cerr << "WikiDialog::loadRequest() ERROR: INVALID TYPE";
				std::cerr << std::endl;
				break;
		}
	}
}
	
	
	
	

/************************** Group Widget Stuff *********************************/


void WikiDialog::subscribeToGroup()
{
        wikiSubscribe(true);
}

void WikiDialog::unsubscribeToGroup()
{
        wikiSubscribe(false);
}

void WikiDialog::wikiSubscribe(bool subscribe)
{
        if (mGroupId.empty()) {
                return;
        }

        uint32_t token;
        rsWiki->subscribeToGroup(token, mGroupId, subscribe);
}


void WikiDialog::wikiGroupChanged(const QString &groupId)
{
	mGroupId = groupId.toStdString();

        if (mGroupId.empty()) {
                return;
        }

	std::list<RsGxsGroupId> groupIds;
	groupIds.push_back(mGroupId);
	requestPages(groupIds);

	int subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId));
	ui.toolButton_NewPage->setEnabled(IS_GROUP_ADMIN(subscribeFlags));
	ui.toolButton_Republish->setEnabled(IS_GROUP_ADMIN(subscribeFlags));

}


void WikiDialog::groupListCustomPopupMenu(QPoint /*point*/)
{

	int subscribeFlags = ui.groupTreeWidget->subscribeFlags(QString::fromStdString(mGroupId));

	QMenu contextMnu(this);

	std::cerr << "WikiDialog::groupListCustomPopupMenu()";
	std::cerr << std::endl;
	std::cerr << "    mGroupId: " << mGroupId;
	std::cerr << std::endl;
	std::cerr << "    subscribeFlags: " << subscribeFlags;
	std::cerr << std::endl;
	std::cerr << "    IS_GROUP_SUBSCRIBED(): " << IS_GROUP_SUBSCRIBED(subscribeFlags);
	std::cerr << std::endl;
	std::cerr << "    IS_GROUP_ADMIN(): " << IS_GROUP_ADMIN(subscribeFlags);
	std::cerr << std::endl;
	std::cerr << std::endl;

	QAction *action = contextMnu.addAction(QIcon(IMAGE_SUBSCRIBE), tr("Subscribe to Group"), this, SLOT(subscribeToGroup()));
	action->setDisabled (mGroupId.empty() || IS_GROUP_SUBSCRIBED(subscribeFlags));

	action = contextMnu.addAction(QIcon(IMAGE_UNSUBSCRIBE), tr("Unsubscribe to Group"), this, SLOT(unsubscribeToGroup()));
	action->setEnabled (!mGroupId.empty() && IS_GROUP_SUBSCRIBED(subscribeFlags));

	/************** NOT ENABLED YET *****************/

	//if (!Settings->getForumOpenAllInNewTab()) {
	//	action = contextMnu.addAction(QIcon(""), tr("Open in new tab"), this, SLOT(openInNewTab()));
	//	if (mForumId.empty() || forumThreadWidget(mForumId)) {
	//		action->setEnabled(false);
	//	}
	//}

	//contextMnu.addSeparator();

	//contextMnu.addAction(QIcon(IMAGE_NEWFORUM), tr("New Forum"), this, SLOT(newforum()));

	//action = contextMnu.addAction(QIcon(IMAGE_INFO), tr("Show Forum Details"), this, SLOT(showForumDetails()));
	//action->setEnabled (!mForumId.empty ());

	//action = contextMnu.addAction(QIcon(":/images/settings16.png"), tr("Edit Forum Details"), this, SLOT(editForumDetails()));
	//action->setEnabled (!mForumId.empty () && IS_GROUP_ADMIN(subscribeFlags));

	//QAction *shareKeyAct = new QAction(QIcon(":/images/gpgp_key_generate.png"), tr("Share Forum"), &contextMnu);
	//connect( shareKeyAct, SIGNAL( triggered() ), this, SLOT( shareKey() ) );
	//shareKeyAct->setEnabled(!mForumId.empty() && IS_GROUP_ADMIN(subscribeFlags));
	//contextMnu.addAction( shareKeyAct);

	//QAction *restoreKeysAct = new QAction(QIcon(":/images/settings16.png"), tr("Restore Publish Rights for Forum" ), &contextMnu);
	//connect( restoreKeysAct , SIGNAL( triggered() ), this, SLOT( restoreForumKeys() ) );
	//restoreKeysAct->setEnabled(!mForumId.empty() && !IS_GROUP_ADMIN(subscribeFlags));
	//contextMnu.addAction( restoreKeysAct);

	//action = contextMnu.addAction(QIcon(IMAGE_COPYLINK), tr("Copy RetroShare Link"), this, SLOT(copyForumLink()));
	//action->setEnabled(!mForumId.empty());

	//contextMnu.addSeparator();

	contextMnu.exec(QCursor::pos());
}





void WikiDialog::insertGroupsData(const std::list<RsGroupMetaData> &wikiList)
{
	std::list<RsGroupMetaData>::const_iterator it;

	QList<GroupItemInfo> adminList;
	QList<GroupItemInfo> subList;
	QList<GroupItemInfo> popList;
	QList<GroupItemInfo> otherList;
	std::multimap<uint32_t, GroupItemInfo> popMap;

	for (it = wikiList.begin(); it != wikiList.end(); it++) {
		/* sort it into Publish (Own), Subscribed, Popular and Other */
		uint32_t flags = it->mSubscribeFlags;

		GroupItemInfo groupItemInfo;
		GroupMetaDataToGroupItemInfo(*it, groupItemInfo);

		if (IS_GROUP_ADMIN(flags)) {
			adminList.push_back(groupItemInfo);
		} else if (IS_GROUP_SUBSCRIBED(flags)) {
			/* subscribed forum */
			subList.push_back(groupItemInfo);
		} else {
			/* rate the others by popularity */
			popMap.insert(std::make_pair(it->mPop, groupItemInfo));
		}
	}

	/* iterate backwards through popMap - take the top 5 or 10% of list */
	uint32_t popCount = 5;
	if (popCount < popMap.size() / 10)
	{
		popCount = popMap.size() / 10;
	}

	uint32_t i = 0;
	uint32_t popLimit = 0;
	std::multimap<uint32_t, GroupItemInfo>::reverse_iterator rit;
	for(rit = popMap.rbegin(); ((rit != popMap.rend()) && (i < popCount)); rit++, i++) ;
	if (rit != popMap.rend()) {
		popLimit = rit->first;
	}

	for (rit = popMap.rbegin(); rit != popMap.rend(); rit++) {
		if (rit->second.popularity < (int) popLimit) {
			otherList.append(rit->second);
		} else {
			popList.append(rit->second);
		}
	}

	/* now we can add them in as a tree! */
	ui.groupTreeWidget->fillGroupItems(mYourGroups, adminList);
	ui.groupTreeWidget->fillGroupItems(mSubscribedGroups, subList);
	ui.groupTreeWidget->fillGroupItems(mPopularGroups, popList);
	ui.groupTreeWidget->fillGroupItems(mOtherGroups, otherList);

}

void WikiDialog::GroupMetaDataToGroupItemInfo(const RsGroupMetaData &groupInfo, GroupItemInfo &groupItemInfo)
{

	groupItemInfo.id = QString::fromStdString(groupInfo.mGroupId);
	groupItemInfo.name = QString::fromUtf8(groupInfo.mGroupName.c_str());
	//groupItemInfo.description = QString::fromUtf8(groupInfo.forumDesc);
	groupItemInfo.popularity = groupInfo.mPop;
	groupItemInfo.lastpost = QDateTime::fromTime_t(groupInfo.mLastPost);
	groupItemInfo.subscribeFlags = groupInfo.mSubscribeFlags;

	groupItemInfo.icon = QIcon(IMAGE_FORUM);

}

