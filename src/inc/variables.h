#ifndef _VARIABLES_H
#define _VARIABLES_H

#define HASHSIZE 64

#define VAR_GLOBAL     0x0001
#define VAR_SITE       0x0002
#define VAR_LIST       0x0004
#define VAR_TEMP       0x0008
#define VAR_INTERNAL   0x0010
#define VAR_LOCKED     0x0020
#define VAR_NOEXPAND   0x0040
#define VAR_RESTRICTED 0x0080

#define VAR_ALL    VAR_GLOBAL|VAR_SITE|VAR_LIST|VAR_TEMP
#define VAR_ANY    VAR_GLOBAL|VAR_SITE|VAR_LIST|VAR_TEMP
#define VAR_NOLIST VAR_GLOBAL|VAR_SITE|VAR_TEMP

enum var_type {
    VAR_STRING, VAR_BOOL, VAR_INT, VAR_DURATION, VAR_DATA, VAR_TIME,
    VAR_CHOICE
};

struct var_data {
   char *name;
   char *description;
   char *section;
   char *example;
   char *defval;
   char *global;
   char *site;
   char *list;
   char *temp;
   char *expanded;
   char *choices;
   int flags;
   enum var_type type;
   struct var_data *next;
};

struct list_vars {
    struct var_data *bucket[HASHSIZE];
};

extern struct list_vars *listdata;

/* non-API functions */
extern void nuke_vars(void);
extern void init_vars(void);
extern void init_regvars(void);

/* Variable setup and cleanup routines */
extern void register_var(const char *varname, const char *defval,
                         const char *section, const char *desc, 
                         const char *example, enum var_type type, int flags);
extern void set_var(const char *varname, const char *varval, int level);
extern void clean_var(const char *varname, int level);
extern void wipe_vars(int level);

/* variable locking routines */
extern void lock_var(const char *varname);
extern void unlock_var(const char *varname);
extern void restrict_var(const char *varname);

/* variable querying routines */
extern const char *get_var(const char *varname);
extern const int get_bool(const char *varname);
extern const int get_number(const char *varname);
extern const char *get_string(const char *varname);
extern const int get_seconds(const char *varname);
extern const void *get_data(const char *varname);
extern const char *get_var_unexpanded(const char *varname);

/* Utility function */
extern const char *get_cur_varval(struct var_data *var);
extern const char *get_cur_varval_level(struct var_data *var, int level);
extern const char *get_cur_varval_level_default(struct var_data *var,
                                                int level);
extern struct var_data *find_var_rec(const char *varname);
extern int check_duration(const char *duration);

/* variable iteration routines */
extern struct var_data *start_varlist(void);
extern struct var_data *next_varlist(void);
extern void finish_varlist(void);

/* config file writing routines */
extern void write_configfile(const char *filename, int level,
                             const char *sortorder);
extern void write_configfile_section(const char *filename, int level, 
                                     const char *section);

/* Variable cheatsheet file routine */
extern void write_cheatsheet(const char *filename, const char *sortorder);
#endif /* _VARIABLES_H */
