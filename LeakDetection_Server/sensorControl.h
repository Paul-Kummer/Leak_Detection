#ifndef SENSOR_CONTROL_H
#define SENSOR_CONTROL_H

#include <vector>
#include <string>
#include <deque>
#include <set>
#include <ArduinoJson.h>

struct Sensor {
    std::string id;
    std::string name;
    std::string location;
    std::string timestamp;
    std::string status;
    bool triggered;
    bool ignored;
};

class SensorControl {
public:
    SensorControl(const std::string& jsonFile);
    std::vector<Sensor> getSensors() const;
    bool setupSensors();
    bool saveSensors();
    bool addSensor(const Sensor& sensor); // Add a new sensor
    bool addSavedSensor(const Sensor& sensor);
    bool removeSavedSensor(const std::string& id);
    Sensor* getSensorById(const std::string& id);
    bool getAvailableSensorById(const std::string& id, Sensor& sensor) const;
    void updateSensor(const std::string& id, const std::string& status, bool triggered, const std::string& timestamp);
    void updateSensorCheckInStatus();
    bool addRecentSensor(const Sensor& sensor); // Updated to take a full Sensor object
    std::vector<Sensor> getAvailableSensors() const; // Updated to return Sensor objects
    void checkSensorBroadcast(const StaticJsonDocument<256>& jsonDoc); // Use correct type
    bool addSensorFromAvailable(const std::string& id);
    bool removeAvailableSensor(const std::string& id);
    unsigned long parseISO8601ToEpoch(const std::string& timestamp);
    std::string generateISO8601Timestamp() const;

private:
    std::string jsonFile;
    std::vector<Sensor> sensors;              // List of all sensors
    std::deque<Sensor> recentSensorsQueue;    // Queue to hold recent sensors
    std::set<std::string> recentSensorsSet;   // Set for quick lookup of recent sensors
    bool loadSensorsFromFile();

    void printSensors() const; // For debugging
};

#endif
