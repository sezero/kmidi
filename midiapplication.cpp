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

#include <kuniqueapp.h>

#include "kmidi.h"
#include "midiapplication.h"
#include "kmidiframe.h"

#include <kstddirs.h>
#include <kglobal.h>
#include <klocale.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>

#include "config.h"
#include "output.h"
#include "instrum.h"
#include "playmidi.h"
//#include "constants.h"
#include "controls.h"
#include "ctl.h"


MidiApplication * thisapp;
extern KMidiFrame *kmidiframe;
extern KMidi *kmidi;

int pipenumber;

static KCmdLineOptions options[] = 
{
   { "o <file>", I18N_NOOP("Output to another file (or device)."), 0 },
   { "O <mode>", I18N_NOOP("Select output mode and format (see below for list)."), 0 },
   { "s <freq>", I18N_NOOP("Set sampling frequency to f (Hz or kHz)."), 0 },
   { "a", I18N_NOOP("Enable the antialiasing filter."), 0 },
   { "f", I18N_NOOP("Enable fast decay mode."), 0 },
   { "d", I18N_NOOP("Enable dry mode."), 0 },
   { "p <n>", I18N_NOOP("Allow n-voice polyphony."), 0 },
   { "A <n>", I18N_NOOP("Amplify volume by n percent (may cause clipping)."), 0 },
   { "C <n>", I18N_NOOP("Set ratio of sampling and control frequencies."), 0 },
   { "# <n>", I18N_NOOP("Select patch set n."), 0 },
   { "E", I18N_NOOP("Effects filter."), 0 },
   { "L <dir>", I18N_NOOP("Append dir to search path."), 0 },
   { "c <file>", I18N_NOOP("Read extra configuration file."), 0 },
   { "I <n>", I18N_NOOP("Use program n as the default."), 0 },
   { "P <file>", I18N_NOOP("Use patch file for all programs."), 0 },
   { "D <n>", I18N_NOOP("Play drums on channel n."), 0 },
   { "Q <n>", I18N_NOOP("Ignore channel n."), 0 },
   { "R <n>", I18N_NOOP("Reverb options (1)(+2)(+4) [7]."), 0 },
   { "k <n>", I18N_NOOP("Resampling interpolation option (0-3) [1]."), 0 },
   { "r <n>", I18N_NOOP("Max ram to hold patches (in megabytes) [60]."), 0 },
   { "X <n>", I18N_NOOP("Midi expression is linear (0) or exponential (1-2) [1]."), 0 },
   { "V <n>", I18N_NOOP("Midi volume is linear (0) or exponential (1-2) [1]."), 0 },
   { "F", I18N_NOOP("Enable fast panning."), 0 },
   { "U", I18N_NOOP("Unload instruments from memory between MIDI files."), 0 },
   { "i <letter>", I18N_NOOP("Select user interface (letter=d(umb)/n(curses)/s(lang))."), 0 },
   { "B <n>", I18N_NOOP("Set number of buffer fragments."), 0 },
   { 0, 0, 0 } // End of options.
};

MidiApplication::MidiApplication(int &argc, char *argv[], const QCString &appName)
  : KUniqueApplication(argc, argv, appName)
  //: KUniqueApplication()
{
//fprintf(stderr,"2: argc=%d argv[1]={%s}\n", argc, (argc>0)? argv[1] : "none");

    KAboutData about( appName, I18N_NOOP("KMidi"), "1.3alpha");

    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions( options );

    KUniqueApplication::addCmdLineOptions();
}

bool MidiApplication::process(const QCString &fun, const QByteArray &data,
		        QCString &replyType, QByteArray &replyData)
{
  QDataStream stream(data, IO_ReadOnly);
  QDataStream stream2(replyData, IO_WriteOnly);
  QString res;
  int exitcode;

  if (fun == "newInstance(QValueList<QCString>)") {
    QValueList<QCString> arg;
    stream >> arg;
    newInstance(arg);
    replyType = "int";
    exitcode = 0;
    stream2 << exitcode;
    return true;
  } else if (fun == "play(QString)") {
    QString arg;
    stream >> arg;
    replyType = "void";
    kmidi->stopClicked();
    kmidi->playlist->clear();
    kmidi->playlist->append(arg);
    kmidi->redoplaybox();
    kmidi->singleplay = true;
    kmidi->setSong(0);
    return true;
  } else {
    fprintf(stderr,"unknown function call %s to MidiApplication::process()\n", fun.data());
    return false;
  }
}
/*#define DO_IT_MYSELF*/

int MidiApplication::newInstance(QValueList<QCString> params)
{
#ifndef DO_IT_MYSELF
    char mbuff[5];
    int newones = 0;
    QValueList<QCString>::Iterator it = params.begin();
    QFileInfo file;

//printf("count %d [%s]\n", (*it).count(), (*it).data());
    it++; // skip program name


    if (kmidi) {

        for (; it != params.end(); it++) {
//printf("NI:[%s]\n", (*it).data());

	    file.setFile(*it);
	    if (!file.isReadable()) continue;

            QFile f(*it);
            if (!f.open( IO_ReadOnly )) continue;
            if (f.readBlock(mbuff, 4) != 4) {
//printf("couldn't read 4 bytes\n");
		f.close();
		continue;
            }
            mbuff[4] = '\0';
            if (strcmp(mbuff, "MThd")) {
//printf("is not midi file\n");
		f.close();
		continue;
            }
            f.close();

	    file.setFile(*it);
	    //kmidi->playlist->insert(0, *it);
	    kmidi->playlist->append(file.absFilePath());
	    newones++;
        }
 
        if (newones) {
	    kmidi->restartPlaybox();
        }
    }
#endif
    return 0;
}



int createKApplication(int *argc, char **argv) {

       int deref = *argc;

       if (!MidiApplication::start(deref, &*argv, "kmidi")) {
	    return 0;
       }
	thisapp = new MidiApplication(*argc, argv, "kmidi");

	KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

	return 1;
}
    
int Launch_KMidi_Process(int _pipenumber) {

	pipenumber = _pipenumber;

	if (thisapp->isRestored()) {
	   int n = 1;
	   while (KTMainWindow::canBeRestored(n)) {
	      (new KMidiFrame)->restore(n);
	      n++;
	   }
	}
	else {
	   kmidiframe = new KMidiFrame( "_kmidiframe" );
	   //// KWM::setWmCommand(kmidiframe->winId(),"_kmidiframe");
	   kmidiframe->setCaption( i18n("Midi Player") );
	   //kmidiframe->setCaption( QString::null );
	   //kmidiframe->setFontPropagation( QWidget::AllChildren );
           //thisapp->setFont(default_font, TRUE);
           //kmidiframe->setFont(default_font, TRUE);
	}

	kmidiframe->show();
	thisapp->exec();

	return 0;
}


#include "midiapplication.moc"
