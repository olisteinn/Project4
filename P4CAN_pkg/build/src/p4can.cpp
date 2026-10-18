#include <cstdint>
#include <CLI/CLI.hpp>
#include "can_class.h"
#include <cstdlib>
#include <csignal>
#include <map>
#include <poll.h>

static const std::map<std::string, std::uint16_t> objects = { // get addressur
    {"device", 0x1000}, {"type",   0x2000}, {"kp",  0x2001}, {"ti",  0x2002},
    {"duty",   0x2010}, {"speed",  0x606C}, {"acc", 0x6083}, {"dec", 0x6084},
    {"target", 0x60FF},
};

void sigint_handler(int) { // fyrir ctrl+c
    std::cout << "\n";
    close(STDIN_FILENO);
}

int main(int argc, char** argv) {
    CLI::App app{"CAN handler fyrir project 4 - Embedded system programming"};
    char state_selector;
    float parms_value;
    std::uint16_t parms_address;
    float speed_value;
    std::string bus_name = (argc > 1) ? argv[1] : "can0";
    std::string bitrate = (argc > 2) ? argv[2] : "125000";

    //Parser:
    CLI::App* state_ptr = app.add_subcommand("state","Switch motor controller states through NMT");
        state_ptr->add_option("selected_state", state_selector,"p = pre-operational, o = operational, s = stopped, r = reset");
    CLI::App* parms_ptr = app.add_subcommand("parms","Modify setup parameters through SDO");
        CLI::App* addr_ptr = parms_ptr->add_subcommand("address","Send value to specified address");
            addr_ptr->add_option("hex_address",parms_address,"CAN address as hexadecimal");
                addr_ptr->add_option("custom_value",parms_value,"Set target parameter");
        CLI::App* kp_ptr = parms_ptr->add_subcommand("kp","Set Kp as float");
            kp_ptr->add_option("kp_value",parms_value,"Kp value");
        CLI::App* ti_ptr = parms_ptr->add_subcommand("ti","Set Ti as float");
            ti_ptr->add_option("ti_value",parms_value,"Kp value");
    CLI::App* speed_ptr = app.add_subcommand("speed","Set target speed through PDO");
        speed_ptr->add_option("target_speed",speed_value,"Target speed as float");
    std::string get_object;
    CLI::App* get_ptr = app.add_subcommand("get","Read a value from the node through SDO");
        get_ptr->add_option("object",get_object,"device, type, kp, ti, duty, speed, acc, dec, target, all, or a hex address");
    int listen_count = 0;
    CLI::App* listen_ptr = app.add_subcommand("listen","Print TPDO1 frames (velocity and duty) until Enter is pressed");
        listen_ptr->add_option("count",listen_count,"Stop after this many frames instead");

    std::string cmd = "sudo ip link set " + bus_name + " up type can bitrate " + bitrate + " 2>/dev/null";
    std::system(cmd.c_str());
    canbus_obj active_bus(bus_name);
    std::string response = active_bus.CAN_read();
    std::cout << response << "\n";
    
    std::string line;
    while (true) {
        std::signal(SIGINT, sigint_handler);
        std::cout << "[" << bus_name << "] "; 
        if (!std::getline(std::cin, line)) {
            break;
        }
        if (line.empty()) {
            continue;
        }
        if (line == "exit" || line == "quit") {
            break;
        }
        listen_count = 0;
        try {
            app.clear();
            app.parse(line, false);
        } catch (const CLI::ParseError &e) {
            app.exit(e);
            continue;
        }
        if (*state_ptr) {
            active_bus.NMT(state_selector);
        } else if (*parms_ptr) {
            if (*kp_ptr) {
                parms_address = 0x2001;
            } else if (*ti_ptr) {
                parms_address = 0x2002;
            }
            std::cout << active_bus.SDO(parms_address,parms_value) << "\n";
        } else if (*speed_ptr) {
            active_bus.PDO(speed_value);
        } else if (*get_ptr) {
            if (get_object == "all") {
                for (const auto &o : objects) {
                    std::cout << o.first << ": " << active_bus.SDO_read(o.second) << "\n";
                }
            } else if (objects.count(get_object)) {
                std::cout << active_bus.SDO_read(objects.at(get_object)) << "\n";
            } else {
                try {
                    std::uint16_t addr = std::stoul(get_object, nullptr, 0); // e.g. 0x2001
                    std::cout << active_bus.SDO_read(addr) << "\n";
                } catch (...) {
                    std::cout << "Unknown object: " << get_object << "\n";
                }
            }
        } else if (*listen_ptr) {
            if (listen_count <= 0) {
                std::cout << "Listening to TPDO1, press Enter to stop\n";
            }
            for (int i = 0; listen_count <= 0 || i < listen_count; i++) {
                std::string frame = active_bus.TPDO_read();
                std::cout << frame << "\n";
                if (frame.rfind("No TPDO1", 0) == 0) {
                    break;
                }
                struct pollfd in = {STDIN_FILENO, POLLIN, 0};
                if (poll(&in, 1, 0) > 0) {
                    if (in.revents & POLLIN) {
                        std::getline(std::cin, line);
                    }
                    break;
                }
            }
        }
    }
    return 0;
}
