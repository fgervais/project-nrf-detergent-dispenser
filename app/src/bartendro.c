#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bartendro, LOG_LEVEL_DBG);


static int bartendro_reset(const struct gpio_dt_spec *reset_pin) {
	int ret;

	// Apply reset (high)
	ret = gpio_pin_set_dt(reset_pin, 1);
	if (ret < 0) {
		LOG_ERR("Failed to set pin, error: %d", ret);
		return ret;
	}

	// Minimum 1ms
	k_sleep(K_MSEC(100));

	// Release reset
	ret = gpio_pin_set_dt(reset_pin, 0);
	if (ret < 0) {
		LOG_ERR("Failed to set pin, error: %d", ret);
		return ret;
	}

	k_sleep(K_MSEC(500));

	return 0;
}

static int bartendro_text_mode() {
	return 0;
}

int bartendro_init(const struct gpio_dt_spec *reset_pin) {
	int ret = 0;

	ret = bartendro_reset(reset_pin);
	if (ret < 0) {
		LOG_ERR("Failed to reset bartendro");
		return ret;
	}

	LOG_INF("☎️  Making contact with dispenser");
	ret = bartendro_text_mode();
	if (ret < 0) {
		LOG_ERR("Could not enter text mode");
		return ret;
	}

	LOG_INF("👍 Dispenser ready!"); 

	return 0;
}
