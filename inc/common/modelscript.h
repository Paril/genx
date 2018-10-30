#define MAX_MODEL_SCRIPTS 256

typedef struct {
	char		path[MAX_QPATH];

	bool			loaded;
	modelhandle_t	model;

	uint32_t		add_effects;
	int				add_renderfx;
	byte			skin;
	vec3_t			translate;
	float			scale;
} modelentry_t;

typedef struct {
	char			name[MAX_QPATH];
	modelentry_t	entries[GAME_TOTAL];
} modelscript_t;

modelentry_t *MSCR_EntryForHandle(modelhandle_t handle, gametype_t game);