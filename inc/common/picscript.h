#define MAX_PIC_SCRIPTS 256

typedef struct
{
	char		path[MAX_QPATH];

	bool		loaded;
	qhandle_t	pic;
} picentry_t;

typedef struct
{
	char		name[MAX_QPATH];
	picentry_t	entries[GAME_TOTAL];
} picscript_t;

picentry_t *PSCR_EntryForHandle(qhandle_t handle, gametype_t game);