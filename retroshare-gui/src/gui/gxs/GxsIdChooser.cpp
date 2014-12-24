/*
 * Retroshare Gxs Support
 *
 * Copyright 2012-2013 by Robert Fernie.
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

#include "GxsIdChooser.h"
#include "GxsIdDetails.h"
#include "RsGxsUpdateBroadcastBase.h"
#include "gui/Identity/IdEditDialog.h"

#include <QSortFilterProxyModel>
#include <algorithm>

#include <retroshare/rspeers.h>

#include <iostream>

#define ROLE_SORT Qt::UserRole + 1 // Qt::UserRole is reserved for data
#define ROLE_TYPE Qt::UserRole + 2 //

/* Used for sorting too */
#define TYPE_NO_ID       1
#define TYPE_FOUND_ID    2
#define TYPE_UNKNOWN_ID  3
#define TYPE_CREATE_ID   4

#define IDCHOOSER_REFRESH  1

//#define IDCHOOSER_DEBUG

/** Constructor */
GxsIdChooser::GxsIdChooser(QWidget *parent)
    : QComboBox(parent), mFlags(IDCHOOSER_ANON_DEFAULT)
{
	mBase = new RsGxsUpdateBroadcastBase(rsIdentity, this);
	connect(mBase, SIGNAL(fillDisplay(bool)), this, SLOT(fillDisplay(bool)));

	/* Initialize ui */
	setSizeAdjustPolicy(QComboBox::AdjustToContents);

	mIdQueue = NULL;
	mFirstLoad = true;

	mDefaultId.clear() ;
	mDefaultIdName.clear();

	/* Enable sort with own role */
	QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
	proxy->setSourceModel(model());
	model()->setParent(proxy);
	setModel(proxy);

	proxy->setSortRole(ROLE_SORT);

	/* Connect signals */
	connect(this, SIGNAL(currentIndexChanged(int)), this, SLOT(myCurrentIndexChanged(int)));
	connect(this, SIGNAL(activated(int)), this, SLOT(indexActivated(int)));

	mIdQueue = new TokenQueue(rsIdentity->getTokenService(), this);
}

void GxsIdChooser::setFlags(uint32_t flags)
{
	mFlags = flags ;
	updateDisplay(true);
}

GxsIdChooser::~GxsIdChooser()
{
	if (mIdQueue) {
		delete(mIdQueue);
		mIdQueue = NULL;
	}
}

void GxsIdChooser::fillDisplay(bool complete)
{
	updateDisplay(complete);
	update(); // Qt flush
}

void GxsIdChooser::showEvent(QShowEvent *event)
{
	mBase->showEvent(event);
	QComboBox::showEvent(event);
}

void GxsIdChooser::loadIds(uint32_t chooserFlags, RsGxsId defId)
{
	mFlags = chooserFlags;
	mDefaultId = defId;
	clear();
	mFirstLoad = true;
}

static void loadPrivateIdsCallback(GxsIdDetailsType type, const RsIdentityDetails &details, QObject *object, const QVariant &/*data*/)
{
	GxsIdChooser *chooser = dynamic_cast<GxsIdChooser*>(object);
	if (!chooser) {
		return;
	}

	QString text = GxsIdDetails::getNameForType(type, details);
	QString id = QString::fromStdString(details.mId.toStdString());

	/* Find and replace text of exisiting item */
	int index = chooser->findData(id);
	if (index >= 0) {
		chooser->setItemText(index, text);
	} else {
		/* Add new item */
		chooser->addItem(text, id);
		index = chooser->count() - 1;
	}

	chooser->setItemData(index, QString("%1_%2").arg((type == GXS_ID_DETAILS_TYPE_DONE) ? TYPE_FOUND_ID : TYPE_UNKNOWN_ID).arg(text), ROLE_SORT);
	chooser->setItemData(index, (type == GXS_ID_DETAILS_TYPE_DONE) ? TYPE_FOUND_ID : TYPE_UNKNOWN_ID, ROLE_TYPE);
	chooser->model()->sort(0);
}

void GxsIdChooser::loadPrivateIds(uint32_t token)
{
	if (mFirstLoad) {
		clear();
	}

	std::list<RsGxsId> ids;
	//rsIdentity->getOwnIds(ids);
	std::vector<RsGxsIdGroup> datavector;
	if (!rsIdentity->getGroupData(token, datavector)) {
		std::cerr << "GxsIdChooser::loadPrivateIds() Error getting GroupData";
		std::cerr << std::endl;
		return;
	}

	for (std::vector<RsGxsIdGroup>::iterator vit = datavector.begin(); vit != datavector.end(); ++vit) {
		RsGxsIdGroup data = (*vit);
		if (data.mMeta.mSubscribeFlags & GXS_SERV::GROUP_SUBSCRIBE_ADMIN) {
			ids.push_back((RsGxsId) data.mMeta.mGroupId);
		}

		if (mDefaultIdName == data.mMeta.mGroupName) {
			mDefaultId = (RsGxsId) data.mMeta.mGroupId;
		}
	}

	//rsIdentity->getDefaultId(defId);
	// Prefer to use an application specific default???

	if (mFirstLoad) {
		if (!(mFlags & IDCHOOSER_ID_REQUIRED)) {
			/* add No Signature option */
			QString str = tr("No Signature");
			QString id = "";

			addItem(str, id);
			setItemData(count() - 1, QString("%1_%2").arg(TYPE_NO_ID).arg(str), ROLE_SORT);
			setItemData(count() - 1, TYPE_NO_ID, ROLE_TYPE);
		}
	} else {
		for (int idx = 0; idx < count(); ++idx) {
			QVariant type = itemData(idx, ROLE_TYPE);
			switch (type.toInt()) {
			case TYPE_NO_ID:
			case TYPE_CREATE_ID:
				break;
			case TYPE_FOUND_ID:
			case TYPE_UNKNOWN_ID:
				{
					QVariant var = itemData(idx);
					RsGxsId gxsId = RsGxsId(var.toString().toStdString());
					std::list<RsGxsId>::iterator lit = std::find(ids.begin(), ids.end(), gxsId);
					if (lit == ids.end()) {
						removeItem(idx);
						idx--;
					}
				}
			}
		}
	}

	if (ids.empty()) {
		mFirstLoad = false;
		std::cerr << "GxsIdChooser::loadPrivateIds() ERROR no ids";
		std::cerr << std::endl;
		return;
	}

	for (std::list<RsGxsId>::iterator it = ids.begin(); it != ids.end(); ++it) {
		/* add to Chooser */
		GxsIdDetails::process(*it, loadPrivateIdsCallback, this);
	}

	if (mFirstLoad) {
		if (!(mFlags & IDCHOOSER_NO_CREATE)) {
			/* add Create Identity option */
			QString str = tr("Create new Identity");
			QString id = "";

			addItem(str, id);
			setItemData(count() - 1, QString("%1_%2").arg(TYPE_CREATE_ID).arg(str), ROLE_SORT);
			setItemData(count() - 1, TYPE_CREATE_ID, ROLE_TYPE);
		}
	}

	mFirstLoad = false;

	setDefaultItem();
}

void GxsIdChooser::setDefaultItem()
{
	int def = -1;

	if ((mFlags & IDCHOOSER_ANON_DEFAULT) && !(mFlags & IDCHOOSER_ID_REQUIRED)) {
		def = findData(TYPE_NO_ID, ROLE_TYPE);
	} else {
		if (!mDefaultId.isNull()) {
			QString id = QString::fromStdString(mDefaultId.toStdString());
			def = findData(id);
		}
	}

	if (def >= 0) {
		setCurrentIndex(def);
	}
}

bool GxsIdChooser::setChosenId(RsGxsId &gxsId)
{
	QString id = QString::fromStdString(gxsId.toStdString());

	/* Find text of exisiting item */
	int index = findData(id);
	if (index >= 0) {
		setCurrentIndex(index);
		return true;
	}
	return false;
}

GxsIdChooser::ChosenId_Ret GxsIdChooser::getChosenId(RsGxsId &gxsId)
{
	if (count() < 1) {
		return None;
	}

	int idx = currentIndex();

	QVariant var = itemData(idx);
	gxsId = RsGxsId(var.toString().toStdString());
	QVariant type = itemData(idx, ROLE_TYPE);
	switch (type.toInt()) {
	case TYPE_NO_ID:
		return NoId;
	case TYPE_FOUND_ID:
		return KnowId;
	case TYPE_UNKNOWN_ID:
		return UnKnowId;
	case TYPE_CREATE_ID:
		break;
	}

	return None;
}

void GxsIdChooser::myCurrentIndexChanged(int index)
{
	Q_UNUSED(index);

	QFontMetrics fm = QFontMetrics(font());
	QString text = currentText();
	if (width() < fm.boundingRect(text).width()) {
		setToolTip(text);
	} else {
		setToolTip("");
	}
}

void GxsIdChooser::indexActivated(int index)
{
	int type = itemData(index, ROLE_TYPE).toInt();
	if (type == TYPE_CREATE_ID) {
		IdEditDialog dlg(this);
		dlg.setupNewId(false);
		dlg.exec();
		setDefaultId(dlg.getLastIdName());
	}
}

void GxsIdChooser::updateDisplay(bool complete)
{
	Q_UNUSED(complete)

	/* Update identity list */
	requestIdList();
}

void GxsIdChooser::requestIdList()
{
	if (!mIdQueue) {
		return;
	}

	mIdQueue->cancelActiveRequestTokens(IDCHOOSER_REFRESH);

	RsTokReqOptions opts;
	opts.mReqType = GXS_REQUEST_TYPE_GROUP_DATA;

	uint32_t token;

	mIdQueue->requestGroupInfo(token, RS_TOKREQ_ANSTYPE_DATA, opts, IDCHOOSER_REFRESH);
}

void GxsIdChooser::loadRequest(const TokenQueue *queue, const TokenRequest &req)
{
	Q_UNUSED(queue)

#ifdef IDCHOOSER_DEBUG
	std::cerr << "IdDialog::loadRequest() UserType: " << req.mUserType;
	std::cerr << std::endl;
#endif

	switch(req.mUserType) {
	case IDCHOOSER_REFRESH:
		loadPrivateIds(req.mToken);
		break;
	default:
		std::cerr << "IdDialog::loadRequest() ERROR";
		std::cerr << std::endl;
		break;
	}
}
