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

#include "ManageKeysTest.h"
#include "AddAgentWrapper.h"

extern "C" {
    #include "manage_agents.h"
}

using ::testing::Return;

void ManageKeysTest::SetUp()
{

}

void ManageKeysTest::TearDown() 
{
}

TEST_F(ManageKeysTest, k_import_successful)
{
    size_t bufSize = 256;
    char * keyString = (char *)malloc(bufSize);
    strncpy(keyString, "013 ubuntu22agent any 5f2202b62ed52ca67eb0dc2fdfd6f89f6ce09ef3c567596e3a1573136b763dbf", bufSize);

    putenv(const_cast<char*>("OSSEC_ACTION_CONFIRMED=y"));
    mkdir("tmp", 0775);

    pAddAgentWrapper = std::make_shared<AddAgentWrapper>();

    EXPECT_CALL(*pAddAgentWrapper, decode_base64)
        .WillOnce(Return(keyString));
    EXPECT_CALL(*pAddAgentWrapper, OS_RemoveCounter);

    int retValue = ::k_import("MDEzIHVidW50dTIyYWdlbnQgYW55IDVmMjIwMmI2MmVkNTJjYTY3ZWIwZGMyZmRmZDZmODlmNmNlMDllZjNjNTY3NTk2ZTNhMTU3MzEzNmI3NjNkYmY=");
    EXPECT_EQ(retValue, 1);

    pAddAgentWrapper.reset();
}

TEST_F(ManageKeysTest, k_import_keyinvalid)
{
    size_t bufSize = 256;
    char * keyString = (char *)malloc(bufSize);
    strncpy(keyString, "013-invalidformat-any 5f2202b62ed52ca67eb0dc2fdfd6f89f6ce09ef3c567596e3a1573136b763dbf", bufSize);

    putenv(const_cast<char*>("OSSEC_ACTION_CONFIRMED=y"));
    mkdir("tmp", 0775);

    pAddAgentWrapper = std::make_shared<AddAgentWrapper>();

    EXPECT_CALL(*pAddAgentWrapper, decode_base64)
        .WillOnce(Return(keyString));
    EXPECT_CALL(*pAddAgentWrapper, read_from_user)
        .WillOnce(Return(const_cast<char *>("y")));

    int retValue = ::k_import("MDEzLWludmFsaWRmb3JtYXQtYW55IDVmMjIwMmI2MmVkNTJjYTY3ZWIwZGMyZmRmZDZmODlmNmNlMDllZjNjNTY3NTk2ZTNhMTU3MzEzNmI3NjNkYmY=");
    EXPECT_EQ(retValue, 0);

    pAddAgentWrapper.reset();
}
