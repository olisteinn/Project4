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
    public:
        canbus_obj(std::string bus);
        ~canbus_obj();
        void NMT(char selection);
        void SDO(std::uint16_t address, float value_arg);
        void PDO(float value_arg);
};