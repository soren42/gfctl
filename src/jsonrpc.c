/*
 * jsonrpc.c - JSON-RPC Layer for gfctl
 *
 * Implements the JSON-RPC 2.0 protocol for communicating with
 * OpenWRT's ubus interface.
 *
 * Request format:
 * {
 *   "jsonrpc": "2.0",
 *   "id": <sequence>,
 *   "method": "call",
 *   "params": [<session_token>, <object>, <method>, {<params>}]
 * }
 *
 * Response format:
 * {
 *   "jsonrpc": "2.0",
 *   "id": <sequence>,
 *   "result": [<code>, {<data>}]
 * }
 * or on error:
 * {
 *   "jsonrpc": "2.0",
 *   "id": <sequence>,
 *   "error": {"code": <code>, "message": "<msg>"}
 * }
 */

#include "gfctl.h"

/*
 * gfctl_jsonrpc_build_request - Build a JSON-RPC request string
 *
 * @ctx: gfctl context
 * @object: ubus object name (e.g., "data_repo.webinfo")
 * @method: ubus method name (e.g., "system")
 * @params: Additional parameters as JSON object string (can be "{}")
 *
 * Returns: Newly allocated JSON string (caller must free) or NULL
 */
char *gfctl_jsonrpc_build_request(gfctl_context_t *ctx, const char *object,
                                   const char *method, const char *params) {
    char *request;
    size_t max_len;
    int written;
    
    if (!ctx || !object || !method) return NULL;
    
    /* Use empty object if params is NULL */
    if (!params) params = "{}";
    
    /* Estimate max length needed */
    max_len = 256 + strlen(object) + strlen(method) + strlen(params) +
              strlen(ctx->config.session_token);
    
    request = (char *)malloc(max_len);
    if (!request) return NULL;
    
    ctx->request_id++;
    
    written = snprintf(request, max_len,
        "{\"jsonrpc\":\"2.0\",\"id\":%d,\"method\":\"call\","
        "\"params\":[\"%s\",\"%s\",\"%s\",%s]}",
        ctx->request_id,
        ctx->config.session_token,
        object,
        method,
        params);
    
    if (written < 0 || (size_t)written >= max_len) {
        free(request);
        return NULL;
    }
    
    return request;
}

/*
 * gfctl_jsonrpc_parse_error - Check for JSON-RPC error in response
 *
 * Returns: GFCTL_OK if no error, error code otherwise
 */
static int gfctl_jsonrpc_parse_error(const char *response) {
    char *error_obj;
    int code;
    
    /* Check for error object */
    error_obj = json_get_object(response, "error");
    if (error_obj) {
        code = json_get_int(error_obj, "code", 0);
        free(error_obj);
        
        /* Map JSON-RPC error codes to gfctl errors */
        switch (code) {
            case -32002:  /* Access denied */
                return GFCTL_ERR_AUTH;
            case -32600:  /* Invalid request */
            case -32601:  /* Method not found */
            case -32602:  /* Invalid params */
                return GFCTL_ERR_INVALID_ARG;
            case -32700:  /* Parse error */
                return GFCTL_ERR_JSON_PARSE;
            default:
                return GFCTL_ERR_API;
        }
    }
    
    return GFCTL_OK;
}

/*
 * gfctl_jsonrpc_call - Make a JSON-RPC call to ubus
 *
 * @ctx: gfctl context
 * @object: ubus object name
 * @method: ubus method name
 * @params: Additional parameters as JSON object string
 * @response: Buffer to receive raw response
 *
 * Returns: GFCTL_OK on success, error code on failure
 */
int gfctl_jsonrpc_call(gfctl_context_t *ctx, const char *object,
                       const char *method, const char *params,
                       gfctl_buffer_t *response) {
    char *request;
    int ret;
    
    if (!ctx || !object || !method || !response) {
        return GFCTL_ERR_INVALID_ARG;
    }
    
    /* Build request */
    request = gfctl_jsonrpc_build_request(ctx, object, method, params);
    if (!request) {
        return GFCTL_ERR_MEMORY;
    }
    
    if (ctx->config.verbose) {
        fprintf(stderr, "Request: %s\n", request);
    }
    
    /* Make HTTP POST request */
    ret = gfctl_http_post(ctx, "/ubus", request, response);
    free(request);
    
    if (ret != GFCTL_OK) {
        return ret;
    }
    
    if (ctx->config.verbose) {
        fprintf(stderr, "Response: %s\n", response->data);
    }
    
    /* Check for JSON-RPC errors */
    ret = gfctl_jsonrpc_parse_error(response->data);
    
    return ret;
}

/*
 * gfctl_jsonrpc_get_result - Extract result data from JSON-RPC response
 *
 * The result is typically an array: [0, {data}] where 0 is the status code
 *
 * Returns: Newly allocated JSON string of the data object (caller must free)
 */
char *gfctl_jsonrpc_get_result(const char *response) {
    char *result_array;
    char *data;
    
    if (!response) return NULL;
    
    /* Get the result array */
    result_array = json_get_array(response, "result");
    if (!result_array) return NULL;
    
    /* The first element is the status code, second is the data */
    data = json_array_get(result_array, 1);
    free(result_array);
    
    return data;
}

/*
 * gfctl_jsonrpc_get_status - Get status code from JSON-RPC result
 *
 * Returns: Status code (0 = success) or -1 on error
 */
int gfctl_jsonrpc_get_status(const char *response) {
    char *result_array;
    char *status_str;
    int status;
    
    if (!response) return -1;
    
    result_array = json_get_array(response, "result");
    if (!result_array) return -1;
    
    status_str = json_array_get(result_array, 0);
    free(result_array);
    
    if (!status_str) return -1;
    
    status = atoi(status_str);
    free(status_str);
    
    return status;
}
