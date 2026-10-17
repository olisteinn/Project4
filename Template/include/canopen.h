// translate what the message means

#pragma once
#include "can_bus.h"


void canopen_handle(const CanMsg &msg);  // act on arrieved message
void canopen_process_pending_nmt();
void canopen_send_bootup();
void canopen_send_tpdo1();
