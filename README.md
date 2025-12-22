# gfctl - Google Fiber Router Control Utility

Version 1.0.2

A command-line tool for monitoring and administering Google Fiber routers running OpenWRT with the ubus JSON-RPC interface.

## Features

- **Automatic Authentication**: Automatically establishes a session with the router using local network authentication
- **System Information**: View firmware version, serial number, uptime, and operation mode
- **Network Configuration**: Display LAN and WAN interface details including IPv4 and IPv6 addresses
- **Network Topology**: See all mesh devices (router and extenders), their interfaces, and radio configurations
- **Wireless Networks**: View SSID configurations for all bands (2.4GHz, 5GHz, 6GHz)
- **Connected Clients**: List all devices connected to your network with connection details
- **Multiple Output Formats**: Text (default), JSON, XML, and CSV
- **Raw API Access**: Make direct JSON-RPC calls to the router

## Authentication

The tool automatically authenticates with the router when run from the local network. Google Fiber routers allow read-only access to network information for users connecting from the LAN using the "local" user credentials (no password required).

The authentication flow:
1. gfctl sends a `session.login` request with username "local" and empty password
2. The router returns a session token (valid for the duration of the session)
3. All subsequent API calls use this session token

**Note**: This tool must be run from a device connected to your Google Fiber network.

## Requirements

- C99 compatible compiler (gcc, clang)
- libcurl with SSL support
- POSIX-compatible operating system (macOS, Linux)

### Installing Dependencies

**macOS (Homebrew):**
```bash
brew install curl
```

**Ubuntu/Debian:**
```bash
sudo apt-get install build-essential libcurl4-openssl-dev
```

**RHEL/CentOS/Fedora:**
```bash
sudo yum install gcc make libcurl-devel
# or
sudo dnf install gcc make libcurl-devel
```

## Building

```bash
# Clone or extract the source
cd gfctl

# Check dependencies
make check-deps

# Build release version
make

# Or build debug version
make debug

# Install to /usr/local/bin (optional)
sudo make install
```

## Usage

```
gfctl [options] <command> [command-options]
```

### Global Options

| Option | Description |
|--------|-------------|
| `-h, --help` | Show help message |
| `-V, --version` | Show version information |
| `-H, --host <host>` | Router hostname/IP (default: 10.0.0.1) |
| `-p, --port <port>` | Router port (default: 443) |
| `-j, --json` | Output in JSON format |
| `-x, --xml` | Output in XML format |
| `-c, --csv` | Output in CSV format (where applicable) |
| `-r, --raw` | Output raw JSON-RPC response |
| `-v, --verbose` | Enable verbose output |
| `-t, --timeout <sec>` | Request timeout in seconds (default: 30) |
| `-s, --session <tok>` | Session token for authenticated requests |

### Commands

| Command | Description |
|---------|-------------|
| `status` | Show overall router status (default) |
| `system` | Show system information |
| `devices` | Show LAN/WAN network configuration |
| `topology` | Show network topology (router, extenders, radios) |
| `wireless` | Show wireless network configuration |
| `clients` | Show connected client devices |
| `raw` | Make raw JSON-RPC call to the router |

### Examples

```bash
# Show overall status (default command)
gfctl

# Show system information in JSON format
gfctl -j system

# Show all connected clients in CSV format
gfctl -c clients

# Show network topology
gfctl topology

# Show wireless networks in XML format
gfctl -x wireless

# Connect to a different router
gfctl -H 192.168.1.1 status

# Make a raw API call
gfctl raw data_repo.webinfo system

# Make a raw API call with parameters
gfctl raw network topology '{}'

# Verbose output for debugging
gfctl -v status
```

## Output Formats

### Text (Default)
Human-readable formatted output suitable for terminal display.

### JSON (`--json`)
Structured JSON output for parsing by other tools or scripts.

### XML (`--xml`)
XML output for integration with enterprise tools.

### CSV (`--csv`)
Comma-separated values for import into spreadsheets (available for `clients` command).

### Raw (`--raw`)
Unprocessed JSON-RPC response from the router.

## Architecture

The project is designed with extensibility in mind:

```
gfctl/
├── include/
│   └── gfctl.h          # Main header with all type definitions
├── src/
│   ├── main.c           # Entry point and argument parsing
│   ├── core.c           # Context management and utilities
│   ├── http.c           # HTTP/HTTPS transport layer (libcurl)
│   ├── jsonrpc.c        # JSON-RPC 2.0 protocol implementation
│   ├── json_utils.c     # Minimal JSON parser
│   ├── api.c            # High-level API functions
│   ├── output.c         # Output formatters (text, JSON, XML, CSV)
│   └── commands.c       # CLI command handlers
├── Makefile
└── README.md
```

### Key Components

1. **Transport Layer** (`http.c`): Handles HTTPS communication with libcurl
2. **JSON-RPC Layer** (`jsonrpc.c`): Implements the JSON-RPC 2.0 protocol for ubus
3. **API Layer** (`api.c`): Provides high-level functions for specific data retrieval
4. **Output Layer** (`output.c`): Formats data for different output types
5. **Command Layer** (`commands.c`): Implements CLI subcommands

### Extending the Tool

**Adding a new command:**
1. Add command handler in `commands.c`
2. Add help function in `commands.c`
3. Register command in the `commands[]` table in `main.c`

**Adding a new output format:**
1. Add format type to `output_format_t` enum in `gfctl.h`
2. Update `gfctl_parse_format()` in `core.c`
3. Add format case to output functions in `output.c`

**Adding a new API endpoint:**
1. Add data structures to `gfctl.h`
2. Implement fetch function in `api.c`
3. Implement output function in `output.c`

## Future Enhancements

The architecture supports planned future additions:

- **Web Interface**: The modular design allows adding a web server that reuses the API layer
- **Multi-Router Aggregation**: Context-based design allows managing multiple routers
- **Configuration Changes**: API layer can be extended for write operations
- **Monitoring/Alerts**: Output formatters support integration with monitoring systems

## API Reference

The router exposes these ubus endpoints (discovered from HAR analysis):

| Object | Method | Description |
|--------|--------|-------------|
| `session` | `access`, `get`, `login`, `destroy` | Session management |
| `data_repo.webinfo` | `system` | Firmware version, serial, uptime |
| `data_repo.webinfo` | `devices` | LAN/WAN interface info |
| `data_repo.webinfo` | `wireless` | Wireless network config |
| `network` | `topology` | Full network topology |
| `op-mode` | `get` | Operation mode (router/bridge) |

## Troubleshooting

### Connection Errors

- Verify router IP address (default: 10.0.0.1)
- Check network connectivity: `ping 10.0.0.1`
- Ensure HTTPS port 443 is accessible

### SSL Certificate Errors

The router uses a self-signed certificate. The tool disables SSL verification by default. If you encounter SSL errors, ensure libcurl was built with SSL support.

### Access Denied Errors

Some endpoints require authentication. The tool uses an unauthenticated session by default. For full access, you may need to obtain a session token from the router's web interface.

### Verbose Mode

Use `-v` flag to see detailed request/response information:
```bash
gfctl -v status
```

## License

MIT License - See LICENSE file for details.

## Contributing

Contributions are welcome! Please ensure code follows the existing style and includes appropriate documentation.
