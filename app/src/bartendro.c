#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/reboot.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(bartendro, LOG_LEVEL_DBG);


#define PACKET_TICK_SPEED_DISPENSE	22
#define RAW_PACKET_SIZE			10


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
		LOG_ERR("Failed to read byte");
		return ret;
	}

	return 0;
}

static int bartendro_crc16(uint8_t *data, size_t data_size, uint16_t *crc) {
	int i, j;

	*crc = 0;

	for (i = 0; i < data_size; i++) {
		*crc ^= data[i];
		for (j = 0; j < 8; j++) {
			if (*crc & 1) {
				*crc = (*crc >> 1) ^ 0xA001;
			}
			else {
				*crc = (*crc >> 1);
			}
		}
	}

	return 0;
}

static void pack_7bit(uint8_t *in, uint8_t in_count,
		uint8_t *out, uint8_t *out_count)
{
	uint16_t buffer = 0;
	uint8_t  bitcount = 0;

	*out_count = 0;
	for(;;)
	{
		if (bitcount < 7)
		{
			buffer <<= 8;
			buffer |= *in++;
			in_count--;
			bitcount += 8;
		}
		*out = (buffer >> (bitcount - 7));
		out++;
		(*out_count)++;

		buffer &= (1 << (bitcount - 7)) - 1;
		bitcount -= 7;

		if (in_count == 0)
			break;
	}
	*out = buffer << (7 - bitcount);
	(*out_count)++;
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
#else
int bartendro_dispense(const struct device *uart) {
	int ret;
	int i;
	uint8_t payload[8] = { id,
			       PACKET_TICK_SPEED_DISPENSE,
			       50, 0,
			       200, 0,
			       0, 0 };
	uint8_t packed[RAW_PACKET_SIZE + 2]; // +2 for the header
	uint8_t packed_size;
	unsigned char ack;
	uint16_t crc;

	bartendro_crc16(payload, 6, &crc);
	payload[6] = (uint8_t)crc;
	payload[7] = (uint8_t)(crc >> 8);

	packed[0] = 0xff;
	packed[1] = 0xff;
	pack_7bit(payload, sizeof(payload), &packed[2], &packed_size);

	LOG_INF("Sending dispense command");

	for (i = 0; i < sizeof(packed); i++) {
		uart_poll_out(uart, packed[i]);
	}

	LOG_INF("✅ Dispense command sent");

	LOG_INF("Reading ACK");

	ret = bartendro_read_byte(uart, &ack);
	if (ret < 0) {
		return ret;
	}

	if (ack != 0) {
		LOG_INF("❌ NoACK");
		return -1;
	}

	LOG_INF("✅ ACK received");

	return 0;
}
#endif

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
