/*
 *              KSCD -- a simpole cd player for the KDE project
 *
 * $Id$
 *
 *              Copyright (C) 1997 Bernd Johannes Wuebben
 *                      wuebben@math.cornell.edu
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "kmidi.h"

#include <qtooltip.h>
#include <kwm.h>
#include <kapp.h>

#include "docking.h"
#include <klocale.h>
#include <kiconloader.h>
#include <kglobal.h>


extern KMidi *kmidi;
extern KMidiFrame *kmidiframe;

extern bool dockinginprogress;

DockWidget::DockWidget(const char *name)
  : QWidget(0, name, 0) {

  docked = false;

  QString tmp;

  //     printf("trying to load %s\n",pixdir.data());
  // load pixmaps

  kmidi_pixmap = BarIcon("mini-kmidi");

  // popup menu for right mouse button
  popup_m = new QPopupMenu();

  popup_m->insertItem(i18n("Play/Pause"), this, SLOT(play_pause()));
  popup_m->insertItem(i18n("Stop"), this, SLOT(stop()));
  popup_m->insertItem(i18n("Forward"), this, SLOT(forward()));
  popup_m->insertItem(i18n("Backward"), this, SLOT(backward()));
  popup_m->insertItem(i18n("Next"), this, SLOT(next()));
  popup_m->insertItem(i18n("Previous"), this, SLOT(prev()));
  popup_m->insertItem(i18n("Eject"), this, SLOT(eject()));
  popup_m->insertSeparator();
  toggleID = popup_m->insertItem(i18n("Restore"),
				 this, SLOT(toggle_window_state()));
  popup_m->insertItem(i18n("Quit"), this, SLOT(kquit()));



  //  QToolTip::add( this, statstring.data() );

}

DockWidget::~DockWidget() {
}

void DockWidget::dock() {

  if (!docked) {


    // prepare panel to accept this widget
    KWM::setDockWindow (this->winId());

    // that's all the space there is
    this->setFixedSize(24, 24);

    // finally dock the widget
    this->show();
    docked = true;
  }
}

void DockWidget::undock()
{
    if(docked){
        // the widget's window has to be destroyed in order
        // to undock from the panel. Simply using hide() is
        // not enough.
        this->destroy(true, true);

        // recreate window for further dockings
        this->create(0, false, false);

        docked = false;
    }
    /*
     allthough this un-docks the applet from the panel, it seems to stop
     the applet from being able to re-dock with a call to this->dock(),
     which docks it fine when newly created
                    -- thuf
     */
}

const bool DockWidget::isDocked() {

  return docked;

}

void DockWidget::paintEvent (QPaintEvent *e) {

  (void) e;

  paintIcon();

}

void DockWidget::paintIcon () {

  bitBlt(this, 0, 0, &kmidi_pixmap);


}

void DockWidget::timeclick() {

  if(this->isVisible()){
    paintIcon();
  }
}


void DockWidget::mousePressEvent(QMouseEvent *e) {

  // open popup menu on left mouse button
  if ( e->button() == LeftButton ) {
    toggle_window_state();
  }

  // open/close connect-window on right mouse button
  if ( e->button() == RightButton  || e->button() == MidButton) {
    int x = e->x() + this->x();
    int y = e->y() + this->y();

    QString text;
    if(kmidiframe->isVisible())
      text = i18n("Minimize");
    else
      text = i18n("Restore");

    popup_m->changeItem(text, toggleID);
    popup_m->popup(QPoint(x, y));
    popup_m->exec();
  }

}

void DockWidget::toggle_window_state() {
  // restore/hide connect-window
    if(kmidiframe != 0L)  {
        if (kmidiframe->isVisible()){
            dockinginprogress = true;
            toggled = true;
            kmidiframe->hide();
            kmidiframe->recreate(0, 0, QPoint(kmidiframe->x(), kmidiframe->y()), FALSE);
            kapp->setTopWidget( kmidiframe );

        }
        else {
            toggled = false;
            kmidiframe->show();
            dockinginprogress = false;
            KWM::activate(kmidiframe->winId());
	    kmidi->check_meter_visible();
        }
    }
}

void DockWidget::setToggled(int totoggle)
{
    toggled = totoggle;
}

const bool DockWidget::isToggled()
{
    return(toggled);
}

void DockWidget::eject() {

  kmidi->ejectClicked();

}

void DockWidget::next() {

  kmidi->nextClicked();

}


void DockWidget::stop() {

  kmidi->stopClicked();

}

void DockWidget::prev() {

  kmidi->prevClicked();

}

void DockWidget::play_pause() {

  kmidi->playClicked();
}


void DockWidget::forward() {

  kmidi->fwdClicked();

}


void DockWidget::backward() {

  kmidi->bwdClicked();

}

void DockWidget::kquit() {

  kmidi->quitClicked();

}


#include "docking.moc"

