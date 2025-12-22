/*
 * commands.c - Command Handlers for gfctl
 *
 * Implementation of CLI subcommands.
 */

#include "gfctl.h"

/*
 * cmd_status - Show overall system status (combines system + devices)
 */
int cmd_status(gfctl_context_t *ctx, int argc, char **argv) {
    system_info_t sys_info;
    device_info_t dev_info;
    char mode[32];
    int ret;
    
    (void)argc;
    (void)argv;
    
    /* Get system info */
    ret = gfctl_get_system_info(ctx, &sys_info);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error getting system info: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    /* Get operation mode */
    ret = gfctl_get_op_mode(ctx, mode, sizeof(mode));
    if (ret != GFCTL_OK) {
        strcpy(mode, "unknown");
    }
    
    /* Get device info */
    ret = gfctl_get_device_info(ctx, &dev_info);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error getting device info: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    /* Output based on format */
    switch (ctx->config.output_format) {
        case OUTPUT_FORMAT_JSON:
            printf("{\n");
            printf("  \"system\": {\n");
            printf("    \"version\": \"%s\",\n", sys_info.version);
            printf("    \"serial_number\": \"%s\",\n", sys_info.serial_number);
            printf("    \"uptime\": %lu,\n", sys_info.uptime);
            printf("    \"uptime_formatted\": \"%s\",\n", gfctl_format_uptime(sys_info.uptime));
            printf("    \"mode\": \"%s\"\n", mode);
            printf("  },\n");
            printf("  \"lan\": {\n");
            printf("    \"up\": %s", dev_info.lan.up ? "true" : "false");
            if (dev_info.lan.ipv4_count > 0) {
                printf(",\n    \"ipv4\": \"%s/%d\"",
                       dev_info.lan.ipv4_addrs[0].address,
                       dev_info.lan.ipv4_addrs[0].mask);
            }
            printf("\n  },\n");
            printf("  \"wan\": {\n");
            printf("    \"up\": %s", dev_info.wan.up ? "true" : "false");
            if (dev_info.wan.ipv4_count > 0) {
                printf(",\n    \"ipv4\": \"%s/%d\"",
                       dev_info.wan.ipv4_addrs[0].address,
                       dev_info.wan.ipv4_addrs[0].mask);
            }
            if (dev_info.wan.ipv6_count > 0) {
                printf(",\n    \"ipv6\": \"%s/%d\"",
                       dev_info.wan.ipv6_addrs[0].address,
                       dev_info.wan.ipv6_addrs[0].mask);
            }
            printf("\n  }\n");
            printf("}\n");
            break;
            
        case OUTPUT_FORMAT_XML:
            printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            printf("<status>\n");
            printf("  <system>\n");
            printf("    <version>%s</version>\n", sys_info.version);
            printf("    <serial_number>%s</serial_number>\n", sys_info.serial_number);
            printf("    <uptime>%lu</uptime>\n", sys_info.uptime);
            printf("    <mode>%s</mode>\n", mode);
            printf("  </system>\n");
            printf("  <lan up=\"%s\"", dev_info.lan.up ? "true" : "false");
            if (dev_info.lan.ipv4_count > 0) {
                printf(" ipv4=\"%s/%d\"",
                       dev_info.lan.ipv4_addrs[0].address,
                       dev_info.lan.ipv4_addrs[0].mask);
            }
            printf("/>\n");
            printf("  <wan up=\"%s\"", dev_info.wan.up ? "true" : "false");
            if (dev_info.wan.ipv4_count > 0) {
                printf(" ipv4=\"%s/%d\"",
                       dev_info.wan.ipv4_addrs[0].address,
                       dev_info.wan.ipv4_addrs[0].mask);
            }
            printf("/>\n");
            printf("</status>\n");
            break;
            
        default:  /* TEXT */
            printf("Google Fiber Router Status\n");
            printf("==========================\n\n");
            printf("Firmware:  %s\n", sys_info.version);
            printf("Serial:    %s\n", sys_info.serial_number);
            printf("Mode:      %s\n", mode);
            printf("Uptime:    %s\n\n", gfctl_format_uptime(sys_info.uptime));
            
            printf("LAN:       %s", dev_info.lan.up ? "Up" : "Down");
            if (dev_info.lan.ipv4_count > 0) {
                printf("  %s/%d",
                       dev_info.lan.ipv4_addrs[0].address,
                       dev_info.lan.ipv4_addrs[0].mask);
            }
            printf("\n");
            
            printf("WAN:       %s", dev_info.wan.up ? "Up" : "Down");
            if (dev_info.wan.ipv4_count > 0) {
                printf("  %s/%d",
                       dev_info.wan.ipv4_addrs[0].address,
                       dev_info.wan.ipv4_addrs[0].mask);
            }
            printf("\n");
            if (dev_info.wan.ipv6_count > 0) {
                printf("           %s/%d\n",
                       dev_info.wan.ipv6_addrs[0].address,
                       dev_info.wan.ipv6_addrs[0].mask);
            }
            break;
    }
    
    gfctl_free_device_info(&dev_info);
    return GFCTL_OK;
}

/*
 * cmd_system - Show system information
 */
int cmd_system(gfctl_context_t *ctx, int argc, char **argv) {
    system_info_t info;
    int ret;
    
    (void)argc;
    (void)argv;
    
    ret = gfctl_get_system_info(ctx, &info);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    gfctl_output_system_info(ctx, &info);
    return GFCTL_OK;
}

/*
 * cmd_devices - Show LAN/WAN device information
 */
int cmd_devices(gfctl_context_t *ctx, int argc, char **argv) {
    device_info_t info;
    int ret;
    
    (void)argc;
    (void)argv;
    
    ret = gfctl_get_device_info(ctx, &info);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    gfctl_output_device_info(ctx, &info);
    gfctl_free_device_info(&info);
    return GFCTL_OK;
}

/*
 * cmd_topology - Show network topology
 */
int cmd_topology(gfctl_context_t *ctx, int argc, char **argv) {
    network_topology_t topo;
    int ret;
    
    (void)argc;
    (void)argv;
    
    ret = gfctl_get_network_topology(ctx, &topo);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    gfctl_output_topology(ctx, &topo);
    gfctl_free_topology(&topo);
    return GFCTL_OK;
}

/*
 * cmd_wireless - Show wireless network information
 */
int cmd_wireless(gfctl_context_t *ctx, int argc, char **argv) {
    wireless_network_t *networks = NULL;
    int count = 0;
    int ret;
    
    (void)argc;
    (void)argv;
    
    ret = gfctl_get_wireless_info(ctx, &networks, &count);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    gfctl_output_wireless(ctx, networks, count);
    gfctl_free_wireless_info(networks);
    return GFCTL_OK;
}

/*
 * cmd_clients - Show connected clients
 */
int cmd_clients(gfctl_context_t *ctx, int argc, char **argv) {
    network_topology_t topo;
    int ret;
    
    (void)argc;
    (void)argv;
    
    ret = gfctl_get_network_topology(ctx, &topo);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    gfctl_output_clients(ctx, &topo);
    gfctl_free_topology(&topo);
    return GFCTL_OK;
}

/*
 * cmd_raw - Make raw JSON-RPC call
 */
int cmd_raw(gfctl_context_t *ctx, int argc, char **argv) {
    int ret;
    const char *object, *method, *params;
    
    if (argc < 2) {
        fprintf(stderr, "Usage: raw <object> <method> [params]\n");
        fprintf(stderr, "Example: raw data_repo.webinfo system {}\n");
        return GFCTL_ERR_INVALID_ARG;
    }
    
    /* Ensure we have a valid session */
    ret = gfctl_ensure_session(ctx);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    object = argv[0];
    method = argv[1];
    params = (argc > 2) ? argv[2] : "{}";
    
    ret = gfctl_jsonrpc_call(ctx, object, method, params, &ctx->response_buffer);
    if (ret != GFCTL_OK) {
        fprintf(stderr, "Error: %s\n", gfctl_error_string(ret));
        return ret;
    }
    
    gfctl_output_raw_json(ctx->response_buffer.data);
    return GFCTL_OK;
}

/*
 * Help functions
 */
void help_status(const char *progname) {
    printf("Usage: %s status [options]\n\n", progname);
    printf("Display overall router status including system info and network status.\n\n");
    printf("Options:\n");
    printf("  --json    Output in JSON format\n");
    printf("  --xml     Output in XML format\n");
}

void help_system(const char *progname) {
    printf("Usage: %s system [options]\n\n", progname);
    printf("Display system information including firmware version, serial number,\n");
    printf("and uptime.\n\n");
    printf("Options:\n");
    printf("  --json    Output in JSON format\n");
    printf("  --xml     Output in XML format\n");
}

void help_devices(const char *progname) {
    printf("Usage: %s devices [options]\n\n", progname);
    printf("Display network interface information for LAN and WAN.\n");
    printf("Shows IP addresses (IPv4 and IPv6), netmasks, and interface status.\n\n");
    printf("Options:\n");
    printf("  --json    Output in JSON format\n");
    printf("  --xml     Output in XML format\n");
}

void help_topology(const char *progname) {
    printf("Usage: %s topology [options]\n\n", progname);
    printf("Display network topology including router, extenders, and their\n");
    printf("interfaces, radios, and connected client counts.\n\n");
    printf("Options:\n");
    printf("  --json    Output in JSON format\n");
    printf("  --xml     Output in XML format\n");
}

void help_wireless(const char *progname) {
    printf("Usage: %s wireless [options]\n\n", progname);
    printf("Display wireless network configuration including SSIDs, bands,\n");
    printf("and enabled status for primary and guest networks.\n\n");
    printf("Options:\n");
    printf("  --json    Output in JSON format\n");
    printf("  --xml     Output in XML format\n");
}

void help_clients(const char *progname) {
    printf("Usage: %s clients [options]\n\n", progname);
    printf("Display all connected client devices including their MAC addresses,\n");
    printf("IP addresses, hostnames, and connection type.\n\n");
    printf("Options:\n");
    printf("  --json    Output in JSON format\n");
    printf("  --xml     Output in XML format\n");
    printf("  --csv     Output in CSV format\n");
}

void help_raw(const char *progname) {
    printf("Usage: %s raw <object> <method> [params]\n\n", progname);
    printf("Make a raw JSON-RPC call to the router's ubus interface.\n\n");
    printf("Arguments:\n");
    printf("  object    The ubus object (e.g., data_repo.webinfo)\n");
    printf("  method    The method to call (e.g., system)\n");
    printf("  params    Optional JSON parameters (default: {})\n\n");
    printf("Examples:\n");
    printf("  %s raw data_repo.webinfo system\n", progname);
    printf("  %s raw network topology {}\n", progname);
    printf("  %s raw session access '{\"scope\":\"ubus\"}'\n", progname);
}
