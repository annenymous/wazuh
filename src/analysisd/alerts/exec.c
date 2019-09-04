/* Copyright (C) 2015-2019, Wazuh Inc.
 * Copyright (C) 2009 Trend Micro Inc.
 * All right reserved.
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General Public
 * License (version 2) as published by the FSF - Free Software
 * Foundation
 */

#include "shared.h"
#include "rules.h"
#include "alerts.h"
#include "config.h"
#include "active-response.h"
#include "os_net/os_net.h"
#include "os_regex/os_regex.h"
#include "os_execd/execd.h"
#include "eventinfo.h"


void OS_Exec(int execq, int arq, const Eventinfo *lf, const active_response *ar)
{
    char exec_msg[OS_SIZE_1024 + 1];
    const char *ip;
    const char *user;
    char *filename = NULL;
    char *extra_args = NULL;

    ip = user = "-";

    /* Clean the IP */
    if (lf->srcip && (ar->ar_cmd->expect & SRCIP)) {
        if (strncmp(lf->srcip, "::ffff:", 7) == 0) {
            ip = lf->srcip + 7;
        } else {
            ip = lf->srcip;
        }

        /* Check if IP is to be ignored */
        if (Config.white_list) {
            if (OS_IPFoundList(ip, Config.white_list)) {
                return;
            }
        }

        /* Check if it is a hostname */
        if (Config.hostname_white_list) {
            size_t srcip_size;
            OSMatch **wl;

            srcip_size = strlen(ip);

            wl = Config.hostname_white_list;
            while (*wl) {
                if (OSMatch_Execute(ip, srcip_size, *wl)) {
                    return;
                }
                wl++;
            }
        }
    }

    /* Get username */
    if (lf->dstuser && (ar->ar_cmd->expect & USERNAME)) {
        user = lf->dstuser;
    }

    /* Get filename */
    if (lf->filename && (ar->ar_cmd->expect & FILENAME)) {
        filename = os_shell_escape(lf->filename);
    }

    /* Get extra_args */
    if (ar->ar_cmd->extra_args) {
        extra_args = os_shell_escape(ar->ar_cmd->extra_args);
    }

    /* Active Response on the server
     * The response must be here if the ar->location is set to AS
     * or the ar->location is set to local (REMOTE_AGENT) and the
     * event location is from here.
     */
    if ((ar->location & AS_ONLY) ||
            ((ar->location & REMOTE_AGENT) && (lf->location[0] != '(')) ) {
        if (!(Config.ar & LOCAL_AR)) {
            goto cleanup;
        }

        snprintf(exec_msg, OS_SIZE_1024,
                 "%s %s %s %ld.%ld %d %s %s %s",
                 ar->name,
                 user,
                 ip,
                 (long int)lf->time.tv_sec,
                 __crt_ftell,
                 lf->generated_rule->sigid,
                 lf->location,
                 filename ? filename : "-",
                 extra_args ? extra_args : "-");

        if (OS_SendUnix(execq, exec_msg, 0) < 0) {
            merror("Error communicating with execd.");
        }
    }

    /* Active Response to the forwarder */
    else if ((Config.ar & REMOTE_AR)) {
        int rc;
        /* If lf->location start with a ( was generated by remote agent and its
         * ID is included in lf->location if missing then it must have been
         * generated by the local analysisd, so prepend a false id tag */
        if (lf->location[0] == '(') {
            snprintf(exec_msg, OS_SIZE_1024,
                     "%s %c%c%c %s %s %s %s %ld.%ld %d %s %s %s",
                     lf->location,
                     (ar->location & ALL_AGENTS) ? ALL_AGENTS_C : NONE_C,
                     (ar->location & REMOTE_AGENT) ? REMOTE_AGENT_C : NONE_C,
                     (ar->location & SPECIFIC_AGENT) ? SPECIFIC_AGENT_C : NONE_C,
                     ar->agent_id != NULL ? ar->agent_id : "(null)",
                     ar->name,
                     user,
                     ip,
                     (long int)lf->time.tv_sec,
                     __crt_ftell,
                     lf->generated_rule->sigid,
                     lf->location,
                     filename ? filename : "-",
                     extra_args ? extra_args : "-");
        } else {
            snprintf(exec_msg, OS_SIZE_1024,
                     "(local_source) %s %c%c%c %s %s %s %s %ld.%ld %d %s %s %s",
                     lf->location,
                     (ar->location & ALL_AGENTS) ? ALL_AGENTS_C : NONE_C,
                     (ar->location & REMOTE_AGENT) ? REMOTE_AGENT_C : NONE_C,
                     (ar->location & SPECIFIC_AGENT) ? SPECIFIC_AGENT_C : NONE_C,
                     ar->agent_id != NULL ? ar->agent_id : "(null)",
                     ar->name,
                     user,
                     ip,
                     (long int)lf->time.tv_sec,
                     __crt_ftell,
                     lf->generated_rule->sigid,
                     lf->location,
                     filename ? filename : "-",
                     extra_args ? extra_args : "-");
        }

        if ((rc = OS_SendUnix(arq, exec_msg, 0)) < 0) {
            if (rc == OS_SOCKBUSY) {
                merror("AR socket busy.");
            } else {
                merror("AR socket error (shutdown?).");
            }
            merror("Error communicating with ar queue (%d).", rc);
        }
    }

    cleanup:

    /* Clean up Memory */
    free(filename);
    free(extra_args);

    return;
}
