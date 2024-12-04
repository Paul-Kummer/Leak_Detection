let pollInterval = null;
let fastPolling = false;
const statusToAlert = ["Alert", "No Check-In"]; // Values that will cause a sensor to be in the alert table
let loadSensorTablesTimeout;


document.addEventListener("DOMContentLoaded", async function () {
    console.log("DOM fully loaded and parsed.");

    // Check for and handle the slider, if it exists
    const slider = document.getElementById("valveSlider");
    if (slider) {
        console.log("Slider detected, adding event listeners.");
        slider.addEventListener("input", updateSliderValue);

        document.querySelector(".set-button")?.addEventListener("click", () => {
            setSliderValue(parseInt(slider.value, 10));
        });
    } else {
        console.warn("Slider not found, skipping slider setup.");
    }

    // Check for and handle the settings form, if it exists
    const form = document.getElementById('settingsForm');
    if (form) {
        console.log("Settings form detected, adding event listener.");

        await loadSettings();  // Load settings if the form exists

        form.addEventListener('submit', async (event) => {
            event.preventDefault();  // Prevent page refresh
            console.log("Saving settings...");
            await saveSettings();
        });
    } else {
        console.warn("Settings form not found, skipping form setup.");
    }

    // Add event listeners for buttons, if they exist
    document.querySelector(".close-button")?.addEventListener("click", () => setSliderValue(0));
    document.querySelector(".open-button")?.addEventListener("click", () => setSliderValue(100));

    document.querySelector(".stop-button")?.addEventListener("click", async () => {
        try {
            const response = await fetch('/getValvePosition');
            if (!response.ok) throw new Error('Failed to fetch valve position');
            const data = await response.json();
            console.log(`Setting desired position to current: ${data.valvePosition}%`);
            setSliderValue(data.valvePosition)
        } catch (error) {
            console.error('Error fetching valve position:', error);
        }
    });

    // Fetch and display the IP address, if the element exists
    const ipAddressElement = document.getElementById('ipAddress');
    if (ipAddressElement) {
        console.log("Fetching IP address...");
        await fetchIPAddress();
    } else {
        console.warn("IP address element not found.");
    }

    // Fetch and display the IP address, if the element exists
    const alertTableElement = document.getElementById('alertTable');
    const monitorTableElement = document.getElementById('monitoredTable');
    if (alertTableElement && monitorTableElement) {
        console.log("Loading sensor tables...");
        await loadSensorData();
    } else {
        console.warn("Sensor table elements not found.");
    }

    const savedSensorsTableElement  = document.getElementById('savedSensorsTable');
    const availableSensorsTableElement  = document.getElementById('availableSensorsTable');
    if (savedSensorsTableElement && availableSensorsTableElement) {
        console.log("Loading saved and available sensor tables...");
        await loadSensorTables();
    } else {
        console.warn("Sensor table elements not found.");
    }

    // Start regular polling only if a slider exists
    if (slider) startPolling(false);
});

function updateSliderValue() {
    const slider = document.getElementById("valveSlider");
    const display = document.getElementById("sliderValueDisplay");
    if (slider && display) {
        display.innerHTML = `${slider.value}%`;
    }
}

function setSliderValue(value) {
    const slider = document.getElementById("valveSlider");
    if (slider) {
        slider.value = value;
        console.log(`Setting slider to: ${value}%`);
        updateSliderValue();
        sendDesiredPosition(value);
        startPolling(true);  // Start fast polling if needed
    } else {
        console.warn("Slider element not found.");
    }
}

function updateSliderValue() {
    const slider = document.getElementById("valveSlider");
    const display = document.getElementById("sliderValueDisplay");
    if (slider && display) {
        display.textContent = `${slider.value}%`;
        console.log(`Slider display updated to: ${slider.value}%`);
    }
}

async function sendDesiredPosition(value) {
    console.log(`Sending desired position: ${value}%`);

    try {
        const response = await fetch('/controlValve', {
            method: 'POST',
            headers: {
                'Content-Type': 'application/json',
                'Authorization': 'LeakDetection_Motor'
            },
            body: JSON.stringify({ desiredPosition: value })
        });

        if (!response.ok) {
            throw new Error(`Server responded with status: ${response.status}`);
        }

        const data = await response.json();
        console.log('Desired position set:', data);
    } catch (error) {
        console.error('Error sending desired position:', error);
    }
}


function startPolling(fast = false) {
    const slider = document.getElementById("valveSlider");
    if (!slider) {
        console.warn("Slider not found, polling skipped.");
        return;
    }

    if (pollInterval) clearInterval(pollInterval);  // Clear any existing polling

    fastPolling = fast;  // Set polling mode
    const pollingRate = fast ? 500 : 10000;  // 500ms for fast, 10s for regular

    pollInterval = setInterval(async () => {
        try {
            // Fetch valve position
            const valveResponse = await fetch('/getValvePosition');
            if (!valveResponse.ok) throw new Error('Failed to fetch valve position');
            const valveData = await valveResponse.json();
            console.log('Current valve position:', valveData.valvePosition);
            updateValvePositionDisplay(valveData.valvePosition);

            // Fetch saved sensors
            const sensorsResponse = await fetch('/getSavedSensors');
            if (!sensorsResponse.ok) throw new Error('Failed to fetch saved sensors');
            const sensorsData = await sensorsResponse.json();
            console.log('Updated sensor data:', sensorsData);

            // Update sensor tables
            populateAlertTable(sensorsData);
            populateMonitoredTable(sensorsData);

            if (fastPolling && valveData.valvePosition === parseInt(slider.value)) {
                console.log('Valve reached the desired position');
                stopFastPolling();  // Stop fast polling if goal reached
            }

        } catch (error) {
            console.error('Error during polling:', error);
        }
    }, pollingRate);
}


function updateValvePositionDisplay(position) {
    const retroDisplay = document.getElementById("currentValvePosition");
    if (retroDisplay) {
        retroDisplay.innerHTML = `${position}%`;
    }
}

function stopFastPolling() {
    fastPolling = false;
    startPolling(false);  // Switch to regular polling
}

async function fetchIPAddress() {
    try {
        const response = await fetch('/getIPAddress');
        if (!response.ok) throw new Error('Failed to fetch IP address');
        const data = await response.json();
        const ipAddressElement = document.getElementById('ipAddress');
        if (ipAddressElement) {
            ipAddressElement.textContent = `IP: ${data.ip}`;
        }
    } catch (error) {
        console.error('Failed to fetch IP address:', error);
    }
}

// Fetch sensors and populate tables
// Fetch all saved sensors and populate tables
async function loadSensorData() {
    try {
        // Fetch all saved sensors
        const response = await fetch('/getSavedSensors');
        if (!response.ok) throw new Error('Failed to fetch saved sensors');
        const sensors = await response.json();

        // Parse and distribute sensors into appropriate tables
        populateAlertTable(sensors);
        populateMonitoredTable(sensors);
    } catch (error) {
        console.error('Error loading sensor data:', error);
    }
}


// Populate the alerts table
function populateAlertTable(sensors) {
    const alertTableBody = document.getElementById("alertTable").querySelector("tbody");
    alertTableBody.innerHTML = ""; // Clear the table

    sensors.forEach((sensor) => {
        // Check if the sensor status matches any entry in statusToAlert (case-insensitive)
        // and the sensor is not ignored
        const isInAlert =
            sensor.Status &&
            !sensor.Ignored && // Exclude ignored sensors
            statusToAlert.some((alertStatus) =>
                sensor.Status.toLowerCase().includes(alertStatus.toLowerCase())
            );

        if (isInAlert) {
            const row = document.createElement("tr");

            row.innerHTML = `
                <td>${sensor.Timestamp}</td>
                <td>${sensor.Name}</td>
                <td>${sensor.Location}</td>
                <td>${sensor.ID}</td>
                <td>${sensor.Status}</td>
                <td>${sensor.Triggered ? "Yes" : "No"}</td>
                <td>
                    <button class="ignored-button" 
                            style="background-color: ${sensor.Ignored ? "red" : "green"}" 
                            onclick="toggleIgnored('${sensor.ID}', ${sensor.Ignored})">
                        ${sensor.Ignored ? "Ignoring" : "Monitoring"}
                    </button>
                </td>
            `;

            alertTableBody.appendChild(row);
        }
    });
}

// Populate the monitored sensors table
function populateMonitoredTable(sensors) {
    const monitoredTableBody = document.getElementById("monitoredTable").querySelector("tbody");
    monitoredTableBody.innerHTML = ""; // Clear the table

    sensors.forEach((sensor) => {
        // Criteria for being in the monitored table
        const isNotInAlertList =
            !statusToAlert.some((alertStatus) =>
                sensor.Status.toLowerCase().includes(alertStatus.toLowerCase())
            );
        const isInAlertButIgnored =
            sensor.Ignored &&
            statusToAlert.some((alertStatus) =>
                sensor.Status.toLowerCase().includes(alertStatus.toLowerCase())
            );

        if (isNotInAlertList || isInAlertButIgnored) {
            const row = document.createElement("tr");

            row.innerHTML = `
                <td>${sensor.Timestamp}</td>
                <td>${sensor.Name}</td>
                <td>${sensor.Location}</td>
                <td>${sensor.ID}</td>
                <td>${sensor.Status}</td>
                <td>${sensor.Triggered ? "Yes" : "No"}</td>
                <td>
                    <button class="ignored-button" 
                            style="background-color: ${sensor.Ignored ? "red" : "green"}" 
                            onclick="toggleIgnored('${sensor.ID}', ${sensor.Ignored})">
                        ${sensor.Ignored ? "Ignoring" : "Monitoring"}
                    </button>
                </td>
            `;

            monitoredTableBody.appendChild(row);
        }
    });
}


// Populate the saved sensors table
function populateSavedSensorsTable(sensors) {
    const savedTableBody = document.getElementById("savedSensorsTable").querySelector("tbody");
    savedTableBody.innerHTML = ""; // Clear the table

    if (!Array.isArray(sensors)) {
        console.error("Invalid data for saved sensors:", sensors);
        return;
    }

    sensors.forEach((sensor) => {
        const row = document.createElement("tr");

        row.innerHTML = `
            <td><button class="remove-button" onclick="removeSensor('${sensor.ID}')">Remove</button></td>
            <td>${sensor.Timestamp}</td>
            <td>${sensor.Name}</td>
            <td>${sensor.Location}</td>
            <td>${sensor.ID}</td>
            <td>${sensor.Status}</td>
            <td>${sensor.Triggered ? "Yes" : "No"}</td>
        `;

        savedTableBody.appendChild(row);
    });
}


// Populate the available sensors table
function populateAvailableSensorsTable(sensors) {
    const availableTableBody = document.getElementById("availableSensorsTable").querySelector("tbody");
    availableTableBody.innerHTML = ""; // Clear the table

    if (!Array.isArray(sensors) || sensors.length === 0) {
        addEmptyRow(availableTableBody, "No available sensors found.");
        return;
    }

    sensors.forEach((sensor) => {
        const row = document.createElement("tr");

        row.innerHTML = `
            <td><button class="add-button" onclick="addSensor('${sensor.ID}')">Add</button></td>
            <td>${sensor.Timestamp || "N/A"}</td>
            <td>${sensor.Name || "Unnamed"}</td>
            <td>${sensor.Location || "Unknown"}</td>
            <td>${sensor.ID || "N/A"}</td>
            <td>${sensor.Status || "Unknown"}</td>
            <td>${sensor.Triggered ? "Yes" : "No"}</td>
        `;

        availableTableBody.appendChild(row);
    });
}

// Toggle the ignored status of a sensor
async function toggleIgnored(sensorID, currentStatus) {
    const newStatus = !currentStatus;

    try {
        const response = await fetch("/updateIgnoredStatus", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ ID: sensorID, Ignored: newStatus }),
        });

        if (!response.ok) throw new Error("Failed to update ignored status");
        console.log(`Sensor ${sensorID} ignored status updated to ${newStatus}`);

        // Refresh both tables to reflect the updated data
        await loadSensorData();
    } catch (error) {
        console.error("Error updating ignored status:", error);
    }
}

// Always have a row for data
function addEmptyRow(tableBody, message) {
    const row = document.createElement("tr");
    const cell = document.createElement("td");
    cell.colSpan = 7; // Adjust based on the number of columns
    cell.textContent = message;
    cell.style.textAlign = "center";
    row.appendChild(cell);
    tableBody.appendChild(row);
}

function debounceLoadSensorTables() {
    // Clear the existing timeout if it exists
    if (typeof loadSensorTablesTimeout !== "undefined") {
        clearTimeout(loadSensorTablesTimeout);
    }

    // Set a new timeout
    loadSensorTablesTimeout = setTimeout(() => {
        loadSensorTables(); 
    }, 500); // Adjust debounce delay as needed
}


// Add a sensor to saved sensors
async function addSensor(sensorID) {
    try {
        const response = await fetch("/handleAddSensor", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ ID: sensorID }),
        });

        if (!response.ok) throw new Error(`Failed to add sensor: ${response.statusText}`);
        const data = await response.json();
        console.log(`Sensor ${sensorID} added to saved sensors.`, data);

        // Refresh the tables
        await loadSensorTables();
    } catch (error) {
        console.error("Error adding sensor:", error);
    }
}

// Remove a sensor from saved sensors
async function removeSensor(sensorID) {
    try {
        const response = await fetch("/handleRemoveSensor", {
            method: "POST",
            headers: { "Content-Type": "application/json" },
            body: JSON.stringify({ ID: sensorID }),
        });

        if (!response.ok) throw new Error(`Failed to remove sensor: ${response.statusText}`);
        const data = await response.json();
        console.log(`Sensor ${sensorID} removed from saved sensors.`, data);

        // Refresh the tables
        await loadSensorTables();
    } catch (error) {
        console.error("Error removing sensor:", error);
    }
}

async function loadSensorTables() {
    try {
        // Fetch saved sensors
        const savedSensorsResponse = await fetch("/getSavedSensors");
        if (!savedSensorsResponse.ok) {
            throw new Error("Failed to fetch saved sensors");
        }
        const savedSensors = await savedSensorsResponse.json();

        // Fetch available sensors
        const availableSensorsResponse = await fetch("/getAvailableSensors");
        if (!availableSensorsResponse.ok) {
            throw new Error("Failed to fetch available sensors");
        }
        const availableSensors = await availableSensorsResponse.json();

        // Debug responses
        console.log("Saved Sensors:", savedSensors);
        console.log("Available Sensors:", availableSensors);

        // Update tables
        populateSavedSensorsTable(savedSensors);
        populateAvailableSensorsTable(availableSensors);

        console.log("Sensor tables updated.");
    } catch (error) {
        console.error("Error loading sensor tables:", error);
    }
}

function updateTable(tableId, sensorData) {
    const table = document.getElementById(tableId);
    if (!table) {
        console.warn(`Table with ID '${tableId}' not found.`);
        return;
    }

    const tbody = table.querySelector("tbody");
    if (!tbody) {
        console.warn(`Table '${tableId}' is missing a <tbody> element.`);
        return;
    }

    // Clear the table body
    tbody.innerHTML = "";

    // Add rows for each sensor
    if (!Array.isArray(sensorData) || sensorData.length === 0) {
        const row = tbody.insertRow();
        const cell = row.insertCell(0);
        cell.colSpan = table.querySelectorAll("thead th").length; // Dynamically set colspan
        cell.textContent = "No sensors found.";
        cell.style.textAlign = "center";
        return;
    }

    sensorData.forEach((sensor) => {
        const row = tbody.insertRow();
        row.insertCell(0).textContent = sensor.ID || "N/A";
        row.insertCell(1).textContent = sensor.Name || "Unnamed";
        row.insertCell(2).textContent = sensor.Location || "Unknown";
        row.insertCell(3).textContent = sensor.Status || "Unknown";
        row.insertCell(4).textContent = sensor.Ignored ? "Yes" : "No";
    });
}

async function loadSettings() {
    try {
        const response = await fetch('/settings.json');
        if (!response.ok) throw new Error('Failed to load settings');
        const settings = await response.json();

        // Null checks instead of optional chaining for older browsers
        const ssidElement = document.getElementById('wifiSSID');
        if (ssidElement) ssidElement.value = settings.wifiSSID || "";

        const passwordElement = document.getElementById('wifiPassword');
        if (passwordElement) passwordElement.value = settings.wifiPassword || "";

        const tokenElement = document.getElementById('webToken');
        if (tokenElement) tokenElement.value = settings.webToken || "";

        const motorIPElement = document.getElementById('motorIP');
        if (motorIPElement) motorIPElement.value = settings.motorIP || "";
    } catch (error) {
        console.error('Error loading settings:', error);
    }
}

async function saveSettings() {
    const settings = {
        wifiSSID: document.getElementById('wifiSSID')?.value || "",
        wifiPassword: document.getElementById('wifiPassword')?.value || "",
        webToken: document.getElementById('webToken')?.value || "",
        motorIP: document.getElementById('motorIP')?.value || ""
    };

    try {
        const response = await fetch('/saveSettings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify(settings)
        });

        if (response.ok) {
            document.getElementById('statusMessage').textContent = 'Settings saved successfully!';
        } else {
            throw new Error('Failed to save settings');
        }
    } catch (error) {
        console.error('Error saving settings:', error);
        document.getElementById('statusMessage').textContent = 'Error saving settings.';
    }
}