
/*
 *
 *    kmidi
 *
 * $Id$
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

#include <kapp.h>
#include <qpushbutton.h>
#include <qlabel.h>
#include <qobject.h>
#include <qlistbox.h>
#include <qgroupbox.h>
#include <qevent.h>
#include <qcombobox.h>
#include <qlineedit.h>
#include <qradiobutton.h>
#include <qcheckbox.h>
#include <qtabdialog.h>
#include <qtooltip.h>
#include <qmessagebox.h>
#include <qtabbar.h>
#include <qpalette.h>
#include <qmultilinedit.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <klocale.h>
#include <ktabctl.h>
#include <kiconloader.h>
#include <kstddirs.h>
#include <kglobal.h>
//#include <kcolorbtn.h>

#include "configdlg.h"
#include "kmidi.h"
#include "version.h"

//QFont default_font("Helvetica", 12);

extern KApplication *thisapp;
extern KMidi *kmidi;

ConfigDlg::ConfigDlg(QWidget *parent, struct configstruct *data, const char *name)
    : QDialog(parent, name, TRUE)
{
    setCaption("kmidi configuration");
    setMinimumSize(300, 200);

  configdata.background_color = black;
  configdata.led_color = green;
  configdata.tooltips = true;
  configdata.max_patch_megs = 60;
  
  if(data){
    configdata.background_color = data->background_color;
    configdata.led_color = data->led_color;
    configdata.tooltips = data->tooltips;
    configdata.max_patch_megs = data->max_patch_megs;
  }


    /*
     * add a tabctrl widget
     */
    
    test = new KTabCtl(this, "test");
    connect(test, SIGNAL(tabSelected(int)), this, SLOT(tabChanged(int)));


    tl = new QVBoxLayout(this, 5);
    bbox = new KButtonBox(this);
    ok = bbox->addButton("OK");
    ok->setDefault(TRUE);
    cancel = bbox->addButton("Cancel");
    cancel->setDefault(TRUE);

    connect(ok, SIGNAL(clicked()), this, SLOT(accept()));
    connect(cancel, SIGNAL(clicked()), this, SLOT(reject()));
    bbox->addStretch(1);
    helpbutton = bbox->addButton("Help");
    connect(helpbutton, SIGNAL(clicked()), this, SLOT(help()));

    //helpmenu = new KHelpMenu(this, "kmidi plays midi files");
    what = new QWhatsThis(this);

// Page 1
    QWidget *w = new QWidget(test, "_page1");

    //box = new QGroupBox(w, "box");
    //box->setGeometry(10,10,320,260);
    configsize = QSize( 320+20, 260+80 );

    QString str = i18n("The led color is used in<br>the panel and meter.");
    label1 = new QLabel(w);
    label1->setGeometry(60,25,135,25);
    label1->setText(i18n("LED Color:"));
    what->add(label1, str);

    button1 = new KColorButton(configdata.led_color, w);
    button1->setGeometry(205,25,100,45);
    connect(button1,SIGNAL(changed( const QColor & )),this,SLOT(set_led_color( const QColor & )));
    what->add(button1, str);

    str = i18n("This color is for<br>the panel and meter backgrounds.");
    label2 = new QLabel(w);
    label2->setGeometry(60,85,135,25);
    label2->setText(i18n("Background Color:"));
    what->add(label2, str);

    button2 = new KColorButton(configdata.background_color, w);
    button2->setGeometry(205,85,100,45);
    connect(button2,SIGNAL(changed( const QColor & )),this,SLOT(set_background_color( const QColor & )));
    what->add(button2, str);

    ttcheckbox = new QCheckBox(i18n("Show Tool Tips"), w, "tooltipscheckbox");
    ttcheckbox->setGeometry(30,150,135,25);
    ttcheckbox->setFixedSize( ttcheckbox->sizeHint() );
    ttcheckbox->setChecked(configdata.tooltips);
    connect(ttcheckbox,SIGNAL(clicked()),this,SLOT(ttclicked()));
    what->add(ttcheckbox, i18n("Provide brief descriptions<br>" \
			"when cursor is left over a<br>" \
			"object on the screen"));

    meg = new KIntNumInput("Limit megabytes of patch memory",
                          0, 255, 1, configdata.max_patch_megs, "megs", 10, true, w, "hex_with_slider");
    meg->setSpecialValueText("no limit");
    meg->move(30, 200);
    connect(meg,SIGNAL(valueChanged(int)),this,SLOT(megChanged(int)));
    what->add(meg, i18n("Try to stop KMidi from<br>using up too much of your ram."));

    w->resize(320, 260);
    test->addTab(w, "Configure");
    pages[0] = w;

// Page 2
    w = new QWidget(test, "_page2");
    about = new KAboutWidget(w, "_about");
    QPixmap pm = BarIcon("kmidilogo");
    about->setLogo(pm);
    about->setCaption(i18n("About KMidi"));
    about->setVersion(i18n(KMIDIVERSION));
    about->setAuthor
    ("Bernd Johannes Wuebben", "wuebben@kde.org", "", i18n("Initial developer."));
    // ----- set the application maintainer:
    about->setMaintainer("Greg Lee", // name
		      "lee@hawaii.edu", // email address 
		      "", // URL
		      i18n("lyrics, IW patches, current maintainer.")); // description
    // ----- add some contributors:
    about->addContributor("Tuukka Toivonen", 
		       "toivonen@clinet.fi", 
		       "http://www.cgs.fi/~tt/discontinued.html", 
		       i18n("TiMidity sound, patch loader, interface design"));
    about->addContributor("Takashi Iwai", "iwai@dragon.mm.t.u-tokyo.ac.jp",
			"http://bahamut.mm.t.u-tokyo.ac.jp/~iwai/midi.html", 
		       i18n("soundfonts, window communication"));
    about->addContributor("Nicolas Witczak", "witczak@geocities.fr",
			"http://www.geocities.com/SiliconValley/Lab/6307/", 
		       i18n("effects: reverb, chorus, phaser, celeste"));
    about->addContributor("Masanao Izumo", "mo@goice.co.jp",
			"http://www.goice.co.jp/member/mo/hack-progs/timidity.html", 
		       i18n("mod wheel, portamento"));

//    connect(about, SIGNAL( sendEmail(const QString& , const QString& ) ),
//	this, SLOT ( sendEmailSlot(const QString& , const QString& ) ) );
//    connect(about, SIGNAL( openURL(const QString& ) ),
//	this, SLOT ( openURLSlot(const QString& ) ) );

    // ----- contents of the dialog have changed, adapt sizes:
    about->adjust(); 
    aboutsize = QSize( about->width() + 30, about->height() + 80 );
    w->resize(width(), height());
    test->addTab(w, "About");
    pages[1] = w;


    //w = new QWidget(test, "_page3");
    //test->addTab(w, "Seite 3");
    //pages[2] = w;

    test->resize(configsize);
    test->move(0, 0);
    tl->addWidget(test,1);
    bbox->layout();
    tl->addWidget(bbox,0);
    tl->activate();

    move(20, 20);
    resize(400, 500);
    adjustSize();
}

void ConfigDlg::resizeEvent( QResizeEvent * )
{
    test->resize(width(), height());
    about->setGeometry(10, 10, pages[1]->width() - 20, pages[1]->height() - 20);
}

void ConfigDlg::tabChanged(int newpage)
{
    //printf("tab number %d selected\n", newpage);
    //if(newpage == 1)
    //    e->setFocus();
    if (newpage == 1) resize(aboutsize);
    else resize(configsize);
}


void ConfigDlg::okbutton() {
//    hide();
}

void ConfigDlg::ttclicked(){

  if(ttcheckbox->isChecked())
    configdata.tooltips = TRUE;
  else
    configdata.tooltips = FALSE;

}
void ConfigDlg::help(){

//  if(thisapp)
//    thisapp->invokeHTMLHelp("","");

//the damn thing won't go away
//   (helpmenu->menu())->move( QCursor::pos() );
//   (helpmenu->menu())->show();
//    helpmenu->aboutApp();
    QWhatsThis::enterWhatsThisMode();
}

void ConfigDlg::cancelbutton() {
//  reject();
}

void ConfigDlg::set_led_color( const QColor &newColor ) {

  //KColorDialog::getColor(configdata.led_color);
  configdata.led_color = newColor;
  //qframe1->setBackgroundColor(configdata.led_color);


}

void ConfigDlg::set_background_color( const QColor &newColor ) {

  //KColorDialog::getColor(configdata.background_color);
  configdata.background_color = newColor;
  //qframe2->setBackgroundColor(configdata.background_color);

}

void ConfigDlg::megChanged( int newMegs ) {
  configdata.max_patch_megs = newMegs;
}

struct configstruct * ConfigDlg::getData(){

  return &configdata;

}

#include "configdlg.moc"

