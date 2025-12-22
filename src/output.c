/*
 * output.c - Output Formatters for gfctl
 *
 * Handles formatting output as text, JSON, XML, etc.
 */

#include "gfctl.h"

/*
 * gfctl_output_system_info - Output system information
 */
void gfctl_output_system_info(gfctl_context_t *ctx, const system_info_t *info) {
    if (!ctx || !info) return;
    
    switch (ctx->config.output_format) {
        case OUTPUT_FORMAT_JSON:
            printf("{\n");
            printf("  \"version\": \"%s\",\n", info->version);
            printf("  \"serial_number\": \"%s\",\n", info->serial_number);
            printf("  \"uptime\": %lu,\n", info->uptime);
            printf("  \"uptime_formatted\": \"%s\",\n", gfctl_format_uptime(info->uptime));
            printf("  \"last_acs_inform\": \"%s\"\n", info->last_acs_inform);
            printf("}\n");
            break;
            
        case OUTPUT_FORMAT_XML:
            printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            printf("<system>\n");
            printf("  <version>%s</version>\n", info->version);
            printf("  <serial_number>%s</serial_number>\n", info->serial_number);
            printf("  <uptime>%lu</uptime>\n", info->uptime);
            printf("  <uptime_formatted>%s</uptime_formatted>\n", 
                   gfctl_format_uptime(info->uptime));
            printf("  <last_acs_inform>%s</last_acs_inform>\n", info->last_acs_inform);
            printf("</system>\n");
            break;
            
        default:  /* TEXT */
            printf("System Information\n");
            printf("==================\n");
            printf("Firmware Version: %s\n", info->version);
            printf("Serial Number:    %s\n", info->serial_number);
            printf("Uptime:           %s (%lu seconds)\n", 
                   gfctl_format_uptime(info->uptime), info->uptime);
            printf("Last ACS Inform:  %s\n", info->last_acs_inform);
            break;
    }
}

/*
 * gfctl_output_device_info - Output LAN/WAN device information
 */
void gfctl_output_device_info(gfctl_context_t *ctx, const device_info_t *info) {
    int i;
    
    if (!ctx || !info) return;
    
    switch (ctx->config.output_format) {
        case OUTPUT_FORMAT_JSON:
            printf("{\n");
            
            /* LAN */
            printf("  \"lan\": {\n");
            printf("    \"up\": %s,\n", info->lan.up ? "true" : "false");
            printf("    \"ipv4\": [");
            for (i = 0; i < info->lan.ipv4_count; i++) {
                if (i > 0) printf(",");
                printf("\n      {\"address\": \"%s\", \"mask\": %d}",
                       info->lan.ipv4_addrs[i].address,
                       info->lan.ipv4_addrs[i].mask);
            }
            printf("\n    ],\n");
            printf("    \"ipv6\": [");
            for (i = 0; i < info->lan.ipv6_count; i++) {
                if (i > 0) printf(",");
                printf("\n      {\"address\": \"%s\", \"mask\": %d}",
                       info->lan.ipv6_addrs[i].address,
                       info->lan.ipv6_addrs[i].mask);
            }
            printf("\n    ]\n");
            printf("  },\n");
            
            /* WAN */
            printf("  \"wan\": {\n");
            printf("    \"up\": %s,\n", info->wan.up ? "true" : "false");
            printf("    \"ipv4\": [");
            for (i = 0; i < info->wan.ipv4_count; i++) {
                if (i > 0) printf(",");
                printf("\n      {\"address\": \"%s\", \"mask\": %d}",
                       info->wan.ipv4_addrs[i].address,
                       info->wan.ipv4_addrs[i].mask);
            }
            printf("\n    ],\n");
            printf("    \"ipv6\": [");
            for (i = 0; i < info->wan.ipv6_count; i++) {
                if (i > 0) printf(",");
                printf("\n      {\"address\": \"%s\", \"mask\": %d, \"preferred\": %d, \"valid\": %d}",
                       info->wan.ipv6_addrs[i].address,
                       info->wan.ipv6_addrs[i].mask,
                       info->wan.ipv6_addrs[i].preferred,
                       info->wan.ipv6_addrs[i].valid);
            }
            printf("\n    ]\n");
            printf("  }\n");
            printf("}\n");
            break;
            
        case OUTPUT_FORMAT_XML:
            printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            printf("<devices>\n");
            
            printf("  <lan>\n");
            printf("    <up>%s</up>\n", info->lan.up ? "true" : "false");
            for (i = 0; i < info->lan.ipv4_count; i++) {
                printf("    <ipv4 address=\"%s\" mask=\"%d\"/>\n",
                       info->lan.ipv4_addrs[i].address,
                       info->lan.ipv4_addrs[i].mask);
            }
            for (i = 0; i < info->lan.ipv6_count; i++) {
                printf("    <ipv6 address=\"%s\" mask=\"%d\"/>\n",
                       info->lan.ipv6_addrs[i].address,
                       info->lan.ipv6_addrs[i].mask);
            }
            printf("  </lan>\n");
            
            printf("  <wan>\n");
            printf("    <up>%s</up>\n", info->wan.up ? "true" : "false");
            for (i = 0; i < info->wan.ipv4_count; i++) {
                printf("    <ipv4 address=\"%s\" mask=\"%d\"/>\n",
                       info->wan.ipv4_addrs[i].address,
                       info->wan.ipv4_addrs[i].mask);
            }
            for (i = 0; i < info->wan.ipv6_count; i++) {
                printf("    <ipv6 address=\"%s\" mask=\"%d\" preferred=\"%d\" valid=\"%d\"/>\n",
                       info->wan.ipv6_addrs[i].address,
                       info->wan.ipv6_addrs[i].mask,
                       info->wan.ipv6_addrs[i].preferred,
                       info->wan.ipv6_addrs[i].valid);
            }
            printf("  </wan>\n");
            printf("</devices>\n");
            break;
            
        default:  /* TEXT */
            printf("Network Configuration\n");
            printf("=====================\n\n");
            
            printf("LAN Interface:\n");
            printf("  Status: %s\n", info->lan.up ? "Up" : "Down");
            for (i = 0; i < info->lan.ipv4_count; i++) {
                printf("  IPv4:   %s/%d\n",
                       info->lan.ipv4_addrs[i].address,
                       info->lan.ipv4_addrs[i].mask);
            }
            for (i = 0; i < info->lan.ipv6_count; i++) {
                printf("  IPv6:   %s/%d\n",
                       info->lan.ipv6_addrs[i].address,
                       info->lan.ipv6_addrs[i].mask);
            }
            
            printf("\nWAN Interface:\n");
            printf("  Status: %s\n", info->wan.up ? "Up" : "Down");
            for (i = 0; i < info->wan.ipv4_count; i++) {
                printf("  IPv4:   %s/%d\n",
                       info->wan.ipv4_addrs[i].address,
                       info->wan.ipv4_addrs[i].mask);
            }
            for (i = 0; i < info->wan.ipv6_count; i++) {
                printf("  IPv6:   %s/%d (preferred: %ds, valid: %ds)\n",
                       info->wan.ipv6_addrs[i].address,
                       info->wan.ipv6_addrs[i].mask,
                       info->wan.ipv6_addrs[i].preferred,
                       info->wan.ipv6_addrs[i].valid);
            }
            break;
    }
}

/*
 * gfctl_output_wireless - Output wireless network information
 */
void gfctl_output_wireless(gfctl_context_t *ctx, const wireless_network_t *networks, int count) {
    int i;
    
    if (!ctx || !networks) return;
    
    switch (ctx->config.output_format) {
        case OUTPUT_FORMAT_JSON:
            printf("{\n  \"wireless\": [\n");
            for (i = 0; i < count; i++) {
                printf("    {\n");
                printf("      \"ssid\": \"%s\",\n", networks[i].ssid);
                printf("      \"network\": \"%s\",\n", networks[i].network);
                printf("      \"band\": \"%s\",\n", networks[i].band);
                printf("      \"enabled\": %s,\n", networks[i].enable ? "true" : "false");
                printf("      \"up\": %s\n", networks[i].up ? "true" : "false");
                printf("    }%s\n", (i < count - 1) ? "," : "");
            }
            printf("  ]\n}\n");
            break;
            
        case OUTPUT_FORMAT_XML:
            printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            printf("<wireless>\n");
            for (i = 0; i < count; i++) {
                printf("  <network>\n");
                printf("    <ssid>%s</ssid>\n", networks[i].ssid);
                printf("    <type>%s</type>\n", networks[i].network);
                printf("    <band>%s</band>\n", networks[i].band);
                printf("    <enabled>%s</enabled>\n", networks[i].enable ? "true" : "false");
                printf("    <up>%s</up>\n", networks[i].up ? "true" : "false");
                printf("  </network>\n");
            }
            printf("</wireless>\n");
            break;
            
        default:  /* TEXT */
            printf("Wireless Networks\n");
            printf("=================\n\n");
            for (i = 0; i < count; i++) {
                printf("%-20s [%s]\n", networks[i].ssid, networks[i].network);
                printf("  Band:    %s\n", networks[i].band);
                printf("  Enabled: %s\n", networks[i].enable ? "Yes" : "No");
                printf("  Status:  %s\n", networks[i].up ? "Up" : "Down");
                if (i < count - 1) printf("\n");
            }
            break;
    }
}

/*
 * gfctl_output_topology - Output network topology information
 */
void gfctl_output_topology(gfctl_context_t *ctx, const network_topology_t *topo) {
    int i, j;
    const mesh_device_t *dev;
    
    if (!ctx || !topo) return;
    
    switch (ctx->config.output_format) {
        case OUTPUT_FORMAT_JSON:
            printf("{\n  \"devices\": [\n");
            for (i = 0; i < topo->device_count; i++) {
                dev = &topo->devices[i];
                printf("    {\n");
                printf("      \"mac\": \"%s\",\n", dev->mac);
                printf("      \"ip\": \"%s\",\n", dev->ip);
                printf("      \"hostname\": \"%s\",\n", dev->hostname);
                printf("      \"ipv6\": \"%s\",\n", dev->ipv6);
                printf("      \"is_router\": %s,\n", dev->is_router ? "true" : "false");
                
                /* Interfaces */
                printf("      \"interfaces\": [\n");
                for (j = 0; j < dev->interface_count; j++) {
                    printf("        {\"name\": \"%s\", \"mac\": \"%s\", \"type\": \"%s\"",
                           dev->interfaces[j].name,
                           dev->interfaces[j].mac,
                           gfctl_medium_type_string(dev->interfaces[j].medium_type));
                    if (dev->interfaces[j].speed > 0) {
                        printf(", \"speed\": %d", dev->interfaces[j].speed);
                    }
                    printf("}%s\n", (j < dev->interface_count - 1) ? "," : "");
                }
                printf("      ],\n");
                
                /* Radios */
                printf("      \"radios\": [\n");
                for (j = 0; j < dev->radio_count; j++) {
                    printf("        {\"mac\": \"%s\", \"type\": \"%s\", \"channel\": %d, \"bandwidth\": \"%s\"}%s\n",
                           dev->radios[j].mac,
                           gfctl_medium_type_string(dev->radios[j].type),
                           dev->radios[j].channel,
                           dev->radios[j].bandwidth,
                           (j < dev->radio_count - 1) ? "," : "");
                }
                printf("      ],\n");
                
                /* Client count (not full list in JSON summary) */
                printf("      \"client_count\": %d", dev->client_count);
                
                /* Backhaul */
                if (dev->has_backhaul) {
                    printf(",\n      \"backhaul\": {\n");
                    printf("        \"method\": \"%s\",\n", dev->backhaul_method);
                    printf("        \"connected_to\": \"%s\"\n", dev->backhaul_mac);
                    printf("      }");
                }
                
                printf("\n    }%s\n", (i < topo->device_count - 1) ? "," : "");
            }
            printf("  ]\n}\n");
            break;
            
        case OUTPUT_FORMAT_XML:
            printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            printf("<topology>\n");
            for (i = 0; i < topo->device_count; i++) {
                dev = &topo->devices[i];
                printf("  <device mac=\"%s\" type=\"%s\">\n",
                       dev->mac, dev->is_router ? "router" : "extender");
                printf("    <ip>%s</ip>\n", dev->ip);
                printf("    <hostname>%s</hostname>\n", dev->hostname);
                if (dev->ipv6[0]) {
                    printf("    <ipv6>%s</ipv6>\n", dev->ipv6);
                }
                
                printf("    <interfaces>\n");
                for (j = 0; j < dev->interface_count; j++) {
                    printf("      <interface name=\"%s\" mac=\"%s\" type=\"%s\"",
                           dev->interfaces[j].name,
                           dev->interfaces[j].mac,
                           gfctl_medium_type_string(dev->interfaces[j].medium_type));
                    if (dev->interfaces[j].speed > 0) {
                        printf(" speed=\"%d\"", dev->interfaces[j].speed);
                    }
                    printf("/>\n");
                }
                printf("    </interfaces>\n");
                
                printf("    <radios>\n");
                for (j = 0; j < dev->radio_count; j++) {
                    printf("      <radio mac=\"%s\" type=\"%s\" channel=\"%d\" bandwidth=\"%s\"/>\n",
                           dev->radios[j].mac,
                           gfctl_medium_type_string(dev->radios[j].type),
                           dev->radios[j].channel,
                           dev->radios[j].bandwidth);
                }
                printf("    </radios>\n");
                
                printf("    <client_count>%d</client_count>\n", dev->client_count);
                
                if (dev->has_backhaul) {
                    printf("    <backhaul method=\"%s\" connected_to=\"%s\"/>\n",
                           dev->backhaul_method, dev->backhaul_mac);
                }
                
                printf("  </device>\n");
            }
            printf("</topology>\n");
            break;
            
        default:  /* TEXT */
            printf("Network Topology\n");
            printf("================\n\n");
            
            for (i = 0; i < topo->device_count; i++) {
                dev = &topo->devices[i];
                
                printf("%s: %s (%s)\n",
                       dev->is_router ? "Router" : "Extender",
                       dev->hostname,
                       dev->mac);
                printf("  IP Address: %s\n", dev->ip);
                if (dev->ipv6[0]) {
                    printf("  IPv6:       %s\n", dev->ipv6);
                }
                
                printf("\n  Interfaces:\n");
                for (j = 0; j < dev->interface_count; j++) {
                    printf("    %-6s  %s  %-12s",
                           dev->interfaces[j].name,
                           dev->interfaces[j].mac,
                           gfctl_medium_type_string(dev->interfaces[j].medium_type));
                    if (dev->interfaces[j].speed > 0) {
                        printf("  %d Mbps", dev->interfaces[j].speed);
                    }
                    printf("\n");
                }
                
                printf("\n  Radios:\n");
                for (j = 0; j < dev->radio_count; j++) {
                    printf("    %-12s  Channel %-3d  %s\n",
                           gfctl_medium_type_string(dev->radios[j].type),
                           dev->radios[j].channel,
                           dev->radios[j].bandwidth);
                }
                
                printf("\n  Connected Clients: %d\n", dev->client_count);
                
                if (dev->has_backhaul) {
                    printf("\n  Backhaul: %s to %s\n",
                           dev->backhaul_method, dev->backhaul_mac);
                }
                
                if (i < topo->device_count - 1) printf("\n---\n\n");
            }
            break;
    }
}

/*
 * gfctl_output_clients - Output connected client list
 */
void gfctl_output_clients(gfctl_context_t *ctx, const network_topology_t *topo) {
    int i, j;
    const mesh_device_t *dev;
    const client_device_t *client;
    int total_clients = 0;
    
    if (!ctx || !topo) return;
    
    /* Count total clients */
    for (i = 0; i < topo->device_count; i++) {
        total_clients += topo->devices[i].client_count;
    }
    
    switch (ctx->config.output_format) {
        case OUTPUT_FORMAT_JSON:
            printf("{\n  \"total_clients\": %d,\n  \"clients\": [\n", total_clients);
            {
                int first = 1;
                for (i = 0; i < topo->device_count; i++) {
                    dev = &topo->devices[i];
                    for (j = 0; j < dev->client_count; j++) {
                        client = &dev->clients[j];
                        if (!first) printf(",\n");
                        first = 0;
                        printf("    {\n");
                        printf("      \"mac\": \"%s\",\n", client->mac);
                        printf("      \"ip\": \"%s\",\n", client->ip);
                        printf("      \"hostname\": \"%s\",\n", 
                               client->hostname[0] ? client->hostname : "(unknown)");
                        printf("      \"connection\": \"%s\",\n", client->connect_method);
                        printf("      \"connected_to\": \"%s\"\n", dev->hostname);
                        printf("    }");
                    }
                }
            }
            printf("\n  ]\n}\n");
            break;
            
        case OUTPUT_FORMAT_XML:
            printf("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n");
            printf("<clients total=\"%d\">\n", total_clients);
            for (i = 0; i < topo->device_count; i++) {
                dev = &topo->devices[i];
                for (j = 0; j < dev->client_count; j++) {
                    client = &dev->clients[j];
                    printf("  <client mac=\"%s\">\n", client->mac);
                    printf("    <ip>%s</ip>\n", client->ip);
                    printf("    <hostname>%s</hostname>\n",
                           client->hostname[0] ? client->hostname : "(unknown)");
                    printf("    <connection>%s</connection>\n", client->connect_method);
                    printf("    <connected_to>%s</connected_to>\n", dev->hostname);
                    printf("  </client>\n");
                }
            }
            printf("</clients>\n");
            break;
            
        case OUTPUT_FORMAT_CSV:
            printf("MAC,IP,Hostname,Connection,Connected To\n");
            for (i = 0; i < topo->device_count; i++) {
                dev = &topo->devices[i];
                for (j = 0; j < dev->client_count; j++) {
                    client = &dev->clients[j];
                    printf("%s,%s,%s,%s,%s\n",
                           client->mac,
                           client->ip,
                           client->hostname[0] ? client->hostname : "",
                           client->connect_method,
                           dev->hostname);
                }
            }
            break;
            
        default:  /* TEXT */
            printf("Connected Clients (%d total)\n", total_clients);
            printf("============================\n\n");
            
            for (i = 0; i < topo->device_count; i++) {
                dev = &topo->devices[i];
                if (dev->client_count == 0) continue;
                
                printf("Connected to %s (%d clients):\n",
                       dev->hostname, dev->client_count);
                printf("  %-17s  %-15s  %-20s  %s\n",
                       "MAC", "IP", "Hostname", "Connection");
                printf("  %-17s  %-15s  %-20s  %s\n",
                       "-----------------", "---------------",
                       "--------------------", "----------");
                
                for (j = 0; j < dev->client_count; j++) {
                    client = &dev->clients[j];
                    printf("  %-17s  %-15s  %-20s  %s\n",
                           client->mac,
                           client->ip[0] ? client->ip : "-",
                           client->hostname[0] ? client->hostname : "-",
                           client->connect_method);
                }
                printf("\n");
            }
            break;
    }
}

/*
 * gfctl_output_raw_json - Output raw JSON response
 */
void gfctl_output_raw_json(const char *json) {
    if (json) {
        printf("%s\n", json);
    }
}
