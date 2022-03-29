/* THIS CODE IS HORRIBLE, SORRY! */

#include <errno.h>
#include <stddef.h>
#include <device.h>
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

#define COWBOY_ON_OFF_CHARACTERISTIC BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0XC0B0A001, 0X18EB, 0X499D, 0XB266, 0X2F2910744274))
#define COWBOY_UART_WRITE_CHARACTERSTIC BT_UUID_DECLARE_128(BT_UUID_128_ENCODE(0X6E400002, 0XB5A3, 0XF393, 0XE0A9, 0XE50E24DCCA9E))

static struct bt_gatt_discover_params discover_params;
static struct bt_gatt_subscribe_params subscribe_params;
static bt_addr_le_t MY_COWBOY_ADDRESS;

static int when_connected = 0;
static int shutting_down = 0;
static int on_off_light = 1;

static uint16_t on_off_handle = 25;
static uint16_t uart_handle = 56;


static const struct gpio_dt_spec button1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw0), gpios, {0});
static const struct gpio_dt_spec button2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw1), gpios, {0});
static const struct gpio_dt_spec button3 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(sw2), gpios, {0});

//static struct gpio_callback button1_cb_data;
//static struct gpio_callback button2_cb_data;
//static struct gpio_callback button3_cb_data;

//static struct gpio_dt_spec led1 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led0), gpios, {0});*/
static struct gpio_dt_spec led2 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0});
/*static struct gpio_dt_spec led3 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0});
static struct gpio_dt_spec led4 = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led3), gpios, {0});
*/



static void flash(int duration, int speed) {
    for (int i = 0; i < duration; i++) {
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
    flash(20, 50);

    nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(button0), gpios), NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(button0), gpios), NRF_GPIO_PIN_SENSE_LOW);

    nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(button1), gpios), NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(button1), gpios), NRF_GPIO_PIN_SENSE_LOW);

    nrf_gpio_cfg_input(DT_GPIO_PIN(DT_NODELABEL(button2), gpios), NRF_GPIO_PIN_PULLUP);
    nrf_gpio_cfg_sense_set(DT_GPIO_PIN(DT_NODELABEL(button2), gpios), NRF_GPIO_PIN_SENSE_LOW);

    pm_power_state_force(0u, (struct pm_state_info){PM_STATE_SOFT_OFF, 0, 0});
    k_sleep(K_MSEC(100));
    gpio_pin_set_dt(&led2, 1);

    printk("ERROR: System off failed\n");
    while (true) {
        /* spin to avoid fall-off behavior */
    }
}

static void send_command_with_checksum(uint8_t data[]) {
    // a command is always 9 bytes, we add two more for the checksum
    uint8_t final_data[11];
    memcpy(final_data, data, 9);
    uint16_t crc = CRC16(final_data, 9);
    memcpy(final_data + 9, &crc, 2);
    int err = bt_gatt_write_without_response(default_conn, 0x37, final_data, 11, false);
    if (err) {
        flash(5, 500);
        shut_down();
        return;
    }

    k_sleep(K_MSEC(100));
}



static void bike_lights_off() {
    printk("turning lights off\n");
    uint8_t d[] = {0x0A, 0x10, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x00, 0xd4, 0xb1}; // off
    bt_gatt_write_without_response(default_conn, 0x38, d, 11, false);


//    send_command_with_checksum(d);
}

static void bike_lights_on() {
    printk("turning lights on\n");
    uint8_t d[] = {0x0A, 0x10, 0x00, 0x01, 0x00, 0x01, 0x02, 0x00, 0x01, 0x15, 0x71}; //, 0x15, 0x71}; // on
    //send_command_with_checksum(d);
    bt_gatt_write_without_response(default_conn, 0x38, d, 11, false);
}

static void set_bike_speed_limit(unsigned int speed_limit) {
    if (speed_limit < 20 || speed_limit > 40) {
        printk("not setting speed limit, has to be a good number, not %u\n", speed_limit);
        return;
    }
    printk("setting speed to %u\n", speed_limit);
    uint8_t d[] = {10, 16, 0, 4, 0, 1, 2, 0, speed_limit};
    send_command_with_checksum(&d);
}



static void print_chrc_props(uint8_t properties)
{
    printk(" : ");

    if (properties & BT_GATT_CHRC_BROADCAST) {
        printk("[bcast]");
    }

    if (properties & BT_GATT_CHRC_READ) {
        printk("[read]");
    }

    if (properties & BT_GATT_CHRC_WRITE) {
        printk("[write]");
    }

    if (properties & BT_GATT_CHRC_WRITE_WITHOUT_RESP) {
        printk("[write w/w rsp]");
    }

    if (properties & BT_GATT_CHRC_NOTIFY) {
        printk("[notify]");
    }

    if (properties & BT_GATT_CHRC_INDICATE) {
        printk("[indicate]");
    }

    if (properties & BT_GATT_CHRC_AUTH) {
        printk("[auth]");
    }

    if (properties & BT_GATT_CHRC_EXT_PROP) {
        printk("[ext prop]");
    }

    printk("\n");
}

static void handles_found_lets_really_rock() {
    gpio_pin_set_dt(&led2, 1);
    if (when_connected == 1) {
        bike_turn_on();
        //for (int i = 0; i < 3; i++) {
        //    bike_turn_off();
        //    k_sleep(K_MSEC(2000));
        //    bike_turn_on();
        //    k_sleep(K_MSEC(2000));
        //}
        //for (int i = 0; i < 10; i++) {
        //    bike_lights_off();
        //    gpio_pin_set_dt(&led2, 0);
        //    k_sleep(K_MSEC(2000));
        //    bike_lights_on();
        //    gpio_pin_set_dt(&led2, 1);
        //    k_sleep(K_MSEC(2000));
        //}
    } else if (when_connected == 2) {
        bike_turn_off();
    } else if (when_connected == 3) {
        bike_lights_off();
        k_sleep(K_SECONDS(1));
        bike_lights_on();
        k_sleep(K_SECONDS(1));
        bike_lights_off();
        k_sleep(K_SECONDS(1));
        bike_lights_on();
        k_sleep(K_SECONDS(1));
        bike_lights_off();
        k_sleep(K_SECONDS(1));
        bike_lights_on();
        k_sleep(K_SECONDS(1));
    }
    shut_down();
}


static uint8_t discover_func(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params)
{
    int err;

    if (!attr) {
        printk("Discover complete\n");
        (void)memset(params, 0, sizeof(*params));
        handles_found_lets_really_rock();
        return BT_GATT_ITER_STOP;
    }
    struct bt_gatt_service_val *gatt_service;
    struct bt_gatt_chrc *gatt_chrc;
    char str[37];
    char str2[37];

    gatt_chrc = attr->user_data;
    bt_uuid_to_str(gatt_chrc->uuid, str, sizeof(str));
    bt_uuid_to_str(attr->uuid, str2, sizeof(str2));

    printk("[ATTRIBUTE] handle %u - %s - %s", attr->handle, str2, str);



    print_chrc_props(gatt_chrc->properties);
    int h = 0;

    if (!bt_uuid_cmp(gatt_chrc->uuid, COWBOY_ON_OFF_CHARACTERISTIC)) {
        on_off_handle = attr->handle + 1;
        printk("on off\n");
    } else if (!bt_uuid_cmp(gatt_chrc->uuid, COWBOY_UART_WRITE_CHARACTERSTIC)) {
        uart_handle = attr->handle + 1;
        h = uart_handle;
        printk("uart %d\n", h);
    }
    //    k_sleep(K_SECONDS(1));

/*  if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HRS)) {
        memcpy(&uuid, BT_UUID_HRS_MEASUREMENT, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 1;
        discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            printk("Discover failed (err %d)\n", err);
        }
    } else if (!bt_uuid_cmp(discover_params.uuid, BT_UUID_HRS_MEASUREMENT)) {
        memcpy(&uuid, BT_UUID_GATT_CCC, sizeof(uuid));
        discover_params.uuid = &uuid.uuid;
        discover_params.start_handle = attr->handle + 2;
        discover_params.type = BT_GATT_DISCOVER_DESCRIPTOR;
        subscribe_params.value_handle = bt_gatt_attr_value_handle(attr);

        err = bt_gatt_discover(conn, &discover_params);
        if (err) {
            printk("Discover failed (err %d)\n", err);
        }
    } else {
        subscribe_params.notify = notify_func;
        subscribe_params.value = BT_GATT_CCC_NOTIFY;
        subscribe_params.ccc_handle = attr->handle;

        err = bt_gatt_subscribe(conn, &subscribe_params);
        if (err && err != -EALREADY) {
            printk("Subscribe failed (err %d)\n", err);
        } else {
            printk("[SUBSCRIBED]\n");
        }

        return BT_GATT_ITER_STOP;
    }
    */

    return BT_GATT_ITER_CONTINUE;
}



static void connected(struct bt_conn *conn, uint8_t conn_err)
{
    int err;
    if (conn_err) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
        flash(10, 500);
        shut_down();
        return;
    }
    printk("Connected\n");
    //flash(100, 50);

/*
    GET SOME INFO FROM THE CONNECTION

    struct bt_conn_info conn_info;
    bt_conn_get_info(default_conn, &conn_info);
    printk("bt type is LE: %d\n", conn_info.type == BT_CONN_TYPE_LE || conn_info.type == BT_CONN_TYPE_BR);
    printk("bt role is central: %d\n", conn_info.role == BT_CONN_ROLE_CENTRAL);

*/
    err = bt_conn_set_security(default_conn, BT_SECURITY_L3);
    if (err) {
        printk("Failed to set security (err %u)\n", err);
        shut_down();
        return;
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    char addr[BT_ADDR_LE_STR_LEN];

    bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));

    printk("Disconnected: %s (reason 0x%02x)\n", addr, reason);
    //gpio_pin_set_dt(&led1, 0);

    if (default_conn == conn) {
        bt_conn_unref(default_conn);
        default_conn = NULL;
    }
    shut_down();
}

static void passkey_entry(struct bt_conn *conn)
{
    printk("passkey requested, sending it\n");
    //gpio_pin_set_dt(&led4, 1);
    bt_conn_auth_passkey_entry(conn, YOUR_6_DIGIT_PIN_HERE);
//    flash(4, 1000);
}

static void pairing_complete(struct bt_conn *conn, bool bonded)
{
    printk("Pairing complete\n");
    if (bonded) {
        printk("  ALSO BONDED\n");
    }
}
static void pairing_failed(struct bt_conn *conn, enum bt_security_err reason)
{
    printk("pairing failed\n");
}

static void cancel(struct bt_conn *conn)
{
    printk("pairing cancel\n");
}

static void connection_ready_lets_rock() {
    //memcpy(&uuid, BT_UUID_HRS, sizeof(uuid));
    //discover_params.uuid = &uuid.uuid;
    discover_params.func = discover_func;
    discover_params.start_handle = BT_ATT_FIRST_ATTRIBUTE_HANDLE;
    discover_params.end_handle = BT_ATT_LAST_ATTRIBUTE_HANDLE;
    discover_params.type = BT_GATT_DISCOVER_CHARACTERISTIC;
    //discover_params.type = BT_GATT_DISCOVER_PRIMARY;

    bt_gatt_discover(default_conn, &discover_params);
    //flash(50, 50);
    return;
}


static void security_changed(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
    printk("security changed, level is now: %u\n", level);
    if (level == BT_SECURITY_L3 || level == BT_SECURITY_L4) {
        //connection_ready_lets_rock();
        handles_found_lets_really_rock();
        return;
    }
//    shut_down();
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
/*BT_CONN_CB_DEFINE(conn_callbacks) = {
    .connected = connected,
    .disconnected = disconnected,
    .security_changed = security_changed,
};*/

void initialize_buttons() {
    int ret;

/*
    if (!device_is_ready(button1.port)) {
        printk("Error: button device %s is not ready\n", button1.port->name);
        return;
    }

    ret = gpio_pin_configure_dt(&button, GPIO_INPUT);
    if (ret != 0) {
        printk("Error %d: failed to configure %s pin %d\n", ret, button.port->name, button.pin);
        return;
    }

    ret = gpio_pin_interrupt_configure_dt(&button, GPIO_INT_EDGE_TO_ACTIVE);
    if (ret != 0) {
        printk("Error %d: failed to configure interrupt on %s pin %d\n", ret, button.port->name, button.pin);
        return;
    }
*/

    // No checks, YOLO
    gpio_pin_configure_dt(&button1, GPIO_INPUT);
//    gpio_pin_interrupt_configure_dt(&button1, GPIO_INT_EDGE_TO_ACTIVE);
//    gpio_init_callback(&button1_cb_data, button1_pressed, BIT(button1.pin));
//    gpio_add_callback(button1.port, &button1_cb_data);

    gpio_pin_configure_dt(&button2, GPIO_INPUT);
//    gpio_pin_interrupt_configure_dt(&button2, GPIO_INT_EDGE_TO_ACTIVE);
//    gpio_init_callback(&button2_cb_data, button2_pressed, BIT(button2.pin));
//    gpio_add_callback(button3.port, &button2_cb_data);

    gpio_pin_configure_dt(&button3, GPIO_INPUT);
//    gpio_pin_interrupt_configure_dt(&button3, GPIO_INT_EDGE_TO_ACTIVE);
//    gpio_init_callback(&button3_cb_data, button3_pressed, BIT(button3.pin));
//    gpio_add_callback(button3.port, &button3_cb_data);

    printk("Set up buttons\n");
}

void initialize_led() {
    //gpio_pin_configure_dt(&led1, GPIO_OUTPUT);
    gpio_pin_configure_dt(&led2, GPIO_OUTPUT);
    //gpio_pin_configure_dt(&led3, GPIO_OUTPUT);
    //gpio_pin_configure_dt(&led4, GPIO_OUTPUT);
    //gpio_pin_set_dt(&led1, 0);
    gpio_pin_set_dt(&led2, 1);
    //gpio_pin_set_dt(&led3, 0);
    //gpio_pin_set_dt(&led4, 0);
    printk("Leds configured\n");
}

void main(void)
{
    int err;
    initialize_led();
    initialize_buttons();

    int btn1_pressed = gpio_pin_get_dt(&button1);
    int btn2_pressed = gpio_pin_get_dt(&button2);// || true ; // HACK
    int btn3_pressed = gpio_pin_get_dt(&button3);

    //flash(4, 500);

    gpio_pin_set_dt(&led2, 0);
    //flash(4, 500);

    if (btn1_pressed) {
        when_connected = 1;
        //gpio_pin_set_dt(&led1, 1);
    } else if (btn2_pressed) {
        when_connected = 2;
        //gpio_pin_set_dt(&led2, 1);
    } else if (btn3_pressed) {
        when_connected = 3;
        //gpio_pin_set_dt(&led3, 1);
    }

    if (btn1_pressed || btn2_pressed || btn3_pressed) {
         bt_conn_cb_register(&conn_callbacks);

//      settings_load();
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

        err = bt_addr_le_from_str("YOUR COWBOY MAC HERE", "random", &MY_COWBOY_ADDRESS);
        if (err) {
            printk("Error converting my bike (err %d)\n", err);
            return;
        }


        err = bt_conn_auth_cb_register(&conn_auth_callbacks);
        if (err) {
            printk("Error registering auth cv (err %d)\n", err);
            return;
        }


        struct bt_le_conn_param *param;
        param = BT_LE_CONN_PARAM_DEFAULT;
        err = bt_conn_le_create(&MY_COWBOY_ADDRESS, BT_CONN_LE_CREATE_CONN, param, &default_conn);
        if (err) {
            printk("Create conn failed (err %d)\n", err);
            return;
        }

        printk("Bluetooth initialized\n");
//        flash(800, 6);
        gpio_pin_set_dt(&led2, 1);
        for (int i = 0; i < 6000; i++) {
            k_sleep(K_MSEC(100));
            k_yield();
        }
        shut_down();
        return;

        //while (!shutting_down) {
        //    //gpio_pin_set_dt(&led3, 1);
        //}
        //return;

    }

    shut_down();
}
