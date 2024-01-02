#include <zephyr/drivers/gpio.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <app_version.h>


#define SUSPEND_CONSOLE		0


static const struct device *const longpress_dev = DEVICE_DT_GET(
	DT_NODELABEL(longpress));

static bool ready = false;


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


	ready = true;

	LOG_INF("🎉 init done 🎉");

#if SUSPEND_CONSOLE
	pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
#endif

	LOG_INF("PM_DEVICE_ACTION_SUSPEND");

	return 0;
}

static void event_handler(struct input_event *evt)
{
	// int ret;

	LOG_INF("GPIO_KEY %s pressed, zephyr_code=%u, value=%d",
		 evt->dev->name, evt->code, evt->value);

	// Do nothing on release
	if (!evt->value) {
		return;
	}

	if (!ready) {
		return;
	}

	// ret = ha_send_trigger_event(&trigger1);
	// if (ret < 0) {
	// 	LOG_ERR("could not send button state");
	// 	// modules/lib/matter/src/platform/nrfconnect/Reboot.cpp
	// 	// zephyr/soc/arm/nordic_nrf/nrf52/soc.c
	// 	sys_reboot(ERROR_BOOT_TOKEN);
	// }
}

INPUT_CALLBACK_DEFINE(longpress_dev, event_handler);
