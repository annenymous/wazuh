/*
 * Wazuh SysInfo
 * Copyright (C) 2015, Wazuh Inc.
 * October 19, 2020.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation.
 */

#include <memory>

#include "AddAgentWrapper.h"

extern "C" {
    char shost[512];

    std::shared_ptr<AddAgentWrapper> pAddAgentWrapper;

    const char * __wrap_print_agent_status(agent_status_t status)
    {
        return pAddAgentWrapper->print_agent_status(status);
    }

    int __wrap_wdbc_close(int* sock)
    {
        return pAddAgentWrapper->wdbc_close(sock);
    }

    void __wrap_OS_RemoveCounter(const char *id)
    {
        pAddAgentWrapper->OS_RemoveCounter(id);
    }

    cJSON* __wrap_wdb_get_agent_info(int id, int *sock)
    {
        return pAddAgentWrapper->wdb_get_agent_info(id, sock);
    }

    int __wrap_auth_connect()
    {
        return pAddAgentWrapper->auth_connect();
    }

    agent_status_t __wrap_get_agent_status(int agent_id)
    {
        return pAddAgentWrapper->get_agent_status(agent_id);
    }

    int __wrap_w_request_agent_add_local(int sock, char *id, const char *name, const char *ip, const char *groups, const char *key, authd_force_options_t *force_options, const int json_format, const char *agent_id, int exit_on_error)
    {
        return pAddAgentWrapper->w_request_agent_add_local(sock, id, name, ip, groups, key, force_options, json_format, agent_id, exit_on_error);
    }

    int __wrap_auth_remove_agent(int sock, const char *id, int json_format)
    {
        return pAddAgentWrapper->auth_remove_agent(sock, id, json_format);
    }

    int __wrap_wdb_remove_agent(int id, int *sock)
    {
        return pAddAgentWrapper->wdb_remove_agent(id, sock);
    }

    char *__wrap_encode_base64(int size, const char *src)
    {
        return pAddAgentWrapper->encode_base64(size, src);
    }

    int __wrap_OS_AddKey(keystore *keys, const char *id, const char *name, const char *ip, const char *key, time_t time_added)
    {
        return pAddAgentWrapper->OS_AddKey(keys, id, name, ip, key, time_added);
    }
    
    int __wrap_wdbc_query_ex(int *sock, const char *query, char *response, const int len)
    {
        return pAddAgentWrapper->wdbc_query_ex(sock, query, response, len);
    }

    int __wrap_OS_MD5_Str(const char *str, ssize_t length, os_md5 output)
    {
        return pAddAgentWrapper->OS_MD5_Str(str, length, output);
    }

    char *__wrap_decode_base64(const char *src)
    {
        return pAddAgentWrapper->decode_base64(src);
    }

    int __wrap_os_random(void)
    {
        return pAddAgentWrapper->os_random();
    }

    int __wrap_auth_close(int sock)
    {
        return pAddAgentWrapper->auth_close(sock);
    }

    int __wrap_delete_agentinfo(const char *id, const char *name)
    {
        return pAddAgentWrapper->delete_agentinfo(id, name);
    }

    char * __wrap_read_from_user()
    {
        return pAddAgentWrapper->read_from_user();
    }

    int __wrap_mkstemp_ex(char *tmp_path)
    {
        int fd;
        mode_t old_mask = umask(0177);

        fd = mkstemp(tmp_path);
        umask(old_mask);

        if (fd == -1) {
            return (-1);
        }

        /* mkstemp() only implicitly does this in POSIX 2008 */
        if (fchmod(fd, 0600) == -1) {
            close(fd);
            unlink(tmp_path);
            return (-1);
        }

        close(fd);
        return (0);
    }

    int __wrap_rename_ex(const char *source, const char *destination)
    {
        (void)source;
        (void)destination;
        return (0);
    }
}
