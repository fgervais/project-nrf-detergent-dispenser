#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bartendro, LOG_LEVEL_DBG);


#if !defined(CONFIG_APP_USE_TEXT_MODE)
static uint8_t id;
#endif


static int bartendro_reset(const struct gpio_dt_spec *reset_pin) {
	int ret;

	// Ensure bartendro is powered up and listening for reset
	k_sleep(K_SECONDS(1));

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

	// Wait for bartendro to reset
	k_sleep(K_SECONDS(1));

	return 0;
}

static int bartendro_read_byte(const struct device *uart,
		unsigned char *rxdata) {
	int ret;

retry:
	ret = uart_poll_in(uart, rxdata);
	if (ret == -1) {
		k_sleep(K_USEC(100));
		goto retry;
	}
	else if (ret < 0) {
		return ret;
	}

	return 0;
}

static int bartendro_get_id(const struct device *uart,
		uint8_t *id) {
	int ret;

	uart_poll_out(uart, '?');

	LOG_INF("Trying to read ID");
	ret = bartendro_read_byte(uart, id);
	if (ret < 0) {
		return ret;
	}

	LOG_INF("Exit address exchange phase");
	uart_poll_out(uart, 0xff);

	return 0;
}

#if defined(CONFIG_APP_USE_TEXT_MODE)
static int bartendro_text_mode(const struct device *uart) {
	int i;

	for (i = 0; i < 3; i++) {
		uart_poll_out(uart, '!');
	}

	k_sleep(K_SECONDS(2));

	return 0;
}
#endif

int bartendro_dispense(const struct device *uart) {
	int i;
	unsigned char *command = "tickdisp 50 200\r";

	LOG_INF("Sending dispense command");

	for (i = 0; i < strlen(command); i++) {
		uart_poll_out(uart, command[i]);
	}

	LOG_INF("✅ Dispense command sent");

	return 0;
}

int bartendro_init(const struct gpio_dt_spec *reset_pin,
		   const struct device *uart) {
	int ret = 0;

	LOG_INF("🏁 Reseting dispenser");
	ret = bartendro_reset(reset_pin);
	if (ret < 0) {
		LOG_ERR("Failed to reset bartendro");
		return ret;
	}

	LOG_INF("☎️  Making contact with dispenser");
#if defined(CONFIG_APP_USE_TEXT_MODE)
	ret = bartendro_text_mode(uart);
	if (ret < 0) {
		LOG_ERR("Could not enter text mode");
		return ret;
	}
#else
	ret = bartendro_get_id(uart, &id);
	if (ret < 0) {
		LOG_ERR("Failed to get ID");
		return ret;
	}
#endif

	LOG_INF("👍 Dispenser ready!"); 

	return 0;
}
