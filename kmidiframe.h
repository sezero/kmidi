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

#ifndef KMIDIFRAME_H
#define KMIDIFRAME_H

#include "version.h"

//#include <qfileinfo.h>
//#include <qdatastream.h> 
//#include <qfile.h> 
//#include <qtabdialog.h> 
//#include <qfiledialog.h>
//#include <qpushbutton.h>
//#include <qradiobutton.h>
//#include <qlabel.h>
//#include <qdialog.h>
//#include <qapplication.h>
//#include <qpopupmenu.h>
//#include <qtimer.h>
//#include <qbitmap.h>
//#include <qslider.h>
//#include <qgroupbox.h>
//#include <qcombobox.h>
//#include <qscrollbar.h>
//#include <qspinbox.h>
//#include <qbuttongroup.h>
//#include <qcheckbox.h>
//#include <qtooltip.h>
//#include <qregexp.h>
//#include <qgroupbox.h>
//#include <qwidget.h>
//#include <qpainter.h>
//#include <qdragobject.h>
//#include <qwhatsthis.h>

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

//#include <kapp.h>
#include <ktmainwindow.h>
//#include <kled.h>
#include <kmenubar.h>


#include "docking.h"


class KMidiFrame : public KTMainWindow {

	Q_OBJECT
public:

	KMidiFrame( const char *name = 0 );

	~KMidiFrame();

	bool		docking;
	KMenuBar	*menuBar;

protected:
	bool		queryClose();  

protected slots:
	void		file_Open();
	void		quitClick();
	void		doViewMenuItem(int);
	void		fixViewItems();
	void		doViewInfoLevel(int);
	void		fixInfoLevelItems();
	void		doStereoMenuItem(int);
	void		doChorusMenuItem(int);
	void		doReverbMenuItem(int);
	void		fixStereoItems();
	void		fixChorusItems();
	void		fixReverbItems();
	void		doEchoLevel(int);
	void		doReverbLevel(int);
	void		doChorusLevel(int);
	void		doDetuneLevel(int);
	void		fixReverbLevelItems();
	void		fixChorusLevelItems();
	void		fixEchoLevelItems();
	void		fixDetuneLevelItems();

private:
	int		m_on_id, m_off_id, i_on_id, i_off_id;
	QPopupMenu	*view_options;
	QPopupMenu	*view_level;
	QPopupMenu	*stereo_options;
	QPopupMenu	*chorus_options;
	QPopupMenu	*chorus_level;
	QPopupMenu	*detune_level;
	QPopupMenu	*reverb_options;
	QPopupMenu	*echo_level;
	QPopupMenu	*reverb_level;

};


#endif 
