#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <app_version.h>


int main(void)
{
	// const struct device *wdt = DEVICE_DT_GET(DT_NODELABEL(wdt0));
	// const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));

	const struct gpio_dt_spec reset_pin = GPIO_DT_SPEC_GET(
		DT_NODELABEL(bartendro_reset), gpios);
	const struct gpio_dt_spec sync_pin = GPIO_DT_SPEC_GET(
		DT_NODELABEL(bartendro_sync), gpios);

	int ret;

	LOG_INF("Version: %s", APP_VERSION_FULL);

	if (!gpio_is_ready_dt(&reset_pin)) {
		LOG_ERR("LED device %s is not ready", reset_pin.port->name);
		return 0;
	}

	if (!gpio_is_ready_dt(&sync_pin)) {
		LOG_ERR("LED device %s is not ready", sync_pin.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&reset_pin, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("Failed to configure the LED pin, error: %d", ret);
		return 0;
	}

	ret = gpio_pin_configure_dt(&sync_pin, GPIO_OUTPUT_LOW);
	if (ret < 0) {
		LOG_ERR("Failed to configure the LED pin, error: %d", ret);
		return 0;
	}

	gpio_pin_set_dt(&reset_pin, 1);
	if (ret < 0) {
		LOG_ERR("Failed to toggle the LED pin, error: %d", ret);
	}


	LOG_INF("****************************************");
	LOG_INF("MAIN DONE");
	LOG_INF("****************************************");

	// pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);

	LOG_INF("PM_DEVICE_ACTION_SUSPEND");

	return 0;
}
