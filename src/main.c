/*
 * main.c - Main Entry Point for gfctl
 *
 * Google Fiber Router Control Utility
 *
 * A command-line tool for monitoring and administering Google Fiber
 * routers running OpenWRT with the ubus JSON-RPC interface.
 *
 * Usage: gfctl [options] <command> [command-options]
 */

#include "gfctl.h"
#include <getopt.h>

/* Command table */
static const gfctl_command_t commands[] = {
    {"status",   "Show overall router status",          NULL, cmd_status,   help_status},
    {"system",   "Show system information",             NULL, cmd_system,   help_system},
    {"devices",  "Show LAN/WAN network configuration",  NULL, cmd_devices,  help_devices},
    {"topology", "Show network topology",               NULL, cmd_topology, help_topology},
    {"wireless", "Show wireless network configuration", NULL, cmd_wireless, help_wireless},
    {"clients",  "Show connected clients",              NULL, cmd_clients,  help_clients},
    {"raw",      "Make raw JSON-RPC call",              NULL, cmd_raw,      help_raw},
    {NULL, NULL, NULL, NULL, NULL}
};

/* Long options */
static const struct option long_options[] = {
    {"help",    no_argument,       NULL, 'h'},
    {"version", no_argument,       NULL, 'V'},
    {"host",    required_argument, NULL, 'H'},
    {"port",    required_argument, NULL, 'p'},
    {"json",    no_argument,       NULL, 'j'},
    {"xml",     no_argument,       NULL, 'x'},
    {"csv",     no_argument,       NULL, 'c'},
    {"raw",     no_argument,       NULL, 'r'},
    {"verbose", no_argument,       NULL, 'v'},
    {"timeout", required_argument, NULL, 't'},
    {"session", required_argument, NULL, 's'},
    {NULL, 0, NULL, 0}
};

/*
 * print_usage - Print program usage
 */
static void print_usage(const char *progname) {
    const gfctl_command_t *cmd;
    
    printf("Usage: %s [options] <command> [command-options]\n\n", progname);
    printf("Google Fiber Router Control Utility v%s\n\n", GFCTL_VERSION_STRING);
    printf("Options:\n");
    printf("  -h, --help          Show this help message\n");
    printf("  -V, --version       Show version information\n");
    printf("  -H, --host <host>   Router hostname/IP (default: %s)\n", GFCTL_DEFAULT_HOST);
    printf("  -p, --port <port>   Router port (default: %d)\n", GFCTL_DEFAULT_PORT);
    printf("  -j, --json          Output in JSON format\n");
    printf("  -x, --xml           Output in XML format\n");
    printf("  -c, --csv           Output in CSV format (where applicable)\n");
    printf("  -r, --raw           Output raw JSON-RPC response\n");
    printf("  -v, --verbose       Enable verbose output\n");
    printf("  -t, --timeout <sec> Request timeout in seconds (default: %d)\n", 
           GFCTL_DEFAULT_TIMEOUT);
    printf("  -s, --session <tok> Session token (for authenticated requests)\n");
    printf("\nCommands:\n");
    
    for (cmd = commands; cmd->name != NULL; cmd++) {
        printf("  %-12s  %s\n", cmd->name, cmd->description);
    }
    
    printf("\nUse '%s <command> --help' for more information about a command.\n", progname);
}

/*
 * print_version - Print version information
 */
static void print_version(void) {
    printf("gfctl version %s\n", GFCTL_VERSION_STRING);
    printf("Google Fiber Router Control Utility\n");
    printf("Copyright (c) 2025\n");
}

/*
 * find_command - Find a command by name
 */
static const gfctl_command_t *find_command(const char *name) {
    const gfctl_command_t *cmd;
    
    for (cmd = commands; cmd->name != NULL; cmd++) {
        if (strcmp(cmd->name, name) == 0) {
            return cmd;
        }
    }
    
    return NULL;
}

/*
 * main - Program entry point
 */
int main(int argc, char **argv) {
    gfctl_context_t *ctx;
    const gfctl_command_t *cmd;
    int opt;
    int ret = 0;
    const char *progname = argv[0];
    
    /* Initialize context */
    ctx = gfctl_init();
    if (!ctx) {
        fprintf(stderr, "Error: Failed to initialize\n");
        return 1;
    }
    
    /* Parse global options */
    while ((opt = getopt_long(argc, argv, "+hVH:p:jxcrvt:s:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'h':
                print_usage(progname);
                gfctl_cleanup(ctx);
                return 0;
                
            case 'V':
                print_version();
                gfctl_cleanup(ctx);
                return 0;
                
            case 'H':
                gfctl_set_host(ctx, optarg);
                break;
                
            case 'p':
                gfctl_set_port(ctx, atoi(optarg));
                break;
                
            case 'j':
                gfctl_set_output_format(ctx, OUTPUT_FORMAT_JSON);
                break;
                
            case 'x':
                gfctl_set_output_format(ctx, OUTPUT_FORMAT_XML);
                break;
                
            case 'c':
                gfctl_set_output_format(ctx, OUTPUT_FORMAT_CSV);
                break;
                
            case 'r':
                gfctl_set_output_format(ctx, OUTPUT_FORMAT_RAW);
                break;
                
            case 'v':
                ctx->config.verbose = true;
                break;
                
            case 't':
                ctx->config.timeout = atoi(optarg);
                break;
                
            case 's':
                gfctl_set_session(ctx, optarg);
                break;
                
            default:
                print_usage(progname);
                gfctl_cleanup(ctx);
                return 1;
        }
    }
    
    /* Check for command */
    if (optind >= argc) {
        /* No command specified, default to status */
        cmd = find_command("status");
    } else {
        /* Check for command-level --help */
        if (strcmp(argv[optind], "--help") == 0 || strcmp(argv[optind], "-h") == 0) {
            print_usage(progname);
            gfctl_cleanup(ctx);
            return 0;
        }
        
        cmd = find_command(argv[optind]);
        if (!cmd) {
            fprintf(stderr, "Error: Unknown command '%s'\n", argv[optind]);
            fprintf(stderr, "Use '%s --help' for a list of commands.\n", progname);
            gfctl_cleanup(ctx);
            return 1;
        }
        optind++;
    }
    
    /* Check for command-specific --help */
    if (optind < argc && (strcmp(argv[optind], "--help") == 0 || 
                          strcmp(argv[optind], "-h") == 0)) {
        if (cmd->help) {
            cmd->help(progname);
        } else {
            print_usage(progname);
        }
        gfctl_cleanup(ctx);
        return 0;
    }
    
    /* Execute command */
    if (cmd->handler) {
        ret = cmd->handler(ctx, argc - optind, argv + optind);
    }
    
    /* Cleanup */
    gfctl_cleanup(ctx);
    
    return (ret == GFCTL_OK) ? 0 : 1;

