/*   
   kmidi - a midi to wav converter
   
   Copyright 1997 Bernd Johannes Wuebben math.cornell.edu
  
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

 
 */

#ifndef MIDIAPPLICATION_H
#define MIDIAPPLICATION_H

#include "version.h"
#include "bwlednum.h"

#include <qfileinfo.h>
#include <qdatastream.h> 
#include <qfile.h> 
#include <qtabdialog.h> 
#include <qfiledialog.h>
#include <qpushbutton.h>
#include <qradiobutton.h>
#include <qlabel.h>
#include <qdialog.h>
#include <qapplication.h>
#include <qpopupmenu.h>
#include <qtimer.h>
#include <qbitmap.h>
#include <qslider.h>
#include <qgroupbox.h>
#include <qcombobox.h>
#include <qscrollbar.h>
#include <qspinbox.h>
#include <qbuttongroup.h>
#include <qcheckbox.h>
#include <qtooltip.h>
#include <qregexp.h>
#include <qgroupbox.h>
#include <qwidget.h>
#include <qpainter.h>
#include <qdragobject.h>
#include <qwhatsthis.h>

#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/utsname.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <kapp.h>
#include <kuniqueapp.h>

class MidiApplication : public KUniqueApplication
{
  Q_OBJECT 

public:
  
  MidiApplication();
  ~MidiApplication();

  virtual int newInstance();

  bool process(const QCString &fun, const QByteArray &data,
		        QCString &replyType, QByteArray &replyData);

};

extern MidiApplication *thisapp;

#endif
