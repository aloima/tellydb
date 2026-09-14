#pragma once

#include "./commands.h"

/* DATABASE COMMANDS */
extern const Command cmd_bgsave;
extern const Command cmd_dbsize;
extern const Command cmd_flushall;
extern const Command cmd_flushdb;
extern const Command cmd_lastsave;
extern const Command cmd_save;
extern const Command cmd_select;
/* /DATABASE COMMANDS */

/* GENERIC COMMANDS */
extern const Command cmd_age;
extern const Command cmd_auth;
extern const Command cmd_client;
extern const Command cmd_command;
extern const Command cmd_discard;
extern const Command cmd_echo;
extern const Command cmd_exec;
extern const Command cmd_hello;
extern const Command cmd_info;
extern const Command cmd_multi;
extern const Command cmd_ping;
extern const Command cmd_pwd;
extern const Command cmd_time;
/* /GENERIC COMMANDS */

/* HASHTABLE COMMANDS */
extern const Command cmd_hdel;
extern const Command cmd_hget;
extern const Command cmd_hgetall;
extern const Command cmd_hkeys;
extern const Command cmd_hlen;
extern const Command cmd_hset;
extern const Command cmd_htype;
extern const Command cmd_hvals;
/* /HASHTABLE COMMANDS */

/* KV COMMANDS */
extern const Command cmd_append;
extern const Command cmd_decr;
extern const Command cmd_decrby;
extern const Command cmd_del;
extern const Command cmd_get;
extern const Command cmd_exists;
extern const Command cmd_incr;
extern const Command cmd_incrby;
extern const Command cmd_rename;
extern const Command cmd_set;
extern const Command cmd_type;
/* /KV COMMANDS */

/* LIST COMMANDS */
extern const Command cmd_lindex;
extern const Command cmd_llen;
extern const Command cmd_lpop;
extern const Command cmd_lpush;
extern const Command cmd_lrange;
extern const Command cmd_ltype;
extern const Command cmd_rpop;
extern const Command cmd_rpush;
/* /LIST COMMANDS */
