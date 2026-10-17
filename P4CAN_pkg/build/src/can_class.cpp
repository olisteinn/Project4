#include "can_class.h"

canbus_obj::canbus_obj(std::string bus) {
    socket_internal = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    struct ifreq ifr;
    std::strcpy(ifr.ifr_name, bus.c_str());
    ioctl(socket_internal, SIOCGIFINDEX, &ifr);
    struct sockaddr_can addr{};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(socket_internal, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        perror("CAN socket bind failed");
    }
}

canbus_obj::~canbus_obj() {
    NMT('p'); // pre operational
    SDO(0x60FF,0); // target speed = 0
    close(socket_internal);
    struct timeval tv{};
    tv.tv_sec = 1; // timeout í sekúndum
    setsockopt(socket_internal, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));  
}

void canbus_obj::NMT(char selection) {
    struct can_frame msg{};
    msg.can_id  = 0x000;
    msg.can_dlc = 2;
    msg.data[1] = 0x01; // Node ID 1
    if (selection == 'o') {
        msg.data[0] = 0x01;
    } else if (selection == 's') {
        msg.data[0] = 0x02;
    } else if (selection == 'p') {
        msg.data[0] = 0x80;
    } else if (selection == 'r') {
        msg.data[0] = 0x81;
    } else if (selection == 'c') {
        msg.data[0] = 0x82;
    }
    write(socket_internal, &msg, sizeof(struct can_frame));
}

void canbus_obj::SDO(std::uint16_t address, float value_arg) {
    struct can_frame msg{};
    msg.can_id  = 0x601;
    msg.can_dlc = 8;
    std::memcpy(&msg.data[1], &address, 2);
    msg.data[3] = 0x00;
    if (address == 0x2000) {
        uint8_t value = value_arg;
        msg.data[0] = 0x2F;
        std::memcpy(&msg.data[4], &value, 1);
    } else if (address == 0x2001 || address == 0x2002) {
        msg.data[0] = 0x23;
        std::memcpy(&msg.data[4], &value_arg, 4);
    } else if (address == 0x6083 || address == 0x6084) {
        uint32_t value = value_arg;
        msg.data[0] = 0x23;
        std::memcpy(&msg.data[4], &value, 4);
    } else if (address == 0x60FF) {
        int32_t value = value_arg;
        msg.data[0] = 0x23;
        std::memcpy(&msg.data[4], &value, 4);
    }

    write(socket_internal, &msg, sizeof(struct can_frame));
}

void canbus_obj::PDO(float value_arg) {
    int32_t value = value_arg;
    struct can_frame msg{};
    msg.can_id  = 0x201;
    msg.can_dlc = 4;
    std::memcpy(&msg.data[0], &value, 4);

    write(socket_internal, &msg, sizeof(struct can_frame));
}

std::string canbus_obj::CAN_read(void) {
    std::string outp;
    struct can_frame msg{};
    if (read(socket_internal, &msg, sizeof(struct can_frame)) < 0) {
        outp = "Read timeout";
    }
    if (msg.can_id == 0x701) {
        outp = "Boot successful";
    } else if (msg.can_id == 0x581) { // SDO
        if (msg.data[0] == 0x60) {
            outp = "Write successful";
        } else if (msg.data[0] == 0x80) {
            uint32_t abort_code = 0;
            std::memcpy(&abort_code, &msg.data[4], 4);
            if (abort_code == 0x06020000) {
                outp = "Write failed, object does not exist";
            } else if (abort_code == 0x08000022) {
                outp = "Write failed due to current state";
            }
        }
    } else if (msg.can_id == 0x181) {
        int32_t true_speed = 0;
        int16_t dutycycle = 0;
        std::memcpy(&true_speed, &msg.data[0], 4);
        std::memcpy(&dutycycle, &msg.data[4], 2);
        outp = "Velocity: " + std::to_string(true_speed) + " RPM, Duty: " + std::to_string(dutycycle * 0.1f) + "%";
    }
    return outp;
}
