#include <hal/nrf_power.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/init.h>
#include <zephyr/input/input.h>
#include <zephyr/kernel.h>
#include <zephyr/pm/device.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_DBG);

#include <app_version.h>


#define SUSPEND_CONSOLE			0
#define DISPENSE_REQUIRED_EVENT		BIT(0)
#define DISPENSE_ERROR_EVENT		BIT(1)


static const struct device *const longpress_dev = DEVICE_DT_GET(
	DT_NODELABEL(longpress));

static bool ready = false;

static K_EVENT_DEFINE(dispense_events);


static int change_output_voltage(void)
{
	if ((nrf_power_mainregstatus_get(NRF_POWER) ==
	     NRF_POWER_MAINREGSTATUS_HIGH) &&
	    ((NRF_UICR->REGOUT0 & UICR_REGOUT0_VOUT_Msk) ==
	     (UICR_REGOUT0_VOUT_DEFAULT << UICR_REGOUT0_VOUT_Pos))) {

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}

		NRF_UICR->REGOUT0 =
		    (NRF_UICR->REGOUT0 & ~((uint32_t)UICR_REGOUT0_VOUT_Msk)) |
		    (UICR_REGOUT0_VOUT_3V3 << UICR_REGOUT0_VOUT_Pos);

		NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
		while (NRF_NVMC->READY == NVMC_READY_READY_Busy) {
			;
		}

		/* a reset is required for changes to take effect */
		NVIC_SystemReset();
	}

	return 0;
}

SYS_INIT(change_output_voltage, PRE_KERNEL_1,
	 CONFIG_KERNEL_INIT_PRIORITY_DEFAULT);

static void beep(const struct pwm_dt_spec *buzzer)
{
	pwm_set_dt(buzzer, PWM_USEC(250U), PWM_USEC(250U) * 0.53);
	k_sleep(K_MSEC(50));
	pwm_set_dt(buzzer, PWM_USEC(250U), 0);
	k_sleep(K_MSEC(50));
}

static void beeps(const struct pwm_dt_spec *buzzer, int number) {
	int i;

	for (i = 0; i < number; i++) {
		beep(buzzer);
	}
}

int main(void)
{
	// const struct device *wdt = DEVICE_DT_GET(DT_NODELABEL(wdt0));
#if SUSPEND_CONSOLE
	const struct device *cons = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
#endif
	const struct pwm_dt_spec buzzer = PWM_DT_SPEC_GET(
		DT_NODELABEL(buzzer));

	const struct gpio_dt_spec reset_pin = GPIO_DT_SPEC_GET(
		DT_NODELABEL(bartendro_reset), gpios);
	const struct gpio_dt_spec sync_pin = GPIO_DT_SPEC_GET(
		DT_NODELABEL(bartendro_sync), gpios);

	int ret;
	uint32_t events;

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

	beeps(&buzzer, 2);
	LOG_INF("🎉 init done 🎉");

#if SUSPEND_CONSOLE
	pm_device_action_run(cons, PM_DEVICE_ACTION_SUSPEND);
#endif

	while (1) {
		events = k_event_wait(&dispense_events,
					(DISPENSE_REQUIRED_EVENT | 
					 DISPENSE_ERROR_EVENT),
					true, K_FOREVER);

		if (events & DISPENSE_REQUIRED_EVENT) {
			LOG_INF("🧼 Dispensing");
			beeps(&buzzer, 1);
		}
		else if (events & DISPENSE_ERROR_EVENT) {
			beeps(&buzzer, 3);
		}
	}

	return 0;
}

static void event_handler(struct input_event *evt)
{
	// int ret;

	LOG_INF("GPIO_KEY %s pressed, zephyr_code=%u, value=%d",
		 evt->dev->name, evt->code, evt->value);

	if (!ready) {
		return;
	}

	if (evt->code == INPUT_KEY_X && evt->value) {
		LOG_INF("🛎️  We need to dispense detergent");
		k_event_post(&dispense_events, DISPENSE_REQUIRED_EVENT);
	}
	else if (evt->code == INPUT_KEY_A && !evt->value) {
		LOG_INF("❌ Button short press");
		k_event_post(&dispense_events, DISPENSE_ERROR_EVENT);
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
