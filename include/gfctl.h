/*
 * gfctl.h - Google Fiber Router Control Utility
 * 
 * Header file containing type definitions, structures, and function
 * declarations for the gfctl command-line utility.
 *
 * Copyright (c) 2025 - Licensed under MIT
 * 
 * Architecture designed for extensibility:
 *   - Modular command handlers
 *   - Pluggable output formatters
 *   - Abstract transport layer (HTTP/HTTPS)
 *   - Future: Web interface, multi-router aggregation
 */

#ifndef GFCTL_H
#define GFCTL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

/* Version information */
#define GFCTL_VERSION_MAJOR    1
#define GFCTL_VERSION_MINOR    0
#define GFCTL_VERSION_PATCH    2
#define GFCTL_VERSION_STRING   "1.0.2"

/* Default configuration */
#define GFCTL_DEFAULT_HOST     "10.0.0.1"
#define GFCTL_DEFAULT_PORT     443
#define GFCTL_DEFAULT_TIMEOUT  30
#define GFCTL_MAX_URL_LEN      512
#define GFCTL_MAX_BUFFER_SIZE  65536
#define GFCTL_MAX_JSON_DEPTH   32

/* Session token for unauthenticated access */
#define GFCTL_UNAUTH_TOKEN     "00000000000000000000000000000000"

/* ============================================================================
 * Enumerations
 * ============================================================================ */

/* Output format types */
typedef enum {
    OUTPUT_FORMAT_TEXT = 0,    /* Human-readable formatted text (default) */
    OUTPUT_FORMAT_JSON,        /* JSON output */
    OUTPUT_FORMAT_XML,         /* XML output */
    OUTPUT_FORMAT_CSV,         /* CSV output (for tabular data) */
    OUTPUT_FORMAT_RAW          /* Raw JSON-RPC response */
} output_format_t;

/* Error codes */
typedef enum {
    GFCTL_OK = 0,
    GFCTL_ERR_MEMORY = -1,
    GFCTL_ERR_NETWORK = -2,
    GFCTL_ERR_JSON_PARSE = -3,
    GFCTL_ERR_API = -4,
    GFCTL_ERR_AUTH = -5,
    GFCTL_ERR_TIMEOUT = -6,
    GFCTL_ERR_INVALID_ARG = -7,
    GFCTL_ERR_NOT_FOUND = -8,
    GFCTL_ERR_CURL_INIT = -9,
    GFCTL_ERR_SSL = -10,
    GFCTL_ERR_UNKNOWN = -99
} gfctl_error_t;

/* Connection medium types */
typedef enum {
    MEDIUM_UNKNOWN = 0,
    MEDIUM_ETHER,
    MEDIUM_WLAN2G,
    MEDIUM_WLAN5G,
    MEDIUM_WLAN6G
} medium_type_t;

/* ============================================================================
 * Core Structures
 * ============================================================================ */

/* Configuration structure */
typedef struct {
    char host[256];
    int port;
    char session_token[65];
    int timeout;
    bool verify_ssl;
    bool verbose;
    output_format_t output_format;
} gfctl_config_t;

/* Memory buffer for HTTP responses */
typedef struct {
    char *data;
    size_t size;
    size_t capacity;
} gfctl_buffer_t;

/* JSON-RPC request structure */
typedef struct {
    int id;
    const char *object;
    const char *method;
    const char *params;  /* Additional params as JSON string */
} jsonrpc_request_t;

/* IPv4 address info */
typedef struct {
    char address[16];
    int mask;
} ipv4_addr_t;

/* IPv6 address info */
typedef struct {
    char address[64];
    int mask;
    int preferred;
    int valid;
} ipv6_addr_t;

/* Network interface info */
typedef struct {
    bool up;
    ipv4_addr_t *ipv4_addrs;
    int ipv4_count;
    ipv6_addr_t *ipv6_addrs;
    int ipv6_count;
} net_interface_t;

/* Device info (LAN/WAN) */
typedef struct {
    net_interface_t lan;
    net_interface_t wan;
} device_info_t;

/* System info */
typedef struct {
    char version[32];
    char serial_number[64];
    unsigned long uptime;
    char last_acs_inform[64];
} system_info_t;

/* Wireless network info */
typedef struct {
    bool enable;
    bool up;
    char ssid[64];
    char network[32];  /* Primary, Guest */
    char band[16];     /* 2.4GHz, 5GHz, 6GHz */
} wireless_network_t;

/* Radio info */
typedef struct {
    char mac[18];
    medium_type_t type;
    int channel;
    char bandwidth[16];
} radio_info_t;

/* Interface info (on router) */
typedef struct {
    char name[16];
    char mac[18];
    medium_type_t medium_type;
    int speed;  /* Mbps, 0 for wireless */
} interface_info_t;

/* Connected client device */
typedef struct {
    char mac[18];
    char ip[16];
    char hostname[64];
    char ipv6[64];
    char connect_interface[18];
    char connect_method[16];
} client_device_t;

/* Mesh device (router or extender) */
typedef struct {
    char mac[18];
    char ip[16];
    char hostname[64];
    char ipv6[64];
    bool is_router;
    
    interface_info_t *interfaces;
    int interface_count;
    
    radio_info_t *radios;
    int radio_count;
    
    client_device_t *clients;
    int client_count;
    
    /* Backhaul info (for extenders) */
    bool has_backhaul;
    char backhaul_method[16];
    char backhaul_mac_connected[18];
    char backhaul_mac[18];
} mesh_device_t;

/* Network topology */
typedef struct {
    mesh_device_t *devices;
    int device_count;
} network_topology_t;

/* ============================================================================
 * Command Handler Structure (for extensibility)
 * ============================================================================ */

/* Forward declaration */
struct gfctl_context;

/* Command handler function type */
typedef int (*command_handler_t)(struct gfctl_context *ctx, int argc, char **argv);
typedef void (*help_handler_t)(const char *progname);

/* Command definition */
typedef struct {
    const char *name;
    const char *description;
    const char *usage;
    command_handler_t handler;
    help_handler_t help;
} gfctl_command_t;

/* Main context structure */
typedef struct gfctl_context {
    gfctl_config_t config;
    int request_id;
    void *curl_handle;  /* CURL* but kept as void* for header compatibility */
    gfctl_buffer_t response_buffer;
} gfctl_context_t;

/* ============================================================================
 * Function Declarations - Core
 * ============================================================================ */

/* Context management */
gfctl_context_t *gfctl_init(void);
void gfctl_cleanup(gfctl_context_t *ctx);

/* Configuration */
int gfctl_set_host(gfctl_context_t *ctx, const char *host);
int gfctl_set_port(gfctl_context_t *ctx, int port);
int gfctl_set_session(gfctl_context_t *ctx, const char *token);
int gfctl_set_output_format(gfctl_context_t *ctx, output_format_t format);

/* Buffer management */
gfctl_buffer_t *gfctl_buffer_new(size_t initial_capacity);
int gfctl_buffer_append(gfctl_buffer_t *buf, const char *data, size_t len);
void gfctl_buffer_clear(gfctl_buffer_t *buf);
void gfctl_buffer_free(gfctl_buffer_t *buf);

/* ============================================================================
 * Function Declarations - HTTP/Transport Layer
 * ============================================================================ */

int gfctl_http_init(gfctl_context_t *ctx);
void gfctl_http_cleanup(gfctl_context_t *ctx);
int gfctl_http_post(gfctl_context_t *ctx, const char *endpoint, 
                    const char *body, gfctl_buffer_t *response);

/* ============================================================================
 * Function Declarations - JSON-RPC Layer
 * ============================================================================ */

char *gfctl_jsonrpc_build_request(gfctl_context_t *ctx, const char *object,
                                   const char *method, const char *params);
int gfctl_jsonrpc_call(gfctl_context_t *ctx, const char *object,
                       const char *method, const char *params,
                       gfctl_buffer_t *response);
char *gfctl_jsonrpc_get_result(const char *response);
int gfctl_jsonrpc_get_status(const char *response);

/* ============================================================================
 * Function Declarations - API Commands
 * ============================================================================ */

/* System information */
int gfctl_get_system_info(gfctl_context_t *ctx, system_info_t *info);
int gfctl_get_op_mode(gfctl_context_t *ctx, char *mode, size_t mode_len);

/* Network/Device information */
int gfctl_get_device_info(gfctl_context_t *ctx, device_info_t *info);
void gfctl_free_device_info(device_info_t *info);
int gfctl_get_network_topology(gfctl_context_t *ctx, network_topology_t *topo);
void gfctl_free_topology(network_topology_t *topo);

/* Wireless information */
int gfctl_get_wireless_info(gfctl_context_t *ctx, wireless_network_t **networks, int *count);
void gfctl_free_wireless_info(wireless_network_t *networks);

/* Session management */
int gfctl_session_login(gfctl_context_t *ctx);
int gfctl_ensure_session(gfctl_context_t *ctx);
int gfctl_check_session(gfctl_context_t *ctx, bool *valid);

/* ============================================================================
 * Function Declarations - Output Formatters
 * ============================================================================ */

/* System info output */
void gfctl_output_system_info(gfctl_context_t *ctx, const system_info_t *info);

/* Device info output */
void gfctl_output_device_info(gfctl_context_t *ctx, const device_info_t *info);

/* Topology output */
void gfctl_output_topology(gfctl_context_t *ctx, const network_topology_t *topo);

/* Wireless info output */
void gfctl_output_wireless(gfctl_context_t *ctx, const wireless_network_t *networks, int count);

/* Clients output */
void gfctl_output_clients(gfctl_context_t *ctx, const network_topology_t *topo);

/* Generic JSON/XML formatters */
void gfctl_output_raw_json(const char *json);

/* ============================================================================
 * Function Declarations - Command Handlers
 * ============================================================================ */

int cmd_status(gfctl_context_t *ctx, int argc, char **argv);
int cmd_system(gfctl_context_t *ctx, int argc, char **argv);
int cmd_devices(gfctl_context_t *ctx, int argc, char **argv);
int cmd_topology(gfctl_context_t *ctx, int argc, char **argv);
int cmd_wireless(gfctl_context_t *ctx, int argc, char **argv);
int cmd_clients(gfctl_context_t *ctx, int argc, char **argv);
int cmd_raw(gfctl_context_t *ctx, int argc, char **argv);

void help_status(const char *progname);
void help_system(const char *progname);
void help_devices(const char *progname);
void help_topology(const char *progname);
void help_wireless(const char *progname);
void help_clients(const char *progname);
void help_raw(const char *progname);

/* ============================================================================
 * Utility Functions
 * ============================================================================ */

const char *gfctl_error_string(gfctl_error_t err);
const char *gfctl_medium_type_string(medium_type_t type);
medium_type_t gfctl_parse_medium_type(const char *str);
char *gfctl_format_uptime(unsigned long seconds);
output_format_t gfctl_parse_format(const char *str);

/* Simple JSON helpers (minimal parser for our specific use case) */
char *json_get_string(const char *json, const char *key);
int json_get_int(const char *json, const char *key, int default_val);
bool json_get_bool(const char *json, const char *key, bool default_val);
char *json_get_object(const char *json, const char *key);
char *json_get_array(const char *json, const char *key);
int json_array_count(const char *json);
char *json_array_get(const char *json, int index);
char **json_object_keys(const char *json, int *count);

#endif /* GFCTL_H */
