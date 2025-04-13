/*
 * This file and associated .cpp file are licensed under the GPLv3 License Copyright (c) 2025 Sam Groveman
 * 
 * External libraries needed:
 * SparkFun VEML7700 Arduino Library: https://github.com/sparkfun/SparkFun_VEML7700_Arduino_Library
 * 
 * Contributors: Sam Groveman
 * 
 * TO DO: Add auto gain/integration adjustment per data sheet: https://www.vishay.com/docs/84323/designingveml7700.pdf
 * 
 */

#pragma once
#include <Sensor.h>
#include <Wire.h>
#include <SparkFun_VEML7700_Arduino_Library.h> 
#include <map>

class VEML7700LightSensor : public Sensor {
	public:
		VEML7700LightSensor(String Name, TwoWire* I2C_bus = &Wire, String configFile = "VEML7700LightSensor.json");
		bool begin();
		bool takeMeasurement();
		String getConfig();
		bool setConfig(String config, bool save);
		
	protected:
		/// @brief Translates gain settings to gain values
		std::map<String, VEML7700_sensitivity_mode_t> gains = {{"1x", VEML7700_SENSITIVITY_x1}, {"2x", VEML7700_SENSITIVITY_x2}, {"1/8x", VEML7700_SENSITIVITY_x1_8}, {"1/4x", VEML7700_SENSITIVITY_x1_4}};
		
		/// @brief Translates integration settings to integration values
		std::map<String, VEML7700_integration_time_t> integrations = {{"25ms", VEML7700_INTEGRATION_25ms}, {"50ms", VEML7700_INTEGRATION_50ms}, {"100ms", VEML7700_INTEGRATION_100ms}, {"200", VEML7700_INTEGRATION_200ms}, {"400ms", VEML7700_INTEGRATION_400ms}, {"800ms", VEML7700_INTEGRATION_800ms}};
		
		/// @brief Translates persistence settings to persistence protect values
		std::map<String, VEML7700_persistence_protect_t> persistenc_values = {{"1", VEML7700_PERSISTENCE_1}, {"2", VEML7700_PERSISTENCE_2}, {"4", VEML7700_PERSISTENCE_4},{"8", VEML7700_PERSISTENCE_8}};

		String gain_array[4] = {"1/8x", "1/4x", "1x", "2x"};
		String integration_array[6] = {"25ms", "50ms", "100ms", "200ms", "400ms", "800ms"};


		/// @brief Output configuration
		struct {
			/// @brief Enables automatic sensitivity adjustments
			bool autoAdjust = false;

			/// @brief The sensitivity (gain) to use
			String gain = "1/4x";

			/// @brief The integration time to use
			String integration = "100ms";
			
			/// @brief The persistence protect mode 
			String persistence = "1";

			/// @brief Enables lux compensation calculations
			bool luxCompensation = true;

		} sensor_config;

		/// @brief The bus in use
		TwoWire* I2Cbus;

		/// @brief Path to configuration file
		String config_path;

		/// @brief The sensor
		VEML7700 VEML_sensor;

		void adjustSensitivity();
};