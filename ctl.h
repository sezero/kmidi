
#include <sys/ipc.h>
#include <sys/shm.h>
/* #include <sys/sem.h> */

/* #define INT_CODE 214 */
#define NQUEUE 800
#define MAXDISPCHAN 32
typedef struct {
	int reset_panel;
	int multi_part;

	int32 last_time, cur_time;

	char v_flags[NQUEUE][MAXDISPCHAN];
/*
	uint8 cnote[MAXDISPCHAN];
	uint8 cvel[MAXDISPCHAN];
*/
	uint8 ctotal[NQUEUE][MAXDISPCHAN];
	uint8 ctotal_sustain[NQUEUE][MAXDISPCHAN];
	int32 ctime[NQUEUE][MAXDISPCHAN];
	int16 notecount[NQUEUE][MAXDISPCHAN];

	uint8 panning[NQUEUE][MAXDISPCHAN];
	uint8 expression[NQUEUE][MAXDISPCHAN];
	uint8 reverberation[NQUEUE][MAXDISPCHAN];
	uint8 chorusdepth[NQUEUE][MAXDISPCHAN];
	uint8 volume[NQUEUE][MAXDISPCHAN];

	uint8 c_bank[MAXDISPCHAN];
	uint8 c_variationbank[MAXDISPCHAN];
	char c_flags[MAXDISPCHAN];
	char currentpatchset;
	/* Channel channel[MAXDISPCHAN]; */
	/* int wait_reset; */
	int buffer_state, various_flags, max_patch_megs;
} PanelInfo;


#define FLAG_NOTE_OFF	1
#define FLAG_NOTE_ON	2

#define FLAG_BANK	1
#define FLAG_PROG	2
#define FLAG_PAN	4
#define FLAG_SUST	8
#define FLAG_PERCUSSION	16

extern PanelInfo *Panel;
extern MidiEvent *current_event;
extern int pipe_read_ready();
extern void pipe_int_write(int c);
extern void pipe_int_read(int *c);
extern void pipe_string_read(char *str);
extern void pipe_string_write(const char *str);

