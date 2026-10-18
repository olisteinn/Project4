#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include "can_bus.h"

#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)
CAN_MSGQ_DEFINE(rx_msgq, 10); // queue for can, size 10
static const struct device *dev = DEVICE_DT_GET(CANBUS_NODE);

static void tx_irq_callback(const struct device *dev, int error, void *user_data)
{
}

static void add_filter(uint32_t id)
{
    struct can_filter filter = {};
    filter.id    = id;
    filter.mask  = CAN_STD_ID_MASK;
    filter.flags = 0;
    
    can_add_rx_filter_msgq(dev, &rx_msgq, &filter);
}

bool can_bus_init()
{
    if (!device_is_ready(dev)) {
        return false;
    }
    if (can_set_bitrate(dev, 125000) != 0) {
        return false;
    }
    if (can_start(dev) != 0) {
        return false;
    }
  
    add_filter(0x000); // NMT
    add_filter(0x601); // SDO request
    add_filter(0x201); // RPDO1
    add_filter(0x080); // SYNC 3.3 optional

    return true;
}

bool can_bus_receive_wait(CanMsg &msg)
{
    struct can_frame frame;
    if (k_msgq_get(&rx_msgq, &frame, K_FOREVER) != 0) {
        return false;
    }

    msg.id  = frame.id;
    msg.dlc = frame.dlc;
    for (int i = 0; i < frame.dlc; i++) {
        msg.data[i] = frame.data[i];
    }
    return true;
}

bool can_bus_send(const CanMsg &msg)
{
    struct can_frame frame = {};
    frame.id = msg.id;
    frame.dlc = msg.dlc;
    for (int i = 0; i < msg.dlc; i++) {
        frame.data[i] = msg.data[i];
    }

    return can_send(dev, &frame, K_NO_WAIT, tx_irq_callback, nullptr) == 0;
}
