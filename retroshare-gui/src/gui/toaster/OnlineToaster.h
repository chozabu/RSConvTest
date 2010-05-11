/*
 * RetroShare
 * Copyright (C) 2006 crypton
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef ONLINETOASTER_H
#define ONLINETOASTER_H

#include "IQtToaster.h"

#include <QtCore/QObject>
#include <QSound>

class QtToaster;

class QWidget;
class QString;
class QPixmap;
namespace Ui { class OnlineToaster; }

/**
 * Shows a toaster when friend is Online .
 *
 *
 */
class OnlineToaster : public QObject, public IQtToaster {
	Q_OBJECT
public:

	OnlineToaster();

	~OnlineToaster();

	void setMessage(const QString & message);

	void setPixmap(const QPixmap & pixmap);

	void show();
        void play();

Q_SIGNALS:

	void chatButtonClicked();

private Q_SLOTS:

	void chatButtonSlot();

	void close();

private:
	Ui::OnlineToaster * _ui;

	QWidget * _onlineToasterWidget;

	QtToaster * _toaster;
};

#endif	//MESSAGETOASTER_H
