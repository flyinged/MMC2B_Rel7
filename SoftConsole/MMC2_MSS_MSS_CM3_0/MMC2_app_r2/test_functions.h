/*
 * test_functions.h
 *
 *  Created on: 14.10.2015
 *      Author: malatesta_a
 */

#ifndef TEST_FUNCTIONS_H_
#define TEST_FUNCTIONS_H_

void display_version(const uint8_t *sw_version, uint8_t *fw_version, uint8_t *fw_version_x);
void test_status_display(uint8_t user_input);
void test_elapsed_time_counter(uint8_t user_input);
void test_i2c_eeproms(void);
void test_spi_flash(void);
void test_heater(uint8_t user_input);
void test_realtime_clock(uint8_t user_input);
void test_power_meters(uint8_t user_input);
void test_energy_meter(uint8_t user_input);
void test_power_monitoring();
void test_spi_eeprom(void);
void test_temperatures(uint8_t user_input);

void test_fan_read(uint8_t user_input);
void test_display(uint32_t addr, uint8_t *name, uint8_t *unit);

//void TEST_fan_setup(void);
//uint16_t TEST_fan_get_status(uint8_t fc_i2c_addr);
//void TEST_fan_setup_monitors(void);
//void TEST_fan_profile(uint8_t fc_i2c_addr);
//void TEST_fan_set_speed(uint8_t fc_i2c_addr, uint16_t min, uint16_t max, uint16_t step);
//void TEST_fan_speed_stability(uint16_t speed, uint8_t nreads);
//void TEST_fan_get_temp_volt(void);

void TEST_nrg_read_all(void);
void TEST_nrg_init_params(void);

#endif /* TEST_FUNCTIONS_H_ */
