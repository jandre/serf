/* Copyright 2013 Justin Erenkrantz and Greg Stein
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <apr_strings.h>

#include "serf.h"
#include "serf_bucket_util.h"
#include "serf_private.h"

/* APR requires the keys in its hash table to be pointers. */
serf_config_key_t serf_config_host_name = SERF_CONFIG_PER_HOST | 0x000001;
serf_config_key_t serf_config_host_port = SERF_CONFIG_PER_HOST | 0x000002;

/*** Config Store ***/
apr_status_t serf__init_config_store(serf_context_t *ctx)
{
    apr_pool_t *pool = ctx->pool;

    ctx->config_store.pool = pool;
    ctx->config_store.per_context = apr_hash_make(pool);
    ctx->config_store.per_host = apr_hash_make(pool);
    ctx->config_store.per_conn = apr_hash_make(pool);

    return APR_ENOTIMPL;
}

/* Defines the key to use for per host settings */
static const char * host_key_for_conn(serf_connection_t *conn,
                                      apr_pool_t *pool)
{
    /* SCHEME://HOSTNAME:PORT, e.g. http://localhost:12345 */
    return conn->host_url;
}

/* Defines the key to use for per connection settings */
static const char * conn_key_for_conn(serf_connection_t *conn,
                                      apr_pool_t *pool)
{
    /* Key needs to be unique per connection, so stringify its pointer value */
    return apr_psprintf(pool, "%x", (unsigned int)conn);
}

/* TODO: when will this be released? Related config to a specific lifecyle:
   connection or context */
apr_status_t serf_get_config_from_store(serf_context_t *ctx,
                                        serf_connection_t *conn,
                                        serf_config_t **config,
                                        apr_pool_t *out_pool)
{
    serf__config_store_t *config_store = &ctx->config_store;

    serf_config_t *cfg = apr_pcalloc(out_pool, sizeof(serf_config_t));
    cfg->pool = out_pool;
    cfg->per_context = config_store->per_context;

    if (conn) {
        const char *host_key, *conn_key;
        apr_hash_t *per_conn, *per_host;
        apr_pool_t *tmp_pool;
        apr_status_t status;

        if ((status = apr_pool_create(&tmp_pool, out_pool)) != APR_SUCCESS)
            return status;

        /* Find the config values for this connection, create empty structure
           if needed */
        conn_key = conn_key_for_conn(conn, tmp_pool);
        per_conn = apr_hash_get(config_store->per_conn, conn_key,
                                APR_HASH_KEY_STRING);
        if (!per_conn) {
            per_conn = apr_hash_make(config_store->pool);
            apr_hash_set(config_store->per_conn,
                         apr_pstrdup(config_store->pool, conn_key),
                         APR_HASH_KEY_STRING, per_conn);
        }
        cfg->per_conn = per_conn;

        /* Find the config values for this host, create empty structure
           if needed */
        host_key = host_key_for_conn(conn, tmp_pool);
        per_host = apr_hash_get(config_store->per_host,
                                host_key,
                                APR_HASH_KEY_STRING);
        if (!per_host) {
            per_host = apr_hash_make(config_store->pool);
            apr_hash_set(config_store->per_host,
                         apr_pstrdup(config_store->pool, host_key),
                         APR_HASH_KEY_STRING, per_host);
        }
        cfg->per_host = per_host;

        apr_pool_destroy(tmp_pool);
    }

    *config = cfg;

    return APR_SUCCESS;
}

apr_status_t
serf__remove_connection_from_config_store(serf__config_store_t config_store,
                                          serf_connection_t *conn)
{
    return APR_ENOTIMPL;
}

apr_status_t
serf__remove_host_from_config_store(serf__config_store_t config_store,
                                    const char *hostname_port)
{
    return APR_ENOTIMPL;
}

/*** Config ***/
apr_status_t serf_set_config_string(serf_config_t *config,
                                    serf_config_key_ptr_t key,
                                    const char *value,
                                    int copy_flags)
{
    const char *cvalue = value;
    apr_hash_t *target;
    serf_config_key_t keyint = *key;

    /* Copy value to our pool's memory? */
    if (copy_flags & SERF_CONFIG_COPY_VALUE) {
        cvalue = apr_pstrdup(config->pool, value);
    }

    /* Set the value in the hash table of the selected category */
    if (keyint & SERF_CONFIG_PER_CONTEXT)
        target = config->per_context;
    else if (keyint & SERF_CONFIG_PER_HOST)
        target = config->per_host;
    else
        target = config->per_conn;


    if (!target) {
        /* Config object doesn't manage keys in this category */
        return APR_EINVAL;
    }

    apr_hash_set(target, key, sizeof(serf_config_key_t), cvalue);

    return APR_SUCCESS;
}

apr_status_t serf_set_config_object(serf_config_t *config,
                                    serf_config_key_ptr_t key,
                                    void *value,
                                    apr_size_t *len,
                                    int copy_flags)
{
    return APR_ENOTIMPL;
}

apr_status_t serf_get_config_string(serf_config_t *config,
                                    serf_config_key_ptr_t key,
                                    char **value)
{
    apr_hash_t *target;
    serf_config_key_t keyint = *key;

    if (keyint & SERF_CONFIG_PER_CONTEXT)
        target = config->per_context;
    else if (keyint & SERF_CONFIG_PER_HOST)
        target = config->per_host;
    else
        target = config->per_conn;

    if (!target) {
        *value = NULL;
        return APR_EINVAL;
    }

    *value = (char*)apr_hash_get(target, key, sizeof(serf_config_key_t));

    return APR_SUCCESS;
}

apr_status_t serf_remove_config_value(serf_config_t *config,
                                      serf_config_key_ptr_t key)
{
    return APR_ENOTIMPL;
}
