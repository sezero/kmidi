/*
 *
 * $Id$
 *
 * kscd -- A simple CD player for the KDE project           
 *
 * 
 * Copyright (C) 1997 Bernd Johannes Wuebben 
 * wuebben@math.cornell.edu
 *
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 */


#ifndef _CONFIG_DLG_H_
#define _CONFIG_DLG_H_

#include <qwidget.h>
#include <kaboutdialog.h>

#include <qgroupbox.h> 
#include <qdialog.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qpainter.h>
#include <qlabel.h>
#include <qframe.h>
#include <qcheckbox.h>
#include <qlayout.h>
#include <kbuttonbox.h>
#include <knuminput.h>
#include <kcolorbtn.h>
//#include <khelpmenu.h>
#include <qwhatsthis.h>

struct configstruct{
  QColor led_color;
  QColor background_color;
  bool   tooltips;
  int	 max_patch_megs;
};

class KTabCtl;
class QPushButton;
class KAboutWidget;
class KIntNumInput;

class ConfigDlg : public QDialog
{
    Q_OBJECT
public:

    ConfigDlg( QWidget *parent=0, struct configstruct * data=0, const char *name=0 );
   ~ConfigDlg() {}

    struct configstruct * getData();

//    KHelpMenu *helpmenu;

protected:
    void resizeEvent(QResizeEvent *);
    KTabCtl *test;
    KAboutWidget *about;
    QWidget *pages[3];
    KIntNumInput *meg;
    QWhatsThis *what;

public slots:
    void tabChanged(int);

protected slots:
  void cancelbutton();
  void okbutton();
  void set_led_color( const QColor & );
  void set_background_color( const QColor & );
  void help();
  void ttclicked();
  void megChanged( int );

public:
  QSize configsize, aboutsize;

private:

  struct configstruct configdata;
  QGroupBox *box;

  KButtonBox *bbox;
  QVBoxLayout *tl;
  QPushButton *ok;
  QPushButton *cancel;
  QPushButton *helpbutton;

  QLabel *label1;
  KColorButton *button1;

  QLabel *label2;
  KColorButton *button2;

  QLabel *tooltipslabel;
  QCheckBox *ttcheckbox;
 
};

#endif
