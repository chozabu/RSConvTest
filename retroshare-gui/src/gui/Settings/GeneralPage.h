/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (c) 2006-2007, crypton
 * Copyright (c) 2006, Matt Edman, Justin Hipple
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

#ifndef GENERALPAGE_H
# define GENERALPAGE_H

#include <QStyleFactory>
#include <QLineEdit>

#include "gui/Preferences/rsharesettings.h"
#include "lang/languagesupport.h"

# include <QtGui/QWidget>
# include "ui_GeneralPage.h"

class GeneralPage: public QWidget
{
  Q_OBJECT

    public:
        GeneralPage(QWidget * parent = 0, Qt::WFlags flags = 0);
        ~GeneralPage() {}
        
    /** Saves the changes on this page */
    bool save(QString &errmsg);
    /** Loads the settings for this page */
    void load();
  bool startMinimized() const;

private slots:

  /** Called when the "show on startup" checkbox is toggled. */
  void toggleShowOnStartup(bool checked);

    private:
       /** A VidaliaSettings object used for saving/loading settings */
       RshareSettings* _settings;
    
    
       void closeEvent (QCloseEvent * event);
        
       Ui::GeneralPage ui;
};

#endif // !GENERALPAGE_H

