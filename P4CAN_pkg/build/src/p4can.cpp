// #include <net/if.h>
// #include <unistd.h>
#include <cstdint>
// #include <cstring>
// #include <csignal>
// #include <fstream>
// #include <iostream>
// #include <vector>
// #include <chrono>
#include <CLI/CLI.hpp>
#include "can_class.h"



int main(int argc, char** argv) {
    CLI::App app{"CAN handler fyrir project 4 - Embedded system programming"};
    char state_selector;
    float parms_value;
    std::uint16_t parms_address;
    float speed_value;
    std::string bus_name;

    app.add_option("bus",bus_name,"CAN bus name, i.e. ""can0""")->required();
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
    
    CLI11_PARSE(app, argc, argv);

    canbus_obj active_bus(bus_name);

    if (*state_ptr) {
        active_bus.NMT(state_selector);
    } else if (*parms_ptr) {
        if (*kp_ptr) {
            parms_address = 0x2001;
        } else if (*ti_ptr) {
            parms_address = 0x2002;
        }
        active_bus.SDO(parms_address,parms_value);
    } else if (*speed_ptr) {
        active_bus.PDO(speed_value);
    }

    
    return 0;
}
