/*

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

*/
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#if defined(__linux__) || defined(__FreeBSD__) || defined(__bsdi__)
#ifndef AU_LINUX
#define AU_LINUX
#endif
#endif

#ifdef __WIN32__
#include <windows.h>
#define rindex strrchr
extern int optind;
extern char *optarg;
int getopt(int, char **, char *);
#else
#include <unistd.h>
#endif

#include <errno.h>
#include "config.h"
#include "common.h"
#include "instrum.h"
#include "playmidi.h"
#include "readmidi.h"
#include "output.h"
#include "controls.h"
#include "tables.h"
#include "version.h"

int free_instruments_afterwards=0;
static char def_instr_name[256]="";
int cfg_select = 0;
#ifdef CHANNEL_EFFECT
extern void effect_activate( int iSwitch ) ;
extern int init_effect(void) ;
#endif /*CHANNEL_EFFECT*/

extern PlayMode arts_play_mode;

int have_commandline_midis = 0;
int output_device_open = 0;

#ifdef __WIN32__
int intr;
CRITICAL_SECTION critSect;

#pragma argsused
static BOOL WINAPI handler (DWORD dw)
	{
	printf ("***BREAK\n");
	intr = TRUE;
	play_mode->purge_output ();
	return TRUE;
	}
#endif

static void help(void)
{
  PlayMode **pmp=play_mode_list;
  ControlMode **cmp=ctl_list;
  cmp = cmp;
  /*
   * glibc headers break this code because printf is a macro there, and
   * you cannot use a preprocessing instruction like #ifdef inside a macro.
   * current workaround is not using printf ( Dirk )
   */
  fprintf(stdout, " TiMidity version " TIMID_VERSION " (C) 1995 Tuukka Toivonen "
	 "<toivonen@clinet.fi>\n"
	 " TiMidity is free software and comes with ABSOLUTELY NO WARRANTY.\n"
	 "\n"
#ifdef __WIN32__
	 " Win32 version by Davide Moretti <dmoretti@iper.net>\n"
#endif
#ifdef KMIDI
         " KMidi version " KMIDIVERSION "  MIDI to WAVE player/converter\n"
         " Copyright (C) 1997 Bernd Johannes Wuebben <wuebben@math.cornell.edu>\n"
         " \n"
         " KMidi is free software and comes with ABSOLUTELY NO WARRANTY.\n"
	 "\n"
#endif
	 "Usage:\n"
	 "  %s [options] filename [...]\n"
	 "\n"
#ifndef __WIN32__
	/*does not work in Win32 */
	 "  Use \"-\" as filename to read a MIDI file from stdin\n"
	 "\n"
#endif
	 "Options:\n"
#if defined(AU_HPUX) || defined(hpux) || defined(__hpux)
	 "  -o file Output to another file (or audio server) (Use \"-\" for stdout)\n"
#elif defined (AU_LINUX)
	 "  -o file Output to another file (or device) (Use \"-\" for stdout)\n"
#else
	 "  -o file Output to another file (Use \"-\" for stdout)\n"
#endif
	 "  -O mode Select output mode and format (see below for list)\n"
	 "  -s f    Set sampling frequency to f (Hz or kHz)\n"
	 "  -a      Enable the antialiasing filter\n"
	 "  -f      "
#ifdef FAST_DECAY
					"Disable"
#else
					"Enable"
#endif
							  " fast decay mode\n"
	 "  -d      dry mode\n"
	 "  -p n    Allow n-voice polyphony\n"
	 "  -A n    Amplify volume by n percent (may cause clipping)\n"
	 "  -C n    Set ratio of sampling and control frequencies\n"
	 "\n"
	 "  -# n    Select patch set\n"
#ifdef CHANNEL_EFFECT
	 "  -E      Effects filter\n"
#endif /*CHANNEL_EFFECT*/
#ifdef tplus
	 "  -m      Disable portamento\n"
#endif
	 "  -L dir  Append dir to search path\n"
	 "  -c file Read extra configuration file\n"
	 "  -I n    Use program n as the default\n"
	 "  -P file Use patch file for all programs\n"
	 "  -D n    Play drums on channel n\n"
	 "  -Q n    Ignore channel n\n"
	 "  -R n    Reverb options (1)(+2)(+4) [7]\n"
	 "  -k n    Resampling interpolation option (0-3) [1]\n"
	 "  -r n    Max ram to hold patches in megabytes [60]\n"
	 "  -X n    Midi expression is linear (0) or exponential (1-2) [1]\n"
	 "  -V n    Midi volume is linear (0) or exponential (1-2) [1]\n"

	 "  -F      Enable fast panning\n"
	 "  -U      Unload instruments from memory between MIDI files\n"
	 "\n"
	 "  -i mode Select user interface (see below for list)\n"
#if defined(AU_LINUX) || defined(AU_WIN32)
	 "  -B n    Set number of buffer fragments\n"
#endif
#ifdef __WIN32__
	 "  -e      Increase thread priority (evil) - be careful!\n"
#endif
	 "  -h      Display this help message\n"
	 "\n"
	 , program_name);

  printf("Available output modes (-O option):\n\n");
  while (*pmp)
	 {
		printf("  -O%c     %s\n", (*pmp)->id_character, (*pmp)->id_name);
		pmp++;
	 }
  printf("\nOutput format options (append to -O? option):\n\n"
	 "   `8'    8-bit sample width\n"
	 "   `1'    16-bit sample width\n"
	 "   `U'    uLaw encoding\n"
	 "   `l'    linear encoding\n"
	 "   `M'    monophonic\n"
	 "   `S'    stereo\n"
	 "   `s'    signed output\n"
	 "   `u'    unsigned output\n"
	 "   `x'    byte-swapped output\n");

  printf("\nAvailable interfaces (-i option):\n\n");
  while (*cmp)
	 {
      printf("  -i%c     %s\n", (*cmp)->id_character, (*cmp)->id_name);
      cmp++;
	 }
  printf("\nInterface options (append to -i? option):\n\n"
	 "   `v'    more verbose (cumulative)\n"
	 "   `q'    quieter (cumulative)\n"
	 "   `t'    trace playing\n\n");
}

#ifndef KMIDI
static void interesting_message(void)
{
  fprintf(stdout,
"\n"
" TiMidity version " TIMID_VERSION " -- Experimental MIDI to WAVE converter\n"
" Copyright (C) 1995 Tuukka Toivonen <toivonen@clinet.fi>\n"
" \n"
#ifdef __WIN32__
" Win32 version by Davide Moretti <dmoretti@iper.net>\n"
"\n"
#endif
" This program is free software; you can redistribute it and/or modify\n"
" it under the terms of the GNU General Public License as published by\n"
" the Free Software Foundation; either version 2 of the License, or\n"
" (at your option) any later version.\n"
" \n"
" This program is distributed in the hope that it will be useful,\n"
" but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
" MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
" GNU General Public License for more details.\n"
" \n"
" You should have received a copy of the GNU General Public License\n"
" along with this program; if not, write to the Free Software\n"
" Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.\n"
"\n"
);

}
#endif

static int set_channel_flag(int32 *flags, int32 i, const char *name)
{
  if (i==0) *flags=0;
  else if ((i<1 || i>16) && (i<-16 || i>-1))
    {
		fprintf(stderr,
	      "%s must be between 1 and 16, or between -1 and -16, or 0\n",
	      name);
      return -1;
	 }
  else
    {
      if (i>0) *flags |= (1<<(i-1));
		else *flags &= ~ ((1<<(-1-i)));
    }
  return 0;
}

static int set_value(int32 *param, int32 i, int32 low, int32 high, const char *name)
{
  if (i<low || i > high)
    {
      fprintf(stderr, "%s must be between %ld and %ld\n", name, low, high);
      return -1;
    }
  else *param=i;
  return 0;
}

int set_play_mode(char *cp)
{
  PlayMode *pmp, **pmpp=play_mode_list;

  while ((pmp=*pmpp++))
    {
      if (pmp->id_character == *cp)
	{
	  play_mode=pmp;
	  while (*(++cp))
		 switch(*cp)
	      {
	      case 'U': pmp->encoding |= PE_ULAW; break; /* uLaw */
			case 'l': pmp->encoding &= ~PE_ULAW; break; /* linear */

			case '1': pmp->encoding |= PE_16BIT; break; /* 1 for 16-bit */
	      case '8': pmp->encoding &= ~PE_16BIT; break;

	      case 'M': pmp->encoding |= PE_MONO; break;
	      case 'S': pmp->encoding &= ~PE_MONO; break; /* stereo */

	      case 's': pmp->encoding |= PE_SIGNED; break;
	      case 'u': pmp->encoding &= ~PE_SIGNED; break;

	      case 'x': pmp->encoding ^= PE_BYTESWAP; break; /* toggle */

			default:
		fprintf(stderr, "Unknown format modifier `%c'\n", *cp);
		return 1;
	      }
	  return 0;
	}
	 }

  fprintf(stderr, "Playmode `%c' is not compiled in.\n", *cp);
  return 1;
}

static int set_ctl(char *cp)
{
  ControlMode *cmp, **cmpp=ctl_list;

  while ((cmp=*cmpp++))
    {
      if (cmp->id_character == *cp)
	{
	  ctl=cmp;
	  while (*(++cp))
	    switch(*cp)
			{
			case 'v': cmp->verbosity++; break;
	   		case 'q': cmp->verbosity--; break;
			case 't': /* toggle */
		cmp->trace_playing= (cmp->trace_playing) ? 0 : 1;
		break;

	      default:
		fprintf(stderr, "Unknown interface option `%c'\n", *cp);
		return 1;
	      }
	  return 0;
	}
	 }

  fprintf(stderr, "Interface `%c' is not compiled in.\n", *cp);
  return 1;
}

char *cfg_names[30];


void clear_config(void)
{
    ToneBank *bank=0;
    int i, j;

    for (i = 0; i < MAXBANK; i++)
    {
		if (tonebank[i])
		{
			bank = tonebank[i];
			if (bank->name) free((void *)bank->name);
			bank->name = 0;
			for (j = 0; j < MAXPROG; j++)
			  if (bank->tone[j].name)
			  {
			     free((void *)bank->tone[j].name);
			     bank->tone[j].name = 0;
			  }
			if (i > 0)
			{
			     free(tonebank[i]);
			     tonebank[i] = 0;
			}
		}
		if (drumset[i])
		{
			bank = drumset[i];
			if (bank->name) free((void *)bank->name);
			bank->name = 0;
			for (j = 0; j < MAXPROG; j++)
			  if (bank->tone[j].name)
			  {
			     free((void *)bank->tone[j].name);
			     bank->tone[j].name = 0;
			  }
			if (i > 0)
			{
			     free(drumset[i]);
			     drumset[i] = 0;
			}
		}
    }

    memset(drumset[0], 0, sizeof(ToneBank));
    memset(tonebank[0], 0, sizeof(ToneBank));
    clear_pathlist(0);
}


int reverb_options=7;
int global_reverb = 0;
int global_chorus = 0;
int global_echo = 0;
int global_detune = 0;

int got_a_configuration=0;

#ifdef KMIDI
extern int createKApplication(int *argc, char ***argv);
#endif

#ifdef __WIN32__
int __cdecl main(int argc, char **argv)
#else
int main(int argc, char **argv)
#endif
{
  int c,
      cmderr=0,
      i,
      try_config_again=0,
      need_stdin=0,
      need_stdout=0;
  int orig_optind;

  int32 tmpi32, output_rate=0;

  char *output_name=0;

#if defined(AU_LINUX) || defined(AU_WIN32)
  int buffer_fragments=-1;
#endif
#ifdef __WIN32__
	int evil=0;
#endif

#ifdef DANGEROUS_RENICE
#include <sys/resource.h>
  int u_uid=getuid();
#endif

#ifdef KMIDI
#define KMIDI_CONFIG_SUBDIR "/share/apps/kmidi/config"
  char *KDEdir;
  char *kmidi_config;


  for (i = 0; i < argc; i++) {
	if (strlen(argv[i]) < 2) continue;
	if (argv[i][0] != '-') continue;
	if (argv[i][1] == 'h') break;
	if (strlen(argv[i]) < 3) continue;
	if (argv[i][1] != 'i') continue;
	if (argv[i][2] == 'c') continue;
	if (argv[i][2] != 'q') break;
  }

  if (i == argc) {
	if (!createKApplication(&argc, &argv)) return 0;
  }
  if ( ! (KDEdir = getenv("KDEDIR"))) {
       kmidi_config = (char *)safe_malloc(strlen(DEFAULT_PATH)+1);
       strcpy(kmidi_config, DEFAULT_PATH);
  }
  else {
       kmidi_config = (char *)safe_malloc(strlen(KDEdir)+strlen(KMIDI_CONFIG_SUBDIR)+1);
       strcpy(kmidi_config, KDEdir);
       strcat(kmidi_config, KMIDI_CONFIG_SUBDIR);
       /* add_to_pathlist(kmidi_config, 0); */
  }
  add_to_pathlist(kmidi_config, 0);
#else
#ifdef DEFAULT_PATH
  add_to_pathlist(DEFAULT_PATH, 0);
#endif
#endif


#ifdef DANGEROUS_RENICE
  if (setpriority(PRIO_PROCESS, 0, DANGEROUS_RENICE) < 0)
	 fprintf(stderr, "Couldn't set priority to %d.\n", DANGEROUS_RENICE);
  setreuid(u_uid, u_uid);
#endif


  if ((program_name=rindex(argv[0], '/'))) program_name++;
  else program_name=argv[0];
#ifndef KMIDI
  if (argc==1)
	 {
		interesting_message();
		return 0;
	 }
#endif

  command_cutoff_allowed = 0;
#ifdef CHANNEL_EFFECT
  init_effect() ;
#endif /*CHANNEL_EFFECT*/

#ifdef KMIDI
  if (argc >= 3 && !strcmp(argv[1],"-miniicon")) {
	  for (i=1; i<argc-2; i++) argv[i] = argv[i+2];
	  argc -= 2;
  }
  if (argc >= 3 && !strcmp(argv[1],"-icon")) {
	  for (i=1; i<argc-2; i++) argv[i] = argv[i+2];
	  argc -= 2;
  }
#endif

  while ((c=getopt(argc, argv, "UI:P:L:c:A:C:ap:fo:O:s:Q:R:FD:hi:#:qEmk:r:X:V:d-::"
#if defined(AU_LINUX) || defined(AU_WIN32)
			"B:" /* buffer fragments */
#endif
#ifdef __WIN32__
			"e"	/* evil (be careful) */
#endif
			))>0)
	 switch(c)
		{
		case 'q': command_cutoff_allowed=1; break;
		case 'U': free_instruments_afterwards=1; break;
		case 'L': add_to_pathlist(optarg, 0); try_config_again=1; break;
		case 'c':
	if (read_config_file(optarg, 1)) cmderr++;
	break;
#ifdef CHANNEL_EFFECT
		case 'E':  effect_activate( 1 ); break ;
#endif /* CHANNEL_EFFECT */

		case 'Q':
	if (set_channel_flag(&quietchannels, atoi(optarg), "Quiet channel"))
	  cmderr++;
	break;

		case 'k':
	current_interpolation = atoi(optarg);
	break;

		case 'd':
	opt_dry = 1;
	break;

		case 'X':
	opt_expression_curve = atoi(optarg);
	break;

		case 'V':
	opt_volume_curve = atoi(optarg);
	break;

		case 'r':
	max_patch_memory = atoi(optarg) * 1000000;
	break;

		case 'R': reverb_options = atoi(optarg); break;

		case 'D':
	if (set_channel_flag(&drumchannels, atoi(optarg), "Drum channel"))
	  cmderr++;
	break;

		case 'O': /* output mode */
	if (set_play_mode(optarg))
	  cmderr++;
	break;

		case 'o':	output_name=optarg; break;

		case 'a': antialiasing_allowed=1; break;

		case 'f': fast_decay=(fast_decay) ? 0 : 1; break;

		case 'F': adjust_panning_immediately=1; break;

		case 's': /* sampling rate */
	i=atoi(optarg);
	if (i < 100) i *= 1000;
	if (set_value(&output_rate, i, MIN_OUTPUT_RATE, MAX_OUTPUT_RATE,
				"Resampling frequency")) cmderr++;
	break;

		case 'P': /* set overriding instrument */
	strncpy(def_instr_name, optarg, 255);
	def_instr_name[255]='\0';
	break;

		case 'I':
	if (set_value(&tmpi32, atoi(optarg), 0, 127,
				"Default program")) cmderr++;
	else default_program=tmpi32;
	break;
		case 'A':
	if (set_value(&amplification, atoi(optarg), 1, MAX_AMPLIFICATION,
				"Amplification")) cmderr++;
	break;
		case 'C':
	if (set_value(&control_ratio, atoi(optarg), 1, MAX_CONTROL_RATIO,
				"Control ratio")) cmderr++;
	break;
		case 'p':
	if (set_value(&tmpi32, atoi(optarg), 1, MAX_VOICES,
				"Polyphony")) cmderr++;
	else voices=tmpi32;
	break;

		case 'i':
	if (set_ctl(optarg))
	  cmderr++;
#if defined(AU_LINUX) || defined(AU_WIN32)
	else if (buffer_fragments==-1 && ctl->trace_playing)
	  /* user didn't specify anything, so use 2 for real-time response */
#if defined(AU_LINUX)
	  buffer_fragments=2;
#else
	  buffer_fragments=3;		/* On Win32 2 is chunky */
#endif
#endif
	break;

#if defined(AU_LINUX) || defined(AU_WIN32)

		case 'B':
	if (set_value(&tmpi32, atoi(optarg), 0, 1000,
				"Buffer fragments")) cmderr++;
	else buffer_fragments=tmpi32;
	break;
#endif
#ifdef __WIN32__
		case 'e': /* evil */
			evil = 1;
			break;

#endif
		case '#':
	cfg_select=atoi(optarg);
	break;
		case 'h':
	help();
	return 0;

		case '-':
	break;

		default:
	cmderr++; break;
		}

  try_config_again = 1;
/* don't use unnecessary memory until we're a child process */
  if (!got_a_configuration)
	 {
		if (!try_config_again || read_config_file(CONFIG_FILE, 1)) cmderr++;
	 }

  /* If there were problems, give up now */
  if (cmderr )
	 {
		fprintf(stderr, "Try %s -h for help\n", program_name);
		return 1; /* problems with command line */
	 }

  /* Set play mode parameters */
  if (output_rate) play_mode->rate=output_rate;
  if (output_name)
	 {
		play_mode->name=output_name;
		if (!strcmp(output_name, "-"))
	need_stdout=1;
	 }

#if defined(AU_LINUX) || defined(AU_WIN32)

  if (buffer_fragments != -1)
	 play_mode->extra_param[0]=buffer_fragments;
#endif

  init_tables();

  orig_optind=optind;

  while (optind<argc) if (!strcmp(argv[optind++], "-")) need_stdin=1;
  optind=orig_optind;

  if(argc-optind > 0 ) have_commandline_midis = argc - optind;
  else have_commandline_midis = 0;

  if (play_mode->open_output()<0){

      /* fprintf(stderr, "KMidi: Sorry, couldn't open %s.\n", play_mode->id_name); */
      /*	  ctl->close();*/

      output_device_open = 0;

      /*return 2;*/
  }
  else output_device_open = 1;

  if (!output_device_open && play_mode != &arts_play_mode) {
	PlayMode *save_play_mode;
	save_play_mode = play_mode;
	play_mode = &arts_play_mode;
	if (output_rate) play_mode->rate=output_rate;
	if (output_name) play_mode->name=output_name;
	if (play_mode->open_output()<0) play_mode = save_play_mode;
	else {
	  output_device_open = 1;
	}
  }

  if (ctl->open(need_stdin, need_stdout)) {

	  fprintf(stderr, "Couldn't open %s\n", ctl->id_name);
	  play_mode->close_output();
	  return 3;
  }

#ifndef KMIDI
  if (!output_device_open) {
	  fprintf(stderr, "Couldn't open %s\n", play_mode->id_name);
	  ctl->close();
	  return 2;
  }
#endif /* not KMIDI */


  if (!control_ratio) {

	  control_ratio = play_mode->rate / CONTROLS_PER_SECOND;
	  if(control_ratio<1)
		 control_ratio=1;
	  else if (control_ratio > MAX_CONTROL_RATIO)
		 control_ratio=MAX_CONTROL_RATIO;
  }

  if (*def_instr_name) set_default_instrument(def_instr_name);

#ifdef __WIN32__

		SetConsoleCtrlHandler (handler, TRUE);
		InitializeCriticalSection (&critSect);

		if(evil)
			if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL))
				fprintf(stderr, "Error raising process priority.\n");
#endif

  if (got_a_configuration < 2) read_config_file(current_config_file, 0);

  ctl->pass_playing_list(argc-optind, (const char **)&argv[orig_optind]);

  play_mode->close_output();
  ctl->close();

#ifdef __WIN32__
			DeleteCriticalSection (&critSect);
#endif
  return 0;
}
