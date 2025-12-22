/*
 * json_utils.c - Minimal JSON Parsing Utilities for gfctl
 *
 * This is a lightweight JSON parser designed specifically for the
 * ubus JSON-RPC responses. It's not a full-featured JSON parser,
 * but handles the specific structures we encounter.
 *
 * For production use, consider linking against cJSON or json-c.
 */

#include "gfctl.h"
#include <ctype.h>

/* Skip whitespace in JSON string */
static const char *skip_ws(const char *p) {
    if (!p) return NULL;
    while (*p && isspace((unsigned char)*p)) p++;
    return p;
}

/* Find the end of a JSON string (handling escapes) */
static const char *find_string_end(const char *p) {
    if (!p || *p != '"') return NULL;
    p++;  /* Skip opening quote */
    while (*p) {
        if (*p == '\\' && *(p+1)) {
            p += 2;  /* Skip escaped character */
        } else if (*p == '"') {
            return p;  /* Found closing quote */
        } else {
            p++;
        }
    }
    return NULL;
}

/* Find matching closing bracket/brace */
static const char *find_matching_bracket(const char *p, char open, char close) {
    int depth = 1;
    if (!p || *p != open) return NULL;
    p++;
    
    while (*p && depth > 0) {
        if (*p == '"') {
            p = find_string_end(p);
            if (!p) return NULL;
        } else if (*p == open) {
            depth++;
        } else if (*p == close) {
            depth--;
            if (depth == 0) return p;
        }
        p++;
    }
    return (depth == 0) ? p - 1 : NULL;
}

/* Find a key in a JSON object and return pointer to its value */
static const char *json_find_key(const char *json, const char *key) {
    const char *p;
    size_t key_len;
    
    if (!json || !key) return NULL;
    key_len = strlen(key);
    
    p = skip_ws(json);
    if (!p || *p != '{') return NULL;
    p++;
    
    while (*p) {
        p = skip_ws(p);
        if (!p || *p == '}') break;
        
        /* Skip comma if present */
        if (*p == ',') {
            p++;
            p = skip_ws(p);
        }
        
        /* Expect a string key */
        if (*p != '"') {
            p++;
            continue;
        }
        
        /* Check if this is our key */
        if (strncmp(p + 1, key, key_len) == 0 && *(p + 1 + key_len) == '"') {
            /* Found the key, skip to value */
            p = p + 2 + key_len;  /* Skip key and quotes */
            p = skip_ws(p);
            if (*p == ':') {
                p++;
                return skip_ws(p);
            }
        }
        
        /* Skip this key */
        p = find_string_end(p);
        if (!p) return NULL;
        p++;
        p = skip_ws(p);
        if (*p == ':') p++;
        p = skip_ws(p);
        
        /* Skip the value */
        if (*p == '"') {
            p = find_string_end(p);
            if (p) p++;
        } else if (*p == '{') {
            p = find_matching_bracket(p, '{', '}');
            if (p) p++;
        } else if (*p == '[') {
            p = find_matching_bracket(p, '[', ']');
            if (p) p++;
        } else {
            /* Number, bool, or null - skip to next delimiter */
            while (*p && *p != ',' && *p != '}') p++;
        }
    }
    
    return NULL;
}

/*
 * json_get_string - Extract a string value for a given key
 *
 * Returns: Newly allocated string (caller must free) or NULL
 */
char *json_get_string(const char *json, const char *key) {
    const char *value, *end;
    char *result;
    size_t len;
    
    value = json_find_key(json, key);
    if (!value || *value != '"') return NULL;
    
    value++;  /* Skip opening quote */
    end = find_string_end(value - 1);
    if (!end) return NULL;
    
    len = end - value;
    result = (char *)malloc(len + 1);
    if (!result) return NULL;
    
    /* Copy with basic unescape */
    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (value[i] == '\\' && i + 1 < len) {
            i++;
            switch (value[i]) {
                case 'n': result[j++] = '\n'; break;
                case 't': result[j++] = '\t'; break;
                case 'r': result[j++] = '\r'; break;
                case '"': result[j++] = '"'; break;
                case '\\': result[j++] = '\\'; break;
                default: result[j++] = value[i]; break;
            }
        } else {
            result[j++] = value[i];
        }
    }
    result[j] = '\0';
    
    return result;
}

/*
 * json_get_int - Extract an integer value for a given key
 *
 * Returns: Integer value or default_val if not found
 */
int json_get_int(const char *json, const char *key, int default_val) {
    const char *value;
    
    value = json_find_key(json, key);
    if (!value) return default_val;
    
    /* Handle string-encoded integers (common in ubus) */
    if (*value == '"') {
        value++;
    }
    
    if (*value == '-' || isdigit((unsigned char)*value)) {
        return atoi(value);
    }
    
    return default_val;
}

/*
 * json_get_bool - Extract a boolean value for a given key
 *
 * Returns: Boolean value or default_val if not found
 */
bool json_get_bool(const char *json, const char *key, bool default_val) {
    const char *value;
    
    value = json_find_key(json, key);
    if (!value) return default_val;
    
    if (strncmp(value, "true", 4) == 0) return true;
    if (strncmp(value, "false", 5) == 0) return false;
    
    /* Handle numeric booleans */
    if (*value == '1') return true;
    if (*value == '0') return false;
    
    /* Handle string-encoded booleans */
    if (*value == '"') {
        if (strncmp(value + 1, "true", 4) == 0) return true;
        if (strncmp(value + 1, "1", 1) == 0) return true;
    }
    
    return default_val;
}

/*
 * json_get_object - Extract a nested object for a given key
 *
 * Returns: Newly allocated JSON string (caller must free) or NULL
 */
char *json_get_object(const char *json, const char *key) {
    const char *value, *end;
    char *result;
    size_t len;
    
    value = json_find_key(json, key);
    if (!value || *value != '{') return NULL;
    
    end = find_matching_bracket(value, '{', '}');
    if (!end) return NULL;
    
    len = end - value + 1;
    result = (char *)malloc(len + 1);
    if (!result) return NULL;
    
    memcpy(result, value, len);
    result[len] = '\0';
    
    return result;
}

/*
 * json_get_array - Extract an array for a given key
 *
 * Returns: Newly allocated JSON string (caller must free) or NULL
 */
char *json_get_array(const char *json, const char *key) {
    const char *value, *end;
    char *result;
    size_t len;
    
    value = json_find_key(json, key);
    if (!value || *value != '[') return NULL;
    
    end = find_matching_bracket(value, '[', ']');
    if (!end) return NULL;
    
    len = end - value + 1;
    result = (char *)malloc(len + 1);
    if (!result) return NULL;
    
    memcpy(result, value, len);
    result[len] = '\0';
    
    return result;
}

/*
 * json_array_count - Count elements in a JSON array
 *
 * Returns: Number of elements or -1 on error
 */
int json_array_count(const char *json) {
    const char *p;
    int count = 0;
    int depth = 0;
    
    if (!json) return -1;
    p = skip_ws(json);
    if (!p || *p != '[') return -1;
    p++;
    p = skip_ws(p);
    
    if (*p == ']') return 0;  /* Empty array */
    
    count = 1;
    while (*p) {
        if (*p == '"') {
            p = find_string_end(p);
            if (!p) return -1;
        } else if (*p == '{') {
            p = find_matching_bracket(p, '{', '}');
            if (!p) return -1;
        } else if (*p == '[') {
            p = find_matching_bracket(p, '[', ']');
            if (!p) return -1;
        } else if (*p == ',' && depth == 0) {
            count++;
        } else if (*p == ']' && depth == 0) {
            break;
        }
        p++;
    }
    
    return count;
}

/*
 * json_array_get - Get element at index from JSON array
 *
 * Returns: Newly allocated JSON string (caller must free) or NULL
 */
char *json_array_get(const char *json, int index) {
    const char *p, *start, *end;
    char *result;
    int current = 0;
    size_t len;
    
    if (!json || index < 0) return NULL;
    p = skip_ws(json);
    if (!p || *p != '[') return NULL;
    p++;
    
    while (*p) {
        p = skip_ws(p);
        if (*p == ']') return NULL;  /* Index out of bounds */
        if (*p == ',') {
            p++;
            continue;
        }
        
        start = p;
        
        /* Find end of this element */
        if (*p == '"') {
            end = find_string_end(p);
            if (!end) return NULL;
            end++;
        } else if (*p == '{') {
            end = find_matching_bracket(p, '{', '}');
            if (!end) return NULL;
            end++;
        } else if (*p == '[') {
            end = find_matching_bracket(p, '[', ']');
            if (!end) return NULL;
            end++;
        } else {
            /* Primitive value */
            end = p;
            while (*end && *end != ',' && *end != ']') end++;
        }
        
        if (current == index) {
            len = end - start;
            result = (char *)malloc(len + 1);
            if (!result) return NULL;
            memcpy(result, start, len);
            result[len] = '\0';
            return result;
        }
        
        current++;
        p = end;
    }
    
    return NULL;
}

/*
 * json_object_keys - Get all keys from a JSON object
 *
 * Returns: Array of newly allocated strings (caller must free each and the array)
 *          count is set to number of keys
 */
char **json_object_keys(const char *json, int *count) {
    const char *p;
    char **keys = NULL;
    int capacity = 8;
    int n = 0;
    
    if (!json || !count) return NULL;
    *count = 0;
    
    p = skip_ws(json);
    if (!p || *p != '{') return NULL;
    p++;
    
    keys = (char **)malloc(capacity * sizeof(char *));
    if (!keys) return NULL;
    
    while (*p) {
        p = skip_ws(p);
        if (*p == '}') break;
        if (*p == ',') {
            p++;
            continue;
        }
        
        if (*p == '"') {
            const char *key_start = p + 1;
            const char *key_end = find_string_end(p);
            if (!key_end) break;
            
            size_t key_len = key_end - key_start;
            
            /* Grow array if needed */
            if (n >= capacity) {
                capacity *= 2;
                char **new_keys = (char **)realloc(keys, capacity * sizeof(char *));
                if (!new_keys) {
                    /* Free what we have */
                    for (int i = 0; i < n; i++) free(keys[i]);
                    free(keys);
                    return NULL;
                }
                keys = new_keys;
            }
            
            keys[n] = (char *)malloc(key_len + 1);
            if (!keys[n]) break;
            memcpy(keys[n], key_start, key_len);
            keys[n][key_len] = '\0';
            n++;
            
            /* Skip to value and then skip the value */
            p = key_end + 1;
            p = skip_ws(p);
            if (*p == ':') p++;
            p = skip_ws(p);
            
            if (*p == '"') {
                p = find_string_end(p);
                if (p) p++;
            } else if (*p == '{') {
                p = find_matching_bracket(p, '{', '}');
                if (p) p++;
            } else if (*p == '[') {
                p = find_matching_bracket(p, '[', ']');
                if (p) p++;
            } else {
                while (*p && *p != ',' && *p != '}') p++;
            }
        } else {
            p++;
        }
    }
    
    *count = n;
    return keys;
}
