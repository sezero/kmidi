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

#include <dcopclient.h>
#include <kconfig.h>
#include <klocale.h>
#include <kmessagebox.h>

#include <khelpmenu.h>
#include <kfiledialog.h>

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


const char * whatsthis_image[] = {
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

#include "playlist.h"
#include "midiapplication.h"
#include <kstddirs.h>
#include <kglobal.h>
#include <kwm.h>
#include "kmidiframe.h"



#include "config.h"
#include "output.h"
#include "instrum.h"
#include "playmidi.h"
#include "constants.h"
#include "ctl.h"
#include "table.h"


static int currplaytime = 0;
static int meterfudge = 0;

enum midistatus{ KNONE, KPLAYING, KSTOPPED, KLOOPING, KFORWARD,
		 KBACKWARD, KNEXT, KPREVIOUS,KPAUSED};

static midistatus status, last_status;
static int nbvoice = 0;
static int settletime = 0;

extern int menubarheight;
extern bool menubarisvisible;

extern QSize requestedframesize;
extern int fixframesizecount;

//QFont default_font("Helvetica", 10, QFont::Bold);


extern MidiApplication * thisapp;
extern KMidiFrame *kmidiframe;
KMidi *kmidi;
Table		*channelwindow;

KMidi::KMidi( QWidget *parent, const char *name )
    : QWidget( parent, name )
{

    playlistdlg = NULL;
    randomplay = false;
    singleplay = false;
    looping = false;
    driver_error = false;
    last_status = status = KNONE;
    flag_new_playlist = false;
    song_number = 1;
    max_sec = 0;
    timestopped = 0;
    fastrewind = 0;
    fastforward = 0;
    current_voices = DEFAULT_VOICES;
    stereo_state = echo_state = detune_state = verbosity_state = 1;
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
    errorlist = new QStrList(TRUE);
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
    connect( fwdPB, SIGNAL(pressed()), SLOT(fwdPressed()) );
    connect( fwdPB, SIGNAL(released()), SLOT(fwdReleased()) );
    connect( bwdPB, SIGNAL(pressed()), SLOT(bwdPressed()) );
    connect( bwdPB, SIGNAL(released()), SLOT(bwdReleased()) );
    connect( quitPB, SIGNAL(clicked()), SLOT(quitClicked()) );	
    connect( whatbutton, SIGNAL(clicked()), SLOT(invokeWhatsThis()) );	
    connect( replayPB, SIGNAL(clicked()), SLOT(replayClicked()) );
    connect( ejectPB, SIGNAL(clicked()), SLOT(ejectClicked()) );
    connect( volSB, SIGNAL(valueChanged(int)), SLOT(volChanged(int)));
    connect( timeSB, SIGNAL(sliderReleased()), SLOT(timeStart()));
    connect( timeSB, SIGNAL(sliderPressed()), SLOT(timeStop()));
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

    myresize( regularsize );

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
	QToolTip::add( fwdPB, 		i18n("Fast Forward") );
	QToolTip::add( bwdPB, 		i18n("Rewind") );
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
	QToolTip::add( timeSB, 		i18n("Time Control") );

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
	QToolTip::remove( timeSB );

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

void KMidi::timeStop()
{
    if (status != KPLAYING) return;
    timestopped = max_sec;
}

void KMidi::timeStart()
{
    if (status != KPLAYING) return;
    int time = timeSB->value();

    timestopped = 3;
    pipe_int_write(MOTIF_CHANGE_LOCATOR);
    pipe_int_write( max_sec * time / 100);

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
	static int lastvol[MAXDISPCHAN],
	 lastamp[MAXDISPCHAN],
	 last_sustain[MAXDISPCHAN],
	 last_expression[MAXDISPCHAN],
	 last_panning[MAXDISPCHAN],
	 last_reverberation[MAXDISPCHAN],
	 last_chorusdepth[MAXDISPCHAN],
	 last_volume[MAXDISPCHAN],
	 meterpainttime = 0;
	int ch, x1, y1, slot, amplitude, notetime, chnotes;

	if (currplaytime + meterfudge < meterpainttime || Panel->reset_panel == 10) {
		erase();
		for (ch = 0; ch < MAXDISPCHAN; ch++) {
			lastvol[ch] =
			lastamp[ch] =
			last_expression[ch] =
			last_reverberation[ch] =
			last_chorusdepth[ch] =
			last_volume[ch] =
			last_sustain[ch] = 0;
			last_panning[ch] = 64;
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
		  int tmp;
		  notetime = Panel->ctime[slot][ch];
		  if (notetime != -1 && notetime <= meterpainttime) {
		    if (chnotes < Panel->notecount[slot][ch])
		        chnotes = Panel->notecount[slot][ch];
		    if (amplitude < Panel->ctotal[slot][ch])
			amplitude = Panel->ctotal[slot][ch];
		    last_sustain[ch] = Panel->ctotal_sustain[slot][ch];
		    if (ch < 16) {
		      tmp = Panel->expression[slot][ch];
		      if (last_expression[ch] != tmp) {
		        last_expression[ch] = tmp;
			channelwindow->c_flags[ch] = Panel->c_flags[ch];
			channelwindow->setExpression(ch, tmp);
		      }
		      tmp = Panel->reverberation[slot][ch];
		      if (last_reverberation[ch] != tmp) {
		        last_reverberation[ch] = tmp;
			channelwindow->setReverberation(ch, tmp);
		      }
		      tmp = Panel->chorusdepth[slot][ch];
		      if (last_chorusdepth[ch] != tmp) {
		        last_chorusdepth[ch] = tmp;
			channelwindow->setChorusDepth(ch, tmp);
		      }
		      tmp = Panel->volume[slot][ch];
		      if (last_volume[ch] != tmp) {
		        last_volume[ch] = tmp;
			channelwindow->c_flags[ch] = Panel->c_flags[ch];
			channelwindow->setVolume(ch, tmp);
		      }
		      tmp = Panel->panning[slot][ch];
		      if (tmp < 128 && last_panning[ch] != tmp) {
		        last_panning[ch] = tmp;
			channelwindow->setPanning(ch, tmp);
		      }
		    }
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
    what->add(aboutPB, i18n("Open up or close lower<br>\npart of panel with<br>\n"
			"channel display and<br>\nother controls") );
    iy += 2 * HEIGHT;

    ix = 0;
    ejectPB = makeButton( ix, iy, WIDTH/2, HEIGHT, "Eject" );
    what->add(ejectPB, i18n("open up the play list editor"));
    infobutton = makeButton( ix +WIDTH/2, iy, WIDTH/2, HEIGHT, "info" );
    what->add(infobutton, i18n("open up or close<br>\nthe display of info about the<br>\nsong being played"));
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
    what->add(configurebutton, i18n("open up or close<br>\nthe configuration window"));


    timeSB = new QSlider( 0, 100, 1,  0, QSlider::Horizontal,
			 this, "tSlider" );
    timeSB->setGeometry( WIDTH , 7*HEIGHT/2 + HEIGHT/2, 2*SBARWIDTH/3, HEIGHT/2 );
    timeSB->setTracking(FALSE);
    what->add(timeSB, i18n("set playing time elapsed"));

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
    what->add(volSB, i18n("Adjust the volume here.<br>\n"
	"(Keep it pretty low, and<br>\n"
	"use <i>Kmix</i> to adjust<br>\n"
	"the mixer volumes, if<br>\n"
	"you want it louder.)"));

    iy = 0;
    ix = WIDTH + SBARWIDTH;

    playPB = makeButton( ix, iy, WIDTH, HEIGHT, i18n("Play/Pause") );
    playPB->setToggleButton( TRUE );
    what->add(playPB, i18n("If playing, pause,<br>\nelse start playing."));

    totalwidth = ix + WIDTH;

    iy += HEIGHT;
    stopPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Stop") );
    what->add(stopPB, i18n("Stop playing this song."));

    ix += WIDTH/2 ;
    replayPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Replay") );
    replayPB->setToggleButton( TRUE );
    what->add(replayPB, i18n("Keep replaying<br>\n the same song."));

    ix = WIDTH + SBARWIDTH;
    iy += HEIGHT;
    bwdPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Bwd") );
    what->add(bwdPB, i18n("Go backwards to play<br>\nan earlier part of the song."));

    ix += WIDTH/2;
    fwdPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Fwd") );
    what->add(fwdPB, i18n("Go forwards to play<br>\na later part of the song."));

    ix = WIDTH + SBARWIDTH;
    iy += HEIGHT;
    //leds here
    iy += HEIGHT/2;

    prevPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Prev") );
    what->add(prevPB, i18n("Play the previous song<br>\n on the play list."));

    ix += WIDTH/2 ;
    nextPB = makeButton( ix, iy, WIDTH/2 , HEIGHT, i18n("Next") );
    what->add(nextPB, i18n("Play the next song<br>\n on the play list."));


    ix = WIDTH + WIDTH/2;
    iy += HEIGHT;

    QString polyled = i18n("These leds show<br>\nthe number of notes<br>\nbeing played.");

    for (int i = 0; i < 17; i++) if (i < 7 || i > 10) {
        led[i] = new KLed(led_color, this);
        led[i]->setLook(KLed::Sunken);
        led[i]->setShape(KLed::Rectangular);
        //led[i]->setGeometry(WIDTH/8 + i * WIDTH/4,3*HEIGHT+HEIGHT/6, WIDTH/6, HEIGHT/5);
        //led[i]->setGeometry(WIDTH/8 + i * WIDTH/4,3*HEIGHT+HEIGHT/8, WIDTH/6, HEIGHT/4);
        led[i]->setGeometry(WIDTH/8 + i * WIDTH/4,3*HEIGHT+HEIGHT/10, WIDTH/6, HEIGHT/3);
	led[i]->setColor(Qt::black);
	if (i>10) what->add(led[i], polyled);
    }

    what->add(led[STATUS_LED], i18n("The <i>status</i> led is<br>\n"
	"<b>red</b> when KMidi is<br>\n"
	"stopped, <b>yellow</b> when<br>\n"
	"paused, otherwise:<br>\n"
	"<b>blue</b> when looping on<br>\n"
	"the same song<br>\n"
	"<b>magenta</b> when selecting<br>\n"
	"songs randomly<br>\n"
	"or <b>green</b> when KMidi is<br>\n"
	"playing or ready to play." ));

    what->add(led[LOADING_LED], i18n("The <i>loading</i> led<br>\n"
	"blinks <b>yellow</b> when<br>\n"
	"patches are being<br>\n"
	"loaded from the hard drive." ));

    what->add(led[LYRICS_LED], i18n("The <i>lyrics</i> led<br>\n"
	"lights up when a<br>\n"
	"synchronized midi text<br>\n"
	"event is being displayed<br>\n"
	"in the <i>info</i> window." ));

    what->add(led[BUFFER_LED], i18n("The <i>buffer</i> led<br>\n"
	"is orangeish when there is<br>\n"
	"danger of a dropout, but<br>\n"
	"greenish when all is ok." ));

    what->add(led[CSPLINE_LED], i18n("The <i>interpolation</i> led<br>\n"
	"is on when c-spline or<br>\nLaGrange interpolation<br>\n"
	"is being used for resampling." ));

    what->add(led[REVERB_LED], i18n("The <i>echo</i> led<br>\n"
	"is on when KMidi is<br>\n"
	"not too busy to generate<br>\n"
	"extra echo notes<br>\n"
	"for reverberation effect.<br>\n"
	"It's bright when you've<br>\n"
	"asked for extra reverberation." ));
    what->add(led[CHORUS_LED], i18n("The <i>detune</i> led<br>\n"
	"is on when KMidi is<br>\n"
	"not too busy to generate<br>\n"
	"extra detuned notes<br>\n"
	"for chorusing effect.<br>\n"
	"It's bright when you've<br>\n"
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
    what->add(ich[3], i18n("Resample using c-spline<br>\ninterpolation with<br>\nthe low pass filter."));
    connect( ichecks, SIGNAL(clicked(int)), SLOT(updateIChecks(int)) );

    regularheight = iy;

    meter = new MeterWidget ( this, "soundmeter" );
    meter->setBackgroundColor( background_color );
    meter->led_color = led_color;
    meter->background_color = background_color;
    meter->setGeometry(ix,iy,SBARWIDTH - WIDTH/2, 3* HEIGHT - 1);
    what->add(meter, i18n("The display shows notes<br>\n being played on the<br>\n16 or 32 midi channels."));
    meter->hide();

    ix = 0;
    iy += 3*HEIGHT;

    int CHHEIGHT = 17 * (HEIGHT/2);
    channelwindow = new Table( totalwidth, CHHEIGHT, this, "channels", Qt::WRepaintNoErase );
    channelwindow->setFont( QFont( "helvetica", 10, QFont::Normal) );

    channelwindow->move(ix, iy);
    channelwindow->hide();

    iy += CHHEIGHT;
    extendedheight = iy;

    extendedsize = QSize (totalwidth, extendedheight);

    topbarssize = QSize (totalwidth, menubarheight);

    logwindow = new LogWindow(this,"logwindow");
    logwindow->setGeometry(ix,extendedheight, totalwidth, infowindowheight);
    what->add(logwindow, i18n("Here is information<br>\n"
	"extracted from the midi<br>\nfile currently being played."));
    logwindow->resize(totalwidth, infowindowheight);
    logwindow->hide();


    regularsize = QSize (totalwidth, regularheight);
    setFixedWidth(totalwidth);

    // Choose patch set
    ix = 0;
    iy = regularheight;
    patchbox = new QComboBox( FALSE, this );
    patchbox->setGeometry(ix, iy, WIDTH + WIDTH/2, HEIGHT);
    what->add(patchbox, i18n("Choose a patchset.<br>\n"
	"(You can add patchsets by<br>\n"
	"downloading them from somewhere<br>\n"
	"and editing the file<br>\n"
	"$KDEDIR/share/apps/kmidi/<br>\nconfig/timidity.cfg .)"));
    patchbox->setFont( QFont( "helvetica", 10, QFont::Normal) );

    int lx;
    for (lx = 30; lx > 0; lx--) if (cfg_names[lx-1]) break;
    for (int sx = 0; sx < lx; sx++)
	if (cfg_names[sx]) patchbox->insertItem( cfg_names[sx] );
	else patchbox->insertItem( "(none)" );

    patchbox->setCurrentItem(Panel->currentpatchset);
    //setPatch(Panel->currentpatchset);
    connect( patchbox, SIGNAL(activated(int)), SLOT(setPatch(int)) );
    patchbox->hide();

    iy += HEIGHT;
    playbox = new QComboBox( FALSE, this, "song" );
    playbox->setGeometry(ix, iy, WIDTH + WIDTH/2, HEIGHT);
    what->add(playbox, i18n("Select a midi file<br>\nto play from this<br>\nplay list."));
    //playbox->setFont( QFont( "helvetica", 10, QFont::Normal) );
    connect( playbox, SIGNAL(activated(int)), SLOT(setSong(int)) );
    playbox->hide();

    iy += HEIGHT;

    playlistbox = new QComboBox( FALSE, this, "_playlists" );
    playlistbox->setGeometry(ix, iy, WIDTH + WIDTH/2, HEIGHT);
    what->add(playlistbox, i18n("These are the playlist files<br>\n"
	"you've created using<br>\n the <i>playlist editor</i>.<br>\n"
	"Click on one, and its<br>\ncontents will replace<br>\n"
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

    what->add(rcb1, i18n("There are three settings:<br>\n"
	"<b>off</b> for no extra stereo notes<br>\n"
	"<b>no change</b> for normal stereo<br>\n"
	"<b>checked/on</b> for notes panned<br>\n"
	"left and right artificially.") );
    what->add(rcb2, i18n("There are three settings:<br>\n"
	"<b>off</b> for no extra echo notes<br>\n"
	"<b>no change</b> for echoing<br>\n"
	"when a patch specifies<br>\nreverberation, or<br>\n"
	"<b>checked/on</b> for added reverb<br>\n"
	"for all instruments.") );
    what->add(rcb3, i18n("There are three settings:<br>\n"
	"<b>off</b> for no extra detuned notes<br>\n"
	"<b>no change</b> for detuned notes<br>\n"
	"when a patch specifies<br>\nchorusing, or<br>\n"
	"<b>checked/on</b> for added chorusing<br>\n"
	"for all instruments.") );
    what->add(rcb4, i18n("There are three settings:<br>\n"
	"<b>off</b> for only name and lyrics<br>\n"
	"shown in the <i>info</i> window;<br>\n"
	"<b>no change</b> for all text<br>\n"
	"messages to be shown, and<br>\n"
	"<b>checked/on</b> to display also<br>\n"
	"information about patches<br>\n"
	"as they are loaded from disk.") );

    iy += HEIGHT;
    effectbutton = makeButton( ix,          iy, WIDTH/2, HEIGHT, "eff" );
    effectbutton->setToggleButton( TRUE );
    effectbutton->setFont( QFont( "helvetica", 10, QFont::Bold) );
    what->add(effectbutton, i18n("When this button is down,<br>\n"
				 "filters are used for the<br>\n"
				 "<i>midi</i> channel effects<br>\n"
				 "<b>chorus</b>, <b>reverberation</b>,<br>\n"
				 "<b>celeste</b>, and <b>phaser</b>.<br>\n"
				 "When it is off, <b>chorus</b> is<br>\n"
				 "done instead with extra detuned<br>\n"
				 "notes, and <b>reverberation</b> is<br>\n"
				 "done with echo notes, but<br>\n"
				 "the other effects are not done." ));

    connect( effectbutton, SIGNAL(toggled(bool)), SLOT(setEffects(bool)) );
    effectbutton->hide();

    voicespin = new QSpinBox( 1, MAX_VOICES, 1, this, "_spinv" );
    voicespin->setValue(current_voices);
    voicespin->setGeometry( ix +WIDTH/2, iy, WIDTH/2, HEIGHT );
    voicespin->setFont( QFont( "helvetica", 10, QFont::Bold) );
    what->add(voicespin, i18n("Use this to set the maximum<br>\n"
			      "notes that can be played<br>\n"
			      "at one time.  Use a lower<br>\n"
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
    what->add(meterspin, i18n("This setting is a delay<br>\n"
			      "in centiseconds before a played<br>\n"
			      "note is displayed on the<br>\n"
			      "channel meter to the left.<br>\n"
			      "It may help to syncronize the<br>\n"
			      "display with the music."));
    meterspin->hide();

    filterbutton = makeButton( ix +WIDTH/2,     iy, WIDTH/2,   HEIGHT, "filt" );
    filterbutton->setToggleButton( TRUE );
    filterbutton->setFont( QFont( "helvetica", 10, QFont::Bold) );
    what->add(filterbutton, i18n("When this filter button<br>\n"
				 "is on, a low pass filter<br>\n"
				 "is used for drum patches<br>\n"
				 "when they are first loaded<br>\n"
				 "and when the patch itself<br>\n"
				 "requests this.  The filter<br>\n"
				 "is also used for melodic voices<br>\n"
				 "if you have chosed the<br>\n<i>cspline+filter</i><br>\n"
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
    int temp = 0;

    switch (which) {
	case 0:
		// stereo voice
	    check_states = (int)rcb1->state();
	    if (check_states != 2) stereo_state = check_states;
	    temp = stereo_state;
	    break;
	case 1:
		// echo voice
	    check_states = (int)rcb2->state();
	    if (check_states != 2) echo_state = check_states;
	    temp = echo_state;
	    break;
	case 2:
		// detune voice
	    check_states = (int)rcb3->state();
	    if (check_states != 2) detune_state = check_states;
	    temp = detune_state;
	    break;
	case 3:
		// info window verbosity
	    check_states = (int)rcb4->state();
	    if (check_states != 2) verbosity_state = check_states;
	    temp = verbosity_state;
	    break;
	default:
	    return;

    }
    if (check_states == 2 && temp >= 2 && temp <= 15)
	check_states = temp;
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
//if (status == KSTOPPED) printf("KSTOPPED\n");
//else if (status == KPAUSED) printf("KPAUSED\n");
//else if (status == KPLAYING) printf("KPLAYING\n");
//else if (status == KNONE) printf("KNONE\n");

    if (status == KNONE) stopClicked();
    flag_new_playlist = false;
    song_number = 0;
    if (!playlist->count()) {
	//redoplaybox();
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

    if (playlist->at(song_number-1)[0] == '#') { stopClicked(); return; }

    fileName = playbox->text(number);
    if(output_device_open)
      updateUI();

    if(status == KPLAYING)
      setLEDs("OO:OO");
    settletime = fastforward = fastrewind = nbvoice = currplaytime = 0;

    playPB->setOn( TRUE );
    statusLA->setText(i18n("Playing"));

    pipe_int_write(MOTIF_PLAY_FILE);
    pipe_string_write(playlist->at(song_number-1));
    status = KPLAYING;
}

void KMidi::redoplaybox()
{
    QString filenamestr;
    int i, index, errindex;
    int last = playlist->count();

    playbox->clear();

    for (i = 0; i < last; i++) {

	filenamestr = playlist->at(i);
//This doesn't work -- the "#" is not there when playbox redone, for some reason.
	errindex = filenamestr.find('#',0,TRUE);
	index = filenamestr.findRev('/',-1,TRUE);
	if (index == -1 && errindex == 0) index = 0;
	if(index != -1)
	    filenamestr = filenamestr.right(filenamestr.length() -index -1);
	filenamestr = filenamestr.replace(QRegExp("_"), " ");

	if(filenamestr.length() > 4){
	if(filenamestr.right(4) == QString(".mid") ||filenamestr.right(4) == QString(".MID"))
	    filenamestr = filenamestr.left(filenamestr.length()-4);
	}
	if (errindex == 0) filenamestr.insert('#',0);
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
void KMidi::setDry( bool down )
{
    dry_state = down;
    pipe_int_write(  MOTIF_DRY );
    if (down) pipe_int_write( 1 );
    else pipe_int_write( 0 );
}

void KMidi::setReverb( int level )
{
    reverb_state = level;
    pipe_int_write(  MOTIF_REVERB );
    pipe_int_write(level);
}
void KMidi::setChorus( int level )
{
    chorus_state = level;
    pipe_int_write(  MOTIF_CHORUS );
    pipe_int_write(level);
}
void KMidi::setExpressionCurve( int curve )
{
    evs_state = (curve << 8) + (evs_state & 0xff);
    pipe_int_write(  MOTIF_EVS );
    pipe_int_write(evs_state);
}
void KMidi::setVolumeCurve( int curve )
{
    evs_state = (curve << 4) + (evs_state & 0xf0f);
    pipe_int_write(  MOTIF_EVS );
    pipe_int_write(evs_state);
}
void KMidi::setSurround( int yesno )
{
    evs_state = yesno + (evs_state & 0xff0);
    pipe_int_write(  MOTIF_EVS );
    pipe_int_write(evs_state);
}

void KMidi::logoClicked(){

    if(meter->isVisible()){
        enableLowerPanel(false);
	if (logwindow->isVisible()) {
	    logwindow->move(0, regularsize.height());
            myresize( regularsize.width(), regularsize.height() + logwindow->height() );
	}
	else myresize( regularsize );
	return;
    }

    enableLowerPanel(true);

    if (logwindow->isVisible()) {
	logwindow->move(0, extendedsize.height());
        myresize( extendedsize.width(), extendedsize.height() + logwindow->height() );
    }
    else myresize( extendedsize );
}

void KMidi::enableLowerPanel(bool on) {

  fixframesizecount = 25;

  if (on) {
    kmidiframe->menuBar->show();
    menubarisvisible = true;
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
    channelwindow->show();

  }
  else {
    kmidiframe->menuBar->hide();
    menubarisvisible = false;
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
    channelwindow->hide();
  }
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
    settletime = fastforward = fastrewind = nbvoice = currplaytime = 0;
    statusLA->setText(i18n("Playing"));
    looplabel->setText(i18n("Random"));

    int index = randomSong() - 1;
    song_number = index + 1;
    playbox->setCurrentItem(index);
    if (playlist->at(index)[0] == '#') { stopClicked(); return; }
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
    settletime = fastforward = fastrewind = nbvoice = currplaytime = 0;
    statusLA->setText(i18n("Playing"));
    looplabel->setText(i18n("Random"));

    int index = randomSong() - 1;
    song_number = index + 1;
    playbox->setCurrentItem(index);
    if (playlist->at(index)[0] == '#') { stopClicked(); return; }
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
    if (playlist->at(index)[0] == '#') { stopClicked(); return; }
    setLEDs("OO:OO");
    settletime = fastforward = fastrewind = nbvoice = currplaytime = 0;
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
     timeSB->setValue(0);
     randomplay = false;
     shufflebutton->setOn( FALSE );
     //if (status != KPAUSED && status != KSTOPPED) pipe_int_write(MOTIF_STOP);
     if (status != KSTOPPED) pipe_int_write(MOTIF_STOP);
     status = KSTOPPED;
     playPB->setOn( FALSE );
     statusLA->setText(i18n("Ready"));
     looplabel->setText("");
     setLEDs("00:00");
     settletime = fastforward = fastrewind = currplaytime = nbvoice = 0;
}



void KMidi::prevClicked(){

    if (flag_new_playlist) song_number = 2;
    flag_new_playlist = false;
    song_number --;

    if (song_number < 1)
      song_number = playlist->count();

    playbox->setCurrentItem(song_number-1);
    if (playlist->at(song_number-1)[0] == '#') { stopClicked(); return; }
    if(status == KPLAYING)
      setLEDs("OO:OO");
    settletime = fastforward = fastrewind = currplaytime = nbvoice = 0;

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
	if (meter->isVisible()) myresize( extendedsize );
	else myresize( regularsize );
	return;
    }

    if (logwindow->height() < 40) logwindow->resize(extendedsize.width(), 40);
    if (meter->isVisible()) {
        myresize( extendedsize.width(), extendedsize.height() + logwindow->height() );
	return;
    }

    logwindow->move(0, regularsize.height());
    logwindow->show();
    myresize( regularsize.width(), regularsize.height() + logwindow->height() );
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
    if (playlist->at(song_number-1)[0] == '#') { stopClicked(); return; }

    if(status == KPLAYING)
      setLEDs("OO:OO");
    settletime = fastforward = fastrewind = currplaytime = nbvoice = 0;

    playPB->setOn( TRUE );
    statusLA->setText(i18n("Playing"));

    pipe_int_write(MOTIF_PLAY_FILE);
    pipe_string_write(playlist->at(song_number-1));
    status = KPLAYING;
}

void KMidi::fwdPressed(){

    if (status != KPLAYING) return;
    settletime = 5;
    fastforward = 1;
}

void KMidi::fwdReleased(){

    if (status != KPLAYING) return;
    fastforward = 0;
    timestopped = 10;
    settletime = 10;
    pipe_int_write(MOTIF_CHANGE_LOCATOR);
    pipe_int_write( currplaytime );
}


void KMidi::bwdPressed(){

    if (status != KPLAYING) return;
    settletime = 5;
    fastrewind = 1;
}

void KMidi::bwdReleased(){

    if (status != KPLAYING) return;
    fastrewind = 0;
    timestopped = 10;
    settletime = 10;
    pipe_int_write(MOTIF_CHANGE_LOCATOR);
    pipe_int_write( currplaytime );
}


void KMidi::invokeWhatsThis(){

    QWhatsThis::enterWhatsThisMode();

}

void KMidi::quitClicked(){

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

void KMidi::restartPlaybox(){
    redoplaybox();
    if (starting_up) return;
    if (status != KPLAYING) setSong(0);
    else flag_new_playlist = true;
    if (status != KPLAYING) timer->start( 200, TRUE );  // single shot
}

void KMidi::acceptPlaylist(){
    updateUI();
    redoplaylistbox();
    playlistbox->setCurrentItem(current_playlist_num);
    restartPlaybox();
}

void KMidi::PlayMOD(){
    int errcount = 0;
    // this method is called "from" ejectClicked once the timer has fired
    // this is done to allow the user interface to settle a bit
    // before playing the mod

    while (!errorlist->isEmpty()) {
	  errcount++;
	  KMessageBox::sorry(this, QString( errorlist->first() ) );
	  errorlist->removeFirst();
    }

    if (errcount) {
	redoplaybox();
	stopClicked();
	return;
    }

    if (status == KPLAYING) return;

    if (!playlist->isEmpty())
	playClicked();
}

void KMidi::postError(const QString& s) {
//if the message contains a quoted substring, this is the
//name of an unplayable file
    if (s.contains( '\"', TRUE) == 2) {
	QString unplayable;
	int index = s.find('\"', 0, TRUE);
	int sindex = s.findRev('\"', -1, TRUE);
	unplayable = s.mid(index+1, sindex - index -1);

	int i;
        int last = playlist->count();
	if (!unplayable.isEmpty()) for (i = 0; i < last; i++) {
	    if (!QString::compare(unplayable, playlist->at(i))) {
		//mark it
		playlist->at(i)[0] = '#';
	    }
	}
    }
    errorlist->append(s.latin1());
    if (!timer->isActive()) timer->start(200, TRUE);
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


  statusLA->setText(i18n("Ready"));
  volChanged(volume);
  //  readtimer->start(10);
  if (showmeterrequest) logoClicked();
  if (showinforequest) infoslot();

  if (!showmeterrequest && !showinforequest)
	myresize(regularsize);

  if (lpfilterrequest) filterbutton->setOn(TRUE);
  if (effectsrequest) effectbutton->setOn(TRUE);
  if (interpolationrequest != 1) {
	ich[interpolationrequest]->setChecked( TRUE );
	updateIChecks(interpolationrequest);
  }
  if (stereo_state != 1) {
	rcb1->setChecked( (stereo_state>=2) );
	updateRChecks(0);
  }
  if (echo_state != 1) {
	rcb2->setChecked( (echo_state>=2) );
	updateRChecks(1);
  }
  if (detune_state != 1) {
	rcb3->setChecked( (detune_state>=2) );
	updateRChecks(2);
  }
  if (verbosity_state != 1) {
	rcb4->setChecked( (verbosity_state>=2) );
	updateRChecks(3);
  }

  setDry(dry_state);
  setReverb(reverb_state);
  setChorus(chorus_state);

  pipe_int_write(  MOTIF_EVS );
  pipe_int_write(evs_state);

  Panel->max_patch_megs = max_patch_megs;

  pipe_int_write(  MOTIF_PATCHSET );
  pipe_int_write( Panel->currentpatchset );

  thisapp->processEvents();
  thisapp->flushX();

  display_playmode();

  thisapp->processEvents();
  thisapp->flushX();

  if (have_commandline_midis>0 && output_device_open && status != KPLAYING)
	setSong(0);
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
    static int diff_currplaytime = 0;
    int rcheck_flags;
    static int last_nbvoice=0;
    int message;

    if (fixframesizecount) {
	fixframesizecount--;
	if (!fixframesizecount) {
	    if (kmidiframe->size() != requestedframesize) {
	        if (requestedframesize != QSize(0,0)) {
		    kmidiframe->resize(requestedframesize);
		    fixframesizecount = 25;
		}
	    }
	}
    }

    if (status == KPLAYING) {
	if (currplaytime) {
	    if (diff_currplaytime < 0) diff_currplaytime++;
	    else {
	        currplaytime++;
		if (diff_currplaytime > 0) diff_currplaytime--;
		if (diff_currplaytime > 2) {
	            currplaytime++;
		    diff_currplaytime--;
		}
	    }
	}
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
#if 0
	      if(have_commandline_midis>0 && output_device_open) {
                playPB->setOn( TRUE );
		volChanged(volume);
		emit play();
	      }
#endif
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
		settletime = fastforward = fastrewind = nbvoice = currplaytime = 0;
		channelwindow->clearChannels();
     		timeSB->setValue(0);
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

/*#define DO_IT_MYSELF*/
		QFileInfo file;
		//have_commandline_midis = 0;

		for (i=0;i<number_of_files;i++)
		  {
		    pipe_string_read(filename);
#ifdef DO_IT_MYSELF
		    file.setFile(filename);
		    if( file.isReadable()){
		//better check if it's a midi file
		      playlist->append(file.absFilePath());
#endif

		      //have_commandline_midis = 1;

#ifdef DO_IT_MYSELF
		    }
		    else postError( i18n("\"%1\"\nis not readable or doesn't exist.").arg(filename) );
#endif

		  }	
		
		if( !have_commandline_midis) {
		  /*don't have cmd line midis to play*/
		  /*or none of them was readable */
		    loadplaylist(current_playlist_num);
		}
#ifdef DO_IT_MYSELF
    		else redoplaybox();
#endif


		if(playlist->count() > 0){
	
		    fileName = playlist->at(0);
		    int index = fileName.findRev('/',-1,TRUE);
		    if(index != -1)
			fileName = fileName.right(fileName.length() -index -1);
		    if(output_device_open)
		      updateUI();

		}
		if(have_commandline_midis>0 && output_device_open) {
                  playPB->setOn( TRUE );
		  volChanged(volume);
		  emit play();
		}


	    }
	    break;
	
	    case PATCH_CHANGED_MESSAGE :
		settletime = fastforward = fastrewind = nbvoice = currplaytime = 0;
	    break;

	    case NEXT_FILE_MESSAGE :
	    case PREV_FILE_MESSAGE :
	    case TUNE_END_MESSAGE :{

	      	/*	printf("RECEIVED: NEXT/PREV/TUNE_MESSAGE\n"); */
	
		/* When a file ends, launch next if auto_next toggle */
		/*	    if ((message==TUNE_END_MESSAGE) &&
			    !XmToggleButtonGetState(auto_next_option))
			    return;
			    */
		settletime = fastforward = fastrewind = nbvoice = currplaytime = 0;
		for (int l=0; l<LYRBUFL; l++) lyric_time[l] = -1;
		lyric_head = 0; lyric_tail = 0;
                led[LYRICS_LED]->setColor(Qt::black);

		if(starting_up){
		    led[STATUS_LED]->setColor(Qt::red);
		    last_status = KNONE;
		    starting_up = false;
		    if( !have_commandline_midis) {
		        loadplaylist(current_playlist_num);
		    }
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
    			settletime = fastforward = fastrewind = nbvoice = currplaytime = 0;
			QString str;
			str.sprintf(i18n("Song: --/%02d"),playlist->count());
			song_count_label->setText(str);
			modlabel->setText("");
			totaltimelabel->setText("--:--");
			playPB->setOn( FALSE );
			status = KSTOPPED;
			return;
		    }
		else if (!singleplay) {
		    status = KPLAYING;
      		    playPB->setOn( TRUE );
		    nextClicked();
		}
		else {
		    singleplay = false;
		    stopClicked();
		}
	

	    }
	    break;

	    case JUMP_MESSAGE : {
		// fprintf(stderr,"RECEIVED: JUMP MESSAGE\n");
	    }
	    break;

	    case CURTIME_MESSAGE : {
		int cseconds;

		/*		printf("RECEIVED: CURTIME_MESSAGE\n");*/
		pipe_int_read(&cseconds);
		pipe_int_read(&nbvoice);

		if (!settletime && !fastforward && !fastrewind)
		    diff_currplaytime = cseconds - currplaytime;
		if (!currplaytime) currplaytime++;
		if (timestopped) {
		    currplaytime = cseconds;
		    diff_currplaytime = 0;
		}

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
	    */
	    case    PROGRAM_MESSAGE :{
		int channel;
		int pgm;
		char progname[100];

		pipe_int_read(&channel);
		pipe_int_read(&pgm);
		pipe_string_read(progname);
		channelwindow->setProgram(channel, pgm, progname,
		    (int)Panel->c_bank[channel], (int)Panel->c_variationbank[channel]);
		//printf("NOTE chn%i %i %s\n",channel,pgm, progname);
	    }
	    break;
	    /*
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
			else led[LYRICS_LED]->setColor( QColor(255, 165, 0) ); //orange1
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
		//logwindow->insertStr(QString(strmessage));
		postError(QString(strmessage));

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
			if (timestopped) timestopped--;
			else timeSB->setValue(100*cseconds/max_sec);

			last_sec=sec;

			if (settletime) settletime--;
	
		    }

		if (fastforward == 1)
		  {
			fastforward = 10;
			if (currplaytime + 220 < max_sec)
				currplaytime += 200;
		  }
		else if (fastforward) fastforward--;
		if (fastrewind == 1)
		  {
			fastrewind = 10;
			if (currplaytime > 221) currplaytime -= 200;
		  }
		else if (fastrewind) fastrewind--;


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
	if (status != KPLAYING) settletime = fastforward = fastrewind = nbvoice = 0;
	else if (looping) c = Qt::blue;
	else if (randomplay) c = Qt::magenta;
	last_status = status;
	led[STATUS_LED]->setColor(c);
    }

    if (last_loading_blink && currplaytime) {
	led[LOADING_LED]->setColor(Qt::black);
	last_loading_blink = 0;
    }

    if (Panel->buffer_state < last_buffer_state - 5 || Panel->buffer_state > last_buffer_state + 5) {
        led[BUFFER_LED]->setColor( QColor(250 - 2*Panel->buffer_state, 150, 50 ) );
	last_buffer_state = Panel->buffer_state;
    }
    rcheck_flags = echo_state << 4 + detune_state;

    if (Panel->various_flags != last_various_flags || rcheck_flags != last_rcheck_flags) {
	last_various_flags = Panel->various_flags;
	//cspline interpolation
	if (last_various_flags & 1) led[CSPLINE_LED]->setColor(Qt::black);
        else led[CSPLINE_LED]->setColor(Qt::darkCyan);
	//reverberation
	if (last_various_flags & 2) led[REVERB_LED]->setColor(Qt::black);
        else {
	    if (!echo_state) led[REVERB_LED]->setColor(Qt::black);
	    else led[REVERB_LED]->setColor(QColor(0,0,5*echo_state+235));
	}
	//chorus
	if (last_various_flags & 4) led[CHORUS_LED]->setColor(Qt::black);
        else {
	    if (!detune_state) led[CHORUS_LED]->setColor(Qt::black);
	    else led[CHORUS_LED]->setColor(QColor(5*echo_state+235,5*echo_state+235,0));
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

void KMidi::set_current_dir(const QString &dir) {

#if 0
   if (!dir.isEmpty()){
    if ( !current_dir.setCurrent(dir)){
      QString str = i18n("Can not enter directory: %1\n").arg(dir);
      KMessageBox::sorry(this, str);
      return;
    }
   }

   current_dir  = QDir::current();
#endif
   current_dir = QDir(dir);
}

void KMidi::readconfig(){
    int e_state, v_state, s_state;

    // let's set the defaults

    randomplay = false;
    /*	int randomplayint = 0;*/

    //////////////////////////////////////////

    config=KApplication::kApplication()->config();
    config->setGroup("KMidi");

    QString str = config->readEntry("Directory");
    if ( !str.isNull() ) set_current_dir(str);
    else set_current_dir(QString(""));

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
    echo_state = config->readNumEntry("EchoNotes", 1);
    reverb_state = config->readNumEntry("Reverb", 0);
    detune_state = config->readNumEntry("DetuneNotes", 1);
    chorus_state = config->readNumEntry("Chorus", 1);
    dry_state = config->readBoolEntry("Dry", FALSE);
    e_state = config->readNumEntry("ExpressionCurve", 1);
    v_state = config->readNumEntry("VolumeCurve", 1);
    s_state = config->readNumEntry("Surround", 1);
    evs_state = (e_state << 8) + (v_state << 4) + s_state;
    verbosity_state = config->readNumEntry("Verbosity", 1);
    Panel->currentpatchset = config->readNumEntry("Patchset", 0);

    QColor defaultback = black;
    QColor defaultled = QColor(107,227,88);

    background_color = config->readColorEntry("BackColor",&defaultback);	
    led_color = config->readColorEntry("LEDColor",&defaultled);

}

void KMidi::writeconfig(){
    int e_state, v_state, s_state;


    config=KApplication::kApplication()->config();
	
    ///////////////////////////////////////////////////

    config->setGroup("KMidi");

    //config->writeEntry("Directory", current_dir.filePath("."));
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
    config->writeEntry("EchoNotes", echo_state);
    config->writeEntry("Reverb", reverb_state);
    config->writeEntry("DetuneNotes", detune_state);
    config->writeEntry("Chorus", chorus_state);
    config->writeEntry("Dry", dry_state);
    e_state = (evs_state >> 8) & 0x0f;
    config->writeEntry("ExpressionCurve", e_state);
    v_state = (evs_state >> 4) & 0x0f;
    config->writeEntry("VolumeCurve", v_state);
    s_state = evs_state & 0x0f;
    config->writeEntry("Surround", s_state);
    config->writeEntry("Verbosity", verbosity_state);
    config->writeEntry("Patchset", Panel->currentpatchset);
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
    channelwindow->setPalette( np );

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

static int bogusresize = 1;

void KMidi::resizeEvent(QResizeEvent *e){

    int h = (e->size()).height();
    int lwheight;


    if (!bogusresize) {
//printf("ignoring resize to h %d\n", h);
        bogusresize--;
	return;
    }
    bogusresize--;

    if (meter->isVisible()) lwheight = h - extendedsize.height();
    else lwheight = h - regularsize.height();

//if (meter->isVisible())
//printf("meter on; h=%d, lwh=%d\n", h, lwheight);
//else
//printf("meter off; h=%d, lwh=%d\n", h, lwheight);

    if (lwheight > 10 && !logwindow->isVisible()) {
	int newy;
	if (meter->isVisible()) newy = extendedsize.height();
	else newy = regularsize.height();
//printf("turning on lw since h is %d\n", lwheight);
        logwindow->resize(extendedsize.width(), lwheight);
	logwindow->move(0, newy);
	logwindow->show();
	return;
    }

    if (h < regularsize.height() + 10) {
//printf("turning off lw since h is %d\n", lwheight);
	if (logwindow->isVisible()) logwindow->hide();
	if (meter->isVisible()) {
	    enableLowerPanel(false);
	}
	return;
    }

    if (h < extendedsize.height() + 10 && meter->isVisible() && logwindow->isVisible()) {
	logwindow->hide();
	return;
    }
    if (lwheight > 0 && logwindow->isVisible()) logwindow->resize(extendedsize.width(), lwheight);

}


void KMidi::myresize(QSize newsize) {

    static int warmingup = 1;

    if (warmingup) bogusresize = 1;
    else bogusresize = -1;

//printf("my resize h %d\n", newsize.height());
    resize(newsize);


    if (warmingup) {
	warmingup = 0;
	return;
    }

    if (menubarisvisible) {
	if (kmidiframe->size() != newsize + topbarssize) {
	    bogusresize = 0;
	    requestedframesize = QSize( newsize.width(), newsize.height() + menubarheight );
//printf("size frame to h = %d + %d\n", newsize.height(), topbarssize.height());
	    kmidiframe->resize(requestedframesize);
	}
    }
    else {
	if (kmidiframe->size() != newsize) {
	    bogusresize = 0;
	    requestedframesize = newsize;
//printf("size frame to h = %d\n", newsize.height());
//damn thing won't let me resize it
	    kmidiframe->resize(newsize);
	}
    }


}

void KMidi::myresize(int w, int h) {
    QSize newsize = QSize(w, h);

    myresize(newsize);
}


#include "kmidi.moc"
