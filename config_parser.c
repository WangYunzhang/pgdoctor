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


#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L /* for strdup */

#include "config_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include "logger.h"
#include "strconst.h"


/* remove comments and whitespace (in-place) from a line in the
 * configuration file */
extern void sanitize_str(char *str)
{
    char *ignore_after, *start_from = str;
    size_t length;

    /* find the first occurrence of # to ignore everything that
     * follows */
    ignore_after = strchr(str, CFG_COMMENT_CHR);
    if (ignore_after)
        // terminate the string at the beginning of the comment
        *ignore_after = '\0';
    else
        /* use the whole string */
        ignore_after = str + strlen(str);

    /* ignore any whitespace before the comment */
    ignore_after--;
    while (ignore_after >= str && isspace(*ignore_after))
        ignore_after--;

    /* ignore whitespace at the beginning of the string */
    while (start_from < ignore_after && isspace(*start_from))
        start_from++;

    /* size of the sanitized configuration str */
    length = ignore_after-start_from;

    /* move the useful bytes to the beginning of the string */
    memmove(str, start_from, length+1);
    /* make sure to terminate the string */
    str[length+1] = '\0';
}

/* parse a (sanitized) line with a custom check; extract the query,
 * operator, and expected value and load them into the proper data
 * structure; if there is no value to compare to, `value` and `op`
 * will be set to an empty string */
static custom_check_t parse_custom_check(const char *line)
{
    char *query, *operator, *expected, *delim = "\"", buf[MAX_STR_CFG];

    snprintf(buf, sizeof(buf), "%s", line);
    query = strtok(buf, delim);
    if (! query) {
        return NULL;
    }
    operator = strtok(NULL, delim);
    if (operator) {
        expected = strtok(NULL, delim);
        if (! expected) {
            return NULL;
        }
        sanitize_str(operator);
        sanitize_str(expected);
        return custom_check_create(query, operator, expected);
    } else {
        return custom_check_create(query, "", "");
    }
}

static int get_param_type(const char *str)
{
    int i;
    static struct {
        int param_const;
        char * param_str;
    } config_param[] = { MK_CONFIG_LIST(MK_CONFIG_PAIR) };

    for (i = 0; i < N_CONFIG_PARAMS; i++) {
        if (strcasecmp(str, config_param[i].param_str) == 0) {
            return config_param[i].param_const;
        }
    }

    /* if everything else failed, let's assume it's a custom health
     * check */
    return CUSTOM_CHECK;
}

static void load_int(int *i, char *value)
{
    sanitize_str(value);
    *i = strtol(value, NULL, 10);
}

static void load_str(char **str, char *value)
{
    if (value) {
        sanitize_str(value);
        *str = strdup(value);
    } else {
        /* if the value for this parameter is NULL, just initialize it
         * to an empty string */
        *str = strdup("");
    }
}

static int append_custom_check(config_t config, custom_check_t custom_check)
{
    checks_list_t checks_list=CFG_CUSTOM_CHECKS(config), new_check;

    new_check = malloc(sizeof(struct checks_list));
    if (! new_check) {
        logger_write(LOG_CRIT, "%m\n");
        return 0;
    }
    CHECKS_LIST_CHECK(new_check) = custom_check;
    CHECKS_LIST_NEXT(new_check) = NULL;

    if (! checks_list) {
        CFG_CUSTOM_CHECKS(config) = new_check;
    }
    else {
        while (CHECKS_LIST_NEXT(checks_list))
            checks_list = CHECKS_LIST_NEXT(checks_list);
        CHECKS_LIST_NEXT(checks_list) = new_check;
    }

    return 1;
}

static int load_parameter(config_t config, const char *line)
{
    char *param, *value, buf[MAX_STR_CFG];
    int param_type;
    custom_check_t custom_check;

    /* strtok changes the original string, which we may not be
     * expecting at other points of the program */
    snprintf(buf, sizeof(buf), "%s", line);
    param = strtok(buf, CFG_DELIMITER_CHR);
    /* make sure there are no spaces */
    sanitize_str(param);
    param_type = get_param_type(param);
    if (param_type == CUSTOM_CHECK) {
        if ((custom_check=parse_custom_check(line))) {
            return append_custom_check(config, custom_check);
        } else {
            return 0;
        }
    } else {
        value = strtok(NULL, CFG_DELIMITER_CHR);
        switch(param_type)
        {
        case HTTP_PORT:
            load_int(&CFG_HTTP_PORT(config), value);
            return 1;
        case PG_PORT:
            load_int(&CFG_PG_PORT(config), value);
            return 1;
        case PG_CONNECTION_TIMEOUT:
            load_int(&CFG_PG_TIMEOUT(config), value);
            return 1;
        case SYSLOG_FACILITY:
            load_str(&CFG_SYSLOG_FACILITY(config), value);
            return 1;
        case PG_HOST:
            load_str(&CFG_PG_HOST(config), value);
            return 1;
        case PG_USER:
            load_str(&CFG_PG_USER(config), value);
            return 1;
        case PG_PASSWORD:
            load_str(&CFG_PG_PASSWORD(config), value);
            return 1;
        case PG_DATABASE:
            load_str(&CFG_PG_DATABASE(config), value);
            return 1;
        default:
            return 0;
        }
    }
}

extern config_t config_parse(const char *file_path)
{
    FILE *fp;
    char line[MAX_STR_CFG];
    config_t config = malloc(sizeof(struct config));

    if (! config) {
        logger_write(LOG_CRIT, "%s\n", strerror(errno));
        return NULL;
    }

    if (! (fp=fopen(file_path, "r"))) {
        logger_write(LOG_CRIT, "%s: %s\n", file_path, strerror(errno));
        free(config);
        return NULL;
    }

    CFG_CUSTOM_CHECKS(config) = NULL;
    while (fgets(line, sizeof(line), fp)) {
        sanitize_str(line);
        if (strcmp(line, "") != 0) {
            if (! load_parameter(config, line)) {
                logger_write(LOG_CRIT, STR_CFG_PARSE_ERROR_FMT, line);
                free(config);
                return NULL;
            } else {
                logger_write(LOG_INFO, STR_CFG_PARSE_SUCCESS_FMT, line);
            }
        }
    }

    fclose(fp);

    return config;
}

static void free_custom_checks(checks_list_t checks)
{
    if (checks == NULL)
        return;

    free_custom_checks(CHECKS_LIST_NEXT(checks));
    custom_check_destroy(CHECKS_LIST_CHECK(checks));
    free(checks);
}

extern void config_destroy(config_t config)
{
    free(CFG_SYSLOG_FACILITY(config));
    free(CFG_PG_HOST(config));
    free(CFG_PG_USER(config));
    free(CFG_PG_PASSWORD(config));
    free(CFG_PG_DATABASE(config));

    free_custom_checks(CFG_CUSTOM_CHECKS(config));

    free(config);
}

static void show_custom_checks(checks_list_t current_check)
{
    custom_check_t check;

    while (current_check) {
        check = CHECKS_LIST_CHECK(current_check);
        printf("%s : %s : %s\n", CUSTOM_CHECK_QUERY(check),
               CUSTOM_CHECK_RESULT(check),
               CUSTOM_CHECK_OPERATOR(check));
        current_check = CHECKS_LIST_NEXT(current_check);
    }
}

extern void config_show(config_t config)
{
    printf("http_port: %d\n", CFG_HTTP_PORT(config));
    printf("syslog_facility: %s\n", CFG_SYSLOG_FACILITY(config));
    printf("pg_host: %s\n", CFG_PG_HOST(config));
    printf("pg_port: %d\n", CFG_PG_PORT(config));
    printf("pg_user: %s\n", CFG_PG_USER(config));
    printf("pg_password: %s\n", CFG_PG_PASSWORD(config));
    printf("pg_database: %s\n", CFG_PG_DATABASE(config));
    printf("pg_connection_timeout: %d\n", CFG_PG_TIMEOUT(config));
    show_custom_checks(CFG_CUSTOM_CHECKS(config));
}
