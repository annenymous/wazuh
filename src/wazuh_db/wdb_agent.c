/*
 * Wazuh SQLite integration
 * Copyright (C) 2015-2020, Wazuh Inc.
 * July 5, 2016.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include "wdb.h"
#include "defs.h"
#include "wazuhdb_op.h"

#ifdef WIN32
#define chown(x, y, z) 0
#endif

#define WDBQUERY_SIZE OS_BUFFER_SIZE
#define WDBOUTPUT_SIZE OS_MAXSTR

int wdb_sock_agent = -1;

static const char *global_db_commands[] = {
    [WDB_INSERT_AGENT] = "global insert-agent %s",
    [WDB_INSERT_AGENT_GROUP] = "global insert-agent-group %s",
    [WDB_INSERT_AGENT_BELONG] = "global insert-agent-belong %s",
    [WDB_UPDATE_AGENT_NAME] = "global update-agent-name %s",
    [WDB_UPDATE_AGENT_VERSION] = "global update-agent-version %s",
    [WDB_UPDATE_AGENT_KEEPALIVE] = "global update-keepalive %s",
    [WDB_UPDATE_AGENT_STATUS] = "global update-agent-status %s",
    [WDB_UPDATE_AGENT_GROUP] = "global update-agent-group %s",
    [WDB_UPDATE_FIM_OFFSET] = "global update-fim-offset %s",
    [WDB_UPDATE_REG_OFFSET] = "global update-reg-offset %s",
    [WDB_SET_AGENT_LABELS] = "global set-labels %d %s",
    [WDB_GET_ALL_AGENTS] = "global get-all-agents start_id %d",
    [WDB_GET_AGENTS_BY_KEEPALIVE] = "global get-agents-by-keepalive condition %s %d start_id %d",
    [WDB_FIND_AGENT] = "global find-agent %s",
    [WDB_GET_AGENT_INFO] = "global get-agent-info %d",
    [WDB_GET_AGENT_LABELS] = "global get-labels %d",
    [WDB_SELECT_AGENT_NAME] = "global select-agent-name %d",
    [WDB_SELECT_AGENT_GROUP] = "global select-agent-group %d",
    [WDB_SELECT_AGENT_STATUS] = "global select-agent-status %d",
    [WDB_SELECT_KEEPALIVE] = "global select-keepalive %s %s",
    [WDB_SELECT_FIM_OFFSET] = "global select-fim-offset %d",
    [WDB_SELECT_REG_OFFSET] = "global select-reg-offset %d",
    [WDB_FIND_GROUP] = "global find-group %s",
    [WDB_SELECT_GROUPS] = "global select-groups",
    [WDB_DELETE_AGENT] = "global delete-agent %d",
    [WDB_DELETE_GROUP] = "global delete-group %s",
    [WDB_DELETE_AGENT_BELONG] = "global delete-agent-belong %d",
    [WDB_DELETE_GROUP_BELONG] = "global delete-group-belong %s"
};

int wdb_insert_agent(int id, const char *name, const char *ip, const char *register_ip, const char *internal_key, const char *group, int keep_date) {
    int result = 0;
    time_t date_add = 0;
    cJSON *data_in = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    if(keep_date) {
        date_add = get_agent_date_added(id);
    } else {
        time(&date_add);
    }

    data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddNumberToObject(data_in, "id", id);
    cJSON_AddStringToObject(data_in, "name", name);
    cJSON_AddStringToObject(data_in, "ip", ip);
    cJSON_AddStringToObject(data_in, "register_ip", register_ip);
    cJSON_AddStringToObject(data_in, "internal_key", internal_key);
    cJSON_AddStringToObject(data_in, "group", group);
    cJSON_AddNumberToObject(data_in, "date_add", date_add);

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_INSERT_AGENT], cJSON_PrintUnformatted(data_in));

    cJSON_Delete(data_in);

    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK == wdbc_parse_result(wdboutput, &payload)) {
                result = wdb_create_agent_db(id, name);
            }
            else {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            break;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            result = OS_INVALID;
    }

    return result;
}

int wdb_insert_group(const char *name) {
    int result = 0;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_INSERT_AGENT_GROUP], name);
    result = wdbc_query_ex( &wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
    }

    return result;
}

int wdb_update_agent_belongs(int id_group, int id_agent) {
    int result = 0;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    cJSON *data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddNumberToObject(data_in, "id_group", id_group);
    cJSON_AddNumberToObject(data_in, "id_agent", id_agent);

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_INSERT_AGENT_BELONG], cJSON_PrintUnformatted(data_in));

    cJSON_Delete(data_in);

    result = wdbc_query_ex( &wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
    }

    return result;
}

int wdb_update_agent_name(int id, const char *name) {
    int result = 0;
    cJSON *data_in = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddNumberToObject(data_in, "id", id);
    cJSON_AddStringToObject(data_in, "name", name);

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_UPDATE_AGENT_NAME], cJSON_PrintUnformatted(data_in));

    cJSON_Delete(data_in);

    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            break;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            result = OS_INVALID;
    }

    return result;
}

int wdb_update_agent_version (int id,
                              const char *os_name,
                              const char *os_version,
                              const char *os_major,
                              const char *os_minor,
                              const char *os_codename,
                              const char *os_platform,
                              const char *os_build,
                              const char *os_uname,
                              const char *os_arch,
                              const char *version,
                              const char *config_sum,
                              const char *merged_sum,
                              const char *manager_host,
                              const char *node_name,
                              const char *agent_ip,
                              wdb_sync_status_t sync_status) {
    int result = 0;
    cJSON *data_in = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddNumberToObject(data_in, "id", id);
    cJSON_AddStringToObject(data_in, "os_name", os_name);
    cJSON_AddStringToObject(data_in, "os_version", os_version);
    cJSON_AddStringToObject(data_in, "os_major", os_major);
    cJSON_AddStringToObject(data_in, "os_minor", os_minor);
    cJSON_AddStringToObject(data_in, "os_codename", os_codename);
    cJSON_AddStringToObject(data_in, "os_platform", os_platform);
    cJSON_AddStringToObject(data_in, "os_build", os_build);
    cJSON_AddStringToObject(data_in, "os_uname", os_uname);
    cJSON_AddStringToObject(data_in, "os_arch", os_arch);
    cJSON_AddStringToObject(data_in, "version", version);
    cJSON_AddStringToObject(data_in, "config_sum", config_sum);
    cJSON_AddStringToObject(data_in, "merged_sum", merged_sum);
    cJSON_AddStringToObject(data_in, "manager_host", manager_host);
    cJSON_AddStringToObject(data_in, "node_name", node_name);
    cJSON_AddStringToObject(data_in, "agent_ip", agent_ip);
    cJSON_AddNumberToObject(data_in, "sync_status", sync_status);

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_UPDATE_AGENT_VERSION], cJSON_PrintUnformatted(data_in));

    cJSON_Delete(data_in);

    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            break;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            result = OS_INVALID;
    }

    return result;
}

int wdb_update_agent_keepalive(int id, wdb_sync_status_t sync_status) {
    int result = 0;
    cJSON *data_in = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddNumberToObject(data_in, "id", id);
    cJSON_AddNumberToObject(data_in, "sync_status", sync_status);

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_UPDATE_AGENT_KEEPALIVE], cJSON_PrintUnformatted(data_in));

    cJSON_Delete(data_in);

    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            break;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            result = OS_INVALID;
    }

    return result;
}

int wdb_set_agent_status(int id_agent, int status) {
    int result = 0;
    const char *str_status = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;
    cJSON *data_in = NULL;

    switch (status) {
    case WDB_AGENT_EMPTY:
        str_status = "empty";
        break;
    case WDB_AGENT_PENDING:
        str_status = "pending";
        break;
    case WDB_AGENT_UPDATED:
        str_status = "updated";
        break;
    default:
        return OS_INVALID;
    }

    data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddNumberToObject(data_in, "id", id_agent);
    cJSON_AddStringToObject(data_in, "status", str_status);

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_UPDATE_AGENT_STATUS], cJSON_PrintUnformatted(data_in));

    cJSON_Delete(data_in);

    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
    }

    return result;
}

int wdb_update_agent_group(int id, char *group) {
    int result = 0;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    cJSON *data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddNumberToObject(data_in, "id", id);
    cJSON_AddStringToObject(data_in, "group", group);

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_UPDATE_AGENT_GROUP], cJSON_PrintUnformatted(data_in));

    cJSON_Delete(data_in);

    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            else if (wdb_update_agent_multi_group(id,group) < 0) {
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
    }

    return result;
}

int wdb_set_agent_offset(int id, int type, long offset) {
    int result = 0;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    cJSON *data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddNumberToObject(data_in, "id", id);
    cJSON_AddNumberToObject(data_in, "offset", offset);

    switch (type) {
    case WDB_SYSCHECK:
        snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_UPDATE_FIM_OFFSET], cJSON_PrintUnformatted(data_in));
        break;
    case WDB_SYSCHECK_REGISTRY:
        snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_UPDATE_REG_OFFSET], cJSON_PrintUnformatted(data_in));
        break;
    default:
        cJSON_Delete(data_in);
        return OS_INVALID;
    }

    cJSON_Delete(data_in);

    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
    }

    return result;
}

int wdb_set_agent_labels(int id, const char *labels) {
    int result = 0;
    // Making use of a big buffer for the query because it
    // will contain all the keys and values.
    // The output will be just a JSON OK.
    char wdbquery[OS_MAXSTR] = "";
    char wdboutput[OS_BUFFER_SIZE] = "";
    char *payload = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_SET_AGENT_LABELS], id, labels);

    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result){
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            break;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            result = OS_INVALID;
    }

    return result;
}

int* wdb_get_all_agents(bool include_manager) {
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    int last_id = include_manager ? -1 : 0;
    int *array = NULL;
    int len = 0;
    wdbc_result status = WDBC_DUE;
    
    while (status == WDBC_DUE) {
        // Query WazuhDB
        snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_GET_ALL_AGENTS], last_id);
        if (wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput)) == 0) {
            // Parse result
            char* payload = NULL;
            status = wdbc_parse_result(wdboutput, &payload);
            if (status == WDBC_OK || status == WDBC_DUE) {
                const char delim = ','; 
                const char sdelim[] = { delim, '\0' };
                //Realloc new size
                int new_len = os_strcnt(payload, delim)+1;
                os_realloc(array, sizeof(int)*(len+new_len+1), array);
                //Append IDs to array
                char* agent_id = NULL;
                char *savedptr = NULL;
                for (agent_id = strtok_r(payload, sdelim, &savedptr); agent_id; agent_id = strtok_r(NULL, sdelim, &savedptr)) {
                    array[len] = atoi(agent_id);
                    len++;
                }
                last_id = array[len-1];
            }
        }
        else {
            status = WDBC_ERROR;
        }
    }
    if (status == WDBC_OK) {
        array[len] = -1;
    }
    else {
        os_free(array);
    }

    return array;
}

int* wdb_get_agents_by_keepalive(const char* condition, int keepalive, bool include_manager) {
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    int last_id = include_manager ? -1 : 0;
    int *array = NULL;
    int len = 0;
    wdbc_result status = WDBC_DUE;

    while (status == WDBC_DUE) {
        // Query WazuhDB
        snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_GET_AGENTS_BY_KEEPALIVE], condition, keepalive, last_id);
        if (wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput)) == 0) {
            // Parse result
            char* payload = NULL;
            status = wdbc_parse_result(wdboutput, &payload);
            if (status == WDBC_OK || status == WDBC_DUE) {
                const char delim = ','; 
                const char sdelim[] = { delim, '\0' };
                //Realloc new size
                int new_len = os_strcnt(payload, delim)+1;
                os_realloc(array, sizeof(int)*(len+new_len+1), array);
                //Append IDs to array
                char* agent_id = NULL;
                char *savedptr = NULL;
                for (agent_id = strtok_r(payload, sdelim, &savedptr); agent_id; agent_id = strtok_r(NULL, sdelim, &savedptr)) {
                    array[len] = atoi(agent_id);
                    len++;
                }
                last_id = array[len-1];
            }
        }
        else {
            status = WDBC_ERROR;
        }
    }

    if (status == WDBC_OK) {
        array[len] = -1;
    }
    else {
        os_free(array);
    }

    return array;
}

int wdb_find_agent(const char *name, const char *ip) {
    int output = OS_INVALID;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    cJSON *data_in = NULL;
    cJSON *root = NULL;
    cJSON *json_id = NULL;

    if (!name || !ip) {
        mdebug1("Empty agent name or ip when trying to get agent name. Agent: (%s) IP: (%s)", name, ip);
        return OS_INVALID;
    }

    data_in = cJSON_CreateObject();

    if (!data_in) {
        mdebug1("Error creating data JSON for Wazuh DB.");
        return OS_INVALID;
    }

    cJSON_AddStringToObject(data_in, "name", name);
    cJSON_AddStringToObject(data_in, "ip", ip);

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_FIND_AGENT], cJSON_PrintUnformatted(data_in));

    cJSON_Delete(data_in);

    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB for agent name.");
        return OS_INVALID;
    }

    json_id = cJSON_GetObjectItem(root->child,"id");
    if (cJSON_IsNumber(json_id)) {
        output = json_id->valueint;
    }

    cJSON_Delete(root);
    return output;
}

cJSON* wdb_get_agent_info(int id) {
    cJSON *root = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";

    sqlite3_snprintf(sizeof(wdbquery), wdbquery, global_db_commands[WDB_GET_AGENT_INFO], id);
    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB to get the agent's %d information.", id);
        return NULL;
    }

    return root;
}

cJSON* wdb_get_agent_labels(int id) {
    cJSON *root = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_GET_AGENT_LABELS], id);
    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB to get the agent's %d labels.", id);
        return NULL;
    }

    return root;
}

char* wdb_get_agent_name(int id) {
    char *output = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    cJSON *root = NULL;
    cJSON *json_name = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_SELECT_AGENT_NAME], id);
    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB to get the agent's %d name.", id);
        return NULL;
    }

    json_name = cJSON_GetObjectItem(root->child,"name");
    if (cJSON_IsString(json_name) && json_name->valuestring != NULL) {
        os_strdup(json_name->valuestring, output);
    }

    cJSON_Delete(root);
    return output;
}

char* wdb_get_agent_group(int id) {
    char *output = NULL;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    cJSON *root = NULL;
    cJSON *json_group = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_SELECT_AGENT_GROUP], id);
    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB to get the agent's %d group.", id);
        return NULL;
    }

    json_group = cJSON_GetObjectItem(root->child,"group");
    if (cJSON_IsString(json_group) && json_group->valuestring != NULL) {
        os_strdup(json_group->valuestring, output);
    }

    cJSON_Delete(root);
    return output;
}

int wdb_get_agent_status(int id_agent) {
    int output = -1;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    cJSON *root = NULL;
    cJSON *json_status = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_SELECT_AGENT_STATUS], id_agent);
    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB to get the agent status.");
        return OS_INVALID;
    }

    json_status = cJSON_GetObjectItem(root->child,"status");
    if (cJSON_IsString(json_status) && json_status->valuestring != NULL) {
        output = !strcmp(json_status->valuestring, "empty") ? WDB_AGENT_EMPTY : !strcmp(json_status->valuestring, "pending") ? WDB_AGENT_PENDING : WDB_AGENT_UPDATED;
    } else {
        output = OS_INVALID;
    }

    cJSON_Delete(root);
    return output;
}

time_t wdb_get_agent_keepalive(const char *name, const char *ip){
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    time_t output = 0;
    cJSON *root = NULL;
    cJSON *json_keepalive = NULL;

    if (!name || !ip) {
        mdebug1("Empty agent name or ip when trying to get last keepalive. Agent: (%s) IP: (%s)", name, ip);
        return OS_INVALID;
    }

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_SELECT_KEEPALIVE], name, ip);
    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB to get the last agent keepalive.");
        return OS_INVALID;
    }

    json_keepalive = cJSON_GetObjectItem(root->child,"last_keepalive");
    output = cJSON_IsNumber(json_keepalive) ? json_keepalive->valueint : 0;

    cJSON_Delete(root);
    return output;
}

long wdb_get_agent_offset(int id, int type) {
    long int output = 0;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    cJSON *root = NULL;
    cJSON *json_offset = NULL;
    char * column = NULL;

    switch (type) {
    case WDB_SYSCHECK:
        snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_SELECT_FIM_OFFSET], id);
        column = "fim_offset";
        break;
    case WDB_SYSCHECK_REGISTRY:
        snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_SELECT_REG_OFFSET],id);
        column = "reg_offset";
        break;
    default:
        return OS_INVALID;
    }

    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));
    if (!root) {
        merror("Error querying Wazuh DB to get agent offset.");
        return OS_INVALID;
    }

    json_offset = cJSON_GetObjectItem(root->child,column);
    output = cJSON_IsNumber(json_offset) ? json_offset->valueint : OS_INVALID;

    cJSON_Delete(root);
    return output;
}

int wdb_find_group(const char *name) {
    int output = OS_INVALID;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    cJSON *root = NULL;
    cJSON *json_group = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_FIND_GROUP], name);
    root = wdbc_query_parse_json(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB to get the agent group id.");
        return OS_INVALID;
    }

    json_group = cJSON_GetObjectItem(root->child,"id");
    output = cJSON_IsNumber(json_group) ? json_group->valueint : OS_INVALID;

    cJSON_Delete(root);
    return output;
}

int wdb_update_groups(const char *dirname) {
    int result = OS_SUCCESS;
    int n = 0;
    int i = 0;
    char **array = NULL;
    cJSON *json_name = NULL;
    cJSON *item = NULL;
    cJSON *root = NULL;
    char wdboutput[WDBOUTPUT_SIZE] = "";

    root = wdbc_query_parse_json(&wdb_sock_agent, global_db_commands[WDB_SELECT_GROUPS], wdboutput, sizeof(wdboutput));

    if (!root) {
        merror("Error querying Wazuh DB to update groups.");
        return OS_INVALID;
    }

    item = root->child;

    while (item)
    {
        json_name = cJSON_GetObjectItem(item,"name");

        if(cJSON_IsString(json_name) && json_name->valuestring != NULL ){
            os_realloc(array, (n + 2) * sizeof(char *), array);
            os_strdup(json_name->valuestring, array[n]);
            array[++n] = NULL;
        }

        item=item->next;
    }

    cJSON_Delete(root);

    for (i=0; array[i]; i++) {
        /* Check if the group exists in dir */
        char group_path[PATH_MAX + 1] = {0};
        DIR *dp;

        if (snprintf(group_path, PATH_MAX + 1, "%s/%s", dirname,array[i]) > PATH_MAX) {
            merror("At wdb_update_groups(): path too long.");
            continue;
        }

        dp = opendir(group_path);

        /* Group doesnt exists anymore, delete it */
        if (!dp) {
            if (wdb_remove_group_db((char *)array[i]) < 0) {
                free_strarray(array);
                return OS_INVALID;
            }
        } else {
            closedir(dp);
        }
    }

    free_strarray(array);

    /* Add new groups from the folder /etc/shared if they dont exists on database */
    DIR *dir;
    struct dirent *dirent = NULL;

    if (!(dir = opendir(dirname))) {
        merror("Couldn't open directory '%s': %s.", dirname, strerror(errno));
        return OS_INVALID;
    }

    while ((dirent = readdir(dir))) {
        if (dirent->d_name[0] != '.') {
            char path[PATH_MAX];
            snprintf(path,PATH_MAX,"%s/%s",dirname,dirent->d_name);

            if (!IsDir(path)) {
                if (wdb_find_group(dirent->d_name) <= 0){
                    wdb_insert_group(dirent->d_name);
                }
            }
        }
    }
    closedir(dir);

    return result;
}

int wdb_remove_agent(int id) {
    int result = 0 ;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;
    char *name = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_DELETE_AGENT], id);
    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK == wdbc_parse_result(wdboutput, &payload)) {
                result = wdb_delete_agent_belongs(id);
                name = wdb_get_agent_name(id);

                result = ((OS_SUCCESS == result) && name) ? wdb_remove_agent_db(id, name) : OS_INVALID;

                if(OS_INVALID == result){
                    mdebug1("Unable to remove agent DB: %d - %s", id, name);
                }
            }
            else {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            break;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            result = OS_INVALID;
    }

    os_free(name);
    return result;
}

int wdb_remove_group_db(const char *name) {
    int result = 0;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    if (OS_INVALID == wdb_remove_group_from_belongs_db(name)) {
        merror("At wdb_remove_group_from_belongs_db(): couldn't delete '%s' from 'belongs' table.", name);
        return OS_INVALID;
    }

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_DELETE_GROUP], name);
    result = wdbc_query_ex( &wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
    }

    return result;
}

int wdb_delete_agent_belongs(int id) {
    int result = 0;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_DELETE_AGENT_BELONG], id);
    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
    }

    return result;
}

int wdb_remove_group_from_belongs_db(const char *name) {
    int result = 0;
    char wdbquery[WDBQUERY_SIZE] = "";
    char wdboutput[WDBOUTPUT_SIZE] = "";
    char *payload = NULL;

    snprintf(wdbquery, sizeof(wdbquery), global_db_commands[WDB_DELETE_GROUP_BELONG], name);
    result = wdbc_query_ex(&wdb_sock_agent, wdbquery, wdboutput, sizeof(wdboutput));

    switch (result) {
        case OS_SUCCESS:
            if (WDBC_OK != wdbc_parse_result(wdboutput, &payload)) {
                mdebug1("Global DB Error reported in the result of the query");
                result = OS_INVALID;
            }
            break;
        case OS_INVALID:
            mdebug1("Global DB Error in the response from socket");
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
        default:
            mdebug1("Global DB Cannot execute SQL query; err database %s/%s.db", WDB2_DIR, WDB2_GLOB_NAME);
            mdebug2("Global DB SQL query: %s", wdbquery);
            return OS_INVALID;
    }

    return result;
}

int wdb_create_agent_db(int id, const char *name) {
    const char *ROOT = "root";
    char path[OS_FLSIZE + 1];
    char buffer[4096];
    FILE *source;
    FILE *dest;
    size_t nbytes;
    int result = 0;
    uid_t uid;
    gid_t gid;

    if (!name)
        return -1;

    snprintf(path, OS_FLSIZE, "%s/%s", WDB_DIR, WDB_PROF_NAME);

    if (!(source = fopen(path, "r"))) {
        mdebug1("Profile database not found, creating.");

        if (wdb_create_profile(path) < 0)
            return -1;

        // Retry to open

        if (!(source = fopen(path, "r"))) {
            merror("Couldn't open profile '%s'.", path);
            return -1;
        }
    }

    snprintf(path, OS_FLSIZE, "%s%s/agents/%03d-%s.db", isChroot() ? "/" : "", WDB_DIR, id, name);

    if (!(dest = fopen(path, "w"))) {
        fclose(source);
        merror("Couldn't create database '%s'.", path);
        return -1;
    }

    while (nbytes = fread(buffer, 1, 4096, source), nbytes) {
        if (fwrite(buffer, 1, nbytes, dest) != nbytes) {
            result = -1;
            break;
        }
    }

    fclose(source);
    if (fclose(dest) == -1 || result < 0) {
        merror("Couldn't write/close file '%s' completely.", path);
        return -1;
    }

    uid = Privsep_GetUser(ROOT);
    gid = Privsep_GetGroup(GROUPGLOBAL);

    if (uid == (uid_t) - 1 || gid == (gid_t) - 1) {
        merror(USER_ERROR, ROOT, GROUPGLOBAL, strerror(errno), errno);
        return -1;
    }

    if (chown(path, uid, gid) < 0) {
        merror(CHOWN_ERROR, path, errno, strerror(errno));
        return -1;
    }

    if (chmod(path, 0660) < 0) {
        merror(CHMOD_ERROR, path, errno, strerror(errno));
        return -1;
    }

    return 0;
}

int wdb_remove_agent_db(int id, const char * name) {
    char path[PATH_MAX];
    char path_aux[PATH_MAX];

    snprintf(path, PATH_MAX, "%s%s/agents/%03d-%s.db", isChroot() ? "/" : "", WDB_DIR, id, name);

    if (!remove(path)) {
        snprintf(path_aux, PATH_MAX, "%s%s/agents/%03d-%s.db-shm", isChroot() ? "/" : "", WDB_DIR, id, name);
        if (remove(path_aux) < 0) {
            mdebug2(DELETE_ERROR, path_aux, errno, strerror(errno));
        }
        snprintf(path_aux, PATH_MAX, "%s%s/agents/%03d-%s.db-wal", isChroot() ? "/" : "", WDB_DIR, id, name);
        if (remove(path_aux) < 0) {
            mdebug2(DELETE_ERROR, path_aux, errno, strerror(errno));
        }
        return OS_SUCCESS;
    } else
        return OS_INVALID;
}

int wdb_update_agent_multi_group(int id, char *group) {
    /* Wipe out the agent multi groups relation for this agent */
    if (wdb_delete_agent_belongs(id) < 0) {
        return OS_INVALID;
    }

    /* Update the belongs table if multi group */
    const char delim[2] = ",";

    if (group) {
        char *multi_group;
        char *save_ptr = NULL;

        multi_group = strchr(group, MULTIGROUP_SEPARATOR);

        if (multi_group) {
            /* Get the first group */
            multi_group = strtok_r(group, delim, &save_ptr);

            while (multi_group != NULL) {
                /* Update de groups table */
                int id_group = wdb_find_group(multi_group);

                if(id_group <= 0 && OS_SUCCESS == wdb_insert_group(multi_group)) {
                    id_group = wdb_find_group(multi_group);
                }

                if (OS_SUCCESS != wdb_update_agent_belongs(id_group,id)) {
                    return OS_INVALID;
                }

                multi_group = strtok_r(NULL, delim, &save_ptr);
            }
        } else {
            /* Update de groups table */
            int id_group = wdb_find_group(group);

            if (id_group <= 0 && OS_SUCCESS == wdb_insert_group(group)) {
                id_group = wdb_find_group(group);
            }

            if (OS_SUCCESS != wdb_update_agent_belongs(id_group,id)) {
                return OS_INVALID;
            }
        }
    }

    return OS_SUCCESS;
}

int wdb_agent_belongs_first_time(){
    int i;
    char *group;
    int *agents;

    if ((agents = wdb_get_all_agents(FALSE))) {

        for (i = 0; agents[i] != -1; i++) {
            group = wdb_get_agent_group(agents[i]);

            if (group) {
                wdb_update_agent_multi_group(agents[i],group);
                os_free(group);
            }
        }
        os_free(agents);
    }

    return OS_SUCCESS;
}

time_t get_agent_date_added(int agent_id) {
    char path[PATH_MAX + 1] = {0};
    char line[OS_BUFFER_SIZE] = {0};
    char * sep;
    FILE *fp;
    struct tm t;
    time_t t_of_sec;

    snprintf(path, PATH_MAX, "%s", isChroot() ? TIMESTAMP_FILE : DEFAULTDIR TIMESTAMP_FILE);

    fp = fopen(path, "r");

    if (!fp) {
        return 0;
    }

    while (fgets(line, OS_BUFFER_SIZE, fp)) {
        if (sep = strchr(line, ' '), sep) {
            *sep = '\0';
        } else {
            continue;
        }

        if(atoi(line) == agent_id){
            /* Extract date */
            char **data;
            char * date = NULL;
            *sep = ' ';

            data = OS_StrBreak(' ', line, 5);

            if(data == NULL) {
                fclose(fp);
                return 0;
            }

            /* Date is 3 and 4 */
            wm_strcat(&date,data[3], ' ');
            wm_strcat(&date,data[4], ' ');

            if(date == NULL) {
                fclose(fp);
                free_strarray(data);
                return 0;
            }

            char *endl = strchr(date, '\n');

            if (endl) {
                *endl = '\0';
            }

            if (sscanf(date, "%d-%d-%d %d:%d:%d",&t.tm_year, &t.tm_mon, &t.tm_mday, &t.tm_hour, &t.tm_min, &t.tm_sec) < 6) {
                merror("Invalid date format in file '%s' for agent '%d'", TIMESTAMP_FILE, agent_id);
                free(date);
                free_strarray(data);
                fclose(fp);
                return 0;
            }
            t.tm_year -= 1900;
            t.tm_mon -= 1;
            t.tm_isdst = 0;
            t_of_sec = mktime(&t);

            free(date);
            fclose(fp);
            free_strarray(data);

            return t_of_sec;
        }
    }

    fclose(fp);
    return 0;
}
