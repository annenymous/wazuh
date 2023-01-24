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
#ifndef _MANAGEKEYS_TEST_H
#define _MANAGEKEYS_TEST_H

#include "gtest/gtest.h"
#include "gmock/gmock.h"

class ManageKeysTest : public ::testing::Test
{
    protected:
        ManageKeysTest() = default;
        virtual ~ManageKeysTest() = default;

        void SetUp() override;
        void TearDown() override;
};

#endif //_MANAGEKEYS_TEST_H