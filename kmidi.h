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

#ifndef KMIDI_PLAYER_H
#define KMIDI_PLAYER_H

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
#include <ktmainwindow.h>
#include <kled.h>

#define PLAYLIST_WIDTH  550
#define PLAYLIST_HEIGHT 440



#include "configdlg.h"
#include "log.h"
#include "docking.h"

class PlaylistEdit;
class ConfigDlg;
//
// MeterWidget - draws meter
//

class MeterWidget : public QWidget {

	Q_OBJECT
public:
    MeterWidget( QWidget *parent=0, const char *name=0 );
   ~MeterWidget();
    QTimer     *metertimer;
    QColor	led_color;
    QColor	background_color;
public slots:
    void	remeter();
protected:
    void	paintEvent( QPaintEvent * );
private:

};


class KMidiFrame : public KTMainWindow {

	Q_OBJECT
public:

	KMidiFrame( const char *name = 0 );

	~KMidiFrame();

	bool		docking;
	bool		autodock;


protected:
	void		closeEvent( QCloseEvent *e );  
//	bool		event( QEvent *e );

};


class KMidi : public QWidget {

	Q_OBJECT
public:

	~KMidi();
	

	QPushButton	*playPB;
	QPushButton	*stopPB;
	QPushButton	*prevPB;
	QPushButton	*nextPB;
	QPushButton	*fwdPB;
	QPushButton	*bwdPB;
	QPushButton	*quitPB;
	QPushButton	*whatbutton;
	QPushButton	*replayPB;
	QPushButton	*ejectPB;
	QPushButton	*aboutPB;
	QPushButton 	*shufflebutton;
	QPushButton 	*configurebutton;
	QPushButton 	*lowerBar;
	QTabDialog      *tabdialog;
	QPushButton 	*infobutton;
	BW_LED_Number	*trackTimeLED[9];

	KLed		*led[18];

	QComboBox	*patchbox;
	QComboBox	*playbox;
	QComboBox	*playlistbox;
	QButtonGroup	*ichecks;
	QCheckBox	*ich[4];
	QButtonGroup	*rchecks;
	QCheckBox	*rcb1;
	QCheckBox	*rcb2;
	QCheckBox	*rcb3;
	QCheckBox	*rcb4;
	QPushButton 	*effectbutton;
	QSpinBox 	*meterspin;
	QSpinBox 	*voicespin;
	QPushButton 	*filterbutton;
	MeterWidget	*meter;
	QSize		regularsize;
	QSize		extendedsize;
    
	QPixmap folder_pixmap;
	QPixmap file_pixmap;
	QPixmap cdup_pixmap;

	QTimer*  readtimer;
	

	QLabel	*statusLA;
	QLabel	*volLA;
	QLabel  *titleLA;
	QLabel  *propertiesLA;
	QLabel  *properties2LA;
	QLabel  *modlabel;
	QLabel  *totaltimelabel;
	QLabel  *looplabel;
	QLabel  *speedLA;
	QLabel  *song_count_label;

	QTimer		*timer;

	QComboBox	*songListCB;

	QSlider		*volSB;
	QWidget 	*backdrop;
	QPushButton     *makeButton( int, int, int, int, const QString & );

	int		mixerFd;
        bool            StopRequested;
	bool 		loop;
	int 		init_volume;
	int 		Child;
	bool 		driver_error;
	bool 		tooltips;
	bool 		starting_up;
	bool 		looping;
	int		max_patch_megs;
	int		volume;
	int		current_voices;
	int 		max_sec;
	int 		song_number;
	int 		last_sec;

	bool		randomplay;
	QColor		background_color;
	QColor		led_color;
	bool 		playing;
	bool 		blink;
	bool		flag_new_playlist;

	bool		showmeterrequest;
	bool		showinforequest;
	bool		lpfilterrequest;
	bool		effectsrequest;
	int		infowindowheight;
	int		interpolationrequest;

	int		stereo_state;
	int		reverb_state;
	int		chorus_state;
	int		verbosity_state;

	QString		fileName;
	QStrList	*playlist;
	int		current_playlist_num;
	QStrList	*listplaylists;

	LogWindow 	*logwindow;
	ConfigDlg       *configdlg;
	PlaylistEdit    *playlistdlg;
	KConfig 	*config;

private:
	void 		display_playmode();
	int		randomSong();
	void 		resetPos();
	void 		setLEDs(const QString &);
	void		drawPanel();
	void		cleanUp();
	void		loadBitmaps();
	void		initMixer( const char *mixer = "/dev/mixer" );
	void 	        playthemod(QString );
	void 		redoplaybox();
	void 		redoplaylistbox();

protected:
        void		resizeEvent(QResizeEvent *e);
	QWhatsThis	*what;

public:
	KMidi( QWidget *parent=0, const char *name=0 );


signals:	
	void		play();

public slots:
	void 		loadplaylist(int);
	void 		randomPlay();
	void 		updateUI();
        void 		randomClicked();
	void 		ReadPipe();
	void 		setToolTips();
	void 		PlayCommandlineMods();
	void            PlayMOD();
	void 		readconfig();
        void 		writeconfig();
	void 		setColors();
	void		playClicked();
	void		stopClicked();
	void 		infoslot();
	void		prevClicked();
	void		nextClicked();
	void		fwdClicked();
	void		bwdClicked();
	void		quitClicked();
	void		replayClicked();
	void		ejectClicked();
	void		aboutClicked();
	void		logoClicked();
	void		check_meter_visible();
	void		volChanged( int );
	void		setPatch( int );
	void		setEffects( bool );
	void		setFilter( bool );
	void		setSong( int );
	void		resetSong();
	void		voicesChanged( int );
	void		meterfudgeChanged( int );
	void		updateRChecks( int );
	void		updateIChecks( int );
	void		dropEvent( QDropEvent * );
	void		dragEnterEvent( QDragEnterEvent *e );
	void		plActivated( int );
	void		acceptPlaylist();
	void		invokeWhatsThis();
};



#endif 
