let pollInterval = null;
let fastPolling = false;

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
            const response = await fetch('/getValvePosition');
            if (!response.ok) throw new Error('Failed to fetch valve position');
            const data = await response.json();
            console.log('Current valve position:', data.valvePosition);
            updateValvePositionDisplay(data.valvePosition);

            if (fastPolling && data.valvePosition === parseInt(slider.value)) {
                console.log('Valve reached the desired position');
                stopFastPolling();  // Stop fast polling if goal reached
            }

            lastPosition = data.valvePosition;
        } catch (error) {
            console.error('Error fetching valve position:', error);
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
