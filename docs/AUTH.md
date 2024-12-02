# Authorization
This file defines authorization system concepts.

## Concept
If a client use a password defined in the server with `AUTH`, this client have all permissions of the password.  

## Limitations
* Server can have up to `2^6-1` passwords.

## Notes
* Password deriving algorithm is HKDF SHA384.
* The length of derived passwords is 48.

## Permissions
* `P_READ`, read a value from database, not included data type and data existence
* `P_WRITE`, write database
* `P_CLIENT`, manage clients, allowed operations: kill, lock, disconnect
* `P_CONFIG`, manage configuration, allowed operations: change config values, read config values 
* `P_AUTH`, manage authorization, allowed operations: add permissions to a password (allowed for permissions held), remove a password, create a password
* `P_SERVER`, manage server, allowed operations: close the server, save the database file

If there is no database file or there is no password in the server, all clients have all permissions.
