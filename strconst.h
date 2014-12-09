#ifndef STRCONST_H
#define STRCONST_H


#define MAX_STR 128
#define MAX_STR_CFG 1024
#define MAX_STR_HTML 4096

#ifdef DEBUG
#define CONFIG_FILE "pgdoctor.cfg"
#else
#define CONFIG_FILE "/etc/pgdoctor.cfg"
#endif

#define STR_PG_CONN_INFO_FMT "host=%s port=%d dbname=%s user=%s password=%s connect_timeout=%d"
#define STR_REPLICATION_LAG_QUERY "SELECT extract(seconds from (now() - pg_last_xact_replay_timestamp())) AS replication_lag;"
#define STR_HTML_FMT "<html><body>%s</body></html>"
#define STR_OK "OK"
#define STR_NO_REPLICATION_INFO "Unknown replication state"
#define STR_BAD_REQUEST "Bad request"

#define COMMENT_CHR '#'


#endif
