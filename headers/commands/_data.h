#pragma once

#include "_commands.h"

/* DATABASE COMMANDS */
extern const struct Command cmd_bgsave;
extern const struct Command cmd_dbsize;
extern const struct Command cmd_lastsave;
extern const struct Command cmd_save;
extern const struct Command cmd_select;
/* /DATABASE COMMANDS */

/* GENERIC COMMANDS */
extern const struct Command cmd_age;
extern const struct Command cmd_auth;
extern const struct Command cmd_client;
extern const struct Command cmd_command;
extern const struct Command cmd_hello;
extern const struct Command cmd_info;
extern const struct Command cmd_multi;
extern const struct Command cmd_ping;
extern const struct Command cmd_pwd;
extern const struct Command cmd_time;
/* /GENERIC COMMANDS */

/* HASHTABLE COMMANDS */
extern const struct Command cmd_hdel;
extern const struct Command cmd_hget;
extern const struct Command cmd_hgetall;
extern const struct Command cmd_hkeys;
extern const struct Command cmd_hlen;
extern const struct Command cmd_hset;
extern const struct Command cmd_htype;
extern const struct Command cmd_hvals;
/* /HASHTABLE COMMANDS */

/* KV COMMANDS */
extern const struct Command cmd_append;
extern const struct Command cmd_decr;
extern const struct Command cmd_del;
extern const struct Command cmd_get;
extern const struct Command cmd_exists;
extern const struct Command cmd_incr;
extern const struct Command cmd_rename;
extern const struct Command cmd_set;
extern const struct Command cmd_type;
/* /KV COMMANDS */

/* LIST COMMANDS */
extern const struct Command cmd_lindex;
extern const struct Command cmd_llen;
extern const struct Command cmd_lpop;
extern const struct Command cmd_lpush;
extern const struct Command cmd_rpop;
extern const struct Command cmd_rpush;
/* /LIST COMMANDS */
