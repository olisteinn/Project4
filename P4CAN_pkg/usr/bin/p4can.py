#!/usr/bin/env python3
import can

def nmt(selection):
    cmd_list = {'reset': 0x81,'op': 0x01, 'stop': 0x02, 'preop': 0x80, 'resetcomms': 0x82}
    command = cmd_list[selection]
    can_msg = can.Me