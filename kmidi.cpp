/*

   kmidi - a midi to wav converter

   $Id$

   Copyright 1997, 1998 Bernd Johannes Wuebben math.cornell.edu

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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>

#include "kmidi.h"

#include "bitmaps/playpaus.xbm"
#include "bitmaps/stop.xbm"
#include "bitmaps/repeat.xbm"
#include "bitmaps/nexttrk.xbm"
#include "bitmaps/prevtrk.xbm"
#include "bitmaps/ff.xbm"
#include "bitmaps/rew.xbm"
#include "bitmaps/poweroff.xbm"
#include "bitmaps/eject.xbm"
#include "bitmaps/midilogo.xbm"
#include "bitmaps/shuffle.xbm"
#include "bitmaps/options.xbm"
#include "bitmaps/info.xbm"

#include "filepic.h"
#include "cduppic.h"
#include "folderpic.h"


static const char * whatsthis_image[] = {
"16 16 3 1",
" 	c None",
"o	c #000000",
"a	c #000080",
"o        aaaaa  ",
"oo      aaa aaa ",
"ooo    aaa   aaa",
"oooo   aa     aa",
"ooooo  aa     aa",
"oooooo  a    aaa",
"ooooooo     aaa ",
"oooooooo   aaa  ",
"ooooooooo aaa   ",
"ooooo     aaa   ",
"oo ooo          ",
"o  ooo    aaa   ",
"    ooo   aaa   ",
"    ooo         ",
"     ooo        ",
"     ooo        "};

#include "config.h"
#include "playlist.h"
#include "output.h"
#include "instrum.h"
#include "playmidi.h"
#include "constants.h"
#include "ctl.h"
#include <kstddirs.h>
#include <kglobal.h>
#include <kwm.h>


extern "C" {

int pipe_read_ready();
void pipe_int_write(int c);
void pipe_int_read(int *c);
void pipe_string_read(char *str);
void pipe_string_write(char *str);
}

extern PlayMode *play_mode;
extern int have_commandline_midis;
extern int output_device_open;
extern int32 control_ratio;
extern char *cfg_names[];

static int currplaytime = 0;
static int meterfudge = 0;

enum midistatus{ KNONE, KPLAYING, KSTOPPED, KLOOPING, KFORWARD,
		 KBACKWARD, KNEXT, KPREVIOUS,KPAUSED};

static midistatus status, last_status;
static int nbvoice = 0;

//QFont default_font("Helvetica", 10, QFont::Bold);

KApplication * thisapp;
KMidiFrame *kmidiframe;
KMidi *kmidi;

int pipenumber;

DockWidget*     dock_widget;
bool dockinginprogress = 0;
bool quitPending = 0;

KMidiFrame::KMidiFrame( const char *name ) :
    KTMainWindow( name )
{
    kmidi = new KMidi(this, "_kmidi" );
    setView(kmidi,TRUE);
    quitPending = false;
    docking = true;
    //autodock = false;
    autodock = true;
    dockinginprogress = false;
    dock_widget = new DockWidget(this, "dockw");
        if(docking){
        dock_widget->show();
    }

}

KMidiFrame::~KMidiFrame(){
}


#if 0
bool KMidiFrame::event( QEvent *e ){
    if(e->type() == QEvent::Hide && autodock && docking){
        if(dockinginprogress || quitPending)
            return(FALSE);
        sleep(1); // give kwm some time..... ugly I know.
        if (!KWM::isIconified(winId())) // maybe we are just on another desktop
            return FALSE;
        dock_widget->setToggled(true);
        // a trick to remove the window from the taskbar (Matthias)
        recreate(0, 0, QPoint(x(), y()), FALSE);
        kapp->setTopWidget( this );
        return TRUE;
    }
    return QWidget::event(e);
}
#endif

void KMidiFrame::closeEvent( QCloseEvent *e ){

    quitPending = true;
    kmidi->quitClicked();
    e->accept();
}


KMidi::KMidi( QWidget *parent, const char *name )
    : QWidget( parent, name )
{

    playlistdlg = NULL;
    randomplay = false;
    looping = false;
    driver_error = false;
    last_status = status = KNONE;
    flag_new_playlist = false;
    song_number = 1;
    current_voices = DEFAULT_VOICES;
    stereo_state = reverb_state = chorus_state = verbosity_state = 1;
    starting_up = true;

    readconfig();

    struct configstruct config;
    config.background_color = background_color;
    config.led_color = led_color;
    config.tooltips = tooltips;
    config.max_patch_megs = max_patch_megs;

    what = new QWhatsThis(this);
    drawPanel();
    loadBitmaps();
    setPalettePropagation( QWidget::AllChildren );
    setColors();
    setToolTips();

    playlist = new QStrList(TRUE);
    listplaylists = new QStrList(TRUE);

    QStringList listf = KGlobal::dirs()->findAllResources("appdata", "*.plist");

    for ( QStringList::Iterator it = listf.begin(); it != listf.end(); ++it ) {
	    listplaylists->inSort( *it );
    }


    if ( !folder_pixmap.loadFromData(folder_bmp_data, folder_bmp_len) )
	KMessageBox::error(this, i18n("Error"), i18n("Could not load folder.bmp"));

    if ( !cdup_pixmap.loadFromData(cdup_bmp_data, cdup_bmp_len) )
	KMessageBox::error(this, i18n("Error"), i18n("Could not load cdup.bmp"));

    if ( !file_pixmap.loadFromData(file_bmp_data, file_bmp_len) )
	KMessageBox::error(this, i18n("Error"), i18n("Could not load file.bmp"));

    timer = new QTimer( this );
    readtimer = new QTimer( this);


    configdlg = new ConfigDlg(this,&config, "_configdlg");

    connect( timer, SIGNAL(timeout()),this, SLOT(PlayCommandlineMods()) );
    timer->start( 100, FALSE );

    connect( playPB, SIGNAL(clicked()), SLOT(playClicked()) );
    connect( stopPB, SIGNAL(clicked()), SLOT(stopClicked()) );
    connect( prevPB, SIGNAL(clicked()), SLOT(prevClicked()) );
    connect( nextPB, SIGNAL(clicked()), SLOT(nextClicked()) );
    connect( fwdPB, SIGNAL(clicked()), SLOT(fwdClicked()) );
    connect( bwdPB, SIGNAL(clicked()), SLOT(bwdClicked()) );
    connect( quitPB, SIGNAL(clicked()), SLOT(quitClicked()) );	
    connect( whatbutton, SIGNAL(clicked()), SLOT(invokeWhatsThis()) );	
    connect( replayPB, SIGNAL(clicked()), SLOT(replayClicked()) );
    connect( ejectPB, SIGNAL(clicked()), SLOT(ejectClicked()) );
    connect( volSB, SIGNAL(valueChanged(int)), SLOT(volChanged(int)));
    connect( aboutPB, SIGNAL(clicked()), SLOT(logoClicked()));
    connect( configurebutton, SIGNAL(clicked()), SLOT(aboutClicked()));
    connect( shufflebutton, SIGNAL(clicked()), SLOT(randomClicked()));
    connect( infobutton, SIGNAL(clicked()), SLOT(infoslot()));

    connect(this,SIGNAL(play()),this,SLOT(playClicked()));

    redoplaylistbox();
    playlistbox->setCurrentItem(current_playlist_num);

    srandom(time(0L));
    adjustSize();
    setMinimumSize( regularsize );

    resize( regularsize );

    setAcceptDrops(TRUE);

}

void KMidi::dragEnterEvent( QDragEnterEvent *e )
{
    if ( QUrlDrag::canDecode( e ) )
    {
	e->accept();
    }
}


void KMidi::dropEvent( QDropEvent * e )
{

    QStringList files;
    int newones = 0;
    char mbuff[5];
    if ( QUrlDrag::decodeLocalFiles( e, files ) ) {
	for (QStringList::Iterator i=files.begin(); i!=files.end(); ++i) {

            QFile f(*i);
            if (!f.open( IO_ReadOnly )) continue;
            if (f.readBlock(mbuff, 4) != 4) {
		f.close();
		continue;
            }
            mbuff[4] = '\0';
            if (strcmp(mbuff, "MThd")) {
		f.close();
		continue;
            }
            f.close();

	    playlist->insert(0, *i);
	    newones++;
	}
    }
    if (newones) {
        redoplaybox();
	setSong(0);
    }
}


KMidi::~KMidi(){
}

#define STATUS_LED 0
#define LOADING_LED 1
#define LYRICS_LED 2
#define BUFFER_LED 3
#define CSPLINE_LED 4
#define REVERB_LED 5
#define CHORUS_LED 6

void KMidi::setToolTips()
{
    if(tooltips){
	QToolTip::add( aboutPB,		i18n("Bottom Panel") );
	QToolTip::add( playPB, 		i18n("Play/Pause") );
	QToolTip::add( stopPB, 		i18n("Stop") );
	QToolTip::add( replayPB, 	i18n("Loop Song") );
	QToolTip::add( fwdPB, 		i18n("15 Secs Forward") );
	QToolTip::add( bwdPB, 		i18n("15 Secs Backward") );
	QToolTip::add( nextPB, 		i18n("Next Midi") );
	QToolTip::add( prevPB, 		i18n("Previous Midi") );
	QToolTip::add( quitPB, 		i18n("Exit Kmidi") );
	QToolTip::add( whatbutton,	i18n("What's This?") );
	QToolTip::add( shufflebutton, 	i18n("Random Play") );
        QToolTip::add( configurebutton, i18n("Configure Kmidi") );
	QToolTip::add( ejectPB, 	i18n("Open Playlist") );
	QToolTip::add( infobutton, 	i18n("Show Info Window") );
	QToolTip::add( shufflebutton, 	i18n("Random Play") );
	QToolTip::add( volSB, 		i18n("Volume Control") );

	QToolTip::add( patchbox,	i18n("Select Patch Set") );
	QToolTip::add( playbox,		i18n("Select Song") );
	QToolTip::add( playlistbox,	i18n("Select Playlist") );
	QToolTip::add( meterspin,	i18n("Sync Meter") );
	QToolTip::add( rcb1,		i18n("Stereo Voice: norm/xtra/off") );
	QToolTip::add( rcb2,		i18n("Echo: norm/xtra/off") );
	QToolTip::add( rcb3,		i18n("Detune: norm/xtra/off") );
	QToolTip::add( rcb4,		i18n("Verbosity: norm/xtra/off") );
	QToolTip::add( effectbutton,	i18n("Effects") );
	QToolTip::add( voicespin,	i18n("Set Polyphony") );
	QToolTip::add( filterbutton,	i18n("Filter") );
	QToolTip::add( led[STATUS_LED],		i18n("status") );
	QToolTip::add( led[LOADING_LED],	i18n("loading") );
	QToolTip::add( led[LYRICS_LED],		i18n("lyrics") );
	QToolTip::add( led[BUFFER_LED],		i18n("buffer") );
	QToolTip::add( led[CSPLINE_LED],	i18n("interpolation") );
	QToolTip::add( led[REVERB_LED],		i18n("reverb") );
	QToolTip::add( led[CHORUS_LED],		i18n("chorus") );
	QToolTip::add( ich[0],		i18n("linear interpolation") );
	QToolTip::add( ich[1],		i18n("cspline") );
	QToolTip::add( ich[2],		i18n("lagrange") );
	QToolTip::add( ich[3],		i18n("cspline + filter") );
    }
    else{
	QToolTip::remove( aboutPB);
	QToolTip::remove( playPB);
	QToolTip::remove( stopPB);
	QToolTip::remove( replayPB);
	QToolTip::remove( fwdPB );
	QToolTip::remove( bwdPB);
	QToolTip::remove( nextPB );
	QToolTip::remove( prevPB );
	QToolTip::remove( quitPB );
	QToolTip::remove( whatbutton );
	QToolTip::remove( configurebutton );
	QToolTip::remove( shufflebutton );
	QToolTip::remove( ejectPB );
	QToolTip::remove( shufflebutton );
	QToolTip::remove( infobutton );
	QToolTip::remove( volSB );

	QToolTip::remove( patchbox );
	QToolTip::remove( playbox );
	QToolTip::remove( playlistbox );
	QToolTip::remove( meterspin );
	QToolTip::remove( rcb1 );
	QToolTip::remove( rcb2 );
	QToolTip::remove( rcb3 );
	QToolTip::remove( rcb4 );
	QToolTip::remove( effectbutton );
	QToolTip::remove( voicespin );
	QToolTip::remove( filterbutton );
	QToolTip::remove( led[STATUS_LED] );
	QToolTip::remove( led[LOADING_LED] );
	QToolTip::remove( led[LYRICS_LED] );
	QToolTip::remove( led[BUFFER_LED] );
	QToolTip::remove( led[CSPLINE_LED] );
	QToolTip::remove( led[REVERB_LED] );
	QToolTip::remove( led[CHORUS_LED] );
	QToolTip::remove( ich[0] );
	QToolTip::remove( ich[1] );
	QToolTip::remove( ich[2] );
	QToolTip::remove( ich[3] );
    }

}


void KMidi::volChanged( int vol )
{       	
	
    QString volumetext;
    if (vol == 100){
	volumetext.sprintf(i18n("Vol:%03d%%"),vol);
    }
    else{
	volumetext.sprintf(i18n("Vol:%02d%%"),vol);
    }
    volLA->setText( volumetext );

    pipe_int_write(  MOTIF_CHANGE_VOLUME);
    pipe_int_write(vol*255/100);
    volume = vol;
    //  mp_volume = vol*255/100;

}



#define BAR_WID 10
#define BAR_HGT 62
#define BAR_LM  12
#define BAR_TM  10
#define BAR_BOT (BAR_TM + BAR_HGT)
#define WIN_WID 175
#define WIN_HGT 80


#define DELTA_VEL	4
#define FLAG_NOTE_OFF	1
#define FLAG_NOTE_ON	2

#define FLAG_BANK	1
#define FLAG_PROG	2
#define FLAG_PAN	4
#define FLAG_SUST	8

void MeterWidget::remeter()
{
        QPainter paint( this );
	QPen greenpen(led_color, 3);
	QPen yellowpen(yellow, 3);
	QPen erasepen(background_color, 3);
	static int lastvol[MAXDISPCHAN], lastamp[MAXDISPCHAN], last_sustain[MAXDISPCHAN], meterpainttime = 0;
	int ch, x1, y1, slot, amplitude, notetime, chnotes;

	if (currplaytime + meterfudge < meterpainttime || Panel->reset_panel == 10) {
		erase();
		for (ch = 0; ch < MAXDISPCHAN; ch++) {
			lastvol[ch] = lastamp[ch] = last_sustain[ch] = 0;
			for (slot = 0; slot < NQUEUE; slot++)
			    Panel->ctime[slot][ch] = -1;
		}
		Panel->reset_panel = 9;
	}
	meterpainttime = currplaytime + meterfudge;

	if (Panel->reset_panel) {
		Panel->reset_panel--;
		return;
	}

	for (ch = 0; ch < MAXDISPCHAN; ch++) {
		x1 = BAR_LM + (ch & 0x0f) * BAR_WID;
		if (ch >= MAXDISPCHAN/2) x1 += BAR_WID / 2;
		amplitude = -1;
		chnotes = 0;

		for (slot = 0; slot < NQUEUE; slot++) {
		  notetime = Panel->ctime[slot][ch];
		  if (notetime != -1 && notetime <= meterpainttime) {
		    if (chnotes < Panel->notecount[slot][ch])
		        chnotes = Panel->notecount[slot][ch];
		    if (amplitude < Panel->ctotal[slot][ch])
			amplitude = Panel->ctotal[slot][ch];
		    last_sustain[ch] = Panel->ctotal_sustain[slot][ch];
		    Panel->ctime[slot][ch] = -1;
		  }
		} // for each slot


		if (amplitude < 0 && lastamp[ch]) {
			if (Panel->c_flags[ch] & FLAG_PERCUSSION)
				amplitude = lastamp[ch] - 4*DELTA_VEL;
			else amplitude = lastamp[ch] - DELTA_VEL;
			if (amplitude < 0) amplitude = 0;
		}

		if (amplitude != -1) {
			lastamp[ch] = amplitude;
			amplitude += last_sustain[ch];
			if (amplitude > 127) amplitude = 127;
			y1 = (amplitude * BAR_HGT) / 127;
			if (y1 > BAR_HGT) y1 = BAR_HGT;
			if (y1 < lastvol[ch]) {
				paint.setPen( erasepen );
	    			paint.drawLine( x1, BAR_TM, x1, BAR_BOT - y1 );
			}
			else if (y1 > lastvol[ch]) {
				if (Panel->c_flags[ch] & FLAG_PERCUSSION) paint.setPen( yellowpen );
					else paint.setPen( greenpen );
	    			paint.drawLine( x1, BAR_BOT - y1, x1, BAR_BOT );
			}
			lastvol[ch] = y1;
		}
	} // for each ch
}

void MeterWidget::paintEvent( QPaintEvent * )
{
	remeter();
}

MeterWidget::MeterWidget( QWidget *parent, const char *name )
    : QWidget( parent, name )
{
    metertimer = new QTimer( this );
    connect( metertimer, SIGNAL(timeout()), SLOT(remeter()) );
    metertimer->start( 80, FALSE );
}


MeterWidget::~MeterWidget()
{
}



QPushButton *KMidi::makeButton( int x, int y, int w, int h, const QString &n )
{
    QPushButton *pb = new QPushButton( n, this );
    pb->setGeometry( x, y, w, h );
    return pb;
}

void KMidi::drawPanel()
{
    int ix = 0;
    int iy = 0;
    //    int WIDTH = 100;
    int WIDTH = 90;
    //    int HEIGHT = 30;
    int HEIGHT = 27;
    //    int SBARWIDTH = 230;
    int SBARWIDTH = 220; //140
    int totalwidth, regularheight, extendedheight;

    //setCaption( i18n("kmidi") );
    aboutPB = makeButton( ix, iy, WIDTH, 2 * HEIGHT, "About" );
    what->add(aboutPB, i18n("Open up or close lower<br>part of panel with<br>" \
			"channel display and<br>other controls") );
    iy += 2 * HEIGHT;

    ix = 0;
    ejectPB = makeButton( ix, iy, WIDTH/2, HEIGHT, "Eject" );
    what->add(ejectPB, i18n("open up the play list editor"));
    infobutton = makeButton( ix +WIDTH/2, iy, WIDTH/2, HEIGHT, "info" );
    what->add(infobutton, i18n("open up or close<br>the display of info about the<br>song being played"));
    iy += HEIGHT;

    //leds here
    iy += HEIGHT/2;

    quitPB = makeButton( ix, iy, 3*WIDTH/4, HEIGHT, "Quit" );
    whatbutton = makeButton( ix + 3*WIDTH/4, iy, WIDTH/4, HEIGHT, "??" );
    QPixmap p( whatsthis_image );
    whatbutton->setPixmap( p );
    what->add(quitPB, i18n("exit from KMidi"));

    ix += WIDTH;
    iy = 0;

    //	backdrop = new MyBackDrop(this,"mybackdrop");
    backdrop = new QWidget(this,"mybackdrop");
    backdrop->setGeometry(ix,iy,SBARWIDTH, 3* HEIGHT - 1);
    what->add(backdrop, i18n("display of information"));

    int D = 10;
    ix = WIDTH + 4;

    for (int u = 0; u<=4;u++){
	trackTimeLED[u] = new BW_LED_Number(this );	
	trackTimeLED[u]->setGeometry( ix  + u*18, iy + D, 23 ,  30 );
    }
	
    QString zeros("--:--");
    setLEDs(zeros);
    iy += 2 * HEIGHT;

    ix = WIDTH;
    statusLA = new QLabel( this );
    statusLA->setGeometry( WIDTH -25 +2*SBARWIDTH/3, 6, 44, 15 );
    statusLA->setFont( QFont( "Helvetica", 10, QFont::Bold ) );
    statusLA->setAlignment( AlignLeft );
    statusLA->setText("    ");
    what->add(statusLA, i18n("what's happening now"));

    looplabel = new QLabel( this );
    looplabel->setGeometry( WIDTH -25 +2*SBARWIDTH/3 +45, 6, 60, 13 );
    looplabel->setFont( QFont( "Helvetica", 10, QFont::Bold ) );
    looplabel->setAlignment( AlignLeft );
    looplabel->setText("");


    properties2LA = new QLabel( this );

    properties2LA->setGeometry( WIDTH -25 + 2*SBARWIDTH/3 , HEIGHT + 20, 100, 13 );
    properties2LA->setFont( QFont( "Helvetica", 10, QFont::Bold ) );
    properties2LA->setAlignment( AlignLeft );
    properties2LA->setText("");

    propertiesLA = new QLabel( this );

    propertiesLA->setGeometry( WIDTH -25 + 2*SBARWIDTH/3 ,  33, 100, 13 );
    propertiesLA->setFont( QFont( "Helvetica", 10, QFont::Bold ) );
    propertiesLA->setAlignment( AlignLeft );
    propertiesLA->setText("");

    ix += SBARWIDTH/2;
    //leds here
    iy += HEIGHT/2;

    shufflebutton = makeButton( WIDTH + 2*SBARWIDTH/3 ,
				iy+ HEIGHT,SBARWIDTH/6 , HEIGHT, "F" );
    shufflebutton->setToggleButton( TRUE );
    what->add(shufflebutton, i18n("select the next song randomly"));

    configurebutton = makeButton( WIDTH + 5*SBARWIDTH/6 ,
				  iy+HEIGHT, SBARWIDTH/6 , HEIGHT, "S" );
    what->add(configurebutton, i18n("open up or close<br>the configuration window"));


    lowerBar = new QPushButton(this);
    lowerBar->setGeometry( WIDTH ,
			   7*HEIGHT/2 + HEIGHT/2, 2*SBARWIDTH/3 -1, HEIGHT/2 +1 );
    what->add(lowerBar, i18n("(this is just decorative)"));

    ix = WIDTH ;
    iy += HEIGHT;

    volLA = new QLabel( this );
    volLA->setAlignment( AlignLeft );
    volLA->setGeometry( WIDTH -25 + 2*SBARWIDTH/3 , 20, 60, 13 );

    volLA->setFont( QFont( "helvetica", 10, QFont::Bold) );
    what->add(volLA, i18n("the current volume level"));


    QString volumetext;
    volumetext.sprintf(i18n("Vol:%02d%%"),volume);
    volLA->setText( volumetext );


    /*speedLA = new QLabel( this );
      speedLA->setAlignment( AlignLeft );
    speedLA->setGeometry( WIDTH -25 + 2*SBARWIDTH/3 +45, 24, 80, 13 );
    speedLA->setFont( QFont( "helvetica", 10, QFont::Bold) );
    */

    /*    QString speedtext;
    speedtext.sprintf(i18n("Spd:%3.0f%%"),100.0);
    speedLA->setText( speedtext );
    */

    modlabel = new QLabel( this );
    modlabel->setAlignment( AlignLeft );
    modlabel->setGeometry( WIDTH + 9, HEIGHT + 24 + 10, SBARWIDTH - 26, 12 );
    modlabel->setFont( QFont( "helvetica", 10, QFont::Bold) );
    modlabel->setText( "" );

    totaltimelabel = new QLabel( this );
    totaltimelabel->setAlignment( AlignLeft );
    totaltimelabel->setGeometry( WIDTH + 80, HEIGHT + 20, 30, 14 );
    totaltimelabel->setFont( QFont( "helvetica", 10, QFont::Bold) );
    totaltimelabel->setText( "--:--" );


    song_count_label = new QLabel( this );
    song_count_label->setAlignment( AlignLeft );
    song_count_label->setGeometry( WIDTH + 9, HEIGHT + 20, 60, 15 );
    song_count_label->setFont( QFont( "helvetica", 10, QFont::Bold) );

    song_count_label->setText( i18n("Song --/--") );


    iy += HEIGHT / 2 ;
	
    volSB = new QSlider( 0, 100, 5,  volume, QSlider::Horizontal,
			 this, "Slider" );
    volSB->setGeometry( WIDTH , 3*HEIGHT + HEIGHT/2, 2*SBARWIDTH/3, HEIGHT /2 );
    what->add(volSB, i18n("Adjust the volume here.<br>" \
	"(Keep it pretty low, and<br>" \
	"use <i>Kmix</i> to adjust<br>" \
	"the mixer volumes, if<br>" \
	"you want it louder.)"));

    iy = 0;
    ix = WIDTH + SBARWIDTH;

    playPB = makeButton( ix, iy, WIDTH, HEIGHT, i18n("Play/Pause") );
    playPB->setToggleButton( TRUE );
    what->add(playPB, i18n("If playing, pause,<br>else start playing."));

    totalwidth = ix + WIDTH;

    iy += HEIGHT;
    stopPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Stop") );
    what->add(stopPB, i18n("Stop playing this song."));

    ix += WIDTH/2 ;
    replayPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Replay") );
    replayPB->setToggleButton( TRUE );
    what->add(replayPB, i18n("Keep replaying<br> the same song."));

    ix = WIDTH + SBARWIDTH;
    iy += HEIGHT;
    bwdPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Bwd") );
    what->add(bwdPB, i18n("Jump backwards 15 seconds."));

    ix += WIDTH/2;
    fwdPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Fwd") );
    what->add(fwdPB, i18n("Jump forward 15 seconds."));

    ix = WIDTH + SBARWIDTH;
    iy += HEIGHT;
    //leds here
    iy += HEIGHT/2;

    prevPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Prev") );
    what->add(prevPB, i18n("Play the previous song<br> on the play list."));

    ix += WIDTH/2 ;
    nextPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Next") );
    what->add(nextPB, i18n("Play the next song<br> on the play list."));


    ix = WIDTH + WIDTH/2;
    iy += HEIGHT;

    QString polyled = i18n("These leds show<br>the number of notes<br>being played.");

    for (int i = 0; i < 17; i++) if (i < 7 || i > 10) {
        led[i] = new KLed(led_color, this);
        led[i]->setLook(KLed::sunken);
        led[i]->setShape(KLed::Rectangular);
        led[i]->setGeometry(WIDTH/8 + i * WIDTH/4,3*HEIGHT+HEIGHT/6, WIDTH/6, HEIGHT/5);
	led[i]->setColor(Qt::black);
	if (i>10) what->add(led[i], polyled);
    }

    what->add(led[STATUS_LED], i18n("The <i>status</i> led is<br>" \
	"<b>red</b> when KMidi is<br>" \
	"stopped, <b>yellow</b> when<br>" \
	"paused, otherwise:<br>" \
	"<b>blue</b> when looping on<br>" \
	"the same song<br>" \
	"<b>magenta</b> when selecting<br>" \
	"songs randomly<br>" \
	"or <b>green</b> when KMidi is<br>" \
	"playing or ready to play." ));

    what->add(led[LOADING_LED], i18n("The <i>loading</i> led<br>" \
	"blinks <b>yellow</b> when<br>" \
	"patches are being<br>" \
	"loaded from the hard drive." ));

    what->add(led[LYRICS_LED], i18n("The <i>lyrics</i> led<br>" \
	"lights up when a<br>" \
	"synchronized midi text<br>" \
	"event is being displayed<br>" \
	"in the <i>info</i> window." ));

    what->add(led[BUFFER_LED], i18n("The <i>buffer</i> led<br>" \
	"is orangeish when there is<br>" \
	"danger of a dropout, but<br>" \
	"greenish when all is ok." ));

    what->add(led[CSPLINE_LED], i18n("The <i>interpolation</i> led<br>" \
	"is on when c-spline or<br>LaGrange interpolation<br>" \
	"is being used for resampling." ));

    what->add(led[REVERB_LED], i18n("The <i>echo</i> led<br>" \
	"is on when KMidi is<br>" \
	"not too busy to generate<br>" \
	"extra echo notes<br>" \
	"for reverberation effect.<br>" \
	"It's bright when you've<br>" \
	"asked for extra reverberation." ));
    what->add(led[CHORUS_LED], i18n("The <i>detune</i> led<br>" \
	"is on when KMidi is<br>" \
	"not too busy to generate<br>" \
	"extra detuned notes<br>" \
	"for chorusing effect.<br>" \
	"It's bright when you've<br>" \
	"asked for extra chorusing." ));

    ichecks = new QButtonGroup( );
    ichecks->setExclusive(TRUE);
    what->add(ichecks, i18n("ichecks"));
    for (int i = 0; i < 4; i++) {
        ich[i] = new QCheckBox( this );
	ichecks->insert(ich[i],i);
        ich[i]->setGeometry(WIDTH/8 + (i+7) * WIDTH/4,3*HEIGHT, WIDTH/6, HEIGHT/2);
    }
    ich[1]->setChecked( TRUE );
    what->add(ich[0], i18n("Resample using linear interpolation."));
    what->add(ich[1], i18n("Resample using c-spline interpolation."));
    what->add(ich[2], i18n("Resample using LaGrange interpolation."));
    what->add(ich[3], i18n("Resample using c-spline<br>interpolation with<br>the low pass filter."));
    connect( ichecks, SIGNAL(clicked(int)), SLOT(updateIChecks(int)) );

    regularheight = iy;

    meter = new MeterWidget ( this, "soundmeter" );
    meter->setBackgroundColor( background_color );
    meter->led_color = led_color;
    meter->background_color = background_color;
    meter->setGeometry(ix,iy,SBARWIDTH - WIDTH/2, 3* HEIGHT - 1);
    what->add(meter, i18n("The display shows notes<br> being played on the<br>16 or 32 midi channels."));
    meter->hide();
    extendedheight = iy + 3*HEIGHT;
    extendedsize = QSize (totalwidth, extendedheight);
    
    ix = 0;
    logwindow = new LogWindow(this,"logwindow");
    logwindow->setGeometry(ix,extendedheight, totalwidth, infowindowheight);
    what->add(logwindow, i18n("Here is information<br>" \
	"extracted from the midi<br>file currently being played."));
    logwindow->resize(totalwidth, infowindowheight);
    logwindow->hide();

    regularsize = QSize (totalwidth, regularheight);
    setFixedWidth(totalwidth);

    // Choose patch set
    ix = 0;
    iy = regularheight;
    patchbox = new QComboBox( FALSE, this );
    patchbox->setGeometry(ix, iy, WIDTH + WIDTH/2, HEIGHT);
    what->add(patchbox, i18n("Choose a patchset.<br>" \
	"(You can add patchsets by<br>" \
	"downloading them from somewhere<br>" \
	"and editing the file<br>" \
	"$KDEDIR/share/apps/kmidi/<br>config/timidity.cfg .)"));
    patchbox->setFont( QFont( "helvetica", 10, QFont::Normal) );
    int lx;
    for (lx = 30; lx > 0; lx--) if (cfg_names[lx-1]) break;
    for (int sx = 0; sx < lx; sx++)
	if (cfg_names[sx]) patchbox->insertItem( cfg_names[sx] );
	else patchbox->insertItem( "(none)" );
    patchbox->setCurrentItem(Panel->currentpatchset);
    connect( patchbox, SIGNAL(activated(int)), SLOT(setPatch(int)) );
    patchbox->hide();
 
    iy += HEIGHT;
    playbox = new QComboBox( FALSE, this, "song" );
    playbox->setGeometry(ix, iy, WIDTH + WIDTH/2, HEIGHT);
    what->add(playbox, i18n("Select a midi file<br>to play from this<br>play list."));
    //playbox->setFont( QFont( "helvetica", 10, QFont::Normal) );
    connect( playbox, SIGNAL(activated(int)), SLOT(setSong(int)) );
    playbox->hide();

    iy += HEIGHT;

    playlistbox = new QComboBox( FALSE, this, "_playlists" );
    playlistbox->setGeometry(ix, iy, WIDTH + WIDTH/2, HEIGHT);
    what->add(playlistbox, i18n("These are the playlist files<br>" \
	"you've created using<br> the <i>playlist editor</i>.<br>" \
	"Click on one, and its<br>contents will replace<br>" \
	"the play list above."));
    //playlistbox->setFont( QFont( "helvetica", 10, QFont::Normal) );
    connect( playlistbox, SIGNAL( activated( int ) ), this, SLOT( plActivated( int ) ) );
    playlistbox->hide();


    iy = regularheight;
    ix = WIDTH + SBARWIDTH;
    rchecks = new QButtonGroup( );
    rchecks->setExclusive(FALSE);
    rcb1 = new QCheckBox(this);
    rcb2 = new QCheckBox(this);
    rcb3 = new QCheckBox(this);
    rcb4 = new QCheckBox(this);
    rcb1->setGeometry(ix+WIDTH/16, iy+HEIGHT/4, WIDTH/6, HEIGHT/2);
    rcb2->setGeometry(ix+WIDTH/16 + WIDTH/4, iy+HEIGHT/4, WIDTH/6, HEIGHT/2);
    rcb3->setGeometry(ix+WIDTH/16 + WIDTH/2, iy+HEIGHT/4, WIDTH/6, HEIGHT/2);
    rcb4->setGeometry(ix+WIDTH/16 + 3*WIDTH/4, iy+HEIGHT/4, WIDTH/6, HEIGHT/2);
    rchecks->insert(rcb1,0);
    rchecks->insert(rcb2,1);
    rchecks->insert(rcb3,2);
    rchecks->insert(rcb4,3);

    rcb1->setTristate(TRUE);
    rcb2->setTristate(TRUE);
    rcb3->setTristate(TRUE);
    rcb4->setTristate(TRUE);
    rcb1->setNoChange();
    rcb2->setNoChange();
    rcb3->setNoChange();
    rcb4->setNoChange();
    connect( rchecks, SIGNAL(clicked(int)), SLOT(updateRChecks(int)) );
    rcb1->hide();
    rcb2->hide();
    rcb3->hide();
    rcb4->hide();

    what->add(rcb1, i18n("There are three settings:<br>" \
	"<b>off</b> for no extra stereo notes<br>" \
	"<b>no change</b> for normal stereo<br>" \
	"<b>checked/on</b> for notes panned<br>" \
	"left and right artificially.") );
    what->add(rcb2, i18n("There are three settings:<br>" \
	"<b>off</b> for no extra echo notes<br>" \
	"<b>no change</b> for echoing<br>" \
	"when a patch specifies<br>reverberation, or<br>" \
	"<b>checked/on</b> for added reverb<br>" \
	"for all instruments.") );
    what->add(rcb3, i18n("There are three settings:<br>" \
	"<b>off</b> for no extra detuned notes<br>" \
	"<b>no change</b> for detuned notes<br>" \
	"when a patch specifies<br>chorusing, or<br>" \
	"<b>checked/on</b> for added chorusing<br>" \
	"for all instruments.") );
    what->add(rcb4, i18n("There are three settings:<br>" \
	"<b>off</b> for only name and lyrics<br>" \
	"shown in the <i>info</i> window;<br>" \
	"<b>no change</b> for all text<br>" \
	"messages to be shown, and<br>" \
	"<b>checked/on</b> to display also<br>" \
	"information about patches<br>" \
	"as they are loaded from disk.") );

    iy += HEIGHT;
    effectbutton = makeButton( ix,          iy, WIDTH/2, HEIGHT, "eff" );
    effectbutton->setToggleButton( TRUE );
    effectbutton->setFont( QFont( "helvetica", 10, QFont::Bold) );
    what->add(effectbutton, i18n("When this button is down,<br>" \
	"filters are used for the<br>" \
	"<i>midi</i> channel effects<br>" \
	"<b>chorus</b>, <b>reverberation</b>,<br>" \
	"<b>celeste</b>, and <b>phaser</b>.<br>" \
	"When it is off, <b>chorus</b> is<br>" \
	"done instead with extra detuned<br>" \
	"notes, and <b>reverberation</b> is<br>" \
	"done with echo notes, but<br>the other effects are<br>not done." ));

    connect( effectbutton, SIGNAL(toggled(bool)), SLOT(setEffects(bool)) );
    effectbutton->hide();

    voicespin = new QSpinBox( 1, MAX_VOICES, 1, this, "_spinv" );
    voicespin->setValue(current_voices);
    voicespin->setGeometry( ix +WIDTH/2, iy, WIDTH/2, HEIGHT );
    voicespin->setFont( QFont( "helvetica", 10, QFont::Bold) );
    what->add(voicespin, i18n("Use this to set the maximum<br>notes that can be played<br>" \
	"at one time.  Use a lower<br>" \
	"setting to avoid dropouts."));
    connect( voicespin, SIGNAL( valueChanged(int) ),
	     SLOT( voicesChanged(int) ) );
    voicespin->hide();

    iy += HEIGHT;

    meterspin = new QSpinBox( 0, 99, 1, this, "_spinm" );
    meterspin->setValue(meterfudge);
    meterspin->setGeometry( ix, iy, WIDTH/2, HEIGHT );
    connect( meterspin, SIGNAL( valueChanged(int) ),
	     SLOT( meterfudgeChanged(int) ) );
    meterspin->setFont( QFont( "helvetica", 10, QFont::Bold) );
    what->add(meterspin, i18n("This setting is a delay<br>" \
	"in centiseconds before a played<br>" \
	"note is displayed on the<br>" \
	"channel meter to the left.<br>" \
	"It may help to syncronize the<br>" \
	"display with the music."));
    meterspin->hide();

    filterbutton = makeButton( ix +WIDTH/2,     iy, WIDTH/2,   HEIGHT, "filt" );
    filterbutton->setToggleButton( TRUE );
    filterbutton->setFont( QFont( "helvetica", 10, QFont::Bold) );
    what->add(filterbutton, i18n("When this filter button<br>" \
	"is on, a low pass filter<br>" \
	"is used for drum patches<br>" \
	"when they are first loaded<br>" \
	"and when the patch itself<br>" \
	"requests this.  The filter<br>" \
	"is also used for melodic voices<br>" \
	"if you have chosed the<br><i>cspline+filter</i><br>" \
	"interpolation option."));
    connect( filterbutton, SIGNAL(toggled(bool)), SLOT(setFilter(bool)) );
    filterbutton->hide();
}
 
void KMidi::plActivated( int index )
{
    current_playlist_num = index;
    loadplaylist(index);
    if (status != KPLAYING) resetSong();
    else flag_new_playlist = true;
}
 
void KMidi::updateIChecks( int which )
{
    interpolationrequest = which;
    pipe_int_write(MOTIF_INTERPOLATION);
    pipe_int_write(which);
}

void KMidi::updateRChecks( int which )
{
    int check_states = 0;

    switch (which) {
	case 0:
		// stereo voice
	    check_states = stereo_state = (int)rcb1->state();
	    break;
	case 1:
		// echo voice
	    check_states = reverb_state = (int)rcb2->state();
	    break;
	case 2:
		// detune voice
	    check_states = chorus_state = (int)rcb3->state();
	    break;
	case 3:
		// info window verbosity
	    check_states = verbosity_state = (int)rcb4->state();
	    break;
	default:
	    return;

    }
    check_states |= which << 4;
    pipe_int_write(MOTIF_CHECK_STATE);
    pipe_int_write(check_states);
}

void KMidi::meterfudgeChanged( int newfudge )
{
    meterfudge = newfudge;
}

void KMidi::voicesChanged( int newvoices )
{
    if (newvoices < 1 || newvoices > MAX_VOICES) return;
    current_voices = newvoices;
    if (status != KSTOPPED) stopClicked();
    pipe_int_write(MOTIF_CHANGE_VOICES);
    pipe_int_write(newvoices);
}

void KMidi::setPatch( int index )
{
    if (!cfg_names[index]) return;
    if (index == Panel->currentpatchset) return;
    if (status != KSTOPPED) stopClicked();

    pipe_int_write(  MOTIF_PATCHSET );
    pipe_int_write( index );
}

void KMidi::resetSong()
{
    flag_new_playlist = false;
    song_number = 1;
    if (!playlist->count()) {
	redoplaybox();
	return;
    }
    playbox->setCurrentItem(0);

    fileName = playbox->text(0);
    if(output_device_open)
      updateUI();
    setLEDs("OO:OO");
    statusLA->setText(i18n("Ready"));
}

void KMidi::setSong( int number )
{
    flag_new_playlist = false;
    if (!playlist->count() || number > (int)playlist->count()) {
	redoplaybox();
	return;
    }
    playbox->setCurrentItem(number);
    song_number = number + 1;
	
    fileName = playbox->text(number);
    if(output_device_open)
      updateUI();

    if(status == KPLAYING)
      setLEDs("OO:OO");
    nbvoice = currplaytime = 0;

    playPB->setOn( TRUE );
    statusLA->setText(i18n("Playing"));

    pipe_int_write(MOTIF_PLAY_FILE);
    pipe_string_write(playlist->at(song_number-1));
    status = KPLAYING;
}

void KMidi::redoplaybox()
{
    QString filenamestr;
    int i, index;
    int last = playlist->count();

    playbox->clear();

    for (i = 0; i < last; i++) {

	filenamestr = playlist->at(i);
	index = filenamestr.findRev('/',-1,TRUE);
	if(index != -1)
	    filenamestr = filenamestr.right(filenamestr.length() -index -1);
	filenamestr = filenamestr.replace(QRegExp("_"), " ");

	if(filenamestr.length() > 4){
	if(filenamestr.right(4) == QString(".mid") ||filenamestr.right(4) == QString(".MID"))
	    filenamestr = filenamestr.left(filenamestr.length()-4);
	}
	playbox->insertItem(filenamestr);

    }
}

void KMidi::redoplaylistbox()
{
    QString filenamestr;
    int i, index;
    int last = listplaylists->count();

    playlistbox->clear();

    for (i = 0; i < last; i++) {

	filenamestr = listplaylists->at(i);
	index = filenamestr.findRev('/',-1,TRUE);
	if(index != -1)
	    filenamestr = filenamestr.right(filenamestr.length() -index -1);
	filenamestr = filenamestr.replace(QRegExp("_"), " ");

	if(filenamestr.length() > 6){
	if(filenamestr.right(6) == QString(".plist"))
	    filenamestr = filenamestr.left(filenamestr.length()-6);
	}
	playlistbox->insertItem(filenamestr);

    }
}

void KMidi::setEffects( bool down )
{
    pipe_int_write(  MOTIF_EFFECTS );
    if (down) pipe_int_write( 1 );
    else pipe_int_write( 0 );
}

void KMidi::setFilter( bool down )
{
    pipe_int_write(  MOTIF_FILTER );
    if (down) pipe_int_write( 1 );
    else pipe_int_write( 0 );
}

void KMidi::logoClicked(){

    if(meter->isVisible()){
	meter->hide();
	patchbox->hide();
	playbox->hide();
	playlistbox->hide();
        rcb1->hide();
        rcb2->hide();
        rcb3->hide();
        rcb4->hide();
	effectbutton->hide();
	voicespin->hide();
	meterspin->hide();
	filterbutton->hide();
	if (logwindow->isVisible()) {
	    logwindow->move(0, regularsize.height());
            resize( regularsize.width(), regularsize.height() + logwindow->height() );
	}
	else resize( regularsize );
	return;
    }
    meter->show();
    patchbox->show();
    playbox->show();
    playlistbox->show();
    rcb1->show();
    rcb2->show();
    rcb3->show();
    rcb4->show();
    effectbutton->show();
    voicespin->show();
    meterspin->show();
    filterbutton->show();
    if (logwindow->isVisible()) {
	logwindow->move(0, extendedsize.height());
        resize( extendedsize.width(), extendedsize.height() + logwindow->height() );
    }
    else resize( extendedsize );
}

void KMidi::check_meter_visible(){

    if (patchbox->isVisible()) {
// this doesn't work
	meter->show();
    }
}

void KMidi::loadBitmaps() {

    QBitmap playBmp( playpause_width, playpause_height, playpause_bits,
		     TRUE );
    QBitmap stopBmp( stop_width, stop_height, stop_bits, TRUE );
    QBitmap prevBmp( prevtrk_width, prevtrk_height, prevtrk_bits, TRUE );
    QBitmap nextBmp( nexttrk_width, nexttrk_height, nexttrk_bits, TRUE );
    QBitmap replayBmp( repeat_width, repeat_height, repeat_bits, TRUE );
    QBitmap fwdBmp( ff_width, ff_height, ff_bits, TRUE );
    QBitmap bwdBmp( rew_width, rew_height, rew_bits, TRUE );
    QBitmap ejectBmp( eject_width, eject_height, eject_bits, TRUE );
    QBitmap quitBmp( poweroff_width, poweroff_height, poweroff_bits,
		     TRUE );
    QBitmap aboutBmp( midilogo_width, midilogo_height, midilogo_bits, TRUE );

    QBitmap shuffleBmp( shuffle_width, shuffle_height, shuffle_bits, TRUE );
    QBitmap optionsBmp( options_width, options_height, options_bits, TRUE );
    QBitmap infoBmp( info_width, info_height, info_bits, TRUE );

    playPB->setPixmap( playBmp );
    infobutton->setPixmap( infoBmp );
    shufflebutton->setPixmap( shuffleBmp );
    configurebutton->setPixmap( optionsBmp );
    stopPB->setPixmap( stopBmp );
    prevPB->setPixmap( prevBmp );
    nextPB->setPixmap( nextBmp );
    replayPB->setPixmap( replayBmp );
    fwdPB->setPixmap( fwdBmp );
    bwdPB->setPixmap( bwdBmp );
    ejectPB->setPixmap( ejectBmp );
    quitPB->setPixmap( quitBmp );
    aboutPB->setPixmap( aboutBmp );
}


void KMidi::setLEDs(const QString &symbols){

    for(int i=0;i<=4;i++){
	trackTimeLED[i]->display(symbols[i]);
    }

}

int KMidi::randomSong(){

    int j;
    flag_new_playlist = false;
    j=1+(int) (((double)playlist->count()) *rand()/(RAND_MAX+1.0));
    return j;
}


void KMidi::randomClicked(){

    flag_new_playlist = false;
    if(playlist->count() == 0) {
	shufflebutton->setOn( FALSE );
	randomplay = FALSE;
        return;
    }

    if (!shufflebutton->isOn()) {
	randomplay = FALSE;
	looplabel->setText("");
	return;
    }
    looping = FALSE;
    replayPB->setOn( FALSE );

    randomplay = TRUE;
    last_status = KNONE;
    updateUI();
    setLEDs("OO:OO");
    nbvoice = currplaytime = 0;
    statusLA->setText(i18n("Playing"));
    looplabel->setText(i18n("Random"));

    int index = randomSong() - 1;
    song_number = index + 1;
    playbox->setCurrentItem(index);
    pipe_int_write(MOTIF_PLAY_FILE);
    pipe_string_write(playlist->at(index));

    playPB->setOn( TRUE );
    status = KPLAYING;

}

void KMidi::randomPlay(){

    randomplay = TRUE;
    shufflebutton->setOn( TRUE );
    updateUI();
    setLEDs("OO:OO");
    nbvoice = currplaytime = 0;
    statusLA->setText(i18n("Playing"));
    looplabel->setText(i18n("Random"));

    int index = randomSong() - 1;
    song_number = index + 1;
    playbox->setCurrentItem(index);
    pipe_int_write(MOTIF_PLAY_FILE);
    pipe_string_write(playlist->at(index));

    playPB->setOn( TRUE );
    status = KPLAYING;

}

void KMidi::playClicked()
{

  if(!output_device_open){
    playPB->setOn( FALSE );
    return;
  }

  if(status == KPLAYING){
    status = KPAUSED;
    playPB->setOn( FALSE );
    pipe_int_write(MOTIF_PAUSE);
    statusLA->setText(i18n("Paused"));
    return;
  }

  if(status == KPAUSED){
    status = KPLAYING;
    playPB->setOn( TRUE );
    pipe_int_write(MOTIF_PAUSE);
    statusLA->setText(i18n("Playing"));
    return;
  }

  if (flag_new_playlist) {
    flag_new_playlist = false;
    song_number = 1;
  }

  int index;
  index = song_number -1;
  if (index < 0) index = 0;

  if(((int)playlist->count()  > index) && (index >= 0)){
    setLEDs("OO:OO");
    nbvoice = currplaytime = 0;
    statusLA->setText(i18n("Playing"));

    pipe_int_write(MOTIF_PLAY_FILE);
    pipe_string_write(playlist->at(index));

    status = KPLAYING;
    playPB->setOn( TRUE );
  }
  else stopClicked();
}


void KMidi::stopClicked()
{
     looping = false;
     if (flag_new_playlist) resetSong(); 
     replayPB->setOn( FALSE );
     randomplay = false;
     shufflebutton->setOn( FALSE );
     if (status != KPAUSED && status != KSTOPPED) pipe_int_write(MOTIF_PAUSE);
     status = KSTOPPED;
     playPB->setOn( FALSE );
     statusLA->setText(i18n("Ready"));
     looplabel->setText("");
     setLEDs("00:00");
     currplaytime = nbvoice = 0;
}



void KMidi::prevClicked(){

    if (flag_new_playlist) song_number = 2;
    flag_new_playlist = false;
    song_number --;

    if (song_number < 1)
      song_number = playlist->count();

    playbox->setCurrentItem(song_number-1);
    if(status == KPLAYING)
      setLEDs("OO:OO");
    currplaytime = nbvoice = 0;

    playPB->setOn( TRUE );
    statusLA->setText(i18n("Playing"));

    pipe_int_write(MOTIF_PLAY_FILE);
    pipe_string_write(playlist->at(song_number-1));
    status = KPLAYING;
}


void KMidi::infoslot(){

    if(!logwindow)
	return;

    if(logwindow->isVisible()) {
	logwindow->hide();
	if (meter->isVisible()) resize( extendedsize );
	else resize( regularsize );
	return;
    }

    if (logwindow->height() < 40) logwindow->resize(extendedsize.width(), 40);
    if (meter->isVisible()) {
        resize( extendedsize.width(), extendedsize.height() + logwindow->height() );
	return;
    }

    logwindow->move(0, regularsize.height());
    logwindow->show();
    resize( regularsize.width(), regularsize.height() + logwindow->height() );
}

void KMidi::nextClicked(){

    if (flag_new_playlist) {
	song_number = 0;
	flag_new_playlist = false;
    }
    if(playlist->count() == 0)
	return;
    song_number = (randomplay) ? randomSong() : song_number + 1;
    if(song_number > (int)playlist->count())
      song_number = 1;
    playbox->setCurrentItem(song_number-1);

    if(status == KPLAYING)
      setLEDs("OO:OO");
    currplaytime = nbvoice = 0;

    playPB->setOn( TRUE );
    statusLA->setText(i18n("Playing"));

    pipe_int_write(MOTIF_PLAY_FILE);
    pipe_string_write(playlist->at(song_number-1));
    status = KPLAYING;
}

void KMidi::fwdClicked(){

    if (status != KPLAYING) return;

    pipe_int_write(MOTIF_CHANGE_LOCATOR);
    pipe_int_write((last_sec + 15) * 100);

}

void KMidi::bwdClicked(){

    if (status != KPLAYING) return;

    if( (last_sec -15 ) >= 0){
	pipe_int_write(MOTIF_CHANGE_LOCATOR);
	pipe_int_write((last_sec -15) * 100);
    }
    else{
	pipe_int_write(MOTIF_CHANGE_LOCATOR);
	pipe_int_write(0);

    }
}

void KMidi::invokeWhatsThis(){

    QWhatsThis::enterWhatsThisMode();

}

void KMidi::quitClicked(){

    quitPending = true;
    setLEDs("--:--");
    statusLA->setText("");

    writeconfig();
    pipe_int_write(MOTIF_QUIT);

    if(output_device_open == 0){
      thisapp->processEvents();
      thisapp->flushX();
      usleep(100000);
      thisapp->quit();
    }

}

//void KMidi::closeEvent( QCloseEvent *e ){
//
//    e->ignore();
//    quitClicked();
//}


void KMidi::replayClicked(){

    if(status != KPLAYING)
	return;

    last_status = KNONE;

    if (replayPB->isOn()) {
	looping = true;
	randomplay = false;
        shufflebutton->setOn( FALSE );
	looplabel->setText(i18n("Looping"));
    }
    else{
	looping = false;
	looplabel->setText("");
    }

}

void KMidi::ejectClicked(){

    if(!playlistdlg)
	playlistdlg = new PlaylistEdit("_pldlg", playlist, &current_playlist_num, listplaylists);
    else playlistdlg->redoLists();

    playlistdlg->show();

}

void KMidi::acceptPlaylist(){
      updateUI();
      redoplaylistbox();
      playlistbox->setCurrentItem(current_playlist_num);
      redoplaybox();
      if (status != KPLAYING) setSong(0);
      else flag_new_playlist = true;
      if (status != KPLAYING) timer->start( 200, TRUE );  // single shot
}

void KMidi::PlayMOD(){

    // this method is called "from" ejectClicked once the timer has fired
    // this is done to allow the user interface to settle a bit
    // before playing the mod

    if (!playlist->isEmpty())
	playClicked();


}



void KMidi::aboutClicked()
{
   QColor old_background_color = background_color;

   if (configdlg->exec() == QDialog::Accepted) {
	background_color = configdlg->getData()->background_color;
	led_color = configdlg->getData()->led_color;
	meter->led_color = led_color;
	meter->background_color = background_color;
	if (old_background_color != background_color) {
	    meter->setBackgroundColor(background_color);
	    Panel->reset_panel = 10;
	}
	tooltips = configdlg->getData()->tooltips;
	Panel->max_patch_megs = max_patch_megs = configdlg->getData()->max_patch_megs;
	setColors();
	setToolTips();
   }
}


void KMidi::PlayCommandlineMods(){

    // If all is O.K this method is called precisely once right after
    // start up when the timer fires. We check whether we have mods
    // to play that were specified on the command line.
    // if  there was a driver error we well not stop the timer
    // and the timer will continue to call this routine. We check each
    // time whether the driver is finally ready. This is handy for people
    // like me who usually use NAS but forgot to turn it off.


  timer->changeInterval(1000);

  connect(readtimer, SIGNAL(timeout()),this,SLOT(ReadPipe()));
  readtimer->start(10);

  if(!output_device_open){

    // we couldn't initialize the driver
    // this happens e.g. when I still have NAS running
    // let's blink a bit.

    pipe_int_write(TRY_OPEN_DEVICE);

    if (blink){
      blink = false;
      statusLA->setText("           ");
      modlabel->setText(""); // clear the error message
	}
    else{
      blink = true;
      statusLA->setText(i18n("ERROR"));
      modlabel->setText(i18n("Can't open Output Device."));
      song_count_label->setText( i18n("Song --/--") );
    }


    return;
  }

  modlabel->setText(""); // clear the error message
  song_count_label->setText( i18n("Song --/--") );

  // O.K all clear -- the driver is ready.

  timer->stop();

  disconnect( timer, SIGNAL(timeout()),this, SLOT(PlayCommandlineMods()) );
  connect( timer, SIGNAL(timeout()),this, SLOT(PlayMOD()) );
  thisapp->processEvents();
  thisapp->flushX();

  display_playmode();

  thisapp->processEvents();
  thisapp->flushX();

  //status = KSTOPPED; // is this right? --gl
  //patchbox->setEnabled( TRUE );
  statusLA->setText(i18n("Ready"));
  volChanged(volume);
  //  readtimer->start(10);
  if (showmeterrequest) logoClicked();
  if (showinforequest) infoslot();
  if (lpfilterrequest) filterbutton->setOn(TRUE);
  if (effectsrequest) effectbutton->setOn(TRUE);
  if (interpolationrequest != 1) {
	ich[interpolationrequest]->setChecked( TRUE );
	updateIChecks(interpolationrequest);
  }
  if (stereo_state != 1) {
	rcb1->setChecked( (stereo_state==2) );
	updateRChecks(0);
  }
  if (reverb_state != 1) {
	rcb2->setChecked( (reverb_state==2) );
	updateRChecks(1);
  }
  if (chorus_state != 1) {
	rcb3->setChecked( (chorus_state==2) );
	updateRChecks(2);
  }
  if (verbosity_state != 1) {
	rcb4->setChecked( (verbosity_state==2) );
	updateRChecks(3);
  }

  Panel->max_patch_megs = max_patch_megs;

}

void KMidi::loadplaylist( int which ) {

    QString s;
    if (!listplaylists->count()) s = "default.plist";
    else {
        s = listplaylists->at(which);
        int i = s.findRev('/',-1,TRUE);
        if(i != -1) s = s.right(s.length() -i -1);
    }
    QString defaultlist = locate( "appdata", s);

    playlist->clear();

    if (defaultlist.isEmpty()) {
	defaultlist = locateLocal("appdata", "default.plist");
	QFile f(defaultlist);
	f.open( IO_ReadWrite | IO_Translate|IO_Truncate);
	QString tempstring;

	QStringList list = KGlobal::dirs()->findAllResources("appdata", "*.mid");

	for ( QStringList::Iterator it = list.begin(); it != list.end(); ++it ) {
	    tempstring = *it;
	    playlist->append( *it );
	    tempstring += '\n';
	    f.writeBlock(tempstring,tempstring.length());
        }
	f.close();
	listplaylists->inSort(defaultlist);
	redoplaylistbox();
	current_playlist_num = 0;
	playlistbox->setCurrentItem(current_playlist_num);
    }
    else {
	QFile f(defaultlist);

	f.open( IO_ReadOnly | IO_Translate);

	char buffer[1024];
	QString tempstring;

	while(!f.atEnd()){
	  QFileInfo file;

	  buffer[0] = (char) 0;
	  f.readLine(buffer,511);

	  tempstring = buffer;
	  tempstring = tempstring.stripWhiteSpace();

	  if (!tempstring.isEmpty()){
	    file.setFile(tempstring);
	    if (file.isReadable())
	      playlist->append(tempstring);
	  }	
	}

	f.close();
    }
    redoplaybox();

}

#define LYRBUFL 20
static int lyric_head = 0, lyric_tail = 0;
static char lyric_buffer[LYRBUFL][2024];
static int lyric_time[LYRBUFL];

void KMidi::ReadPipe(){

    static int last_buffer_state = 100;
    static int last_various_flags = 0;
    static int last_rcheck_flags = 0;
    static int last_loading_blink = 0;
    int rcheck_flags;
    static int last_nbvoice=0;
    int message;

    if (status == KPLAYING) {
	if (currplaytime) currplaytime++;
	else {
	    last_loading_blink++;
	    if (!last_loading_blink) last_loading_blink++;
	    if (last_loading_blink > 50) {
	        last_loading_blink = -25;
	        led[LOADING_LED]->setColor(Qt::black);
	    }
	    else if (last_loading_blink > 0) led[LOADING_LED]->setColor(Qt::yellow);
	}
    }


    if(pipe_read_ready()){

	pipe_int_read(&message);

	switch (message)
	    {
	    case REFRESH_MESSAGE : {
	      /*		printf("REFRESH MESSAGE IS OBSOLETE !!!\n");*/
	    }
	    break;

	    case DEVICE_NOT_OPEN:
	      output_device_open = 0;
	      break;

	    case DEVICE_OPEN:

	      output_device_open = 1;
	      if(have_commandline_midis && output_device_open) {
                playPB->setOn( TRUE );
		volChanged(volume);
		emit play();
	      }
	      break;

	    case TOTALTIME_MESSAGE : {
		int cseconds;
		int minutes,seconds;
		char local_string[20];

		pipe_int_read(&cseconds);
	
		seconds=cseconds/100;
		minutes=seconds/60;
		seconds-=minutes*60;
		sprintf(local_string,"%02i:%02i",minutes,seconds);
		totaltimelabel->setText(local_string);
		/*		printf("GUI: TOTALTIME %s\n",local_string);*/
		max_sec=cseconds;
		nbvoice = currplaytime = 0;
		for (int l=0; l<LYRBUFL; l++) lyric_time[l] = -1;
		lyric_head = 0; lyric_tail = 0;
                led[LYRICS_LED]->setColor(Qt::black);
	    }
	    break;
	
	    case MASTERVOL_MESSAGE: {
		int vol;
	
		pipe_int_read(&vol);
		//volume = vol*100/255;

	    }
	    break;
	
	    case FILENAME_MESSAGE : {
		char filename[255];
		char *pc;

		pipe_string_read(filename);
		/*		printf("RECEIVED: FILENAME_MESSAGE:%s\n",filename);*/
		/* Extract basename of the file */

		pc=strrchr(filename,'/');
		if (pc==NULL)
		    pc=filename;
		else
		    pc++;
	
		fileName = pc;
		updateUI();
	    }
	    break;
	
	    case FILE_LIST_MESSAGE : {
		char filename[255];
		int i, number_of_files;
		/*		printf("RECEIVED: FILE_LIST_MESSAGE\n");*/
	
		/* reset the playing list : play from the start */
		song_number = 1;

		pipe_int_read(&number_of_files);

		if(number_of_files > 0 )
		    playlist->clear();	

		
		QFileInfo file;
		have_commandline_midis = false;

		for (i=0;i<number_of_files;i++)
		  {
		    pipe_string_read(filename);
		    file.setFile(filename);
		    if( file.isReadable()){
		      playlist->append(filename);
		      have_commandline_midis = true;
		    }
		    else{
		      QString string = i18n("%1\nis not readable or doesn't exist.").arg(filename);
		      KMessageBox::sorry(0, string);

		    }
		  }	
		
		if( !have_commandline_midis) {
		  /*don't have cmd line midis to play*/
		  /*or none of them was readable */
		    loadplaylist(current_playlist_num);
		}
    		else redoplaybox();

		if(playlist->count() > 0){
	
		    fileName = playlist->at(0);
		    int index = fileName.findRev('/',-1,TRUE);
		    if(index != -1)
			fileName = fileName.right(fileName.length() -index -1);
		    if(output_device_open)
		      updateUI();

		}
		if(have_commandline_midis && output_device_open) {
                  playPB->setOn( TRUE );
		  volChanged(volume);
		  emit play();
		}

	    }
	    break;
	
	    case PATCH_CHANGED_MESSAGE :
		nbvoice = currplaytime = 0;
	    break;

	    case NEXT_FILE_MESSAGE :
	    case PREV_FILE_MESSAGE :
	    case TUNE_END_MESSAGE :{

	      /*		printf("RECEIVED: NEXT/PREV/TUNE_MESSAGE\n");*/
	
		/* When a file ends, launch next if auto_next toggle */
		/*	    if ((message==TUNE_END_MESSAGE) &&
			    !XmToggleButtonGetState(auto_next_option))
			    return;
			    */
		nbvoice = currplaytime = 0;
		for (int l=0; l<LYRBUFL; l++) lyric_time[l] = -1;
		lyric_head = 0; lyric_tail = 0;
                led[LYRICS_LED]->setColor(Qt::black);

		if(starting_up){
		    led[STATUS_LED]->setColor(Qt::green);
		    starting_up = false;
		    return;
		}

		setLEDs("OO:OO");
		if(randomplay){
		    randomPlay();
		    return;
		}

		if(looping){
		    status = KNEXT;
      		    playPB->setOn( TRUE );
		    playClicked();
		    return;
		}

		/*
		if (message==PREV_FILE_MESSAGE)
		    song_number--;
		else {
		    song_number++;
		}
		*/

		if (flag_new_playlist)
		    {
			if (playlist->count()) song_number = 0;
			flag_new_playlist = false;
		    }
		else if (song_number < 1)
		    {
			song_number = 1;
		    }
	
	
		if (song_number > (int)playlist->count() )
		    {
			song_number = 1 ;
			setLEDs("--:--");
			looplabel->setText("");
			statusLA->setText(i18n("Ready"));
    			nbvoice = currplaytime = 0;
			QString str;
			str.sprintf(i18n("Song: --/%02d"),playlist->count());
			song_count_label->setText(str);
			modlabel->setText("");
			totaltimelabel->setText("--:--");
			playPB->setOn( FALSE );
			status = KSTOPPED;
			return;
		    }
		else{
		    status = KPLAYING;
      		    playPB->setOn( TRUE );
		    nextClicked();
		}
	

	    }
	    break;

	    case CURTIME_MESSAGE : {
		int cseconds;

		/*		printf("RECEIVED: CURTIME_MESSAGE\n");*/
		pipe_int_read(&cseconds);
		pipe_int_read(&nbvoice);


		currplaytime = cseconds + 1;
	
	    }
	    break;
	    /*	
	    case NOTE_MESSAGE : {
		int channel;
		int note;
	
		pipe_int_read(&channel);
		pipe_int_read(&note);
		printf("NOTE chn%i %i\n",channel,note);
	    }
	    break;
	
	    case    PROGRAM_MESSAGE :{
		int channel;
		int pgm;
	
		pipe_int_read(&channel);
		pipe_int_read(&pgm);
		printf("NOTE chn%i %i\n",channel,pgm);
	    }
	    break;
	
	    case VOLUME_MESSAGE : {
		int channel;
		int volume;
	
		pipe_int_read(&channel);
		pipe_int_read(&volume);
		printf("VOLUME= chn%i %i \n",channel, volume);
	    }
	    break;
	
	
	    case EXPRESSION_MESSAGE : {
		int channel;
		int express;
	
		pipe_int_read(&channel);
		pipe_int_read(&express);
		printf("EXPRESSION= chn%i %i \n",channel, express);
	    }
	    break;
	
	    case PANNING_MESSAGE : {
		int channel;
		int pan;
	
		pipe_int_read(&channel);
		pipe_int_read(&pan);
		printf("PANNING= chn%i %i \n",channel, pan);
	    }
	    break;
	
	    case  SUSTAIN_MESSAGE : {
		int channel;
		int sust;
	
		pipe_int_read(&channel);
		pipe_int_read(&sust);
		printf("SUSTAIN= chn%i %i \n",channel, sust);
	    }
	    break;
	
	    case  PITCH_MESSAGE : {
		int channel;
		int bend;
	
		pipe_int_read(&channel);
		pipe_int_read(&bend);
		printf("PITCH BEND= chn%i %i \n",channel, bend);
	    }
	    break;
	
	    case RESET_MESSAGE : {
		printf("RESET_MESSAGE\n");
	    }
	    break;
	    */

	    case CLOSE_MESSAGE : {
		//	    printf("CLOSE_MESSAGE\n");
		writeconfig();
		thisapp->quit();
	    }
	    break;
	
	    case CMSG_MESSAGE : {
		char strmessage[2024];
		int type = 0, message_time = 0;

		pipe_int_read(&type);
		/*		printf("got a CMSG_MESSAGE of type %d\n", type);*/

		if (type == CMSG_LYRIC) {
			pipe_int_read(&message_time);
			pipe_string_read(strmessage);
			strcpy(lyric_buffer[lyric_head], strmessage);
			lyric_time[lyric_head] = message_time;
			lyric_head++;
			if (lyric_head == LYRBUFL) lyric_head = 0;
			else led[LYRICS_LED]->setColor( QColor(205, 133, 63) ); //peru
			break;
		}

		pipe_string_read(strmessage);
		/*		printf("RECEIVED %s %d\n",strmessage,strlen(strmessage));*/

		logwindow->insertStr(QString(strmessage));

	    }
	    break;

	      	case CMSG_ERROR : {
		char strmessage[2024];
	
		pipe_string_read(strmessage);
		//logwindow->insertStr(strmessage);
		logwindow->insertStr(QString(strmessage));


		}

	    break;
	    default:
		fprintf(stderr,"Kmidi: unknown message %i\n",message);
	
	    }
    }

    if (status == KPLAYING && currplaytime) {
		int cseconds;
		int  sec,seconds, minutes;
		char local_string[20];

		cseconds = currplaytime - 1;
		sec=seconds=cseconds/100;
	
		/* To avoid blinking */
		if (sec!=last_sec)
		    {
			minutes=seconds/60;
			seconds-=minutes*60;
		
			sprintf(local_string,"%02d:%02d",
				minutes, seconds);
			//		    printf("GUI CURTIME %s\n",local_string);	
			setLEDs(local_string);
	
		    }

		last_sec=sec;

		while (lyric_head != lyric_tail && lyric_time[lyric_tail] >= currplaytime+meterfudge) {
			logwindow->insertStr(QString(lyric_buffer[lyric_tail]));
			lyric_time[lyric_tail] = -1;
			lyric_tail++;
			if (lyric_tail == LYRBUFL) lyric_tail = 0;
		}
    }

    if (status != last_status) {
	QColor c;
	switch (status) {
	    case KNONE:		c = Qt::green; break;
	    case KPLAYING:	c = Qt::green; break;
	    case KSTOPPED:	c = Qt::red; break;
	    case KLOOPING:	c = Qt::blue; break;
	    case KFORWARD:	c = Qt::cyan; break;
	    case KBACKWARD:	c = Qt::darkCyan; break;
	    case KNEXT:		c = Qt::magenta; break;
	    case KPREVIOUS:	c = Qt::darkBlue; break;
	    case KPAUSED:	c = Qt::yellow; break;
	    default:		c = led_color; break;
	}
	if (status != KPLAYING) nbvoice = 0;
	else if (looping) c = Qt::blue;
	else if (randomplay) c = Qt::magenta;
	last_status = status;
	led[STATUS_LED]->setColor(c);
	led[STATUS_LED]->setColor(Qt::green);
    }

    if (last_loading_blink && currplaytime) {
	led[LOADING_LED]->setColor(Qt::black);
	last_loading_blink = 0;
    }

    if (Panel->buffer_state < last_buffer_state - 5 || Panel->buffer_state > last_buffer_state + 5) {
        led[BUFFER_LED]->setColor( QColor(250 - 2*Panel->buffer_state, 150, 50 ) );
	last_buffer_state = Panel->buffer_state;
    }
    rcheck_flags = reverb_state << 4 + chorus_state;

    if (Panel->various_flags != last_various_flags || rcheck_flags != last_rcheck_flags) {
	last_various_flags = Panel->various_flags;
	//cspline interpolation
	if (last_various_flags & 1) led[CSPLINE_LED]->setColor(Qt::black);
        else led[CSPLINE_LED]->setColor(Qt::darkCyan);
	//reverberation
	if (last_various_flags & 2) led[REVERB_LED]->setColor(Qt::black);
        else {
	    if (reverb_state & 2) led[REVERB_LED]->setColor(Qt::blue);
	    else if (reverb_state & 1) led[REVERB_LED]->setColor(Qt::darkBlue);
            else led[REVERB_LED]->setColor(Qt::black);
	}
	//chorus
	if (last_various_flags & 4) led[CHORUS_LED]->setColor(Qt::black);
        else {
	    if (chorus_state & 2) led[CHORUS_LED]->setColor(Qt::yellow);
	    else if (chorus_state & 1) led[CHORUS_LED]->setColor(Qt::darkYellow);
            else led[CHORUS_LED]->setColor(Qt::black);
	}
	last_rcheck_flags = rcheck_flags;
    }
    if (nbvoice != last_nbvoice) {
	int quant = current_voices / 6;
	if (nbvoice && !(last_nbvoice)) led[11]->setColor(Qt::cyan);
	if (!nbvoice && last_nbvoice) led[11]->setColor(Qt::black);
	if (nbvoice > quant && !(last_nbvoice > quant)) led[12]->setColor(Qt::cyan);
	if (nbvoice < quant && !(last_nbvoice < quant)) led[12]->setColor(Qt::black);
	if (nbvoice > 2*quant && !(last_nbvoice > 2*quant)) led[13]->setColor(Qt::cyan);
	if (nbvoice < 2*quant && !(last_nbvoice < 2*quant)) led[13]->setColor(Qt::black);
	if (nbvoice > 3*quant && !(last_nbvoice > 3*quant)) led[14]->setColor(Qt::cyan);
	if (nbvoice < 3*quant && !(last_nbvoice < 3*quant)) led[14]->setColor(Qt::black);
	if (nbvoice > 4*quant && !(last_nbvoice > 4*quant)) led[15]->setColor(Qt::cyan);
	if (nbvoice < 4*quant && !(last_nbvoice < 4*quant)) led[15]->setColor(Qt::black);
	if (nbvoice > 5*quant && !(last_nbvoice > 5*quant)) led[16]->setColor(Qt::cyan);
	if (nbvoice < 5*quant && !(last_nbvoice < 5*quant)) led[16]->setColor(Qt::black);
	last_nbvoice = nbvoice;
    }

}


void KMidi::readconfig(){

    // let's set the defaults

    randomplay = false;
    /*	int randomplayint = 0;*/

    //////////////////////////////////////////

    config=KApplication::kApplication()->config();
    config->setGroup("KMidi");

    volume = config->readNumEntry("Volume", 40);
    current_voices = config->readNumEntry("Polyphony", DEFAULT_VOICES);
    meterfudge = config->readNumEntry("MeterAdjust", 0);
    tooltips = config->readBoolEntry("ToolTips", TRUE);
    current_playlist_num = config->readNumEntry("Playlist", 0);
    showmeterrequest = config->readBoolEntry("ShowMeter", TRUE);
    showinforequest = config->readBoolEntry("ShowInfo", TRUE);
    infowindowheight = config->readNumEntry("InfoWindowHeight", 80);
    lpfilterrequest = config->readBoolEntry("Filter", FALSE);
    effectsrequest = config->readBoolEntry("Effects", TRUE);
    interpolationrequest = config->readNumEntry("Interpolation", 2);
    max_patch_megs = config->readNumEntry("MegsOfPatches", 60);
    stereo_state = config->readNumEntry("StereoNotes", 1);
    reverb_state = config->readNumEntry("EchoNotes", 1);
    chorus_state = config->readNumEntry("DetuneNotes", 1);
    verbosity_state = config->readNumEntry("Verbosity", 1);

    QColor defaultback = black;
    QColor defaultled = QColor(107,227,88);

    background_color = config->readColorEntry("BackColor",&defaultback);	
    led_color = config->readColorEntry("LEDColor",&defaultled);

}

void KMidi::writeconfig(){


    config=KApplication::kApplication()->config();
	
    ///////////////////////////////////////////////////

    config->setGroup("KMidi");

    if(tooltips)
	config->writeEntry("ToolTips", 1);
    else
	config->writeEntry("ToolTips", 0);

    config->writeEntry("Volume", volume);
    config->writeEntry("Polyphony", current_voices);
    config->writeEntry("BackColor", background_color);
    config->writeEntry("LEDColor", led_color);
    config->writeEntry("MeterAdjust", meterfudge);
    config->writeEntry("Playlist", current_playlist_num);
    config->writeEntry("ShowMeter", meter->isVisible());
    config->writeEntry("ShowInfo", logwindow->isVisible());
    config->writeEntry("InfoWindowHeight", logwindow->height());
    config->writeEntry("Filter", filterbutton->isOn());
    config->writeEntry("Effects", effectbutton->isOn());
    config->writeEntry("Interpolation", interpolationrequest);
    config->writeEntry("MegsOfPatches", max_patch_megs);
    config->writeEntry("StereoNotes", stereo_state);
    config->writeEntry("EchoNotes", reverb_state);
    config->writeEntry("DetuneNotes", chorus_state);
    config->writeEntry("Verbosity", verbosity_state);
    config->sync();
}


void KMidi::setColors(){

    QPalette cp = palette();

    backdrop->setBackgroundColor(background_color);
    //logwindow->setBackgroundColor(background_color);

/*			 foreground, button, light, dark, mid, text, base   */


    QColorGroup norm( led_color, background_color, cp.normal().light(), cp.normal().dark(),
	cp.normal().mid(), led_color, background_color );
    QColorGroup dis( yellow, background_color, cp.disabled().light(), cp.disabled().dark(),
	cp.disabled().mid(), yellow, background_color );
    QColorGroup act( background_color, led_color, cp.active().light(), cp.active().dark(),
	cp.active().mid(), background_color, led_color );

    QPalette np(norm, dis, act);

//    setPalette( np );
    logwindow->setPalette( np );
    statusLA->setPalette( np );
    looplabel->setPalette( np );

    //    titleLA->setPalette( np );
    propertiesLA->setPalette( np );
    properties2LA->setPalette( np );
    volLA->setPalette( np );
    //    speedLA->setPalette( np );
    totaltimelabel->setPalette( np );
    modlabel->setPalette( np );
    song_count_label->setPalette( np );
				/* normal, disabled, active */
//    patchbox->setPalette( np );
//    playbox->setPalette( np );

    for (int u = 0; u<=4;u++){
	trackTimeLED[u]->setLEDoffColor(background_color);
	trackTimeLED[u]->setLEDColor(led_color,background_color);
    }

}

void KMidi::display_playmode(){

  if(!play_mode)
    return;

    QString properties;
    QString properties2;

    properties = i18n("%1 bit %2 %3")
		       .arg(play_mode->encoding & PE_16BIT ? 16:8)
		       .arg(play_mode->encoding & PE_SIGNED ? i18n("sig"):i18n("usig"))
		       .arg(play_mode->encoding & PE_ULAW ? i18n("uLaw"):i18n("Linear"));

    properties2 = i18n("%1 Hz %2")
		       .arg(play_mode->rate)
		       .arg(play_mode->encoding & PE_MONO ? i18n("Mono"):i18n("Stereo"));

    propertiesLA->setText(properties);
    properties2LA->setText(properties2);

}

void KMidi::updateUI(){

  QString filenamestr;
  filenamestr = fileName;

  filenamestr = filenamestr.replace(QRegExp("_"), " ");

  if(filenamestr.length() > 4){
  if(filenamestr.right(4) == QString(".mid") ||filenamestr.right(4) == QString(".MID"))
    filenamestr = filenamestr.left(filenamestr.length()-4);
  }


    modlabel->setText(filenamestr);
    QString songstr;

    if(playlist->count() >0)
	songstr.sprintf(i18n("Song: %02d/%02d"),song_number,playlist->count());
    else
	songstr = i18n("Song: --/--");

    song_count_label->setText( songstr );

    display_playmode();

}

void KMidi::resizeEvent(QResizeEvent *e){

    int h = (e->size()).height();
    int lwheight;

    if (kmidiframe->size() != e->size()) kmidiframe->resize( e->size() );

    if (meter->isVisible()) lwheight = h - extendedsize.height();
    else lwheight = h - regularsize.height();

    if (lwheight > 10 && !logwindow->isVisible()) {
        logwindow->resize(extendedsize.width(), lwheight);
	if (meter->isVisible()) logwindow->move(0, extendedsize.height());
	else  logwindow->move(0, regularsize.height());
	logwindow->show();
	return;
    }

    if (h < regularsize.height() + 10) {
	if (logwindow->isVisible()) logwindow->hide();
	if (meter->isVisible()) {
	    meter->hide();
	    patchbox->hide();
	    playbox->hide();
	    playlistbox->hide();
    	    rcb1->hide();
    	    rcb2->hide();
    	    rcb3->hide();
    	    rcb4->hide();
	    effectbutton->hide();
	    voicespin->hide();
	    meterspin->hide();
	    filterbutton->hide();
	}
	return;
    }

    if (h < extendedsize.height() + 10 && meter->isVisible() && logwindow->isVisible()) {
	logwindow->hide();
	return;
    }
    if (lwheight > 0 && logwindow->isVisible()) logwindow->resize(extendedsize.width(), lwheight);

}



#include "kmidi.moc"

extern "C" {

    void createKApplication(int *argc, char **argv){
	thisapp = new KApplication(*argc, argv, "kmidi");
    }
    
    int Launch_KMidi_Process(int _pipenumber){

	pipenumber = _pipenumber;

	kmidiframe = new KMidiFrame( "_kmidiframe" );
       	/* thisapp->enableSessionManagement(true); */
	KWM::setWmCommand(kmidiframe->winId(),"_kmidiframe");
	//thisapp->setTopWidget(kmidi);
	//kmidi->setCaption(kapp->makeStdCaption( i18n("Midi Player") ));
	///kmidiframe->setCaption( i18n("Midi Player") );
	// it's hard to drag the window around with no bar
	//KWM::setDecoration(kmidi->winId(), KWM::tinyDecoration);
	//kmidiframe->setFontPropagation( QWidget::AllChildren );
        //thisapp->setFont(default_font, TRUE);
        //kmidiframe->setFont(default_font, TRUE);
	kmidiframe->show();
	thisapp->exec();

	return 0;
    }

}


/*
static void cleanup( int sig ){

  catchSignals();

}

void catchSignals()
{
signal(SIGHUP, cleanup);	
signal(SIGINT, cleanup);		
signal(SIGTERM, cleanup);		
//	signal(SIGCHLD, cleanup);

signal(SIGABRT, cleanup);
//	signal(SIGUSR1, un_minimize);
signal(SIGALRM, cleanup);
signal(SIGFPE, cleanup);
signal(SIGILL, cleanup);
//	signal(SIGPIPE, cleanup);
signal(SIGQUIT, cleanup);
//	signal(SIGSEGV, cleanup);

#ifdef SIGBUS
signal(SIGBUS, cleanup);
#endif
#ifdef SIGPOLL
signal(SIGPOLL, cleanup);
#endif
#ifdef SIGSYS
signal(SIGSYS, cleanup);
#endif
#ifdef SIGTRAP
signal(SIGTRAP, cleanup);
#endif
#ifdef SIGVTALRM
signal(SIGVTALRM, cleanup);
#endif
#ifdef SIGXCPU
signal(SIGXCPU, cleanup);
#endif
#ifdef SIGXFSZ
signal(SIGXFSZ, cleanup);
#endif
}

*/
