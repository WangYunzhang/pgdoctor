# pgDoctor for Amazon Linux 2023

Simple, lightweight, web service used to define and run custom health checks on PostgreSQL instances. This is a modified version adapted for Amazon Linux 2023.

Repository: https://github.com/WangYunzhang/pgdoctor

This is a fork of [original pgDoctor](https://github.com/thumbtack/pgdoctor) with modifications to support Amazon Linux 2023 and remove certain features.

## Modifications
- Removed streaming replication lag check functionality
- Updated systemd service configuration for Amazon Linux 2023
- Adapted build system for AL2023
- Improved error handling and logging
- Added systemd service auto-restart capability

## Dependencies

For Amazon Linux 2023:
```bash
# Install required development packages
sudo dnf install -y git
sudo dnf install -y gcc make
sudo dnf install -y libpq-devel  # PostgreSQL development files
sudo dnf install -y libmicrohttpd-devel # HTTP server library
```

For running tests (optional):
```bash
sudo dnf install -y check-devel # Unit test framework
```

## Build and Install

* Building:
```bash
make
```

* Installing:
```bash
sudo make install
```

This will:
- Install the binary to `/usr/local/bin/pgdoctor`
- Install the configuration file to `/etc/pgdoctor.cfg`
- Install the systemd service file to `/etc/systemd/system/pgdoctor.service`

## Configuration

A default configuration file is created under `/etc/pgdoctor.cfg`. Each setting is preceded by a comment describing it briefly.

### Runtime Settings
| Parameter        | Description           | Default  |
| ------------- |-------------|-----|
| `http_port`      | Port to listen on | 8071 |
| `syslog_facility` | Syslog facility (local) to messages log to | `local7` |

### Target PostgreSQL Instance
| Parameter        | Description           | Default  |
| ------------- |-------------|-----|
| `pg_host` | Host name of the instance pgDoctor will connect to | `localhost` |
| `pg_port` | Port on which the target server is listening | 5432 |
| `pg_user` | User to connect with | `postgres` |
| `pg_password` | Password to use with `pg_user` | *empty* |
| `pg_database` | Name of the database to connect to | `postgres` |

### Health Checks
| Parameter        | Description           | Default  |
| ------------- |-------------|-----|
| `pg_connection_timeout` | Timeout (seconds) when connecting to PostgreSQL | 3 |

### Custom Health Checks
pgDoctor supports the definition of custom health checks in the form of arbitrary SQL queries — one check per line.

These may be defined in two forms:
* A plain SQL query (the health check is considered successful if and only if it is executed without any errors)
* A SQL query (must return exactly one field) **and** a condition (the health check is considered successful when the query is executed without errors *and* the condition evaluates to `true`)

Conditional checks are of the form:
```
"QUERY" comparison_operator "VALUE"
```
where:
- `comparison_operator` is one of `<`, `>`, or `=`
- `QUERY` is any valid SQL command (surrounded by double-quotes)
- `VALUE` is the expected result (also surrounded by double-quotes)

When using `=`, a string comparison is performed. For both `<` and `>` floating point values are used.

Examples:
```
# Simple check - passes if query executes successfully
"SELECT 1"

# Conditional check - passes if on_rotation equals 1
"SELECT on_rotation FROM maintenance WHERE hostname = 'production-replica3'" = "1"
```

## Service Management

Enable and start the service:
```bash
chmod a+r /etc/pgdoctor.cfg
sudo systemctl daemon-reload
sudo systemctl enable pgdoctor
sudo systemctl start pgdoctor
```

Check service status:
```bash
sudo systemctl status pgdoctor
```

Test the health check endpoint:
```bash
curl http://localhost:8071/
```

## Service Configuration

The systemd service is configured to:
- Run as the postgres user
- Depend on postgresql.service
- Auto-restart on failure
- Use standard logging facilities

## License

This project is licensed under the Apache License 2.0 - see the LICENSE file for details.
