
/* 
	$Id$

    TiMidity -- Experimental MIDI to WAVE converter
    Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    linux_audio.c

    Functions to play sound on the VoxWare audio driver (Linux or FreeBSD)

*/

#define _GNU_SOURCE
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include <sys/ioctl.h> /* new with 1.2.0? Didn't need this under 1.1.64 */
#include <signal.h>
#include <sys/wait.h>


#include "config.h"
#include "output.h"
#include "controls.h"


static int open_output(void); /* 0=success, 1=warning, -1=fatal error */
static void close_output(void);
static void output_data(int32 *buf, uint32 count);
static int driver_output_data(unsigned char *buf, uint32 count);
static void flush_output(void);
static void purge_output(void);
static int output_count(uint32 ct);

/* export the playback mode */

#undef dpm
#define dpm arts_play_mode

PlayMode dpm = {
  DEFAULT_RATE, PE_16BIT|PE_SIGNED,
  -1,
  {0}, /* default: get all the buffer fragments you can */
  "Arts device", 'A',
  "/dev/dsp",
  open_output,
  close_output,
  output_data,
  driver_output_data,
  flush_output,
  purge_output,
  output_count
};


/*************************************************************************/
/* We currently only honor the PE_MONO bit, the sample rate, and the
   number of buffer fragments. We try 16-bit signed data first, and
   then 8-bit unsigned if it fails. If you have a sound device that
   can't handle either, let me know. */

static int output_count(uint32 ct)
{
  return ct;
}

static int driver_output_data(unsigned char *buf, uint32 count)
{
  return write(dpm.fd,buf,count);
}

static void output_data(int32 *buf, uint32 count)
{
  int ocount;

  if (!(dpm.encoding & PE_MONO)) count*=2; /* Stereo samples */
  ocount = (int)count;

  if (ocount) {
    if (dpm.encoding & PE_16BIT)
      {
        /* Convert data to signed 16-bit PCM */
        s32tos16(buf, count);
        ocount *= 2;
      }
    else
      {
        /* Convert to 8-bit unsigned and write out. */
        s32tou8(buf, count);
      }
  }

  b_out(dpm.id_character, dpm.fd, (int *)buf, ocount);
}

static int child_pid;

static void close_output(void)
{
  int status;
  close(dpm.fd);
  kill(child_pid, SIGTERM);
  while(wait(&status) != child_pid)
	    ;
}

static void flush_output(void)
{
  output_data(0, 0);
}

static void purge_output(void)
{
  b_out(dpm.id_character, dpm.fd, 0, -1);
}


static int pipeAppli[2],pipeDispatch[2];
static int fpip_in, fpip_out;	

static void arts_pipe_error(const char *st);


static void arts_pipe_error(const char *st)
{
    fprintf(stderr,"Kmidi:Problem with %s due to:%s\n",
	    st,
	    sys_errlist[errno]);
    exit(1);
}




    /*

    Copyright (C) 2000 Stefan Westerfeld
                       stefan@space.twc.de

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Permission is also granted to link this program with the Qt
    library, treating Qt like a library that normally accompanies the
    operating system kernel, whether or not that is in fact the case.

    */

#include <arts/soundserver.h>
#include <arts/stdsynthmodule.h>

//#include <stdio.h>
//#include <unistd.h>
//#include <fcntl.h>

class Sender :	public ByteSoundProducer_skel,
				public StdSynthModule,
				public IONotify
{
protected:
	//FILE *pfile;
	int pfd;
	bool waiting;
	queue< DataPacket<mcopbyte>* > wqueue;
public:
	//Sender(FILE *input) : pfile(input), waiting(false)
	Sender(int infildes) : pfd(input), waiting(false)
	{
		//pfd = fileno(pfile);
		pfd = infildes;

		int rc = fcntl(pfd, F_SETFL, O_NONBLOCK);
		assert(rc != -1);
	}

	~Sender()
	{
		if(waiting) Dispatcher::the()->ioManager()->remove(this,IOType::read);
	}

	long samplingRate() { return 44100; }
	long channels()     { return 2; }
	//long bits()         { return 16; }
	long bits()         { return 32; }
	//bool finished()     { return (pfile == 0); }
	bool finished()     { return (pfd == 0); }

	void start()
	{
		/*
		 * start streaming
		 */
		//outdata.setPull(8, packetCapacity);
		outdata.setPull(16, packetCapacity);
	}

	enum { packetCapacity = 4096 };
	void handle_eof()
	{
		cout << "EOF" << endl;
		/*
		 * cleanup
		 */
		outdata.endPull();
		//pclose(pfile);
		close(pfd);
		//pfile = 0;
		pfd = 0;

		/*
		 * remove all packets from the wqueue
		 */
		while(!wqueue.empty())
		{
			DataPacket<mcopbyte> *packet = wqueue.front();
			packet->size = 0;
			packet->send();
			wqueue.pop();
		}
		
		/*
		 * go out of the waiting mode
		 */
		if(waiting)
		{
			Dispatcher::the()->ioManager()->remove(this,IOType::read);
			waiting = false;
		}
	}
	void request_outdata(DataPacket<mcopbyte> *packet)
	{
		if(!waiting)
		{
			packet->size = read(pfd, packet->contents, packetCapacity);
			if(packet->size > 0)
			{
				// got something - done here
				packet->send();
				return;
			}
			Dispatcher::the()->ioManager()->watchFD(pfd,IOType::read,this);
			waiting = true;
		}

		wqueue.push(packet);
	}

	void notifyIO(int,int)
	{
		assert(waiting);

		DataPacket<mcopbyte> *packet = wqueue.front();
		packet->size = read(pfd, packet->contents, packetCapacity);
		assert(packet->size >= 0);
		if(packet->size == 0) {
			handle_eof();
			return;
		}
		packet->send();

		wqueue.pop();

		if(wqueue.empty())
		{
			Dispatcher::the()->ioManager()->remove(this,IOType::read);
			waiting = false;
		}
	}
};

static int open_output(void) /* 0=success, 1=warning, -1=fatal error */
{
	Dispatcher dispatcher;
	SimpleSoundServer server(Reference("global:Arts_SimpleSoundServer"));

	if(server.isNull())
	{
		cerr << "Can't connect to sound server" << endl;
		return -1;
	}

	//Sender *sender = new Sender(stdin);
	//Sender *sender = new Sender(fildes[0]);
	//server->attach(sender);
	//sender->start();
	//dispatcher.run();

	int res;
	
	res=pipe(pipeAppli);
	if (res != 0) arts_pipe_error("PIPE_APPLI CREATION");
	
	res=pipe(pipeDispatch);
	if (res != 0) arts_pipe_error("PIPE_DISPATCHER CREATION");
 
	if ((child_pid = fork()) == 0)   /*child*/
	{
	    close(pipeDispatch[1]); 
	    close(pipeAppli[0]);
 
	    fpip_in = pipeDispatch[0];
	    fpip_out = pipeAppli[1];

	    ByteSoundProducer sender = new Sender(fpip_in);
	    server.attach(sender);
	    sender.start();
	    dispatcher.run();
	    server.detach(sender); // not reached

	    exit(0);
	}
	
	close(pipeDispatch[0]);
	close(pipeAppli[1]);
	
	fpip_in = pipeAppli[0];
	fpip_out = pipeDispatch[1];
	dpm.fd = pipeDispatch[1];


	//fcntl(dpm.fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);

	dpm.encoding &= ~(PE_ULAW|PE_BYTESWAP);

	if (dpm.encoding & PE_16BIT) dpm.encoding |= PE_SIGNED;
	else dpm.encoding &= ~PE_SIGNED;

	dpm.encoding &= ~PE_MONO;

	return 0;
}
