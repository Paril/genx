#define MAX_SOUND_SCRIPTS 256

typedef struct
{
	char			path[MAX_QPATH];

	bool			loaded;
	qhandle_t		sound;
	int				pitch_min, pitch_max;
} soundentry_t;

typedef struct
{
	char			name[MAX_QPATH];
	soundentry_t	entries[GAME_TOTAL];
} soundscript_t;

soundentry_t *MSCR_EntryForHandle(qhandle_t handle, gametype_t game);