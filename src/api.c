/*
 * api.c - Router API Implementation for gfctl
 *
 * High-level API functions for interacting with the Google Fiber router.
 */

#include "gfctl.h"

/*
 * gfctl_session_login - Establish a session with the router
 *
 * Google Fiber routers use OpenWRT's ubus session mechanism.
 * For local network access, the router accepts login with username "local"
 * and an empty password when connecting from the LAN.
 *
 * Returns: GFCTL_OK on success, error code on failure
 */
int gfctl_session_login(gfctl_context_t *ctx) {
    int ret;
    char *result;
    char *token;
    char params[256];
    
    if (!ctx) return GFCTL_ERR_INVALID_ARG;
    
    /* Try local network authentication (no password required for LAN access) */
    snprintf(params, sizeof(params),
             "{\"username\":\"local\",\"password\":\"\"}");
    
    /* Use the null session token for the login request itself */
    strncpy(ctx->config.session_token, GFCTL_UNAUTH_TOKEN,
            sizeof(ctx->config.session_token) - 1);
    
    ret = gfctl_jsonrpc_call(ctx, "session", "login", params,
                             &ctx->response_buffer);
    
    if (ret != GFCTL_OK) {
        if (ctx->config.verbose) {
            fprintf(stderr, "Login request failed: %s\n", 
                    gfctl_error_string(ret));
        }
        return ret;
    }
    
    /* Extract the session token from the response */
    result = gfctl_jsonrpc_get_result(ctx->response_buffer.data);
    if (!result) {
        if (ctx->config.verbose) {
            fprintf(stderr, "Failed to parse login response\n");
        }
        return GFCTL_ERR_JSON_PARSE;
    }
    
    /* The session token is in the "ubus_rpc_session" field */
    token = json_get_string(result, "ubus_rpc_session");
    if (!token) {
        if (ctx->config.verbose) {
            fprintf(stderr, "No session token in login response\n");
            fprintf(stderr, "Response: %s\n", ctx->response_buffer.data);
        }
        free(result);
        return GFCTL_ERR_AUTH;
    }
    
    /* Store the session token */
    strncpy(ctx->config.session_token, token, 
            sizeof(ctx->config.session_token) - 1);
    ctx->config.session_token[sizeof(ctx->config.session_token) - 1] = '\0';
    
    if (ctx->config.verbose) {
        fprintf(stderr, "Session established: %s\n", ctx->config.session_token);
    }
    
    free(token);
    free(result);
    
    return GFCTL_OK;
}

/*
 * gfctl_ensure_session - Ensure we have a valid session, logging in if needed
 *
 * This should be called before any API call that requires authentication.
 *
 * Returns: GFCTL_OK on success, error code on failure
 */
int gfctl_ensure_session(gfctl_context_t *ctx) {
    /* If we already have a non-null session token, assume it's valid */
    if (ctx && strcmp(ctx->config.session_token, GFCTL_UNAUTH_TOKEN) != 0) {
        return GFCTL_OK;
    }
    
    /* Need to login */
    return gfctl_session_login(ctx);
}

/*
 * gfctl_get_system_info - Get system information
 */
int gfctl_get_system_info(gfctl_context_t *ctx, system_info_t *info) {
    int ret;
    char *result;
    char *str;
    
    if (!ctx || !info) return GFCTL_ERR_INVALID_ARG;
    
    /* Ensure we have a valid session */
    ret = gfctl_ensure_session(ctx);
    if (ret != GFCTL_OK) return ret;
    
    memset(info, 0, sizeof(system_info_t));
    
    ret = gfctl_jsonrpc_call(ctx, "data_repo.webinfo", "system", "{}",
                             &ctx->response_buffer);
    if (ret != GFCTL_OK) return ret;
    
    result = gfctl_jsonrpc_get_result(ctx->response_buffer.data);
    if (!result) return GFCTL_ERR_JSON_PARSE;
    
    str = json_get_string(result, "version");
    if (str) {
        strncpy(info->version, str, sizeof(info->version) - 1);
        free(str);
    }
    
    str = json_get_string(result, "sn");
    if (str) {
        strncpy(info->serial_number, str, sizeof(info->serial_number) - 1);
        free(str);
    }
    
    info->uptime = (unsigned long)json_get_int(result, "uptime", 0);
    
    str = json_get_string(result, "lastACSInform");
    if (str) {
        strncpy(info->last_acs_inform, str, sizeof(info->last_acs_inform) - 1);
        free(str);
    }
    
    free(result);
    return GFCTL_OK;
}

/*
 * gfctl_get_op_mode - Get operation mode
 */
int gfctl_get_op_mode(gfctl_context_t *ctx, char *mode, size_t mode_len) {
    int ret;
    char *result;
    char *str;
    
    if (!ctx || !mode || mode_len == 0) return GFCTL_ERR_INVALID_ARG;
    
    /* Ensure we have a valid session */
    ret = gfctl_ensure_session(ctx);
    if (ret != GFCTL_OK) return ret;
    
    ret = gfctl_jsonrpc_call(ctx, "op-mode", "get", "{}", &ctx->response_buffer);
    if (ret != GFCTL_OK) return ret;
    
    result = gfctl_jsonrpc_get_result(ctx->response_buffer.data);
    if (!result) return GFCTL_ERR_JSON_PARSE;
    
    str = json_get_string(result, "mode");
    if (str) {
        strncpy(mode, str, mode_len - 1);
        mode[mode_len - 1] = '\0';
        free(str);
    } else {
        mode[0] = '\0';
    }
    
    free(result);
    return GFCTL_OK;
}

/*
 * Helper to parse IPv4 addresses from JSON array
 */
static int parse_ipv4_addrs(const char *json, ipv4_addr_t **addrs, int *count) {
    int n, i;
    char *elem;
    char *addr_str;
    
    *addrs = NULL;
    *count = 0;
    
    if (!json) return GFCTL_OK;
    
    n = json_array_count(json);
    if (n <= 0) return GFCTL_OK;
    
    *addrs = (ipv4_addr_t *)calloc(n, sizeof(ipv4_addr_t));
    if (!*addrs) return GFCTL_ERR_MEMORY;
    
    for (i = 0; i < n; i++) {
        elem = json_array_get(json, i);
        if (!elem) continue;
        
        addr_str = json_get_string(elem, "address");
        if (addr_str) {
            strncpy((*addrs)[*count].address, addr_str, 
                    sizeof((*addrs)[*count].address) - 1);
            free(addr_str);
        }
        (*addrs)[*count].mask = json_get_int(elem, "mask", 0);
        (*count)++;
        
        free(elem);
    }
    
    return GFCTL_OK;
}

/*
 * Helper to parse IPv6 addresses from JSON array
 */
static int parse_ipv6_addrs(const char *json, ipv6_addr_t **addrs, int *count) {
    int n, i;
    char *elem;
    char *addr_str;
    
    *addrs = NULL;
    *count = 0;
    
    if (!json) return GFCTL_OK;
    
    n = json_array_count(json);
    if (n <= 0) return GFCTL_OK;
    
    *addrs = (ipv6_addr_t *)calloc(n, sizeof(ipv6_addr_t));
    if (!*addrs) return GFCTL_ERR_MEMORY;
    
    for (i = 0; i < n; i++) {
        elem = json_array_get(json, i);
        if (!elem) continue;
        
        addr_str = json_get_string(elem, "address");
        if (addr_str) {
            strncpy((*addrs)[*count].address, addr_str,
                    sizeof((*addrs)[*count].address) - 1);
            free(addr_str);
        }
        (*addrs)[*count].mask = json_get_int(elem, "mask", 0);
        (*addrs)[*count].preferred = json_get_int(elem, "preferred", 0);
        (*addrs)[*count].valid = json_get_int(elem, "valid", 0);
        (*count)++;
        
        free(elem);
    }
    
    return GFCTL_OK;
}

/*
 * gfctl_get_device_info - Get LAN/WAN device information
 */
int gfctl_get_device_info(gfctl_context_t *ctx, device_info_t *info) {
    int ret;
    char *result;
    char *lan_obj, *wan_obj;
    char *arr;
    
    if (!ctx || !info) return GFCTL_ERR_INVALID_ARG;
    
    /* Ensure we have a valid session */
    ret = gfctl_ensure_session(ctx);
    if (ret != GFCTL_OK) return ret;
    
    memset(info, 0, sizeof(device_info_t));
    
    ret = gfctl_jsonrpc_call(ctx, "data_repo.webinfo", "devices", "{}",
                             &ctx->response_buffer);
    if (ret != GFCTL_OK) return ret;
    
    result = gfctl_jsonrpc_get_result(ctx->response_buffer.data);
    if (!result) return GFCTL_ERR_JSON_PARSE;
    
    /* Parse LAN info */
    lan_obj = json_get_object(result, "lan");
    if (lan_obj) {
        info->lan.up = json_get_bool(lan_obj, "up", false);
        
        arr = json_get_array(lan_obj, "ipv4-address");
        if (arr) {
            parse_ipv4_addrs(arr, &info->lan.ipv4_addrs, &info->lan.ipv4_count);
            free(arr);
        }
        
        arr = json_get_array(lan_obj, "ipv6-address");
        if (arr) {
            parse_ipv6_addrs(arr, &info->lan.ipv6_addrs, &info->lan.ipv6_count);
            free(arr);
        }
        
        free(lan_obj);
    }
    
    /* Parse WAN info */
    wan_obj = json_get_object(result, "wan");
    if (wan_obj) {
        info->wan.up = json_get_bool(wan_obj, "up", false);
        
        arr = json_get_array(wan_obj, "ipv4-address");
        if (arr) {
            parse_ipv4_addrs(arr, &info->wan.ipv4_addrs, &info->wan.ipv4_count);
            free(arr);
        }
        
        arr = json_get_array(wan_obj, "ipv6-address");
        if (arr) {
            parse_ipv6_addrs(arr, &info->wan.ipv6_addrs, &info->wan.ipv6_count);
            free(arr);
        }
        
        free(wan_obj);
    }
    
    free(result);
    return GFCTL_OK;
}

/*
 * Helper to free device_info memory
 */
void gfctl_free_device_info(device_info_t *info) {
    if (!info) return;
    
    if (info->lan.ipv4_addrs) free(info->lan.ipv4_addrs);
    if (info->lan.ipv6_addrs) free(info->lan.ipv6_addrs);
    if (info->wan.ipv4_addrs) free(info->wan.ipv4_addrs);
    if (info->wan.ipv6_addrs) free(info->wan.ipv6_addrs);
}

/*
 * gfctl_get_wireless_info - Get wireless network information
 */
int gfctl_get_wireless_info(gfctl_context_t *ctx, wireless_network_t **networks, int *count) {
    int ret;
    char *result;
    char *arr;
    int n, i;
    char *elem;
    char *str;
    
    if (!ctx || !networks || !count) return GFCTL_ERR_INVALID_ARG;
    
    /* Ensure we have a valid session */
    ret = gfctl_ensure_session(ctx);
    if (ret != GFCTL_OK) return ret;
    
    *networks = NULL;
    *count = 0;
    
    ret = gfctl_jsonrpc_call(ctx, "data_repo.webinfo", "wireless", "{}",
                             &ctx->response_buffer);
    if (ret != GFCTL_OK) return ret;
    
    result = gfctl_jsonrpc_get_result(ctx->response_buffer.data);
    if (!result) return GFCTL_ERR_JSON_PARSE;
    
    arr = json_get_array(result, "wireless");
    if (!arr) {
        free(result);
        return GFCTL_OK;
    }
    
    n = json_array_count(arr);
    if (n <= 0) {
        free(arr);
        free(result);
        return GFCTL_OK;
    }
    
    *networks = (wireless_network_t *)calloc(n, sizeof(wireless_network_t));
    if (!*networks) {
        free(arr);
        free(result);
        return GFCTL_ERR_MEMORY;
    }
    
    for (i = 0; i < n; i++) {
        elem = json_array_get(arr, i);
        if (!elem) continue;
        
        (*networks)[*count].enable = json_get_bool(elem, "enable", false);
        (*networks)[*count].up = json_get_bool(elem, "up", false);
        
        str = json_get_string(elem, "ssid");
        if (str) {
            strncpy((*networks)[*count].ssid, str, 
                    sizeof((*networks)[*count].ssid) - 1);
            free(str);
        }
        
        str = json_get_string(elem, "network");
        if (str) {
            strncpy((*networks)[*count].network, str,
                    sizeof((*networks)[*count].network) - 1);
            free(str);
        }
        
        str = json_get_string(elem, "band");
        if (str) {
            strncpy((*networks)[*count].band, str,
                    sizeof((*networks)[*count].band) - 1);
            free(str);
        }
        
        (*count)++;
        free(elem);
    }
    
    free(arr);
    free(result);
    return GFCTL_OK;
}

void gfctl_free_wireless_info(wireless_network_t *networks) {
    if (networks) free(networks);
}

/*
 * Helper to parse a single mesh device from topology JSON
 */
static int parse_mesh_device(const char *json, const char *mac, mesh_device_t *dev) {
    char *info_obj, *arr, *elem;
    char **keys;
    int key_count;
    char *str;
    int i, n;
    
    memset(dev, 0, sizeof(mesh_device_t));
    strncpy(dev->mac, mac, sizeof(dev->mac) - 1);
    
    str = json_get_string(json, "ip");
    if (str) {
        strncpy(dev->ip, str, sizeof(dev->ip) - 1);
        free(str);
    }
    
    str = json_get_string(json, "hostname");
    if (str) {
        strncpy(dev->hostname, str, sizeof(dev->hostname) - 1);
        free(str);
    }
    
    /* Get first IPv6 address if available */
    arr = json_get_array(json, "ipv6");
    if (arr) {
        str = json_array_get(arr, 0);
        if (str) {
            /* Remove quotes if present */
            if (str[0] == '"') {
                strncpy(dev->ipv6, str + 1, sizeof(dev->ipv6) - 1);
                size_t len = strlen(dev->ipv6);
                if (len > 0 && dev->ipv6[len-1] == '"') dev->ipv6[len-1] = '\0';
            } else {
                strncpy(dev->ipv6, str, sizeof(dev->ipv6) - 1);
            }
            free(str);
        }
        free(arr);
    }
    
    /* Determine if this is the router (has IP 10.0.0.1) */
    dev->is_router = (strcmp(dev->ip, "10.0.0.1") == 0);
    
    /* Parse interfaces */
    info_obj = json_get_object(json, "info");
    if (info_obj) {
        keys = json_object_keys(info_obj, &key_count);
        if (keys && key_count > 0) {
            dev->interfaces = (interface_info_t *)calloc(key_count, sizeof(interface_info_t));
            if (dev->interfaces) {
                dev->interface_count = 0;
                for (i = 0; i < key_count; i++) {
                    char *iface_obj = json_get_object(info_obj, keys[i]);
                    if (iface_obj) {
                        strncpy(dev->interfaces[dev->interface_count].name, keys[i],
                                sizeof(dev->interfaces[dev->interface_count].name) - 1);
                        
                        str = json_get_string(iface_obj, "mac");
                        if (str) {
                            strncpy(dev->interfaces[dev->interface_count].mac, str,
                                    sizeof(dev->interfaces[dev->interface_count].mac) - 1);
                            free(str);
                        }
                        
                        str = json_get_string(iface_obj, "medium_type");
                        if (str) {
                            dev->interfaces[dev->interface_count].medium_type = 
                                gfctl_parse_medium_type(str);
                            free(str);
                        }
                        
                        str = json_get_string(iface_obj, "speed");
                        if (str) {
                            dev->interfaces[dev->interface_count].speed = atoi(str);
                            free(str);
                        }
                        
                        dev->interface_count++;
                        free(iface_obj);
                    }
                    free(keys[i]);
                }
            }
            free(keys);
        }
        free(info_obj);
    }
    
    /* Parse radios */
    arr = json_get_array(json, "radio");
    if (arr) {
        n = json_array_count(arr);
        if (n > 0) {
            dev->radios = (radio_info_t *)calloc(n, sizeof(radio_info_t));
            if (dev->radios) {
                dev->radio_count = 0;
                for (i = 0; i < n; i++) {
                    elem = json_array_get(arr, i);
                    if (elem) {
                        str = json_get_string(elem, "mac");
                        if (str) {
                            strncpy(dev->radios[dev->radio_count].mac, str,
                                    sizeof(dev->radios[dev->radio_count].mac) - 1);
                            free(str);
                        }
                        
                        str = json_get_string(elem, "type");
                        if (str) {
                            dev->radios[dev->radio_count].type = gfctl_parse_medium_type(str);
                            free(str);
                        }
                        
                        str = json_get_string(elem, "channel");
                        if (str) {
                            dev->radios[dev->radio_count].channel = atoi(str);
                            free(str);
                        }
                        
                        str = json_get_string(elem, "bandwidth");
                        if (str) {
                            strncpy(dev->radios[dev->radio_count].bandwidth, str,
                                    sizeof(dev->radios[dev->radio_count].bandwidth) - 1);
                            free(str);
                        }
                        
                        dev->radio_count++;
                        free(elem);
                    }
                }
            }
        }
        free(arr);
    }
    
    /* Parse connected clients (legacy_devices) */
    arr = json_get_array(json, "legacy_devices");
    if (arr) {
        n = json_array_count(arr);
        if (n > 0) {
            dev->clients = (client_device_t *)calloc(n, sizeof(client_device_t));
            if (dev->clients) {
                dev->client_count = 0;
                for (i = 0; i < n; i++) {
                    elem = json_array_get(arr, i);
                    if (elem) {
                        str = json_get_string(elem, "mac");
                        if (str) {
                            strncpy(dev->clients[dev->client_count].mac, str,
                                    sizeof(dev->clients[dev->client_count].mac) - 1);
                            free(str);
                        }
                        
                        str = json_get_string(elem, "ip");
                        if (str) {
                            strncpy(dev->clients[dev->client_count].ip, str,
                                    sizeof(dev->clients[dev->client_count].ip) - 1);
                            free(str);
                        }
                        
                        str = json_get_string(elem, "hostname");
                        if (str) {
                            strncpy(dev->clients[dev->client_count].hostname, str,
                                    sizeof(dev->clients[dev->client_count].hostname) - 1);
                            free(str);
                        }
                        
                        str = json_get_string(elem, "connect_interface");
                        if (str) {
                            strncpy(dev->clients[dev->client_count].connect_interface, str,
                                    sizeof(dev->clients[dev->client_count].connect_interface) - 1);
                            free(str);
                        }
                        
                        str = json_get_string(elem, "connect_method");
                        if (str) {
                            strncpy(dev->clients[dev->client_count].connect_method, str,
                                    sizeof(dev->clients[dev->client_count].connect_method) - 1);
                            free(str);
                        }
                        
                        dev->client_count++;
                        free(elem);
                    }
                }
            }
        }
        free(arr);
    }
    
    /* Parse backhaul info (for extenders) */
    char *backhaul = json_get_object(json, "backhaul");
    if (backhaul) {
        dev->has_backhaul = true;
        
        str = json_get_string(backhaul, "connect_method");
        if (str) {
            strncpy(dev->backhaul_method, str, sizeof(dev->backhaul_method) - 1);
            free(str);
        }
        
        str = json_get_string(backhaul, "mac_connected");
        if (str) {
            strncpy(dev->backhaul_mac_connected, str, sizeof(dev->backhaul_mac_connected) - 1);
            free(str);
        }
        
        str = json_get_string(backhaul, "mac");
        if (str) {
            strncpy(dev->backhaul_mac, str, sizeof(dev->backhaul_mac) - 1);
            free(str);
        }
        
        free(backhaul);
    }
    
    return GFCTL_OK;
}

/*
 * gfctl_get_network_topology - Get complete network topology
 */
int gfctl_get_network_topology(gfctl_context_t *ctx, network_topology_t *topo) {
    int ret;
    char *result;
    char **keys;
    int key_count;
    int i;
    
    if (!ctx || !topo) return GFCTL_ERR_INVALID_ARG;
    
    /* Ensure we have a valid session */
    ret = gfctl_ensure_session(ctx);
    if (ret != GFCTL_OK) return ret;
    
    memset(topo, 0, sizeof(network_topology_t));
    
    ret = gfctl_jsonrpc_call(ctx, "network", "topology", "{}", &ctx->response_buffer);
    if (ret != GFCTL_OK) return ret;
    
    result = gfctl_jsonrpc_get_result(ctx->response_buffer.data);
    if (!result) return GFCTL_ERR_JSON_PARSE;
    
    /* Get all device MACs (keys of the result object) */
    keys = json_object_keys(result, &key_count);
    if (!keys || key_count == 0) {
        free(result);
        return GFCTL_OK;
    }
    
    topo->devices = (mesh_device_t *)calloc(key_count, sizeof(mesh_device_t));
    if (!topo->devices) {
        for (i = 0; i < key_count; i++) free(keys[i]);
        free(keys);
        free(result);
        return GFCTL_ERR_MEMORY;
    }
    
    topo->device_count = 0;
    for (i = 0; i < key_count; i++) {
        char *dev_obj = json_get_object(result, keys[i]);
        if (dev_obj) {
            parse_mesh_device(dev_obj, keys[i], &topo->devices[topo->device_count]);
            topo->device_count++;
            free(dev_obj);
        }
        free(keys[i]);
    }
    
    free(keys);
    free(result);
    return GFCTL_OK;
}

/*
 * gfctl_free_topology - Free network topology memory
 */
void gfctl_free_topology(network_topology_t *topo) {
    int i;
    
    if (!topo) return;
    
    if (topo->devices) {
        for (i = 0; i < topo->device_count; i++) {
            if (topo->devices[i].interfaces) free(topo->devices[i].interfaces);
            if (topo->devices[i].radios) free(topo->devices[i].radios);
            if (topo->devices[i].clients) free(topo->devices[i].clients);
        }
        free(topo->devices);
    }
    
    topo->devices = NULL;
    topo->device_count = 0;
}

/*
 * gfctl_check_session - Check if current session is valid
 */
int gfctl_check_session(gfctl_context_t *ctx, bool *valid) {
    int ret;
    char *result;
    
    if (!ctx || !valid) return GFCTL_ERR_INVALID_ARG;
    
    *valid = false;
    
    ret = gfctl_jsonrpc_call(ctx, "session", "access",
                             "{\"scope\":\"ubus\",\"object\":\"session\",\"function\":\"access\"}",
                             &ctx->response_buffer);
    
    if (ret == GFCTL_ERR_AUTH) {
        *valid = false;
        return GFCTL_OK;
    }
    
    if (ret != GFCTL_OK) return ret;
    
    result = gfctl_jsonrpc_get_result(ctx->response_buffer.data);
    if (result) {
        *valid = json_get_bool(result, "access", false);
        free(result);
    }
    
    return GFCTL_OK;
}
