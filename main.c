/*
 * WARNING: This code is NOT production ready.
 * There are very few checks for return codes, lots of hardcoded things.
 * I published it because there are very few nrf projects available on github,
 * and the world needs better examples ;)
 * */


#include <errno.h>
#include <stddef.h>
#include <device.h>
#include <random/rand32.h>
#include <drivers/gpio.h>
#include <sys/printk.h>
#include <zephyr.h>
#include <zephyr/types.h>

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>
#include <bluetooth/gatt.h>
#include <bluetooth/hci.h>
#include <bluetooth/uuid.h>
#include <sys/byteorder.h>

#include <pm/pm.h>
#include <pm/device.h>

#include <hal/nrf_gpio.h>

static struct bt_conn *default_conn;

#define DO_NOTHING 0
#define DO_TURN_ON 1
#define DO_TURN_OFF 2
#define DO_TOGGLE_LIGHTS 3
#define DO_RESET_SPEED_LIMIT 4
#define DO_CUSTOM_SPEED_LIMIT 5


#define MY_BIKE_MAC "XX:XX:XX:XX:XX:XX"
#define MY_PASSKEY 123456

static uint8_t light_off_cmd[] = {0x0A, 0x10, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x00};
static uint8_t light_on_cmd[] = {0x0A, 0x10, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x01};
static uint8_t write_flash[] = {0x01, 0x10, 0x01, 0xff, 0x00, 0x01, 0x02, 0x7f, 0xff};
static uint8_t close_flash[] = {0x01, 0x10, 0x01, 0xff, 0x00, 0x01, 0x02, 0x00, 0x00};
static uint8_t read_light[] = {0x0a, 0x03, 0x00, 0x01, 0x00, 0x01};

#define COWBOY_ON_OFF_CHARACTERISTIC BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0XC0B0A001, 0X18EB, 0X499D, 0XB266, 0X2F2910744274))
#define COWBOY_UART_WRITE_CHARACTERSTIC BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0X6E400002, 0XB5A3, 0XF393, 0XE0A9, 0XE50E24DCCA9E))

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_write_params bgwp;
static struct bt_gatt_subscribe_params subscribe_params;

static bt_addr_le_t MY_COWBOY_ADDRESS;

static uint8_t when_connected = DO_NOTHING;
static int shutting_down = 0;

static uint16_t on_off_handle = 0x19;
static uint16_t uart_handle = 0x38;

static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0});

static struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0});


static void flash(int times, int speed) {
    for (int i = 0; i < times; i++) {
        gpio_pin_set_dt(&led2, i % 2);
        k_sleep(K_MSEC(speed));
    }
    gpio_pin_set_dt(&led2, 0);
}

uint16_t CRC16 (const uint8_t *nData, uint16_t wLength) {

    // We have to use CRC-16 (Modbus), see https://www.lammertbies.nl/comm/info/crc-calculation
    // This method is taken from https://www.modbustools.com/modbus_crc16.html

    static const uint16_t wCRCTable[] = {
        0X0000, 0XC0C1, 0XC181, 0X0140, 0XC301, 0X03C0, 0X0280, 0XC241,
        0XC601, 0X06C0, 0X0780, 0XC741, 0X0500, 0XC5C1, 0XC481, 0X0440,
        0XCC01, 0X0CC0, 0X0D80, 0XCD41, 0X0F00, 0XCFC1, 0XCE81, 0X0E40,
        0X0A00, 0XCAC1, 0XCB81, 0X0B40, 0XC901, 0X09C0, 0X0880, 0XC841,
        0XD801, 0X18C0, 0X1980, 0XD941, 0X1B00, 0XDBC1, 0XDA81, 0X1A40,
        0X1E00, 0XDEC1, 0XDF81, 0X1F40, 0XDD01, 0X1DC0, 0X1C80, 0XDC41,
        0X1400, 0XD4C1, 0XD581, 0X1540, 0XD701, 0X17C0, 0X1680, 0XD641,
        0XD201, 0X12C0, 0X1380, 0XD341, 0X1100, 0XD1C1, 0XD081, 0X1040,
        0XF001, 0X30C0, 0X3180, 0XF141, 0X3300, 0XF3C1, 0XF281, 0X3240,
        0X3600, 0XF6C1, 0XF781, 0X3740, 0XF501, 0X35C0, 0X3480, 0XF441,
        0X3C00, 0XFCC1, 0XFD81, 0X3D40, 0XFF01, 0X3FC0, 0X3E80, 0XFE41,
        0XFA01, 0X3AC0, 0X3B80, 0XFB41, 0X3900, 0XF9C1, 0XF881, 0X3840,
        0X2800, 0XE8C1, 0XE981, 0X2940, 0XEB01, 0X2BC0, 0X2A80, 0XEA41,
        0XEE01, 0X2EC0, 0X2F80, 0XEF41, 0X2D00, 0XEDC1, 0XEC81, 0X2C40,
        0XE401, 0X24C0, 0X2580, 0XE541, 0X2700, 0XE7C1, 0XE681, 0X2640,
        0X2200, 0XE2C1, 0XE381, 0X2340, 0XE101, 0X21C0, 0X2080, 0XE041,
        0XA001, 0X60C0, 0X6180, 0XA141, 0X6300, 0XA3C1, 0XA281, 0X6240,
        0X6600, 0XA6C1, 0XA781, 0X6740, 0XA501, 0X65C0, 0X6480, 0XA441,
        0X6C00, 0XACC1, 0XAD81, 0X6D40, 0XAF01, 0X6FC0, 0X6E80, 0XAE41,
        0XAA01, 0X6AC0, 0X6B80, 0XAB41, 0X6900, 0XA9C1, 0XA881, 0X6840,
        0X7800, 0XB8C1, 0XB981, 0X7940, 0XBB01, 0X7BC0, 0X7A80, 0XBA41,
        0XBE01, 0X7EC0, 0X7F80, 0XBF41, 0X7D00, 0XBDC1, 0XBC81, 0X7C40,
        0XB401, 0X74C0, 0X7580, 0XB541, 0X7700, 0XB7C1, 0XB681, 0X7640,
        0X7200, 0XB2C1, 0XB381, 0X7340, 0XB101, 0X71C0, 0X7080, 0XB041,
        0X5000, 0X90C1, 0X9181, 0X5140, 0X9301, 0X53C0, 0X5280, 0X9241,
        0X9601, 0X56C0, 0X5780, 0X9741, 0X5500, 0X95C1, 0X9481, 0X5440,
        0X9C01, 0X5CC0, 0X5D80, 0X9D41, 0X5F00, 0X9FC1, 0X9E81, 0X5E40,
        0X5A00, 0X9AC1, 0X9B81, 0X5B40, 0X9901, 0X59C0, 0X5880, 0X9841,
        0X8801, 0X48C0, 0X4980, 0X8941, 0X4B00, 0X8BC1, 0X8A81, 0X4A40,
        0X4E00, 0X8EC1, 0X8F81, 0X4F40, 0X8D01, 0X4DC0, 0X4C80, 0X8C41,
        0X4400, 0X84C1, 0X8581, 0X4540, 0X8701, 0X47C0, 0X4680, 0X8641,
        0X8201, 0X42C0, 0X4380, 0X8341, 0X4100, 0X81C1, 0X8081, 0X4040
    };

    uint8_t nTemp;
    uint16_t wCRCWord = 0xFFFF;

   while (wLength--)
   {
      nTemp = *nData++ ^ wCRCWord;
      wCRCWord >>= 8;
      wCRCWord ^= wCRCTable[nTemp];
   }
   return wCRCWord;
}

static void bike_turn_off() {
    printk("turning off\n");
    uint8_t d[] = {0};
    bt_gatt_write_without_response(default_conn, on_off_handle, d, 1, false);
}

static void bike_turn_on() {
    printk("turning on\n");
    uint8_t d[] = {1};
    bt_gatt_write_without_response(default_conn, on_off_handle, d, 1, false);
}


static void shut_down() {
    shutting_down = 1;
    if (default_conn != NULL) {
        bt_conn_disconnect(default_conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
        k_sleep(K_MSEC(100));
    }

    // configure pins as wake up
    nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(button0), gpios), NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(button0), gpios), NRF_GPIO_PIN_SENSE_LOW);

    nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(button1), gpios), NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(button1), gpios), NRF_GPIO_PIN_SENSE_LOW);

    nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(button2), gpios), NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(button2), gpios), NRF_GPIO_PIN_SENSE_LOW);

    // tell the user that we are good to shut down
    flash(20, 50);

    // configure pins as wake up
    pm_power_state_force(0u, (struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
    k_sleep(K_MSEC(100));

    /* we should not get here, keep the light on to signal sleep failed */
    gpio_pin_set_dt(&led2, 1);

    printk("ERROR: System off failed\n");
    while (true) {
    }
}

static void send_command_with_checksum(uint8_t data[], uint8_t len) {
    // a command is max 9 bytes, we add two more for the checksum
    uint8_t final_data[11];
    memcpy(final_data, data, len);
    uint16_t crc = CRC16(final_data, len);
    memcpy(final_data + len, &crc, 2);
    int err = bt_gatt_write_without_response(default_conn, uart_handle, final_data, len + 2, false);
    if (err) {
        flash(5, 500);
        shut_down();
        return;
    }
    k_sleep(K_MSEC(100));
}

static void bike_lights_off() {
    printk("turning lights off\n");
    send_command_with_checksum(light_off_cmd, 9);
}

static void bike_lights_on() {
    printk("turning lights on\n");
    send_command_with_checksum(light_on_cmd, 9);
}

static void persist_motor_settings() {
    k_sleep(K_MSEC(300));
    send_command_with_checksum(write_flash, 9);
    k_sleep(K_MSEC(300));
    send_command_with_checksum(close_flash, 9);
}

static void set_bike_speed_limit(unsigned int speed_limit) {
    if (speed_limit < 20 || speed_limit > 40) {
        printk("not setting speed limit, has to be a good number, not %u\n", speed_limit);
        return;
    }
    printk("setting speed to %u\n", speed_limit);
    uint8_t d1[] = {10, 16, 0, 4, 0, 1, 2, 0, speed_limit};
    send_command_with_checksum(d1, 9);

    k_sleep(K_MSEC(300));
    uint8_t d2[] = {1, 16, 0, 11, 0, 1, 2, 0, 2}; // set motor to torque mode + speed limit
    send_command_with_checksum(d2, 9);

    k_sleep(K_MSEC(300));
    uint8_t d3[] = {0x01, 0x10, 0x00, 0x81, 0x00, 0x01, 0x02, 0x00, 0x00}; // field weakening of 0
    send_command_with_checksum(d3, 9);

    persist_motor_settings();
}

//static void set_bike_speed_limit_high_speed() {
//    uint8_t d1[] = {1, 16, 0, 11, 0, 1, 2, 0, 1}; // set motor to pure torque mode
//    send_command_with_checksum(d1, 9);
//    k_sleep(K_MSEC(300));
//    uint8_t d2[] = {0x01, 0x10, 0x00, 0x81, 0x00, 0x01, 0x02, 0x02, 0x66}; // field weakening of 15% = 15 * 40.69 = 614 = 0x0266
//    send_command_with_checksum(d2, 9);
//    persist_motor_settings();
//}


static uint8_t notify_func(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, uint8_t data[], uint16_t length) {
    if (!data) {
        printk("Usubscribed\n");
        params->value_handle = 0;
        return BT_GATT_ITER_STOP;
    }

    if (when_connected == DO_TOGGLE_LIGHTS) {
        // we assume that this is a callback with the light status...
        when_connected = DO_NOTHING;
        k_sleep(K_MSEC(300));
        if (length == 7 && data[4] == 0x01) {
            bike_lights_off();
        } else {
            // just assume we have to turn on the lights;
            bike_lights_on();
        }
        shut_down();
    }

    return BT_GATT_ITER_CONTINUE;
}

static void write_func(struct bt_conn *conn, uint8_t conn_err,  struct bt_gatt_subscribe_params *params) {
    // can probably be removed
    printk("Write callback\n");
}

static void handles_found_lets_really_rock() {
    subscribe_params.ccc_handle = 0x3b; // characteristic where we have to send the subscription
    subscribe_params.notify = &notify_func;
    subscribe_params.value = BT_GATT_CCC_NOTIFY;
    subscribe_params.value_handle = 0x3a; // characteristic about which we want to be notiied
    subscribe_params.write  = &write_func; // this should not be needed
    bt_gatt_subscribe(default_conn, &subscribe_params);
    k_sleep(K_MSEC(100));

    gpio_pin_set_dt(&led2, 1);
    if (when_connected == DO_TURN_ON) {
        bike_turn_on();
    } else if (when_connected == DO_TURN_OFF) {
        bike_turn_off();
    } else if (when_connected == DO_TOGGLE_LIGHTS) {
        send_command_with_checksum(read_light, 6);
        // don't shut down but wait for the callback
        return;
    } else if (when_connected == DO_RESET_SPEED_LIMIT) {
        set_bike_speed_limit(25);
        flash(5, 300);
    } else if (when_connected == DO_CUSTOM_SPEED_LIMIT) {
        set_bike_speed_limit(24);
        flash(15, 100);
    }
    shut_down();
}

static void connected(struct bt_conn *conn, uint8_t conn_err) {
    int err;
    if (conn_err) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
        flash(10, 500);
        shut_down();
        return;
    }
    printk("Connected\n");

    err = bt_conn_set_security(default_conn, BT_SECURITY_L3);
    if (err) {
        printk("Failed to set security (err %u)\n", err);
        shut_down();
        return;
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason) {
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);

    if (default_conn == conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
    shut_down();
}

static void passkey_entry(struct bt_conn *conn) {
    printk("passkey requested, sending it\n");
    bt_conn_auth_passkey_entry(conn, MY_PASSKEY);
}

static void pairing_complete(struct bt_conn *conn, bool bonded) {
    printk("Pairing complete\n");
    if (bonded) {
        printk("  ALSO BONDED\n");
    }
}
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason) {
    printk("pairing failed\n");
}

static void cancel(struct bt_conn *conn) {
    printk("pairing cancel\n");
}

static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err) {
    printk("security changed, level is now: %u\n", level);
    if (level == BT_SECURITY_L3 || level == BT_SECURITY_L4) {
        handles_found_lets_really_rock();
        return;
    }
}

static struct bt_conn_auth_cb conn_auth_callbacks = {
    .pairing_complete = pairing_complete,
    .passkey_entry = passkey_entry,
    .pairing_failed = pairing_failed,
    .cancel = cancel,
};

static struct bt_conn_cb conn_callbacks = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};

void initialize_buttons() {
    // No checks, YOLO
    gpio_pin_configure_dt(&button1, GPIO_INPUT);
    gpio_pin_configure_dt(&button2, GPIO_INPUT);
    gpio_pin_configure_dt(&button3, GPIO_INPUT);
    printk("Set up buttons\n");
}

void initialize_led() {
    gpio_pin_configure_dt(&led2, GPIO_OUTPUT);
    gpio_pin_set_dt(&led2, 1);
    printk("Leds configured\n");
}

void main(void) {
    int err;
    initialize_led();
    initialize_buttons();
    bt_set_bondable(false);

    int btn1_pressed = gpio_pin_get_dt(&button1);
    int btn2_pressed = gpio_pin_get_dt(&button2);
    int btn3_pressed = gpio_pin_get_dt(&button3);

    when_connected = DO_NOTHING;
    gpio_pin_set_dt(&led2, 0);
    if (btn2_pressed) {
        k_sleep(K_MSEC(1000));
        btn1_pressed = gpio_pin_get_dt(&button1);
        btn3_pressed = gpio_pin_get_dt(&button3);

        if (btn1_pressed) {
            when_connected = DO_RESET_SPEED_LIMIT;
        } else if (btn3_pressed) {
            when_connected = DO_CUSTOM_SPEED_LIMIT;
        } else {
            when_connected = DO_TOGGLE_LIGHTS;
        }
    } else if (btn3_pressed) {
        when_connected = DO_TURN_ON;
    } else if (btn1_pressed) {
        when_connected = DO_TURN_OFF;
    }

    if (when_connected) {
         bt_conn_cb_register(&conn_callbacks);
        err = bt_enable(NULL);

        if (err) {
            int k = err * -1;
            err = k;
            if (k > 0) {
                printk("Bluetooth init failed (err %d %d)\n", k, err);
                shut_down();
            } else {
                printk("Bluetooth init failed (err %d %d)\n", k, err);
                shut_down();
            }
            printk("Bluetooth init failed (err %d %d)\n", k, err);
            return;
        }

        err = bt_addr_le_from_str(MY_BIKE_MAC, "random", &MY_COWBOY_ADDRESS);
        if (err) {
            printk("Error converting my bike (err %d)\n", err);
            shut_down();
            return;
        }

        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Error registering auth cv (err %d)\n", err);
            shut_down();
            return;
        }

        struct bt_le_conn_param *param;
        param = BT_LE_CONN_PARAM_DEFAULT;
        err = bt_conn_le_create(&MY_COWBOY_ADDRESS, BT_CONN_LE_CREATE_CONN, param, &default_conn);
        if (err) {
            printk("Create conn failed (err %d)\n", err);
            shut_down();
            return;
        }

        printk("Bluetooth initialized\n");
        gpio_pin_set_dt(&led2, 1);
        for (int i = 0; i < 70; i++) {
            k_sleep(K_MSEC(100));
            k_yield();
        }
        shut_down();
        return;
    }

    shut_down();
}
