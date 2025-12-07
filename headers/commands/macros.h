#pragma once

#define WRONG_ARGUMENT_ERROR(name)     RESP_ERROR_MESSAGE("Wrong argument count for '" name "' command")
#define INVALID_TYPE_ERROR(name)       RESP_ERROR_MESSAGE("Invalid type for '" name "' command")
#define MISSING_SUBCOMMAND_ERROR(name) RESP_ERROR_MESSAGE("Missing subcommand for '" name "' command")
#define INVALID_SUBCOMMAND_ERROR(name) RESP_ERROR_MESSAGE("Invalid subcommand for '" name "' command")
#define PASS_COMMAND()                 return EMPTY_STRING()
#define PASS_NO_CLIENT(client) \
  if (!(client)) { \
    PASS_COMMAND(); \
  }

#define CREATE_COMMAND_ENTRY(_client, _data, _database, _password) ({\
  (struct CommandEntry) {\
    .client = (_client),\
    .data = (_data),\
    .database = (_database),\
    .password = (_password)\
  };\
})
