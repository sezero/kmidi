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
//#include <math.h>

#include <kuniqueapp.h>

#include "kmidi.h"
#include "midiapplication.h"
#include "kmidiframe.h"

#include <kstddirs.h>
#include <kglobal.h>
#include <kwm.h>
#include <klocale.h>

extern "C" {

#include "config.h"
#include "output.h"
#include "instrum.h"
#include "playmidi.h"
#include "constants.h"
#include "ctl.h"

}

MidiApplication * thisapp;
extern KMidiFrame *kmidiframe;
extern KMidi *kmidi;

int pipenumber;

MidiApplication::MidiApplication(int &argc, char *argv[], const QCString &appName)
  : KUniqueApplication(argc, argv, appName)
{
//fprintf(stderr,"2: argc=%d argv[1]={%s}\n", argc, (argc>0)? argv[1] : "none");
#if 0
  if (isRestored())
    RESTORE(TopLevel)
  else 
    toplevel = new TopLevel();
  
  setMainWidget(toplevel);
  toplevel->resize(640,400);
#endif
}

bool MidiApplication::process(const QCString &fun, const QByteArray &data,
		        QCString &replyType, QByteArray &replyData)
{
  QDataStream stream(data, IO_ReadOnly);
  QDataStream stream2(replyData, IO_WriteOnly);
  QString res;
  int exitcode;

  if (fun == "doIt(int)") {
    int arg;
    stream >> arg;
    res = "self->doIt(arg)";
    stream2 << res;
    replyType = "QString";
    return true;
  } else if (fun == "newInstance(QValueList<QCString>)") {
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
    fprintf(stderr,"playing %s\n", arg.ascii());
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

int MidiApplication::newInstance(QValueList<QCString> params)
{
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
		f.close();
		continue;
            }
            mbuff[4] = '\0';
            if (strcmp(mbuff, "MThd")) {
		f.close();
		continue;
            }
            f.close();

	    //kmidi->playlist->insert(0, *it);
	    kmidi->playlist->append(file.absFilePath());
	    newones++;
        }
 
        if (newones) {
	    kmidi->restartPlaybox();
        }
    }

    return 0;
}



extern "C" {

int createKApplication(int *argc, char **argv) {

       int deref = *argc;

       //if (!MidiApplication::start(*argc, &*argv, "kmidi")) {
       if (!MidiApplication::start(deref, &*argv, "kmidi")) {
//fprintf(stderr,"n: argc=%d *argc=%d\n", argc, *argc);
	    return 0;
       }
//fprintf(stderr,"1: argc=%d argv[1]={%s}\n", *argc, (*argc>0)? argv[1] : "none");
	//thisapp = new MidiApplication(*argc, argv, "kmidi");
	//thisapp = new MidiApplication(*argc, &*argv, "kmidi");
	thisapp = new MidiApplication(*argc, argv, "kmidi");
//fprintf(stderr,"3: argc=%d argv[1]={%s}\n", *argc, (*argc>0)? argv[1] : "none");



	return 1;
   }
    
   int Launch_KMidi_Process(int _pipenumber){

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
	KWM::setWmCommand(kmidiframe->winId(),"_kmidiframe");
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

}

#include "midiapplication.moc"
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
