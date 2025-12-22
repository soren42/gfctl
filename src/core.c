/*
 * core.c - Core Functions for gfctl
 *
 * Context management, buffer handling, and utility functions.
 */

#include "gfctl.h"

/*
 * gfctl_init - Initialize a new gfctl context
 *
 * Returns: New context or NULL on failure
 */
gfctl_context_t *gfctl_init(void) {
    gfctl_context_t *ctx;
    
    ctx = (gfctl_context_t *)calloc(1, sizeof(gfctl_context_t));
    if (!ctx) return NULL;
    
    /* Set defaults */
    strncpy(ctx->config.host, GFCTL_DEFAULT_HOST, sizeof(ctx->config.host) - 1);
    ctx->config.port = GFCTL_DEFAULT_PORT;
    strncpy(ctx->config.session_token, GFCTL_UNAUTH_TOKEN,
            sizeof(ctx->config.session_token) - 1);
    ctx->config.timeout = GFCTL_DEFAULT_TIMEOUT;
    ctx->config.verify_ssl = false;  /* Router uses self-signed cert */
    ctx->config.verbose = false;
    ctx->config.output_format = OUTPUT_FORMAT_TEXT;
    
    ctx->request_id = 0;
    
    /* Initialize response buffer */
    ctx->response_buffer.data = (char *)malloc(GFCTL_MAX_BUFFER_SIZE);
    if (!ctx->response_buffer.data) {
        free(ctx);
        return NULL;
    }
    ctx->response_buffer.size = 0;
    ctx->response_buffer.capacity = GFCTL_MAX_BUFFER_SIZE;
    
    /* Initialize HTTP layer */
    if (gfctl_http_init(ctx) != GFCTL_OK) {
        free(ctx->response_buffer.data);
        free(ctx);
        return NULL;
    }
    
    return ctx;
}

/*
 * gfctl_cleanup - Clean up and free a gfctl context
 */
void gfctl_cleanup(gfctl_context_t *ctx) {
    if (!ctx) return;
    
    gfctl_http_cleanup(ctx);
    
    if (ctx->response_buffer.data) {
        free(ctx->response_buffer.data);
    }
    
    free(ctx);
}

/*
 * Configuration setters
 */
int gfctl_set_host(gfctl_context_t *ctx, const char *host) {
    if (!ctx || !host) return GFCTL_ERR_INVALID_ARG;
    strncpy(ctx->config.host, host, sizeof(ctx->config.host) - 1);
    ctx->config.host[sizeof(ctx->config.host) - 1] = '\0';
    return GFCTL_OK;
}

int gfctl_set_port(gfctl_context_t *ctx, int port) {
    if (!ctx || port <= 0 || port > 65535) return GFCTL_ERR_INVALID_ARG;
    ctx->config.port = port;
    return GFCTL_OK;
}

int gfctl_set_session(gfctl_context_t *ctx, const char *token) {
    if (!ctx || !token) return GFCTL_ERR_INVALID_ARG;
    strncpy(ctx->config.session_token, token, sizeof(ctx->config.session_token) - 1);
    ctx->config.session_token[sizeof(ctx->config.session_token) - 1] = '\0';
    return GFCTL_OK;
}

int gfctl_set_output_format(gfctl_context_t *ctx, output_format_t format) {
    if (!ctx) return GFCTL_ERR_INVALID_ARG;
    ctx->config.output_format = format;
    return GFCTL_OK;
}

/*
 * Buffer management
 */
gfctl_buffer_t *gfctl_buffer_new(size_t initial_capacity) {
    gfctl_buffer_t *buf;
    
    buf = (gfctl_buffer_t *)malloc(sizeof(gfctl_buffer_t));
    if (!buf) return NULL;
    
    if (initial_capacity == 0) initial_capacity = 4096;
    
    buf->data = (char *)malloc(initial_capacity);
    if (!buf->data) {
        free(buf);
        return NULL;
    }
    
    buf->size = 0;
    buf->capacity = initial_capacity;
    buf->data[0] = '\0';
    
    return buf;
}

int gfctl_buffer_append(gfctl_buffer_t *buf, const char *data, size_t len) {
    size_t new_size;
    
    if (!buf || !data) return GFCTL_ERR_INVALID_ARG;
    
    new_size = buf->size + len;
    
    /* Grow buffer if needed */
    if (new_size >= buf->capacity) {
        size_t new_cap = buf->capacity * 2;
        char *new_data;
        
        while (new_cap <= new_size) new_cap *= 2;
        
        new_data = (char *)realloc(buf->data, new_cap);
        if (!new_data) return GFCTL_ERR_MEMORY;
        
        buf->data = new_data;
        buf->capacity = new_cap;
    }
    
    memcpy(buf->data + buf->size, data, len);
    buf->size = new_size;
    
    return GFCTL_OK;
}

void gfctl_buffer_clear(gfctl_buffer_t *buf) {
    if (buf) {
        buf->size = 0;
        if (buf->data) buf->data[0] = '\0';
    }
}

void gfctl_buffer_free(gfctl_buffer_t *buf) {
    if (buf) {
        if (buf->data) free(buf->data);
        free(buf);
    }
}

/*
 * Utility functions
 */
const char *gfctl_error_string(gfctl_error_t err) {
    switch (err) {
        case GFCTL_OK:            return "Success";
        case GFCTL_ERR_MEMORY:    return "Memory allocation failed";
        case GFCTL_ERR_NETWORK:   return "Network error";
        case GFCTL_ERR_JSON_PARSE: return "JSON parse error";
        case GFCTL_ERR_API:       return "API error";
        case GFCTL_ERR_AUTH:      return "Authentication required";
        case GFCTL_ERR_TIMEOUT:   return "Request timed out";
        case GFCTL_ERR_INVALID_ARG: return "Invalid argument";
        case GFCTL_ERR_NOT_FOUND: return "Not found";
        case GFCTL_ERR_CURL_INIT: return "Failed to initialize HTTP client";
        case GFCTL_ERR_SSL:       return "SSL/TLS error";
        default:                  return "Unknown error";
    }
}

const char *gfctl_medium_type_string(medium_type_t type) {
    switch (type) {
        case MEDIUM_ETHER:  return "Ethernet";
        case MEDIUM_WLAN2G: return "WiFi 2.4GHz";
        case MEDIUM_WLAN5G: return "WiFi 5GHz";
        case MEDIUM_WLAN6G: return "WiFi 6GHz";
        default:            return "Unknown";
    }
}

medium_type_t gfctl_parse_medium_type(const char *str) {
    if (!str) return MEDIUM_UNKNOWN;
    if (strcmp(str, "ETHER") == 0) return MEDIUM_ETHER;
    if (strcmp(str, "WLAN2G") == 0) return MEDIUM_WLAN2G;
    if (strcmp(str, "WLAN5G") == 0) return MEDIUM_WLAN5G;
    if (strcmp(str, "WLAN6G") == 0) return MEDIUM_WLAN6G;
    return MEDIUM_UNKNOWN;
}

char *gfctl_format_uptime(unsigned long seconds) {
    static char buf[64];
    unsigned long days, hours, minutes;
    
    days = seconds / 86400;
    seconds %= 86400;
    hours = seconds / 3600;
    seconds %= 3600;
    minutes = seconds / 60;
    seconds %= 60;
    
    if (days > 0) {
        snprintf(buf, sizeof(buf), "%lud %luh %lum %lus",
                 days, hours, minutes, seconds);
    } else if (hours > 0) {
        snprintf(buf, sizeof(buf), "%luh %lum %lus", hours, minutes, seconds);
    } else if (minutes > 0) {
        snprintf(buf, sizeof(buf), "%lum %lus", minutes, seconds);
    } else {
        snprintf(buf, sizeof(buf), "%lus", seconds);
    }
    
    return buf;
}

output_format_t gfctl_parse_format(const char *str) {
    if (!str) return OUTPUT_FORMAT_TEXT;
    if (strcasecmp(str, "json") == 0) return OUTPUT_FORMAT_JSON;
    if (strcasecmp(str, "xml") == 0) return OUTPUT_FORMAT_XML;
    if (strcasecmp(str, "csv") == 0) return OUTPUT_FORMAT_CSV;
    if (strcasecmp(str, "raw") == 0) return OUTPUT_FORMAT_RAW;
    return OUTPUT_FORMAT_TEXT;
}
