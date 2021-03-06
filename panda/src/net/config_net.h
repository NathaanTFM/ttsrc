// Filename: config_net.h
// Created by:  drose (25Feb00)
//
////////////////////////////////////////////////////////////////////
//
// PANDA 3D SOFTWARE
// Copyright (c) Carnegie Mellon University.  All rights reserved.
//
// All use of this software is subject to the terms of the revised BSD
// license.  You should have received a copy of this license along
// with this source code in a file named "LICENSE."
//
////////////////////////////////////////////////////////////////////

#ifndef CONFIG_NET_H
#define CONFIG_NET_H

#include "pandabase.h"
#include "notifyCategoryProxy.h"
#include "configVariableInt.h"
#include "configVariableBool.h"
#include "configVariableEnum.h"
#include "threadPriority.h"

// Configure variables for net package.

NotifyCategoryDecl(net, EXPCL_PANDA_NET, EXPTP_PANDA_NET);

extern int get_net_max_write_queue();
extern int get_net_max_response_queue();
extern bool get_net_error_abort();
extern double get_max_poll_cycle();
extern string make_thread_name(const string &thread_name, int thread_index);

extern ConfigVariableInt net_max_read_per_epoch;
extern ConfigVariableInt net_max_write_per_epoch;
extern ConfigVariableEnum<ThreadPriority> net_thread_priority;

extern EXPCL_PANDA_NET void init_libnet();

#endif

