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


#include "HelpDialog.h"
#include "rsiface/rsiface.h"
#include "rsiface/rsdisc.h"

#include <iostream>
#include <sstream>

#include <QtCore/QFile>
#include <QtCore/QTextStream>

#include <QContextMenuEvent>
#include <QMenu>
#include <QCursor>
#include <QPoint>
#include <QMouseEvent>
#include <QPixmap>

/* Images for context menu icons */
#define IMAGE_DOWNLOAD       ":/images/start.png"

/** Constructor */
HelpDialog::HelpDialog(QWidget *parent)
:QDialog(parent)
{
  /* Invoke the Qt Designer generated object setup routine */
  ui.setupUi(this);
  
  //QFile licenseFile(QLatin1String(":/images/COPYING"));
  QFile licenseFile(QLatin1String(":/help/licence.html"));
   if (licenseFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&licenseFile);
        ui.license->setText(in.readAll());
   }
  QFile authorsFile(QLatin1String(":/help/authors.html"));
   if (authorsFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&authorsFile);
        ui.authors->setText(in.readAll());
   }
  QFile thanksFile(QLatin1String(":/help/thanks.html"));
   if (thanksFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&thanksFile);
	ui.thanks->setText(in.readAll());
   }

  QFile versionFile(QLatin1String(":/help/version.html"));
   if (versionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
	QTextStream in(&versionFile);
	QString version = in.readAll();

	/* get libretroshare version */
	std::map<std::string, std::string>::iterator vit;
	std::map<std::string, std::string> versions;
	const RsConfig &conf = rsiface->getConfig();
	bool retv = rsDisc->getDiscVersions(versions);
	std::cerr << "Version display own id " << conf.ownId << std::endl;
	std::cerr << "Versions map size : " << versions.size() << std::endl;

	for(vit = versions.begin(); vit != versions.end(); vit++)
	{
		 std::cerr << "vit first " << vit->first << " vit second " << vit->second << std::endl;
	}
	if (retv && versions.end() != (vit = versions.find(conf.ownId)))
	{
	    std::cerr << "Version displaying " << std::endl;
	    version += QString(" ") + QString::fromStdString(vit->second);
	}

	ui.version->setText(version);
   }

   ui.label_2->setMinimumWidth(20);


  /* Hide platform specific features */
#ifdef Q_WS_WIN

#endif
}


