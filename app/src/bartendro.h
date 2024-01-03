#ifndef BARTENDRO_H_
#define BARTENDRO_H_

int bartendro_dispense(const struct device *uart);
int bartendro_init(const struct gpio_dt_spec *reset_pin,
				   const struct device *uart);

#endif /* BARTENDRO_H_ */