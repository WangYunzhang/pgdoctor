/*
 Copyright 2014-2017 Thumbtack, Inc.

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

         http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
*/


#ifndef CONFIG_PARSER_H_
#define CONFIG_PARSER_H_


#include "custom_check.h"

/* map strings from the configuration file to equivalent constants */
#define MK_CONFIG_LIST(f) \
    f(HTTP_PORT) f(SYSLOG_FACILITY) f(PG_HOST) f(PG_PORT) f(PG_USER) \
    f(PG_PASSWORD) f(PG_DATABASE) f(PG_CONNECTION_TIMEOUT) f(CUSTOM_CHECK)
#define N_CONFIG_PARAMS 9
#define MK_CONFIG_PAIR(X) {X, #X},
#define MK_CONFIG_ENUM(X) X,

typedef enum { MK_CONFIG_LIST(MK_CONFIG_ENUM) } config_option_t;

typedef struct checks_list {
    custom_check_t check;
    struct checks_list *next;
} *checks_list_t;

typedef struct config {
    int http_port;
    char *syslog_facility;
    char *host;
    int port;
    char *user;
    char *password;
    char *database;
    int connection_timeout;
    checks_list_t custom_checks;
} *config_t;

#define CHECKS_LIST_CHECK(X) (X->check)
#define CHECKS_LIST_NEXT(X) (X->next)

/* a bit of an overkill since this really isn't an abstract data type,
 * but the definition has changed so many times that these macros just
 * make everything a lot easier */
#define CFG_HTTP_PORT(X) (X->http_port)
#define CFG_SYSLOG_FACILITY(X) (X->syslog_facility)
#define CFG_PG_HOST(X) (X->host)
#define CFG_PG_PORT(X) (X->port)
#define CFG_PG_USER(X) (X->user)
#define CFG_PG_PASSWORD(X) (X->password)
#define CFG_PG_DATABASE(X) (X->database)
#define CFG_PG_TIMEOUT(X) (X->connection_timeout)
#define CFG_CUSTOM_CHECKS(X) (X->custom_checks)

extern void sanitize_str(char *str);
extern config_t config_parse(const char *file_path);
extern void config_destroy(config_t config);
extern void config_show(config_t config);


#endif				/* CONFIG_PARSER_H_ */
