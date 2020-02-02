/* Module API */

/* All Our Happy Includes! */
#include "alias.h"
#include "cmdarg.h"
#include "command.h"
#include "cookie.h"
#include "core.h"
#include "file.h"
#include "fileapi.h"
#include "flag.h"
#include "forms.h"
#include "hooks.h"
#include "lcgi.h"
#include "liscript.h"
#include "list.h"
#include "modes.h"
#include "moderate.h"
#include "module.h"
#include "mystring.h"
#include "parse.h"
#include "passwd.h"
#include "regexp.h"
#include "smtp.h"
#include "tolist.h"
#include "trust.h"
#include "unhtml.h"
#include "unmime.h"
#include "user.h"
#include "userstat.h"
#include "variables.h"
#include "submodes.h"
#include "funcparse.h"

/* And the API structure */
#include "lpm-api.h"

struct LPMAPI API;

void build_lpm_api()
{
    /* alias.h */
    API.register_alias = register_alias;
    API.lookup_alias = lookup_alias;

    /* cmdarg.h */
    API.add_cmdarg = add_cmdarg;
    API.new_cmdargs = new_cmdargs;
    API.find_cmdarg = find_cmdarg;
    API.get_cmdargs = get_cmdargs;

    /* command.h */
    API.new_commands = new_commands;
    API.add_command = add_command;
    API.find_command = find_command;
    API.get_commands = get_commands;

    /* cookie.h */
    API.register_cookie = register_cookie;
    API.match_cookie = match_cookie;
    API.request_cookie = request_cookie;
    API.verify_cookie = verify_cookie;
    API.del_cookie = del_cookie;
    API.expire_all_cookies = expire_all_cookies;
    API.expire_cookies = expire_cookies;
    API.find_cookie = find_cookie;
    API.modify_cookie = modify_cookie;

    /* core.h */
    API.send_textfile = send_textfile;
    API.send_textfile_expand = send_textfile_expand;
    API.send_textfile_expand_append = send_textfile_expand_append;
    API.flagged_send_textfile = flagged_send_textfile;
    API.blacklisted = blacklisted;
    API.match_reg = match_reg;
    API.log_printf = log_printf;
    API.result_printf = result_printf;
    API.result_append = result_append;
    API.quote_command = quote_command;
    API.filesys_error = filesys_error;
    API.internal_error = internal_error;
    API.spit_status = spit_status;
    API.bounce_message = bounce_message;
    API.resolve_error = resolve_error;
    API.nosuch = nosuch;
    API.make_moderated_post = make_moderated_post;
    API.get_date = get_date;

    /* file.h */
    API.add_file = add_file;
    API.add_file_flagged = add_file_flagged;
    API.new_files = new_files;
    API.find_file = find_file;
    API.get_files = get_files;

    /* fileapi.h */
    API.buffer_printf = buffer_printf;
    API.open_file = open_file;
    API.close_file = close_file;
    API.open_exclusive = open_exclusive;
    API.open_shared = open_shared;
    API.exists_file = exists_file;
    API.exists_dir = exists_dir;
    API.write_file = write_file;
    API.read_file = read_file;
    API.rewind_file = rewind_file;
    API.truncate_file = truncate_file;
    API.unlink_file = unlink_file;
    API.putc_file = putc_file;
    API.replace_file = replace_file;
    API.walk_dir = walk_dir;
    API.next_dir = next_dir;
    API.close_dir = close_dir;
    API.mkdirs = mkdirs;
    API.flush_file = flush_file;
    API.ungetc_file = ungetc_file;
    API.getc_file = getc_file;
    API.append_file = append_file;

    /* flag.h */
    API.new_flags = new_flags;
    API.add_flag = add_flag;
    API.get_flag = get_flag;
    API.get_flags = get_flags;

    /* forms.h */
    API.task_heading = task_heading;
    API.task_ending = task_ending;
    API.error_heading = error_heading;
    API.error_ending = error_ending;

    /* funcparse.h */
    API.add_func = add_func;
    API.find_func = find_func;
    API.get_funcs = get_funcs;

    /* hooks.h */
    API.new_hooks = new_hooks;
    API.add_hook = add_hook;
    API.do_hooks = do_hooks;
    API.get_hooks = get_hooks;

    /* lcgi.h */
    API.add_cgi_hook = add_cgi_hook;
    API.find_cgi_hook = find_cgi_hook;
    API.get_cgi_hooks = get_cgi_hooks;
    API.cgi_unparse_template = cgi_unparse_template;
    API.add_cgi_mode = add_cgi_mode;
    API.find_cgi_mode = find_cgi_mode;
    API.add_cgi_tempvar = add_cgi_tempvar;
    API.get_cgi_tempvars = get_cgi_tempvars;
    API.find_cgi_tempvar = find_cgi_tempvar;

    /* liscript.h */
    API.liscript_parse_line = liscript_parse_line;
    API.liscript_parse_file = liscript_parse_file;

    /* list.h */
    API.set_context_list = set_context_list;
    API.read_conf = read_conf;
    API.read_conf_parm = read_conf_parm;
    API.list_valid = list_valid;
    API.list_read_conf = list_read_conf;
    API.list_directory = list_directory;
    API.listdir_file = listdir_file;
    API.walk_lists = walk_lists;
    API.next_lists = next_lists;

    /* modes.h */
    API.add_mode = add_mode;
    API.new_modes = new_modes;
    API.find_mode = find_mode;
    API.get_modes = get_modes;

    /* module.h */
    API.new_modules = new_modules;
    API.add_module = add_module;
    API.get_modules = get_modules;

    /* mystring.h */
    API.lowerstr = lowerstr;
    API.upperstr = upperstr;
    API.strcasestr = strcasestr;
    API.strreplace = strreplace;
    API.strcasereplace = strcasereplace;
    API.address_match = address_match;
    API.check_address = check_address;
    API.trim = trim;

    /* parse.h */
    API.open_adminspit = open_adminspit;
    API.get_adminspit = get_adminspit;
    API.handle_spit_admin = handle_spit_admin;
    API.handle_spit_admin2 = handle_spit_admin2;

    /* passwd.h */
    API.set_pass = set_pass;
    API.find_pass = find_pass;
    API.check_pass = check_pass;

    /* regexp.h */
    API.regcomp = regcomp;
    API.regsub = regsub;
    API.regexec = regexec;

    /* smtp.h */
    API.smtp_start = smtp_start;
    API.smtp_from = smtp_from;
    API.smtp_to = smtp_to;
    API.smtp_body_start = smtp_body_start;
    API.smtp_body_text = smtp_body_text;
    API.smtp_body_line = smtp_body_line;
    API.smtp_body_end = smtp_body_end;
    API.smtp_end = smtp_end;

    /* submode.h */
    API.get_submodes = get_submodes;
    API.get_submode = get_submode;
    API.get_submode_flags = get_submode_flags;

    /* tolist.h */
    API.new_tolist = new_tolist;
    API.nuke_tolist = nuke_tolist;
    API.add_from_list_all = add_from_list_all;
    API.add_from_list_flagged = add_from_list_flagged;
    API.add_from_list_unflagged = add_from_list_unflagged;
    API.remove_user_all = remove_user_all;
    API.remove_flagged_all = remove_flagged_all;
    API.remove_flagged_all_prilist = remove_flagged_all_prilist;
    API.remove_unflagged_all = remove_unflagged_all;
    API.remove_unflagged_all_prilist = remove_unflagged_all_prilist;
    API.remove_list_flagged = remove_list_flagged;
    API.remove_list_unflagged = remove_list_unflagged;
    API.sort_tolist = sort_tolist;
    API.start_tolist = start_tolist;
    API.next_tolist= next_tolist;
    API.finish_tolist = finish_tolist;
    API.send_to_tolist = send_to_tolist;

    /* unhtml.h */
    API.unhtml_file = unhtml_file;

    /* unmime.h */
    API.unmime_file = unmime_file;
    API.unquote_file = unquote_file;
    API.unquote_string = unquote_string;
    API.requote_string = requote_string;
    API.add_mime_handler = add_mime_handler;
    API.mime_eat_header = mime_eat_header;
    API.mime_getfield = mime_getfield;
    API.mime_fieldval = mime_fieldval;
    API.mime_parameter = mime_parameter;

    /* user.h */
    API.user_read = user_read;
    API.user_find = user_find;
    API.user_find_list = user_find_list;
    API.user_add = user_add;
    API.user_remove = user_remove;
    API.user_write = user_write;
    API.user_setflag = user_setflag;
    API.user_unsetflag = user_unsetflag;
    API.user_hasflag = user_hasflag;

    /* userstat.h */
    API.userstat_get_stat = userstat_get_stat;
    API.userstat_set_stat = userstat_set_stat;

    /* variables.h */
    API.register_var = register_var;
    API.set_var = set_var;
    API.clean_var = clean_var;
    API.wipe_vars = wipe_vars;
    API.get_var = get_var;
    API.get_bool = get_bool;
    API.get_number = get_number;
    API.get_string = get_string;
    API.get_seconds = get_seconds;
    API.get_cur_varval = get_cur_varval;
    API.get_cur_varval_level = get_cur_varval_level;
    API.get_cur_varval_level_default = get_cur_varval_level_default;
    API.start_varlist = start_varlist;
    API.next_varlist = next_varlist;
    API.finish_varlist = finish_varlist;
    API.lock_var = lock_var;
    API.unlock_var = unlock_var;
    API.restrict_var = restrict_var;
    API.write_configfile = write_configfile;
    API.write_configfile_section = write_configfile_section;
    API.check_duration = check_duration;

    API.is_trusted = is_trusted;

    API.expand_append = expand_append;
    API.build_hostname = build_hostname;

    API.get_var_unexpanded = get_var_unexpanded;

    API.public_file = public_file;
    API.private_file = private_file;

    /* Phew!! */
}
