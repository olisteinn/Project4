#include <zephyr/kernel.h>
#include <zephyr/drivers/can.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include "can_bus.h"

LOG_MODULE_REGISTER(can_bus, LOG_LEVEL_INF);

#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)
CAN_MSGQ_DEFINE(rx_msgq, 10);  // queue for can that can hold 10 messages 
K_MSGQ_DEFINE(tx_msgq, sizeof(CanMsg), 16, alignof(CanMsg));
static const struct device *dev = DEVICE_DT_GET(CANBUS_NODE);
static atomic_t rx_frames = ATOMIC_INIT(0);
static K_THREAD_STACK_DEFINE(tx_thread_stack, 1024);
static struct k_thread tx_thread;

static void tx_thread_entry(void *, void *, void *);

static bool add_filter(uint32_t id)
{
    struct can_filter filter = {};
    filter.id    = id; // NMT id 
    filter.mask  = CAN_STD_ID_MASK; // all bits must match to get through
    filter.flags = 0; // 0 means normal can messages with 11 bits id
    
    const int filter_id = can_add_rx_filter_msgq(dev, &rx_msgq, &filter);
    if (filter_id < 0) {
        LOG_ERR("Failed to add CAN filter 0x%03x (%d)", id, filter_id);
        return false;
    }
    return true;
}

bool can_bus_init()
{
    if (!device_is_ready(dev))
    {
        LOG_ERR("CAN device not ready");
        return false;
    }
        
    int err = can_set_bitrate(dev, 125000);
    if (err != 0)
    {
        LOG_ERR("Failed to set CAN bitrate (%d)", err);
        return false;
    }
    
    err = can_start(dev);
    if (err != 0)
    {
        LOG_ERR("Failed to start CAN controller (%d)", err);
        return false;
    }
  
    if (!add_filter(0x000) || !add_filter(0x601) || !add_filter(0x201)) {
        return false;
    }

    if (k_thread_create(&tx_thread, tx_thread_stack,
                        K_THREAD_STACK_SIZEOF(tx_thread_stack),
                        tx_thread_entry, nullptr, nullptr, nullptr,
                        K_PRIO_PREEMPT(4), 0, K_NO_WAIT) == nullptr) {
        LOG_ERR("Failed to start CAN transmit thread");
        return false;
    }
    return true;
}

bool can_bus_receive_wait(CanMsg &msg)
{
  struct can_frame frame;
  if (k_msgq_get(&rx_msgq, &frame, K_FOREVER) != 0) {
    LOG_ERR("Failed waiting for CAN receive queue");
    return false;
  }
  if (frame.dlc > sizeof(msg.data)) {
    LOG_ERR("Received CAN frame with invalid DLC %u", frame.dlc);
    return false;
  }

  atomic_inc(&rx_frames);
  msg.id  = frame.id;   // writes to the message what was in mailbox
  msg.dlc = frame.dlc;
  for (int i = 0; i < frame.dlc; i++) {
    msg.data[i] = frame.data[i];
  }
  return true;
}

uint32_t can_bus_get_rx_frames()
{
  return static_cast<uint32_t>(atomic_get(&rx_frames));
}

bool can_bus_send(const CanMsg &msg)
{
    if (msg.dlc > sizeof(msg.data)) {
        LOG_ERR("Refusing CAN frame with invalid DLC %u", msg.dlc);
        return false;
    }

    struct can_frame frame = {};
    frame.id = msg.id;
    frame.dlc = msg.dlc;
    for (int i=0; i < msg.dlc; i++) {
        frame.data[i] = msg.data[i];
    }

    int err = can_send(dev, &frame, K_MSEC(100), NULL, NULL);
    if (err != 0){
        LOG_ERR("CAN send failed (%d)", err);
        return false;
    }
    return true;
}

bool can_bus_send_async(const CanMsg &msg)
{
    if (msg.dlc > sizeof(msg.data)) {
        LOG_ERR("Refusing CAN frame with invalid DLC %u", msg.dlc);
        return false;
    }
    if (k_msgq_put(&tx_msgq, &msg, K_NO_WAIT) != 0) {
        LOG_ERR("CAN transmit queue full");
        return false;
    }
    return true;
}

static void tx_thread_entry(void *, void *, void *)
{
    while (true) {
        CanMsg msg;
        if (k_msgq_get(&tx_msgq, &msg, K_FOREVER) != 0) {
            LOG_ERR("Failed waiting for CAN transmit queue");
            return;
        }
        can_bus_send(msg);
    }
}
