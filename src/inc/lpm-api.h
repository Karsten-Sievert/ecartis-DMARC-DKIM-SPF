/* Module API Definition */

#ifndef LPM_API_H
#define LPM_API_H

#include <stdio.h>
#include <time.h>
#ifndef WIN32
#include <sys/time.h>
#endif

#define LPM_API_VERSION 1

struct LPMAPI {

   /* Functions */
   /* From alias.h */
   int (*register_alias)(const char *alias, const char *real);
   const char *(*lookup_alias)(const char *alias);

   /* From Cmdarg.h */
   void (*add_cmdarg)(const char *arg, int params, char *pdesc, CmdArgFn fn);
   void (*new_cmdargs)(void);
   struct listserver_cmdarg * (*find_cmdarg)(const char *arg);
   struct listserver_cmdarg * (*get_cmdargs)(void);

   /* From Command.h */
   void (*new_commands)(void);
   void (*add_command)(char *name, char *desc, char *syntax, char *altsyntax,
                       char *adminsyntax, int flags, CmdFn cmd);
   struct listserver_cmd * (*find_command)(char *name, int flags);
   struct listserver_cmd * (*get_commands)(void);

   /* From Cookie.h */
   void (*register_cookie)(char type, const char *expirevar, CookieFn create,
                           CookieFn destroy);
   int (*match_cookie)(const char *cookie, const char *match);
   int (*request_cookie)(const char *filename, char *cookie,
                         char type, const char *cookiedata);
   int (*verify_cookie)(const char *filename, const char *cookie,
                        char type, char *cookiedata);
   int (*del_cookie)(const char *filename, const char *cookie);
   void (*expire_all_cookies)(void);

   /* From Core.h */
   int (*send_textfile)(const char *address, const char *textfile);
   int (*flagged_send_textfile)(const char *fromaddy, const char *list,
                                const char *flag, const char *file,
                                const char *subject);
   int (*blacklisted)(const char *address);
   int (*match_reg)(const char *pattern, const char *match);
   void (*log_printf)(int level, char *format, ...);
   void (*result_printf)(char *format, ...);
   void (*quote_command)();
   void (*filesys_error)(const char *filename);
   void (*internal_error)(const char *message);
   void (*spit_status)(const char *statustext, ...);
   void (*bounce_message)(void);
   const char * (*resolve_error)(int error);
   void (*nosuch)(const char *);
   void (*make_moderated_post)(const char *);
   void (*get_date)(char *buffer, int len, time_t now);

   /* From File.h */
   void (*add_file)(const char *filename, const char *varname,
                    const char *desc);
   void (*new_files)(void);
   struct list_file * (*find_file)(const char *filename);
   struct list_file * (*get_files)(void);

   /* From Fileapi.h */
   FILE * (*open_file)(const char *path, const char *mode);
   int (*close_file)(FILE *stream);
   FILE * (*open_exclusive)(const char *path, const char *mode);
   FILE * (*open_shared)(const char *path, const char *mode);
   int (*exists_file)(const char *path);
   int (*exists_dir)(const char *path);
   char * (*read_file)(char *buffer, int size, FILE *stream);
   int (*write_file)(FILE *stream, const char *format, ...);
   void (*rewind_file)(FILE *);
   void (*truncate_file)(FILE *, int);
   int (*unlink_file)(const char *);
   int (*putc_file)(char outchar, FILE *stream);
   int (*replace_file)(const char *src, const char *dest);
   int (*walk_dir)(const char *path, char *buffer, LDIR *dir);
   int (*next_dir)(LDIR dir, char *buffer);
   int (*close_dir)(LDIR dir);
   int (*mkdirs)(const char *path);
   int (*flush_file)(FILE *stream);

   /* From Flag.h */
   void (*new_flags)(void);
   void (*add_flag)(const char *name, const char *desc, int admin);
   struct listserver_flag * (*get_flag)(const char *name);
   struct listserver_flag * (*get_flags)(void);

   /* From Forms.h */
   int (*task_heading)(const char *toaddy);
   void (*task_ending)(void);
   int (*error_heading)(void);
   void (*error_ending)(void);

   /* From Hooks.h */
   void (*new_hooks)(void);
   void (*add_hook)(const char *type, unsigned int priority, HookFn cmd);
   int (*do_hooks)(const char *hooktype);
   struct listserver_hook * (*get_hooks)(void);

   /* From List.h */
   int (*set_context_list)(const char *list);
   int (*read_conf)(const char *filename, int level);
   int (*read_conf_parm)(const char *listname, const char *param, int level);
   int (*list_valid)(const char *listname);
   int (*list_read_conf)(void);

   /* From Modes.h */
   void (*add_mode)(const char *mode, ModeFn fn);
   void (*new_modes)(void);
   struct listserver_mode * (*find_mode)(const char *mode);
   struct listserver_mode * (*get_modes)(void);

   /* From Module.h */
   void (*new_modules)(void);
   void (*add_module)(const char *, const char *);
   struct listserver_module * (*get_modules)(void);

   /* From Mystring.h */
   int (*buffer_printf)(char *buffer, int length, char *format, ...);
   char * (*lowerstr)(const char *);
   char * (*upperstr)(const char *);
   char * (*strcasestr)(const char *, const char *);
   void (*strreplace)(char *, int, const char *, const char *, const char *);
   void (*strcasereplace)(char *, int, const char *, const char *, const char *);
   int (*address_match)(const char *, const char *);
   int (*check_address)(const char *);
   void (*trim)(char *);


   /* From Parse.h */
   int (*open_adminspit)(const char *filename);
   FILE * (*get_adminspit)(void);
   int (*handle_spit_admin)(const char *line);
   int (*handle_spit_admin2)(const char *line);

   /* From Regexp.h */
   regexp * (*regcomp)(char *exp);
   int (*regexec)(regexp *prog, char *string);
   void (*regsub)(regexp *prog, char *source, char *dest);

   /* From Smtp.h */
   int (*smtp_start)(int notify);
   int (*smtp_from)(const char *fromaddy);
   int (*smtp_to)(const char *toaddy);
   int (*smtp_body_start)(void);
   int (*smtp_body_text)(const char *line);
   int (*smtp_body_line)(const char *line);
   int (*smtp_body_end)(void);
   int (*smtp_end)(void);

   /* From Tolist.h */
   void (*new_tolist)(void);
   void (*nuke_tolist)(void);
   void (*add_from_list_all)(const char *list);
   void (*add_from_list_flagged)(const char *list, const char *flag);
   void (*add_from_list_unflagged)(const char *list, const char *flag);
   void (*remove_user_all)(const char *address);
   void (*remove_flagged_all)(const char *flag);
   void (*remove_flagged_all_prilist)(const char *flag, const char *prilist);
   void (*remove_unflagged_all)(const char *flag);
   void (*remove_unflagged_all_prilist)(const char *flag, const char *prilist);
   void (*remove_list_flagged)(const char *list, const char *flag);
   void (*remove_list_unflagged)(const char *list, const char *flag);
   void (*sort_tolist)(void);
   struct tolist * (*start_tolist)(void);
   struct tolist * (*next_tolist)(void);
   void (*finish_tolist)(void);
   int (*send_to_tolist)(const char *fromaddy, const char *file,
                         int do_ackpost, int do_echopost, int fullbounce);

   /* From unhtml.c */
   int (*unhtml_file)(const char *file1, const char *file2);

   /* From Unmime.h */
   void (*unmime_file)(const char *src, const char *dest);
   void (*unquote_file)(const char *src, const char *dest);
   void (*add_mime_handler)(const char *mimetype, int priority, MimeFn handler);
   void (*mime_eat_header)(struct mime_header *);
   struct mime_field *(*mime_getfield)(struct mime_header *, const char *);
   const char *(*mime_fieldval)(struct mime_header *, const char *);
   const char *(*mime_parameter)(struct mime_header *, const char *, const char *);

   /* From User.h */
   int (*user_read)(FILE *userfile, struct list_user *user);
   int (*user_find)(const char *listusers, const char *addy,
                    struct list_user *user);
   int (*user_find_list)(const char *list, const char *addy,
                         struct list_user *user);
   int (*user_add)(const char *listusers, const char *fromaddy);
   int (*user_remove)(const char *listusers, const char *fromaddy);
   int (*user_write)(const char *listusers, struct list_user *user);
   int (*user_setflag)(struct list_user *user, const char *flag, int admin);
   int (*user_unsetflag)(struct list_user *user, const char *flag, int admin);
   int (*user_hasflag)(struct list_user *user, const char *flag);

   /* From Userstat.h */
   int (*userstat_get_stat)(const char *list, const char *user, const char *key,
                            char *val, int vallen);
   int (*userstat_set_stat)(const char *list, const char *user, const char *key,
                            const char *val);

   /* From Variables.h */
   void (*register_var)(const char *varname, const char *defval,
                        const char *section, const char *desc,
                        const char *example, enum var_type type, int flags);
   void (*set_var)(const char *varname, const char *varval, int level);
   void (*clean_var)(const char *varname, int level);
   void (*wipe_vars)(int level);
   const char * (*get_var)(const char *varname);
   void * (*get_data)(const char *varname);
   const int (*get_bool)(const char *varname);
   const int (*get_number)(const char *varname);
   const char * (*get_string)(const char *varname);
   const int (*get_seconds)(const char *varname);
   const char *(*get_cur_varval)(struct var_data *var);
   struct var_data * (*start_varlist)(void);
   struct var_data * (*next_varlist)(void);
   void (*finish_varlist)(void);
   void (*lock_var)(const char *varname);
   void (*unlock_var)(const char *varname);

   /* Recent additions - at bottom to preserve binary API compatibility */
   /* Binary compatible as of LPM definition for 07/13/1999 - 0.125a pre */
   
   /* Liscript additions */
   int (*liscript_parse_line)(const char *inputline, char *buffer, int buflen);
   int (*liscript_parse_file)(const char *file1, const char *file2);

   /* core.h addition */
   int (*send_textfile_expand)(const char *address, const char *textfile);

   /* submodes. additions */
   struct submode *(*get_submodes)(void);
   const char *(*get_submode_flags)(const char *mode);
   struct submode *(*get_submode)(const char *mode);

   /* New fileapi.h additions */
   int (*getc_file)(FILE *stream);
   int (*ungetc_file)(int putback, FILE *stream);

   /* unmime.h additions */
   void (*unquote_string)(const char *orig, char *dest, int len);

   /* lcgi.h additions */
   struct listserver_cgi_hook *(*get_cgi_hooks)(void);
   struct listserver_cgi_hook *(*find_cgi_hook)(const char *name);
   void (*add_cgi_hook)(const char *name, CgiHook function);
   int (*cgi_unparse_template)(const char *name);

   /* more lcgi.h additions */
   void (*add_cgi_mode)(const char *name, CgiMode function);
   struct listserver_cgi_mode *(*find_cgi_mode)(const char *name);

   /* 0.126 config file writing/variable.h additions */
   const char *(*get_cur_varval_level)(struct var_data *var, int level);
   const char *(*get_cur_varval_level_default)(struct var_data *var, int level);
   void (*write_configfile)(const char *filename, int level,
                            const char *sortorder);
   void (*write_configfile_section)(const char *filename, int level,
                                    const char *section);
   int (*check_duration)(const char *duration);
   char *(*find_cookie)(const char *filename, char type, const char *partial);
   int (*modify_cookie)(const char *filename, const char *cookie,
                       const char *newdata);

   /* More 0.126a LCGI additions */
   void (*add_cgi_tempvar)(const char *name, const char *val);
   struct listserver_cgi_tempvar *(*get_cgi_tempvars)(void);

   /* Password additions for 0.126a */
   void (*set_pass)(const char *user, const char *newpass);
   int (*find_pass)(const char *user);
   int (*check_pass)(const char *user, const char *pass);

   /* Cookie addition for 0.126a */
   void (*expire_cookies)(const char *filename);

   /* list_directory for 0.126a */
   char *(*list_directory)(const char *listname);
   int (*listdir_file)(char *buffer, const char *list, const char *filename);
   int (*walk_lists)(char *buf);
   int (*next_lists)(char *buf);

   /* LCGI addition for 0.127a */
   struct listserver_cgi_tempvar *(*find_cgi_tempvar)(const char *name);

   /* fileapi addition for 0.127a */
   int (*append_file)(const char *dest, const char *source);

   /* core addition for 0.127a */
   void (*result_append)(const char *filename);
   int (*send_textfile_expand_append)(const char *address, 
                                      const char *filename,
                                      int includequeue);

   /* 0.128a additions for funcparse.h */
   void (*add_func)(const char *name, int nargs, const char *desc, FuncFn fn);
   struct listserver_funcdef *(*find_func)(const char *name);
   struct listserver_funcdef *(*get_funcs)(void);

   /* 0.129a additions */
   void (*add_file_flagged)(const char *filename, const char *varname,
                            const char *desc, int flags);

   void (*restrict_var)(const char *varname);
   int  (*is_trusted)(const char *list);

   int  (*expand_append)(const char *mainfile, const char *liscript);

   void (*build_hostname)(char *buffer, int len);

   /* 1.0.0 additions */
   const char *(*get_var_unexpanded)(const char *varname);
   void (*requote_string)(const char *orig, char *dest, int len);

   /* More additions */
   int (*public_file)(const char *filename);
   int (*private_file)(const char *filename);
};
#endif
