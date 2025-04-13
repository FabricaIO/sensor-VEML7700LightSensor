#include "VEML7700LightSensor.h"

/// @brief Creates a new multi environmental sensor object
/// @param Name The device name
/// @param I2C_bus The I2C bus attached to the sensor
/// @param configFile Name of the config file to use
VEML7700LightSensor::VEML7700LightSensor(String Name, TwoWire* I2C_bus, String configFile) : Sensor(Name) {
	I2Cbus = I2C_bus;
	config_path = "/settings/sen/" + configFile;
}

/// @brief Starts the environmental sensor
/// @return True on success
bool VEML7700LightSensor::begin() {
	Description.parameterQuantity = 3;
	Description.type = "Ambient Light Sensor";
	Description.parameters = {"Ambient Light", "White Level", "Ambient Lux"};
	Description.units = {"raw", "raw", "lx"};
	values.resize(Description.parameterQuantity);
	I2Cbus->begin();
	if (VEML_sensor.begin(*I2Cbus)) {
		if (!checkConfig(config_path)) {
			return setConfig(getConfig(), true);
		} else {
			return setConfig(Storage::readFile(config_path), false);
		}
	}
	Logger.println("Could not initialize VEML7700");
	return false;
}

/// @brief Takes a measurement
/// @return True on success
bool VEML7700LightSensor::takeMeasurement() {
	if (sensor_config.autoAdjust) {
		adjustSensitivity();
	}
	values[0] = VEML_sensor.getAmbientLight();
	values[1] = VEML_sensor.getWhiteLevel();
	double lux = VEML_sensor.getLux();
	if (sensor_config.luxCompensation && (lux > 1000 || sensor_config.gain == "1/8x" || sensor_config.gain == "1/4x")) {
		lux = (((6.0135e-13 * lux - 9.3924e-9) * lux + 8.1488e-5) * lux + 1.0023) * lux; 
	}
	values[2] = lux;
	return true;
}

/// @brief Adjusts the sensitivity of the sensor
/// @link https://www.vishay.com/docs/84323/designingveml7700.pdf
/// @note This is not a fast proces especially changing to low light conditions can take several seconds. This could be improved.
void VEML7700LightSensor::adjustSensitivity() {
	int max = 10000;
	int min = 100;
    uint16_t ambient = VEML_sensor.getAmbientLight();
	int newIntegration = (int)VEML_sensor.getIntegrationTime();
    int newGain = (int)VEML_sensor.getSensitivityMode();
	bool maxed = false;
	if (newGain < 2) {
		newGain += 2;
	} else 
	{
		newGain -= 2;
	}
	// Check if adjustment is needed
	if (ambient <= min || ambient > max) {
		// Check if already maxed out
		if ((ambient > max && newIntegration == 0 && newGain == 0) || (ambient <= min && newIntegration == 5 && newGain == 3)) {
			maxed = true;
		} else {
			// Sensor must be shut down to change settings
			VEML_sensor.shutdown();
			// Wait one integration period to ensure shutdown is complete
			delay(25 * pow(2, newIntegration));
			// Reset integration value to allow proper tuning of sensitivity, or see if that just fixes it
			newIntegration = 2;
			VEML_sensor.setIntegrationTime(integrations[integration_array[newIntegration]]);
			VEML_sensor.powerOn();
			// Wait two integration period for reading to stabilize
			delay(25 * pow(2, newIntegration + 1));
			ambient = VEML_sensor.getAmbientLight();
		}
	}
	while (!maxed && (ambient <= min || ambient > max)) {
		// Reading too low
		if (ambient <= min) {
			do {
				if (newIntegration == 5 && newGain == 3) {
					maxed = true;
					break;
				}
				VEML_sensor.shutdown();
				delay(25 * pow(2, newIntegration));
				if (newGain < 3) {
					newGain++;
					VEML_sensor.setSensitivityMode(gains[gain_array[newGain]]);
				} else if (newIntegration < 5){
					newIntegration++;
					VEML_sensor.setIntegrationTime(integrations[integration_array[newIntegration]]);
				}
				VEML_sensor.powerOn();
				delay(25 * pow(2, newIntegration + 1));
				ambient = VEML_sensor.getAmbientLight();
			} while (ambient <= min);
		// Reading too high
		} else {
			do {
				if (newIntegration == 0 && newGain == 0) {
					maxed = true;
					break;
				}
				VEML_sensor.shutdown();
				delay(25 * pow(2, newIntegration));
				if (newGain > 0) {
					newGain--;
					VEML_sensor.setSensitivityMode(gains[gain_array[newGain]]);
				} else if (newIntegration > 0) {
					newIntegration--;
					VEML_sensor.setIntegrationTime(integrations[integration_array[newIntegration]]);
				}
				VEML_sensor.powerOn();
				delay(25 * pow(2, newIntegration + 1));
				ambient = VEML_sensor.getAmbientLight();
			} while (ambient > max);
		} 
	}
	// Update settings
	sensor_config.integration = integration_array[newIntegration];
	sensor_config.gain = gain_array[newGain];
}

/// @brief Gets the current config
/// @return A JSON string of the config
String VEML7700LightSensor::getConfig() {
	// Allocate the JSON document
	JsonDocument doc;
	// Assign current values
	doc["Name"] = Description.name;
	doc["autoAdjust"] = sensor_config.autoAdjust;
	doc["Gain"]["current"] = sensor_config.gain;
	doc["Gain"]["options"][0] = "1/8x";
	doc["Gain"]["options"][1] = "1/4x";
	doc["Gain"]["options"][2] = "1x";
	doc["Gain"]["options"][3] = "2x";
	doc["Integration"]["current"] = sensor_config.integration;
	doc["Integration"]["options"][0] = "25ms";
	doc["Integration"]["options"][1] = "50ms";
	doc["Integration"]["options"][2] = "100ms";
	doc["Integration"]["options"][3] = "200ms";
	doc["Integration"]["options"][4] = "400ms";
	doc["Integration"]["options"][5] = "800ms";
	doc["Persistence"]["current"] = sensor_config.persistence;
	doc["Persistence"]["options"][0] = "1";
	doc["Persistence"]["options"][2] = "2";
	doc["Persistence"]["options"][2] = "4";
	doc["Persistence"]["options"][3] = "8";
	doc["luxCompensation"] = sensor_config.luxCompensation;

	// Create string to hold output
	String output;
	// Serialize to string
	serializeJson(doc, output);
	return output;
}

/// @brief Sets the configuration for this device
/// @param config A JSON string of the configuration settings
/// @param save If the configuration should be saved to a file
/// @return True on success
bool VEML7700LightSensor::setConfig(String config, bool save) {
	// Allocate the JSON document
  	JsonDocument doc;
	// Deserialize file contents
	DeserializationError error = deserializeJson(doc, config);
	// Test if parsing succeeds.
	if (error) {
		Logger.print(F("Deserialization failed: "));
		Logger.println(error.f_str());
		return false;
	}
	// Assign loaded values
	Description.name = doc["Name"].as<String>();
	sensor_config.autoAdjust = doc["autoAdjust"].as<bool>();
	sensor_config.luxCompensation = doc["luxCompensation"].as<bool>();
	
	if (!sensor_config.autoAdjust) {

		sensor_config.gain = doc["Gain"]["current"].as<String>();
		sensor_config.integration = doc["Integration"]["current"].as<String>();
		sensor_config.persistence = doc["Persistence"]["current"].as<String>();
		// Apply settings
		if (VEML_sensor.setSensitivityMode(gains[sensor_config.gain]) != VEML7700_ERROR_SUCCESS) {
			Logger.println("Could not set gain value");
			return false;
		}
		if (VEML_sensor.setIntegrationTime(integrations[sensor_config.integration]) != VEML7700_ERROR_SUCCESS) {
			Logger.println("Could not set integration value");
			return false;
		}
		if (VEML_sensor.setPersistenceProtect(persistenc_values[sensor_config.persistence]) != VEML7700_ERROR_SUCCESS) {
			Logger.println("Could not set persistence protect value");
			return false;
		}
	}
	
	if (save) {
		if (!saveConfig(config_path, getConfig())) {
			return false;
		}
	}
	return true;
}