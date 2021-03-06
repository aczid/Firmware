#include <sdkconfig.h>

#ifdef CONFIG_SHA_BADGE_TOUCH_DEBUG
#define LOG_LOCAL_LEVEL ESP_LOG_DEBUG
#endif // CONFIG_SHA_BADGE_TOUCH_DEBUG

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <esp_log.h>

#include <badge_pins.h>
#include <badge_i2c.h>
#include <badge_portexp.h>
#include <badge_touch.h>

#ifdef I2C_TOUCHPAD_ADDR

static const char *TAG = "badge_touch";

badge_touch_event_t badge_touch_handler = NULL;

static inline int
badge_touch_read_event(void)
{
	uint8_t buf[3];
	esp_err_t ret = badge_i2c_read_event(I2C_TOUCHPAD_ADDR, buf);

	if (ret == ESP_OK) {
		ESP_LOGD(TAG, "event: 0x%02x, 0x%02x, 0x%02x", buf[0], buf[1], buf[2]);
		return (buf[0] << 16) | (buf[1] << 8) | (buf[2]);
	} else {
		ESP_LOGE(TAG, "i2c master read: error %d", ret);
		return -1;
	}
}

void
badge_touch_intr_handler(void *arg)
{
	while (1)
	{
		// read touch-controller interrupt line
		int x = badge_portexp_get_input();
		if (x == -1) // error
			continue; // retry..

		if (x & (1 << PORTEXP_PIN_NUM_TOUCH)) // no events waiting
			break;

		// event waiting
		int event = badge_touch_read_event();

		if (event == -1) // error
			continue;

		if (badge_touch_handler != NULL)
			badge_touch_handler(event);
	}
}

esp_err_t
badge_touch_init(void)
{
	static bool badge_touch_init_done = false;

	if (badge_touch_init_done)
		return ESP_OK;

	ESP_LOGD(TAG, "init called");

	esp_err_t res;
	res = badge_portexp_init();
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_input_default_state(PORTEXP_PIN_NUM_TOUCH, 1);
	if (res != ESP_OK)
		return res;
	res = badge_portexp_set_interrupt_enable(PORTEXP_PIN_NUM_TOUCH, 1);
	if (res != ESP_OK)
		return res;
	badge_portexp_set_interrupt_handler(PORTEXP_PIN_NUM_TOUCH, badge_touch_intr_handler, NULL);

	// read pending old events
	badge_touch_intr_handler(NULL);

	badge_touch_init_done = true;

	ESP_LOGD(TAG, "init done");

	return ESP_OK;
}

void
badge_touch_set_event_handler(badge_touch_event_t handler)
{
	badge_touch_handler = handler;
}

#endif // I2C_TOUCHPAD_ADDR
