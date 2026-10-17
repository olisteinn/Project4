#pragma once
#include <linux/can.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <cstdint>
#include <cstring>
#include <string>
#include <unistd.h>


class canbus_obj {
    private:
        int socket_internal;
        void flush_rx();                                              // drop old frames
        bool wait_for(canid_t id, struct can_frame &msg, std::uint16_t index = 0);
    public:
        canbus_obj(std::string bus);
        ~canbus_obj();
        void NMT(char selection);
        std::string SDO(std::uint16_t address, float value_arg);     // write, returns the reply
        std::string SDO_read(std::uint16_t address);                 // read, returns the value
        void PDO(float value_arg);
        std::string TPDO_read(void);                                  // next TPDO1 frame
        std::string CAN_read(void);
};