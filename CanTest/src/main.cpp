// glæra 3 í L4.4
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/can.h>
#include <zephyr/kernel.h>
/* Devicetree */
#define CANBUS_NODE DT_CHOSEN(zephyr_canbus)

int main(void)
{
    const struct device *dev = DEVICE_DT_GET(CANBUS_NODE);
    if (!device_is_ready(dev))
    {
        printk("CAN device not ready");
        return 0;
    }

    int err = can_set_bitrate(dev, 125000);

    if (err != 0)
    {
        printk("Error setting CAN bitrate (err %d)", err);
        return 0;
    }

    err = can_start(dev);

    if (err != 0)
    {
        printk("Error starting CAN controller (err %d)", err);
        return 0;
    }

    struct can_frame frame = {
    .id = 0x123,
    .dlc = 8,
    .flags = 0,
    .data = {1, 2, 3, 4, 5, 6, 7, 8}};

    while (1)
    {
        k_msleep(10);
        err = can_send(dev, &frame, K_MSEC(100), NULL, NULL);
        if (err != 0)
        {
            printk("Sending failed (err %d)", err);
            return 0;
        }
    }
}