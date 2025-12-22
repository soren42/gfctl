/*
 * http.c - HTTP Transport Layer for gfctl
 *
 * Provides HTTPS communication with the OpenWRT ubus interface
 * using libcurl.
 */

#include "gfctl.h"
#include <curl/curl.h>

/* Callback for writing received data */
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    gfctl_buffer_t *buf = (gfctl_buffer_t *)userdata;
    size_t total = size * nmemb;
    
    if (gfctl_buffer_append(buf, ptr, total) != GFCTL_OK) {
        return 0;  /* Signal error to curl */
    }
    
    return total;
}

/*
 * gfctl_http_init - Initialize HTTP layer
 */
int gfctl_http_init(gfctl_context_t *ctx) {
    CURL *curl;
    
    if (!ctx) return GFCTL_ERR_INVALID_ARG;
    
    /* Global curl init (safe to call multiple times) */
    curl_global_init(CURL_GLOBAL_DEFAULT);
    
    /* Create curl handle */
    curl = curl_easy_init();
    if (!curl) {
        return GFCTL_ERR_CURL_INIT;
    }
    
    ctx->curl_handle = curl;
    return GFCTL_OK;
}

/*
 * gfctl_http_cleanup - Clean up HTTP layer
 */
void gfctl_http_cleanup(gfctl_context_t *ctx) {
    if (ctx && ctx->curl_handle) {
        curl_easy_cleanup((CURL *)ctx->curl_handle);
        ctx->curl_handle = NULL;
    }
}

/*
 * gfctl_http_post - Send HTTP POST request
 *
 * @ctx: gfctl context
 * @endpoint: URL path (e.g., "/ubus")
 * @body: POST body data
 * @response: Buffer to receive response
 *
 * Returns: GFCTL_OK on success, error code on failure
 */
int gfctl_http_post(gfctl_context_t *ctx, const char *endpoint,
                    const char *body, gfctl_buffer_t *response) {
    CURL *curl;
    CURLcode res;
    char url[GFCTL_MAX_URL_LEN];
    struct curl_slist *headers = NULL;
    long http_code;
    
    if (!ctx || !ctx->curl_handle || !endpoint || !body || !response) {
        return GFCTL_ERR_INVALID_ARG;
    }
    
    curl = (CURL *)ctx->curl_handle;
    
    /* Build full URL */
    snprintf(url, sizeof(url), "https://%s:%d%s",
             ctx->config.host, ctx->config.port, endpoint);
    
    /* Reset curl handle for reuse */
    curl_easy_reset(curl);
    
    /* Set URL */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    
    /* Set POST data */
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, strlen(body));
    
    /* Set headers */
    headers = curl_slist_append(headers, "Content-Type: application/json;charset=utf-8");
    headers = curl_slist_append(headers, "Accept: application/json, text/plain, */*");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    /* Set response handling */
    gfctl_buffer_clear(response);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, response);
    
    /* SSL options */
    if (!ctx->config.verify_ssl) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
    }
    
    /* Timeout */
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, (long)ctx->config.timeout);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    
    /* Verbose output for debugging */
    if (ctx->config.verbose) {
        curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
    }
    
    /* Perform request */
    res = curl_easy_perform(curl);
    
    /* Clean up headers */
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        if (ctx->config.verbose) {
            fprintf(stderr, "curl error: %s\n", curl_easy_strerror(res));
        }
        
        switch (res) {
            case CURLE_COULDNT_CONNECT:
            case CURLE_COULDNT_RESOLVE_HOST:
                return GFCTL_ERR_NETWORK;
            case CURLE_OPERATION_TIMEDOUT:
                return GFCTL_ERR_TIMEOUT;
            case CURLE_SSL_CONNECT_ERROR:
            case CURLE_SSL_CERTPROBLEM:
            case CURLE_SSL_CIPHER:
                return GFCTL_ERR_SSL;
            default:
                return GFCTL_ERR_NETWORK;
        }
    }
    
    /* Check HTTP response code */
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    if (http_code != 200) {
        if (ctx->config.verbose) {
            fprintf(stderr, "HTTP error: %ld\n", http_code);
        }
        if (http_code == 401 || http_code == 403) {
            return GFCTL_ERR_AUTH;
        }
        return GFCTL_ERR_API;
    }
    
    /* Null-terminate the response */
    gfctl_buffer_append(response, "\0", 1);
    
    return GFCTL_OK;
}
