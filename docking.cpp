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
#include "kmidiframe.h"

#include <qtooltip.h>
#include <kapplication.h>

#include "docking.h"
#include <klocale.h>
#include <kiconloader.h>
#include <kglobal.h>
#include <kpopupmenu.h>

DockWidget::DockWidget( KMidiFrame* parent, const char *name)
  : KDockWindow( parent, name )
{

  setPixmap( BarIcon("kmidi") );

  // popup menu for right mouse button
  QPopupMenu* popup_m = contextMenu();

  popup_m->insertItem(i18n("Play/Pause"), kmidi, SLOT(playClicked()));
  popup_m->insertItem(i18n("Stop"), kmidi, SLOT(stopClicked()));
  //popup_m->insertItem(i18n("Forward"), kmidi, SLOT(fwdClicked()));
  //popup_m->insertItem(i18n("Backward"), kmidi, SLOT(bwdClicked()));
  popup_m->insertItem(i18n("Next"), kmidi, SLOT(nextClicked()));
  popup_m->insertItem(i18n("Previous"), kmidi, SLOT(prevClicked()));
  popup_m->insertItem(i18n("Eject"), kmidi, SLOT(ejectClicked()));
//  popup_m->insertSeparator();
//  popup_m->insertItem(i18n("Quit"), kmidi, SLOT(quitClicked()));


}

DockWidget::~DockWidget() {
}


#include "docking.moc"


