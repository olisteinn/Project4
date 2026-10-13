#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include "can_bus.h"

#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)
CAN_MSGQ_DEFINE(rx_msgq, 10);  // queue for can that can hold 10 messages 
const struct device *dev = DEVICE_DT_GET(CANBUS_NODE);

static void add_filter(uint32_t id)
{
    struct can_filter nmt_filter = {};
    nmt_filter.id    = id; // NMT id 
    nmt_filter.mask  = CAN_STD_ID_MASK; // all bits must match to get through
    nmt_filter.flags = 0; // 0 means normal can messages with 11 bits id
    
    can_add_rx_filter_msgq(dev, &rx_msgq, &nmt_filter); // put every message that matches nmt_filter into the mailbox rx_msgq
}

bool can_bus_init()
{
    if (!device_is_ready(dev))
    {
        printk("CAN device not ready\n");
        return false;
    }
        
    int err = can_set_bitrate(dev, 125000);
    if (err != 0)
    {
        printk("Error setting CAN bitrate (err %d)\n", err);
        return false;
    }
    
    err = can_start(dev);
    if (err != 0)
    {
        printk("Error starting CAN controller (err %d)\n", err);
        return false;
    }
  
    add_filter(0x000); //NMT
    add_filter(0x601); //SDO request
    add_filter(0x201); //RPDO1 
  
    return true;
}
