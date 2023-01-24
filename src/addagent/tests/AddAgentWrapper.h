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
#ifndef _ADDAGENTWRAPPER_H
#define _ADDAGENTWRAPPER_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"

#include "headers/read-agents.h"
#include "headers/wazuhdb_op.h"
#include "os_crypto/md5/md5_op.h"


class AddAgentWrapper
{
    public:
        AddAgentWrapper() = default;
        ~AddAgentWrapper() = default;
        MOCK_METHOD(const char *, print_agent_status, (agent_status_t));
        MOCK_METHOD(int, wdbc_close, (int*));
        MOCK_METHOD(void, OS_RemoveCounter, (const char *));
        MOCK_METHOD(cJSON*, wdb_get_agent_info, (int, int *));
        MOCK_METHOD(int, auth_connect, ());
        MOCK_METHOD(agent_status_t, get_agent_status, (int));
        MOCK_METHOD(int, w_request_agent_add_local, (int, char *, const char *, const char *, const char *, const char *, authd_force_options_t *, const int, const char *, int));
        MOCK_METHOD(int, auth_remove_agent, (int, const char *, int));
        MOCK_METHOD(int, wdb_remove_agent, (int, int *));
        MOCK_METHOD(char *, encode_base64, (int, const char *));
        MOCK_METHOD(int, OS_AddKey, (keystore *, const char *, const char *, const char *, const char *, time_t));
        MOCK_METHOD(int, wdbc_query_ex, (int *, const char *, char *, const int));
        MOCK_METHOD(int, OS_MD5_Str, (const char *, ssize_t, os_md5));
        MOCK_METHOD(char *, decode_base64, (const char *));
        MOCK_METHOD(int, os_random, ());
        MOCK_METHOD(int, auth_close, (int));
        MOCK_METHOD(int, delete_agentinfo, (const char *, const char *));
        MOCK_METHOD(char *, read_from_user, ());
};

extern "C" {
    extern std::shared_ptr<AddAgentWrapper> pAddAgentWrapper;
}

#endif //_ADDAGENTWRAPPER_H