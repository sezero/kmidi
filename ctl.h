
#define INT_CODE 214
#define NQUEUE 30
typedef struct {
	int reset_panel;
	int multi_part;

	int32 last_time, cur_time;

	char v_flags[NQUEUE][MAXCHAN];
	int16 cnote[MAXCHAN];
	int16 cvel[MAXCHAN];
	int16 ctotal[NQUEUE][MAXCHAN];
	int16 ctime[NQUEUE][MAXCHAN];
	int16 notecount[NQUEUE][MAXCHAN];
	int cindex[MAXCHAN], mindex[MAXCHAN];

	char c_flags[MAXCHAN];
	char currentpatchset;
	Channel channel[MAXCHAN];
} PanelInfo;


#define FLAG_NOTE_OFF	1
#define FLAG_NOTE_ON	2

#define FLAG_BANK	1
#define FLAG_PROG	2
#define FLAG_PAN	4
#define FLAG_SUST	8
#define FLAG_PERCUSSION	16

extern PanelInfo *Panel;
