
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>

#define INT_CODE 214
#define NQUEUE 60
#define MAXDISPCHAN 32
typedef struct {
	int reset_panel;
	int multi_part;

	int32 last_time, cur_time;

	char v_flags[NQUEUE][MAXDISPCHAN];
	int16 cnote[MAXDISPCHAN];
	int16 cvel[MAXDISPCHAN];
	int16 ctotal[NQUEUE][MAXDISPCHAN];
	int16 ctime[NQUEUE][MAXDISPCHAN];
	int16 notecount[NQUEUE][MAXDISPCHAN];
	int cindex[MAXDISPCHAN], mindex[MAXDISPCHAN];

	char c_flags[MAXDISPCHAN];
	char currentpatchset;
	/* Channel channel[MAXDISPCHAN]; */
	int wait_reset;
	int buffer_state, various_flags;
} PanelInfo;


#define FLAG_NOTE_OFF	1
#define FLAG_NOTE_ON	2

#define FLAG_BANK	1
#define FLAG_PROG	2
#define FLAG_PAN	4
#define FLAG_SUST	8
#define FLAG_PERCUSSION	16

extern PanelInfo *Panel;
