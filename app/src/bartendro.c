#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bartendro, LOG_LEVEL_DBG);


#define PROMPT_CHAR_LENGTH	47


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

static int bartendro_text_mode(const struct device *uart) {
	int ret;
	int i;
	unsigned char rxdata;
	// unsigned char *prompt = "\r\nParty Robotics Dispenser at your service!\r\n\r\n>";

	LOG_INF("Waiting for prompt");

	for (i = 0; i < PROMPT_CHAR_LENGTH; i++) {
retry:
		ret = uart_poll_in(uart, &rxdata);
		if (ret == -1) {
			k_sleep(K_USEC(100));
			goto retry;
		}
		else if (ret < 0) {
			return ret;
		}
	}

	LOG_INF("✅ Prompt received");

	for (i = 0; i < 3; i++) {
		uart_poll_out(uart, '!');
	}

	k_sleep(K_SECONDS(2));

	return 0;
}

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

	ret = bartendro_reset(reset_pin);
	if (ret < 0) {
		LOG_ERR("Failed to reset bartendro");
		return ret;
	}

	LOG_INF("☎️  Making contact with dispenser");
	ret = bartendro_text_mode(uart);
	if (ret < 0) {
		LOG_ERR("Could not enter text mode");
		return ret;
	}

	LOG_INF("👍 Dispenser ready!"); 

	return 0;
}
