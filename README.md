# pgbalancer — Modern PostgreSQL Connection Pooler with REST API

[![PostgreSQL](https://img.shields.io/badge/PostgreSQL-13+-blue.svg)](https://postgresql.org/)
[![License](https://img.shields.io/badge/License-PostgreSQL-yellow.svg)](COPYING)
[![Documentation](https://img.shields.io/badge/docs-latest-blue.svg)](https://pgelephant.github.io/pgbalancer/)

## Build Status

| Platform | PostgreSQL 13 | PostgreSQL 14 | PostgreSQL 15 | PostgreSQL 16 | PostgreSQL 17 | PostgreSQL 18 |
|----------|---------------|---------------|---------------|---------------|---------------|---------------|
| **Ubuntu** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **macOS** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |
| **Rocky** | ✓ | ✓ | ✓ | ✓ | ✓ | ✓ |

**pgbalancer** is a modern, production-ready PostgreSQL connection pooler and load balancer that provides a comprehensive REST API and professional CLI tool. Built as a fork of pgpool-II with YAML configuration and modern HTTP-based management.

**Now part of the unified pgElephant high-availability suite.**
- Consistent UI and documentation across [pgbalancer](https://pgelephant.com/pgbalancer), [RAM](https://pgelephant.com/ram), [RALE](https://pgelephant.com/rale), and [FauxDB](https://pgelephant.com/fauxdb).
- All product pages use a single, professional template for a seamless experience.
- See the [website](https://pgelephant.com) for live demos and feature comparisons.

**Supported PostgreSQL versions**: 13, 14, 15, 16, 17, 18

## Quick Links

- **[Documentation](https://pgelephant.github.io/pgbalancer/)** - Complete documentation site
- **[Quick Start Guide](https://pgelephant.github.io/pgbalancer/getting-started/quick-start/)** - Get running in minutes
- **[REST API Reference](https://pgelephant.github.io/pgbalancer/api/rest/)** - Complete API documentation
- **[CLI Reference](https://pgelephant.github.io/pgbalancer/user-guide/cli/)** - bctl command reference
- **[Contributing](CONTRIBUTING.md)** - How to contribute

## Key Features

- **Modern REST API**: Production-ready HTTP/JSON API server integrated as child process (port 8080)
- **Professional CLI Tool**: `bctl` command-line client with table/JSON output and real-time pgbalancer data
- **JWT Authentication**: Optional HMAC-SHA256 JWT tokens for secure API access
- **YAML Configuration**: Modern configuration format with libyaml integration
- **Connection Pooling**: Efficient PostgreSQL connection pooling and load balancing
- **High Availability**: Automatic failover and node recovery capabilities
- **Health Monitoring**: Built-in health checks and performance monitoring
- **Watchdog Support**: Automatic failover and recovery management
- **Query Routing**: Intelligent query distribution across backend servers with AI/heuristic algorithms
- **Session Management**: Persistent connections with automatic cleanup
- **Security**: SSL/TLS support, JWT authentication, and access control

## What's New vs pgpool-II

### **REST API Management**
- **Before**: Binary PCP protocol requiring specialized clients
- **After**: Production HTTP/JSON REST API integrated as child process
- **Features**: 17 endpoints, real-time backend data, JWT authentication, < 10ms response time

### **Unified CLI Tool**
- **Before**: Multiple separate pcp_* commands for different operations
- **After**: Single `bctl` tool with 3 output formats (default/table/JSON)
- **Features**: Box-drawing tables, real pgbalancer data, remote connections, verbose mode

### **YAML Configuration**
- **Before**: Traditional `.conf` file format
- **After**: Modern YAML configuration with validation

### **Authentication**
- **Before**: Basic password authentication only
- **After**: Optional JWT authentication with HMAC-SHA256 tokens
- **Features**: Login endpoint, token expiry, Bearer token format, backwards compatible

## Installation

### Quick install (60 seconds)

Prerequisites: PostgreSQL 13+ with development headers, make, gcc/clang, json-c, libyaml

```bash
# Clone and configure
git clone https://github.com/pgelephant/pgbalancer.git
cd pgbalancer
./configure --with-openssl --with-pam --with-ldap

# Build core components
make -C src startup.o  # Test compilation
make -C bin/bctl      # Build CLI tool

# Install
sudo make install
```

**For detailed installation instructions, see the [Installation Guide](https://pgelephant.github.io/pgbalancer/getting-started/installation/).**

## Configuration

### YAML Configuration (Recommended)

Create `pgbalancer.yaml`:

```yaml
# pgBalancer YAML Configuration
server:
  listen_addresses: "*"
  port: 5432
  unix_socket_directories: ["/tmp"]
  unix_socket_permissions: 0777
  pid_file: "/tmp/pgbalancer.pid"

# Backend database servers
backend_servers:
  - hostname: "localhost"
    port: 5433
    weight: 1
    role: "primary"
    data_directory: "/usr/local/pgsql.17/data1"
    
  - hostname: "localhost"
    port: 5434
    weight: 1
    role: "standby"
    data_directory: "/usr/local/pgsql.17/data2"

# Connection pooling
pool:
  num_init_children: 32
  max_pool: 4
  child_life_time: 300
  child_max_connections: 0

# Health checking
health_check:
  health_check_period: 30
  health_check_timeout: 20
  health_check_user: "postgres"
  health_check_password: "postgres"
  health_check_database: "postgres"
```

**For complete configuration reference, see the [Configuration Guide](https://pgelephant.github.io/pgbalancer/user-guide/configuration/).**

## Quick Start

### 1. Start pgbalancer

```bash
# Start with YAML config
pgbalancer -f pgbalancer.yaml

# Or with traditional .conf
pgbalancer -f /etc/pgbalancer/pgbalancer.conf
```

### 2. Use the CLI tool

```bash
# Check status
bctl status

# List nodes
bctl nodes

# Attach a node
bctl nodes attach 1

# Check health
bctl health

# Reload configuration
bctl reload
```

### 3. Connect via REST API

```bash
# Get server status
curl http://localhost:8080/api/status

# Get node information
curl http://localhost:8080/api/nodes

# Reload configuration
curl -X POST http://localhost:8080/api/reload
```

**For complete setup instructions, see the [Quick Start Guide](https://pgelephant.github.io/pgbalancer/getting-started/quick-start/).**

## CLI Tool (bctl)

The `bctl` tool provides comprehensive management of pgbalancer instances with multiple output formats:

### Output Formats

```bash
# Default format (verbose)
bctl nodes

# Table format (beautiful box-drawing tables)
bctl --table nodes
bctl -t nodes

# JSON format (machine-readable)
bctl --json nodes
bctl -j nodes
```

**Example Table Output:**
```
┌────┬─────────────────┬───────┬──────────┬────────┬─────────┬──────────┐
│ ID │ Host            │ Port  │ Status   │ Weight │ Role    │ Rep Lag  │
├────┼─────────────────┼───────┼──────────┼────────┼─────────┼──────────┤
│ 0  │ localhost       │ 5432  │ up       │ 1      │ primary │ 0        │
│ 1  │ localhost       │ 5433  │ down     │ 1      │ standby │ 0        │
└────┴─────────────────┴───────┴──────────┴────────┴─────────┴──────────┘
```

### Core Commands

```bash
# Server management
bctl status              # Show server status with real-time data
bctl stop                # Stop server gracefully
bctl reload              # Reload configuration without restart
bctl logrotate           # Rotate log files

# Node management (uses real pgbalancer backend data)
bctl nodes               # List all backend nodes
bctl -t nodes            # List nodes in table format
bctl nodes-count         # Show total node count
bctl nodes-attach ID     # Attach node by ID
bctl nodes-detach ID     # Detach node by ID
bctl nodes-recovery ID   # Initiate node recovery
bctl nodes-promote ID    # Promote node to primary

# Process management
bctl processes           # List processes
bctl processes-count     # Show process count

# Monitoring
bctl health              # Health monitoring
bctl cache               # Cache management

# Watchdog management
bctl watchdog-status     # Show watchdog status
bctl watchdog-start      # Start watchdog
bctl watchdog-stop       # Stop watchdog
```

### Options

```bash
bctl -H localhost -p 8080 -U admin -v --json status
```

**For complete CLI reference, see the [CLI Guide](https://pgelephant.github.io/pgbalancer/user-guide/cli/).**

## REST API

Production-ready HTTP/JSON REST API server integrated as pgbalancer child process. The REST API provides real-time access to pgbalancer state and management functions.

### Architecture

The REST API runs as a dedicated child process (PT_REST_API) within pgbalancer:

```
pgbalancer (main process)
    ├─ REST API child (port 8080) - Mongoose HTTP server
    ├─ PCP child (port 9898) - Legacy binary protocol  
    ├─ Worker processes
    ├─ Health check processes
    └─ Watchdog processes
```

### REST API Endpoints (17 total)

**Authentication** (JWT optional, disabled by default):
```bash
POST   /api/v1/auth/login           # Get JWT token
```

**Server Management**:
```bash
GET    /api/v1/status               # Server status (real-time data)
GET    /api/v1/health/stats         # Health check statistics
POST   /api/v1/control/stop         # Stop server
POST   /api/v1/control/reload       # Reload configuration
POST   /api/v1/control/logrotate    # Rotate logs
```

**Node Management** (real pgbalancer backend data):
```bash
GET    /api/v1/nodes                # List all backend nodes
GET    /api/v1/nodes/{id}           # Get specific node info
POST   /api/v1/nodes/{id}/attach    # Attach node
POST   /api/v1/nodes/{id}/detach    # Detach node
POST   /api/v1/nodes/{id}/recovery  # Initiate recovery
POST   /api/v1/nodes/{id}/promote   # Promote to primary
```

**Process & Cache**:
```bash
GET    /api/v1/processes            # List processes
POST   /api/v1/cache/invalidate     # Invalidate query cache
```

**Watchdog**:
```bash
GET    /api/v1/watchdog/info        # Watchdog information
GET    /api/v1/watchdog/status      # Watchdog status
POST   /api/v1/watchdog/start       # Start watchdog
POST   /api/v1/watchdog/stop        # Stop watchdog
```

### Example API Usage

**Basic Queries** (no authentication required by default):
```bash
# Get server status (real-time data)
curl http://localhost:8080/api/v1/status
# Response: {"status":"running","uptime":100,"connections":5,"nodes":3,"healthy_nodes":1}

# List all backend nodes (real pgbalancer backends)
curl http://localhost:8080/api/v1/nodes | jq '.'
# Response: {"nodes":[{"id":0,"host":"localhost","port":5432,"status":"up",...}]}

# Get health statistics
curl http://localhost:8080/api/v1/health/stats | jq '.'
```

**Node Operations**:
```bash
# Attach node 0
curl -X POST http://localhost:8080/api/v1/nodes/0/attach

# Detach node 1
curl -X POST http://localhost:8080/api/v1/nodes/1/detach

# Promote node 1 to primary
curl -X POST http://localhost:8080/api/v1/nodes/1/promote

# Reload configuration
curl -X POST http://localhost:8080/api/v1/control/reload
```

**JWT Authentication** (optional, enable by setting `JWT_ENABLED = 1`):
```bash
# Get JWT token
TOKEN=$(curl -s -X POST http://localhost:8080/api/v1/auth/login | jq -r .token)

# Use token for authenticated requests
curl -H "Authorization: Bearer $TOKEN" http://localhost:8080/api/v1/status

# Token expires in 1 hour (3600 seconds)
```

**Integration with Scripts/Monitoring**:
```bash
# Check if pgbalancer is healthy
if curl -s http://localhost:8080/api/v1/status | jq -e '.healthy_nodes > 0'; then
    echo "pgbalancer has healthy backends"
fi

# Get node count
curl -s http://localhost:8080/api/v1/nodes | jq '.nodes | length'

# Monitor uptime
curl -s http://localhost:8080/api/v1/status | jq '.uptime'
```

**For complete API documentation, see the [REST API Reference](https://pgelephant.github.io/pgbalancer/api/rest/).**

## How It Works

```
Client Application
    ↓ (PostgreSQL protocol)
pgbalancer (Connection Pooler)
    ↓ (Connection pooling)
PostgreSQL Backend Servers
    ↓ (Health monitoring)
Watchdog (High Availability)
```

**Components:**
- **Connection Pooler**: Efficient connection management and load balancing
- **REST API Server**: HTTP/JSON management interface
- **CLI Tool**: Professional command-line management
- **Health Checker**: Automatic backend monitoring
- **Watchdog**: High availability and failover management

**For detailed architecture, see the [Architecture Guide](https://pgelephant.github.io/pgbalancer/concepts/architecture/).**

## High Availability

pgbalancer provides comprehensive high availability features:

### Automatic Failover
- **Health Monitoring**: Continuous backend server health checks
- **Automatic Detection**: Fast detection of failed nodes
- **Seamless Failover**: Automatic routing to healthy backends
- **Recovery Support**: Automatic reconnection when nodes recover

### Watchdog Integration
- **Multi-Node Support**: Multiple pgbalancer instances
- **Leader Election**: Automatic leader selection
- **Failover Coordination**: Coordinated failover across instances

**Learn more: [High Availability Guide](https://pgelephant.github.io/pgbalancer/concepts/ha/)**

## Examples

### Three-Node Setup

Configure pgbalancer with multiple backends:

```yaml
backend_servers:
  - hostname: "pg-primary"
    port: 5432
    weight: 1
    role: "primary"
    
  - hostname: "pg-replica1"
    port: 5432
    weight: 1
    role: "standby"
    
  - hostname: "pg-replica2"
    port: 5432
    weight: 1
    role: "standby"
```

### Load Balancing

```bash
# Connect through pgbalancer
psql -h localhost -p 5432 -U postgres mydb

# Check which backend is being used
bctl processes

# Monitor load distribution
bctl health
```

**For complete examples, see the [Examples Guide](https://pgelephant.github.io/pgbalancer/user-guide/examples/).**

## Monitoring

### CLI Monitoring

```bash
# Quick health check
bctl health

# Detailed node status
bctl nodes

# Process information
bctl processes

# Cache statistics
bctl cache
```

### REST API Monitoring

```bash
# Get comprehensive status
curl http://localhost:8080/api/status | jq '.'

# Monitor health
curl http://localhost:8080/api/health | jq '.'

# Check processes
curl http://localhost:8080/api/processes | jq '.'
```

**For comprehensive monitoring guide, see [Monitoring](https://pgelephant.github.io/pgbalancer/operations/monitoring/).**

## Troubleshooting

**Common issues:**

- **Cannot connect**: Check `listen_addresses` and firewall settings
- **Backend not found**: Verify backend server configuration and connectivity
- **CLI connection failed**: Ensure pgbalancer is running and REST API is enabled
- **YAML parsing errors**: Validate YAML syntax and indentation

**For complete troubleshooting guide, see [Troubleshooting](https://pgelephant.github.io/pgbalancer/operations/troubleshooting/).**

## Development

Build and test:

```bash
# Build core components
make -C src

# Build CLI tool
make -C bin/bctl

# Run tests
python3 test_system.py

# Check configuration
bctl --help
```

**For development guide, see [Development](https://pgelephant.github.io/pgbalancer/development/).**

## Performance

- **Connection Pooling**: Efficient connection reuse and management
- **Load Balancing**: Intelligent query distribution
- **Health Checks**: Configurable monitoring intervals
- **Memory Usage**: Optimized for production workloads
- **Throughput**: High-performance connection handling

## Architecture

pgbalancer uses a modern architecture with REST API management, YAML configuration, and professional CLI tools.

**For detailed architecture information, see the [Architecture Guide](https://pgelephant.github.io/pgbalancer/concepts/architecture/).**

## License

PostgreSQL License - see COPYING file for details.

## Documentation

**Complete documentation is available at: https://pgelephant.github.io/pgbalancer/**

### Documentation Sections

- **[Getting Started](https://pgelephant.github.io/pgbalancer/getting-started/)** - Installation and quick start
- **[User Guide](https://pgelephant.github.io/pgbalancer/user-guide/)** - Configuration and usage
- **[API Reference](https://pgelephant.github.io/pgbalancer/api/)** - REST API and CLI documentation
- **[Core Concepts](https://pgelephant.github.io/pgbalancer/concepts/)** - Architecture and features
- **[Operations](https://pgelephant.github.io/pgbalancer/operations/)** - Monitoring and best practices
- **[Development](https://pgelephant.github.io/pgbalancer/development/)** - Building and contributing

## Community and Support

- **Documentation**: [https://pgelephant.github.io/pgbalancer/](https://pgelephant.github.io/pgbalancer/)
- **Issues**: [GitHub Issues](https://github.com/pgelephant/pgbalancer/issues)
- **Contributing**: [CONTRIBUTING.md](CONTRIBUTING.md)
- **License**: [PostgreSQL License](COPYING)

## Project Status

**Status**: Production Ready  
**Version**: 1.0.0  
**Base**: pgpool-II fork with modern REST API  
**Quality**: Professional CLI tool and comprehensive REST API

## Related Projects

- **[pgpool-II](https://www.pgpool.net/)** - Original PostgreSQL connection pooler
- **[PostgreSQL](https://www.postgresql.org/)** - The world's most advanced open source database
- **[pgraft](https://pgelephant.com/pgraft)** - Raft-based PostgreSQL extension for high availability
- **[RAM](https://pgelephant.com/ram)** - PostgreSQL clustering and failover manager
- **[RALE](https://pgelephant.com/rale)** - Distributed consensus and key-value store
- **[FauxDB](https://pgelephant.com/fauxdb)** - MongoDB-compatible query proxy for PostgreSQL

## SEO/Discoverability keywords

PostgreSQL connection pooler, pgpool REST API, PostgreSQL load balancer, pgbalancer CLI, PostgreSQL high availability, connection pooling, PostgreSQL YAML configuration, REST API management, bctl command line tool, PostgreSQL cluster management, pgpool-II fork, modern PostgreSQL tools

---

## License

Copyright (c) 2003-2021 PgPool Global Development Group  
Copyright (c) 2024-2025, pgElephant, Inc.

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

Made with care for the PostgreSQL community
