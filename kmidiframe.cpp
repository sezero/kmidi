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

//#include <dcopclient.h>
#include <kconfig.h>
#include <klocale.h>
//#include <kmessagebox.h>

#include <khelpmenu.h>
//#include <kfiledialog.h>
#include <kmidifiledlg.h>

#include "kmidi.h"

#include "kmidiframe.h"


int menubarheight = 0;
bool menubarisvisible;

QSize requestedframesize = QSize (0, 0);
int fixframesizecount = 0;

//QFont default_font("Helvetica", 10, QFont::Bold);


extern KMidi *kmidi;
KMidiFrame *kmidiframe;

DockWidget*     dock_widget;

KMidiFrame::KMidiFrame( const char *name ) :
    KTMainWindow( name )
{

    menuBar = new KMenuBar(this);

    QPopupMenu *fileMenu = new QPopupMenu;
    menuBar->insertItem("&File", fileMenu);

    fileMenu->insertItem( i18n("&Open..."), this,
                        SLOT(file_Open()), CTRL+Key_O );
    fileMenu->insertSeparator();
    fileMenu->insertItem( i18n("&Quit"), this, SLOT(quitClick()), CTRL+Key_Q );

    kmidi = new KMidi(this, "_kmidi" );

    view_options = new QPopupMenu();
    CHECK_PTR( view_options );
    view_options->setCheckable( TRUE );
    menuBar->insertItem( i18n("&View"), view_options );
    connect( view_options, SIGNAL(activated(int)), this, SLOT(doViewMenuItem(int)) );
    connect( view_options, SIGNAL(aboutToShow()), this, SLOT(fixViewItems()) );
	m_on_id = view_options->insertItem(  i18n("Meter shown") );
	view_options->setWhatsThis(m_on_id, i18n("When you can see this<br>\n"
				"menu, the channel meter<br>\n"
				"is always turned on.") );
	m_off_id = view_options->insertItem(  i18n("Meter off") );
	view_options->setWhatsThis(m_off_id, i18n("This turns off the channel<br>\n"
				"meter display, and also hides<br>\n"
				"this menu bar.") );
	view_options->insertSeparator();
	i_on_id = view_options->insertItem(  i18n("Info shown") );
	view_options->setWhatsThis(i_on_id, i18n("Shows a window at the<br>\n"
				"bottom with info about the<br>\n"
				"midi file being played.") );
	i_off_id = view_options->insertItem(  i18n("Info off") );
	view_options->setWhatsThis(i_off_id, i18n("Turns off the info window<br>\n"
				"at the bottom.") );
    view_level = new QPopupMenu();
    CHECK_PTR( view_level );
    view_level->setCheckable( TRUE );
    view_options->insertSeparator();
    view_options->insertItem( i18n("Info level"), view_level);
        view_level->insertItem( i18n("Lyrics only") , 100);
	view_level->setWhatsThis(100, i18n("The info window will<br>\n"
				"show only lyrics, if any,<br>\n"
				"and the name of the midi file.") );
        view_level->insertItem( i18n("Normal") , 101);
	view_level->setWhatsThis(101, i18n("The info window shows<br>\n"
				"all displayable midi<br>\n"
				"messages.") );
        view_level->insertItem( i18n("Loading msgs") , 102);
	view_level->setWhatsThis(102, i18n("Also shows new instruments<br>\n"
				"being loaded for playing<br>\n"
				"the next song.") );
        view_level->insertItem( i18n("debug 1") , 103);
	view_level->setWhatsThis(103, i18n("Also shows instrument<br>\n"
				"volume computation.") );
        view_level->insertItem( i18n("debug 2") , 104);
	view_level->setWhatsThis(104, i18n("Shows lots of additional<br>\n"
				"information (probably not useful).") );
    connect( view_level, SIGNAL(activated(int)), this, SLOT(doViewInfoLevel(int)) );
    connect( view_level, SIGNAL(aboutToShow()), this, SLOT(fixInfoLevelItems()) );


    QPopupMenu *editMenu = new QPopupMenu;
    menuBar->insertItem( i18n("&Edit"), editMenu, CTRL+Key_E);
    editMenu->insertItem( i18n("Edit Playlist"), kmidi, SLOT(ejectClicked()), 0, 108);
	editMenu->setWhatsThis(108, i18n("Transfer the current<br>\n"
				"play list to the Playlist Editor<br>\n"
				"and start him up.") );



    stereo_options = new QPopupMenu();
    CHECK_PTR( stereo_options );
    stereo_options->setCheckable( TRUE );
    menuBar->insertItem( i18n("Stereo"), stereo_options );
    connect( stereo_options, SIGNAL(activated(int)), this, SLOT(doStereoMenuItem(int)) );
    connect( stereo_options, SIGNAL(aboutToShow()), this, SLOT(fixStereoItems()) );
	stereo_options->insertItem(  i18n("No stereo patch"), 110 );
	stereo_options->setWhatsThis(110, i18n("Prevents playing the<br>\n"
				"second instrument for those<br>\n"
				"patches that have two<br>\n"
				"(you probably don't want<br>\n"
				"to choose this option).") );
	stereo_options->insertItem(  i18n("Normal stereo") , 111);
	stereo_options->setWhatsThis(111, i18n("Plays both instruments<br>\n"
				"for those (sf2) patches<br>\n"
				"which have two.") );
	stereo_options->insertItem(  i18n("Extra stereo") , 112);
	stereo_options->setWhatsThis(112, i18n("For keyboard instruments,<br>\n"
				"lower notes come from the left<br>\n"
				"For other instruments, position is<br>\n"
				"made a function of note<br>\n"
				"velocity.") );
	stereo_options->insertSeparator();
	stereo_options->insertItem(  i18n("Surround stereo") , 113);
	stereo_options->setWhatsThis(112, i18n("Extra stereo, echo, and<br>" \
				"detuned notes are spread out more<br>" \
				"to left and right.") );

    reverb_options = new QPopupMenu();
    CHECK_PTR( reverb_options );
    reverb_options->setCheckable( TRUE );
    menuBar->insertItem( i18n("Reverb"), reverb_options );
    connect( reverb_options, SIGNAL(activated(int)), this, SLOT(doReverbMenuItem(int)) );
    connect( reverb_options, SIGNAL(aboutToShow()), this, SLOT(fixReverbItems()) );
	reverb_options->insertItem(  i18n("Dry"), 119 );
	reverb_options->setWhatsThis(119, i18n("With the dry setting,<br>\n"
				"after notes are released,<br>\n"
				"they are ended by playing<br>\n"
				"through the ends of their<br>\n"
				"patches (which may cause some<br>\n"
				"clicking).  The wet setting makes<br>\n"
				"notes continues to the ends of<br>\n"
				"their volume envelopes.") );
    reverb_level = new QPopupMenu();
    CHECK_PTR( reverb_level );
    reverb_level->setCheckable( TRUE );
    reverb_options->insertSeparator();
    reverb_options->insertItem( i18n("Reverb level"), reverb_level);
        reverb_level->insertItem( i18n("default") , 160);
	reverb_level->setWhatsThis(160, i18n("The reverberation level<br>\n"
				"is set according to the midi<br>\n"
				"channel setting and what the<br>\n"
				"instrument patch specifies.") );
        reverb_level->insertItem( i18n("midi level  32") , 161);
	reverb_level->setWhatsThis(161, i18n("The reverberation level<br>\n"
				"is set to a minimum level of 32." ) );
        reverb_level->insertItem( i18n("midi level  64") , 162);
	reverb_level->setWhatsThis(162, i18n("The reverberation level<br>\n"
				"is set to a minimum level of 64." ) );
        reverb_level->insertItem( i18n("midi level  96") , 163);
	reverb_level->setWhatsThis(163, i18n("The reverberation level<br>\n"
				"is set to a minimum level of 96." ) );
        reverb_level->insertItem( i18n("midi level 127") , 164);
	reverb_level->setWhatsThis(164, i18n("The reverberation level<br>\n"
				"is set to the maximem level of 127." ) );
    connect( reverb_level, SIGNAL(activated(int)), this, SLOT(doReverbLevel(int)) );
    connect( reverb_level, SIGNAL(aboutToShow()), this, SLOT(fixReverbLevelItems()) );
    reverb_options->insertSeparator();
	reverb_options->insertItem(  i18n("No echo"), 120 );
	reverb_options->setWhatsThis(120, i18n("Prevents playing extra<br>\n"
				"echo notes for reverberation.") );
	reverb_options->insertItem(  i18n("Normal echo") , 121);
	reverb_options->setWhatsThis(121, i18n("Extra echo notes are<br>\n"
					       "played to get the effect<br>\n"
					       "of reverberation.") );
    echo_level = new QPopupMenu();
    CHECK_PTR( echo_level );
    echo_level->setCheckable( TRUE );
    reverb_options->insertSeparator();
    reverb_options->insertItem( i18n("Echo level"), echo_level);
        echo_level->insertItem( i18n("default") , 130);
	echo_level->setWhatsThis(130, i18n("The level for echo notes<br>\n"
				"is set according to the midi<br>\n"
				"channel setting and what the<br>\n"
				"instrument patch specifies.") );
        echo_level->insertItem( i18n("midi level  32") , 131);
	echo_level->setWhatsThis(131, i18n("The echo level<br>\n"
				"is set to a minimum level of 32." ) );
        echo_level->insertItem( i18n("midi level  64") , 132);
	echo_level->setWhatsThis(132, i18n("The echo level<br>\n"
				"is set to a minimum level of 64." ) );
        echo_level->insertItem( i18n("midi level  96") , 133);
	echo_level->setWhatsThis(133, i18n("The echo level<br>\n"
				"is set to a minimum level of 96." ) );
        echo_level->insertItem( i18n("midi level 127") , 134);
	echo_level->setWhatsThis(133, i18n("The echo level<br>\n"
				"is set to the maximem level of 127." ) );
    connect( echo_level, SIGNAL(activated(int)), this, SLOT(doEchoLevel(int)) );
    connect( echo_level, SIGNAL(aboutToShow()), this, SLOT(fixEchoLevelItems()) );


    chorus_options = new QPopupMenu();
    CHECK_PTR( chorus_options );
    chorus_options->setCheckable( TRUE );
    menuBar->insertItem( i18n("Chorus"), chorus_options );
    connect( chorus_options, SIGNAL(activated(int)), this, SLOT(doChorusMenuItem(int)) );
    connect( chorus_options, SIGNAL(aboutToShow()), this, SLOT(fixChorusItems()) );
    chorus_level = new QPopupMenu();
    CHECK_PTR( chorus_level );
    chorus_level->setCheckable( TRUE );
    chorus_options->insertItem( i18n("Chorus level"), chorus_level);
    chorus_level->insertItem( i18n("default") , 170);
    chorus_level->setWhatsThis(170, i18n("The chorus level<br>\n"
					 "is set according to the midi<br>\n"
					 "channel setting and what the<br>\n"
					 "instrument patch specifies.") );
    chorus_level->insertItem( i18n("midi level  32") , 171);
    chorus_level->setWhatsThis(171, i18n("The choruslevel<br>\n"
					 "is set to a minimum level of 32." ) );
    chorus_level->insertItem( i18n("midi level  64") , 172);
    chorus_level->setWhatsThis(172, i18n("The choruslevel<br>\n"
					 "is set to a minimum level of 64." ) );
    chorus_level->insertItem( i18n("midi level  96") , 173);
    chorus_level->setWhatsThis(173, i18n("The choruslevel<br>\n"
					 "is set to a minimum level of 96." ) );
    chorus_level->insertItem( i18n("midi level 127") , 174);
    chorus_level->setWhatsThis(174, i18n("The choruslevel<br>\n"
					 "is set to the maximum level of 127." ) );
    connect( chorus_level, SIGNAL(activated(int)), this, SLOT(doChorusLevel(int)) );
    connect( chorus_level, SIGNAL(aboutToShow()), this, SLOT(fixChorusLevelItems()) );
    chorus_options->insertSeparator();
    chorus_options->insertItem(  i18n("No detune"), 140 );
    chorus_options->setWhatsThis(140, i18n("Prevents playing extra<br>\n"
					   "detuned notes for chorus effect.") );
    chorus_options->insertItem(  i18n("Normal detune") , 141);
    chorus_options->setWhatsThis(141, i18n("Extra detuned notes are<br>\n"
					   "played to get the effect<br>\n"
					   "of chorusing") );
    detune_level = new QPopupMenu();
    CHECK_PTR( detune_level );
    detune_level->setCheckable( TRUE );
    chorus_options->insertSeparator();
    chorus_options->insertItem( i18n("Detune level"), detune_level);
    detune_level->insertItem( i18n("default") , 150);
    detune_level->setWhatsThis(150, i18n("The level for detuned notes<br>\n"
					 "is set according to the midi<br>\n"
					 "channel setting and what the<br>\n"
					 "instrument patch specifies for"
					 "chorus level.") );
    detune_level->insertItem( i18n("midi level  32") , 151);
    detune_level->setWhatsThis(151, i18n("The detuning level<br>\n"
					 "is set to a minimum level of 32." ) );
    detune_level->insertItem( i18n("midi level  64") , 152);
    detune_level->setWhatsThis(152, i18n("The detuning level<br>\n"
					 "is set to a minimum level of 64." ) );
    detune_level->insertItem( i18n("midi level  96") , 153);
    detune_level->setWhatsThis(153, i18n("The detuning level<br>\n"
					 "is set to a minimum level of 96." ) );
    detune_level->insertItem( i18n("midi level 127") , 154);
    detune_level->setWhatsThis(154, i18n("The detuning level<br>\n"
					 "is set to the maximum level of 127." ) );
    connect( detune_level, SIGNAL(activated(int)), this, SLOT(doDetuneLevel(int)) );
    connect( detune_level, SIGNAL(aboutToShow()), this, SLOT(fixDetuneLevelItems()) );

    volume_options = new QPopupMenu();
    CHECK_PTR( volume_options );
    //volume_options->setCheckable( TRUE );
    menuBar->insertItem( i18n("Volume"), volume_options );
    //connect( volume_options, SIGNAL(activated(int)), this, SLOT(doVolumeMenuItem(int)) );
    volume_curve = new QPopupMenu();
    CHECK_PTR( volume_curve );
    volume_curve->setCheckable( TRUE );
    volume_options->insertItem( i18n("Volume Curve"), volume_curve);
    connect( volume_curve, SIGNAL(activated(int)), this, SLOT(doVolumeCurve(int)) );
    connect( volume_curve, SIGNAL(aboutToShow()), this, SLOT(fixVolumeCurveItems()) );
        volume_curve->insertItem( i18n("linear") , 180);
	volume_curve->setWhatsThis(180, i18n("The midi volume controller<br>" \
				"changes the volume linearly." ) );
        volume_curve->insertItem( i18n("exp 4") , 181);
	volume_curve->setWhatsThis(181, i18n("The midi volume controller<br>" \
				"changes the volume exponentially." ) );
        volume_curve->insertItem( i18n("exp 6") , 182);
	volume_curve->setWhatsThis(182, i18n("The midi volume controller<br>" \
				"changes the volume exponentially." ) );
    volume_options->insertSeparator();
    expression_curve = new QPopupMenu();
    CHECK_PTR( expression_curve );
    expression_curve->setCheckable( TRUE );
    volume_options->insertItem( i18n("Expression Curve"), expression_curve);
    connect( expression_curve, SIGNAL(activated(int)), this, SLOT(doExpressionCurve(int)) );
    connect( expression_curve, SIGNAL(aboutToShow()), this, SLOT(fixExpressionCurveItems()) );
        expression_curve->insertItem( i18n("linear") , 190);
	expression_curve->setWhatsThis(190, i18n("The midi expression controller<br>" \
				"changes the expression linearly." ) );
        expression_curve->insertItem( i18n("exp 4") , 191);
	expression_curve->setWhatsThis(191, i18n("The midi expression controller<br>" \
				"changes the expression exponentially." ) );
        expression_curve->insertItem( i18n("exp 6") , 192);
	expression_curve->setWhatsThis(192, i18n("The midi expression controller<br>" \
				"changes the expression exponentially." ) );

    menuBar->insertSeparator();

    QString aboutapp = i18n("KDE midi file player\n\n"
                     "A software synthesizer for playing\n"
                     "midi songs using Tuukka Toivonen's\n"
                     "TiMidity");

    QPopupMenu *about = helpMenu(aboutapp);
    menuBar->insertItem( i18n("&Help"), about);

    setMenu(menuBar);

    menuBar->hide();
    menubarheight = menuBar->heightForWidth(90+220+90);
    menubarisvisible = false;

    //kmidi = new KMidi(this, "_kmidi" );
    setView(kmidi,TRUE);

    docking = true;
    dock_widget = new DockWidget(this, "dockw");
        if(docking){
        dock_widget->show();
    }

}

KMidiFrame::~KMidiFrame(){
}

//void KMidiFrame::resizeEvent(QResizeEvent *e){
//    int h = (e->size()).height();
//    int w = (e->size()).width();
//
//printf("frame resize %d x %d\n", w, h);
//    if (e->size() != requestedframesize)
//	resize(requestedframesize);
//}

bool KMidiFrame::queryClose() {

    kmidi->quitClicked();
    return true;
}

void KMidiFrame::quitClick(){

    kmidi->quitClicked();
}

void KMidiFrame::file_Open() {
#if 0
    QStringList files;
    int newones = 0;
    char mbuff[5];

    files = KFileDialog::getOpenFileNames(QString::null, QString::null, this);

printf("file count %d\n", files.count());
    for (QStringList::Iterator i=files.begin(); i!=files.end(); ++i) {
printf("file %s\n", (*i).ascii());
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

	    kmidi->playlist->insert(0, *i);
	    newones++;
    }

    if (newones) {
        kmidi->redoplaybox();
	kmidi->setSong(0);
    }
#endif

#if 0
    //QString filename=KFileDialog::getOpenFileURL(QString::null, QString::null,this);
    //QString filename = KFileDialog::getOpenFileName(QString::null, QString::null, this);
    QString filename = KMidiFileDlg::getOpenFileName(QString::null, QString::null, this);
    if (!filename.isNull())
    {
	kmidi->playlist->insert(0, filename);
	kmidi->restartPlaybox();
        //kmidi->redoplaybox();
	//kmidi->resetSong();
    }
#endif

    QString dpath = kmidi->current_dir.absPath();
    KMidiFileDlg::getOpenDialog(dpath, QString::null, this);
}
void KMidiFrame::doViewMenuItem(int id) {
    if (id == m_off_id) kmidi->logoClicked();
    else if (id == i_on_id || id == i_off_id) kmidi->infoslot();
}

void KMidiFrame::fixViewItems() {
    view_options->setItemChecked( m_on_id, true);
    view_options->setItemChecked( m_off_id, false);
    view_options->setItemChecked( i_on_id, kmidi->logwindow->isVisible());
    view_options->setItemChecked( i_off_id, !kmidi->logwindow->isVisible());
}
void KMidiFrame::doViewInfoLevel(int id) {
    if (id >= 100 && id <= 104 && (id-100 != kmidi->verbosity_state) ) {
	if (id == 100) kmidi->rcb4->setChecked(false);
	else if (id == 101) kmidi->rcb4->setNoChange();
	else if (id >= 102) kmidi->rcb4->setChecked(true);
	kmidi->verbosity_state = id-100;
	kmidi->updateRChecks(3);
    }
}
void KMidiFrame::fixInfoLevelItems() {
    view_level->setItemChecked( 100, kmidi->verbosity_state == 0);
    view_level->setItemChecked( 101, kmidi->verbosity_state == 1);
    view_level->setItemChecked( 102, kmidi->verbosity_state == 2);
    view_level->setItemChecked( 103, kmidi->verbosity_state == 3);
    view_level->setItemChecked( 104, kmidi->verbosity_state == 4);
}
void KMidiFrame::doStereoMenuItem(int id) {
    if (id >= 110 && id <= 112 && (id-110 != kmidi->stereo_state) ) {
	if (id == 110) kmidi->rcb1->setChecked(false);
	else if (id == 111) kmidi->rcb1->setNoChange();
	else if (id == 112) kmidi->rcb1->setChecked(true);
	kmidi->updateRChecks(0);
    }
    else if (id == 113) {
        if (( kmidi->evs_state  & 0x0f ) == 1) kmidi->setSurround(0);
	else kmidi->setSurround(1);
    }
}
void KMidiFrame::fixStereoItems() {
    stereo_options->setItemChecked( 110, kmidi->stereo_state == 0);
    stereo_options->setItemChecked( 111, kmidi->stereo_state == 1);
    stereo_options->setItemChecked( 112, kmidi->stereo_state == 2);
    stereo_options->setItemChecked( 113, ( kmidi->evs_state  & 0x0f ) == 1);
}
void KMidiFrame::doReverbMenuItem(int id) {
    if (id == 119) kmidi->setDry(!kmidi->dry_state);
    else if (id >= 120 && id <= 121 && (id-120 != kmidi->echo_state) ) {
	if (id == 120) kmidi->rcb2->setChecked(false);
	else if (id == 121) kmidi->rcb2->setNoChange();
	kmidi->updateRChecks(1);
    }
}
void KMidiFrame::fixReverbItems() {
    reverb_options->setItemChecked( 119, kmidi->dry_state);
    reverb_options->setItemChecked( 120, kmidi->echo_state == 0);
    reverb_options->setItemChecked( 121, kmidi->echo_state == 1);
}
void KMidiFrame::doReverbLevel(int id) {
    kmidi->setReverb(id - 160);
}
void KMidiFrame::doEchoLevel(int id) {
    if (id == 130 && kmidi->echo_state >= 2) {
	//kmidi->rcb2->setChecked(false);
	kmidi->rcb2->setNoChange();
	kmidi->updateRChecks(1);
    }
    else if (kmidi->echo_state != id-129) {
	kmidi->rcb2->setChecked(true);
	kmidi->echo_state = id-129;
	kmidi->updateRChecks(1);
    }
}
void KMidiFrame::fixEchoLevelItems() {
    echo_level->setItemChecked( 130, kmidi->echo_state < 2);
    echo_level->setItemChecked( 131, kmidi->echo_state == 2);
    echo_level->setItemChecked( 132, kmidi->echo_state == 3);
    echo_level->setItemChecked( 133, kmidi->echo_state == 4);
    echo_level->setItemChecked( 134, kmidi->echo_state == 5);
}
void KMidiFrame::fixReverbLevelItems() {
    reverb_level->setItemChecked( 160, kmidi->reverb_state == 0);
    reverb_level->setItemChecked( 161, kmidi->reverb_state == 1);
    reverb_level->setItemChecked( 162, kmidi->reverb_state == 2);
    reverb_level->setItemChecked( 163, kmidi->reverb_state == 3);
    reverb_level->setItemChecked( 164, kmidi->reverb_state == 4);
}
void KMidiFrame::doChorusMenuItem(int id) {
    if (id >= 140 && id <= 141 && (id-140 != kmidi->detune_state) ) {
	if (id == 140) kmidi->rcb3->setChecked(false);
	else if (id == 141) kmidi->rcb3->setNoChange();
	kmidi->updateRChecks(2);
    }
}
void KMidiFrame::fixChorusItems() {
    chorus_options->setItemChecked( 140, kmidi->detune_state == 0);
    chorus_options->setItemChecked( 141, kmidi->detune_state == 1);
}
void KMidiFrame::doChorusLevel(int id) {
    kmidi->setChorus(id - 170);
}
void KMidiFrame::doDetuneLevel(int id) {
    if (id == 150 && kmidi->detune_state >= 2) {
	kmidi->rcb3->setNoChange();
	kmidi->updateRChecks(2);
    }
    else if (kmidi->detune_state != id-149) {
	kmidi->rcb3->setChecked(true);
	kmidi->detune_state = id-149;
	kmidi->updateRChecks(2);
    }
}
void KMidiFrame::fixChorusLevelItems() {
    chorus_level->setItemChecked( 170, kmidi->chorus_state == 0);
    chorus_level->setItemChecked( 171, kmidi->chorus_state == 1);
    chorus_level->setItemChecked( 172, kmidi->chorus_state == 2);
    chorus_level->setItemChecked( 173, kmidi->chorus_state == 3);
    chorus_level->setItemChecked( 174, kmidi->chorus_state == 4);
}
void KMidiFrame::fixDetuneLevelItems() {
    detune_level->setItemChecked( 150, kmidi->detune_state < 2);
    detune_level->setItemChecked( 151, kmidi->detune_state == 2);
    detune_level->setItemChecked( 152, kmidi->detune_state == 3);
    detune_level->setItemChecked( 153, kmidi->detune_state == 4);
    detune_level->setItemChecked( 154, kmidi->detune_state == 5);
}
void KMidiFrame::doVolumeCurve(int id) {
    kmidi->setVolumeCurve(id - 180);
}
void KMidiFrame::fixVolumeCurveItems() {
    int v_state = (kmidi->evs_state >> 4) & 0x0f;
    volume_curve->setItemChecked( 180, v_state == 0);
    volume_curve->setItemChecked( 181, v_state == 1);
    volume_curve->setItemChecked( 182, v_state == 2);
}
void KMidiFrame::doExpressionCurve(int id) {
    kmidi->setExpressionCurve(id - 190);
}
void KMidiFrame::fixExpressionCurveItems() {
    int e_state = (kmidi->evs_state >> 8) & 0x0f;
    expression_curve->setItemChecked( 190, e_state == 0);
    expression_curve->setItemChecked( 191, e_state == 1);
    expression_curve->setItemChecked( 192, e_state == 2);
}

#include "kmidiframe.moc"

