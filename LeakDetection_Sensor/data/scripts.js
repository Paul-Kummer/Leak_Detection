document.addEventListener('DOMContentLoaded', async () => {
    const tabs = document.querySelectorAll('.tab-link');
    const stepIncrement = document.getElementById('stepIncrement');
    const incrementButton = document.getElementById('incrementButton');
    const decrementButton = document.getElementById('decrementButton');
    const setOpenPositionButton = document.getElementById('setOpenPosition');
    const setClosedPositionButton = document.getElementById('setClosedPosition');
    const settingsForm = document.getElementById('settingsForm');
    const statusMessage = document.getElementById('statusMessage');

    // Fetch and display the IP address
    await fetchIPAddress();

    // Handle tab switching
    tabs.forEach(tab => {
        tab.addEventListener('click', (event) => {
            event.preventDefault();
            window.location.href = tab.getAttribute('href');
        });
    });

    // Load and save settings functionality
    if (settingsForm) {
        // Load settings when form is present
        await loadSettings();

        settingsForm.addEventListener('submit', async (event) => {
            event.preventDefault();  // Prevent page refresh

            const wifiSSID = document.getElementById('wifiSSID');
            const wifiPassword = document.getElementById('wifiPassword');
            const motorSpeed = document.getElementById('motorSpeed');
            const motorAcceleration = document.getElementById('motorAcceleration');
            const stepsToClose = document.getElementById('stepsToClose');
            const webToken = document.getElementById('webToken');

            // Ensure all required elements are available
            if (!wifiSSID || !wifiPassword || !motorSpeed || !motorAcceleration || !stepsToClose || !webToken) {
                console.error('One or more required form elements are missing.');
                return;
            }

            // Collect settings from the form fields
            const settings = {
                wifiSSID: wifiSSID.value,
                wifiPassword: wifiPassword.value,
                motorSpeed: parseInt(motorSpeed.value),
                motorAcceleration: parseInt(motorAcceleration.value),
                stepsToClose: parseInt(stepsToClose.value),
                webToken: webToken.value
            };

            try {
                const response = await fetch('/saveSettings', {
                    method: 'POST',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(settings)
                });

                const result = await response.json();
                displayStatus(result.status === 'success'
                    ? 'Settings saved successfully!'
                    : `Error: ${result.error}`);
            } catch (error) {
                console.error('Failed to save settings:', error);
                displayStatus('Error saving settings.');
            }
        });
    } else {
        console.log("Settings form not found on this page.");
    }

    // Motor control buttons
    if (incrementButton && decrementButton) {
        incrementButton.addEventListener('click', async () => {
            const increment = parseInt(stepIncrement.value) || 0;
            if (increment > 0) {
                await sendMotorCommand('increment', increment);
            } else {
                displayStatus('Invalid increment value.');
            }
        });

        decrementButton.addEventListener('click', async () => {
            const decrement = parseInt(stepIncrement.value) || 0;
            if (decrement > 0) {
                await sendMotorCommand('decrement', decrement);
            } else {
                displayStatus('Invalid decrement value.');
            }
        });
    }

    // Open and Closed Position Logic
    if (setOpenPositionButton && setClosedPositionButton) {
        setOpenPositionButton.addEventListener('click', async () => {
            const success = await setMotorOpenPosition();
            if (success) {
                setClosedPositionButton.disabled = false;
                displayStatus('Open position set. Move motor to closed position.');
            } else {
                displayStatus('Failed to set open position.');
            }
        });

        setClosedPositionButton.addEventListener('click', async () => {
            const success = await setMotorClosedPosition();
            if (success) {
                displayStatus('Closed position set successfully.');
                await saveStepsToClosed();
            } else {
                displayStatus('Failed to set closed position.');
            }
        });
    }

    // Load settings from the server and populate form fields
    async function loadSettings() {
        try {
            const response = await fetch('/getSettings');
            if (!response.ok) throw new Error('Failed to load settings');

            const settings = await response.json();

            // Populate form fields only if elements exist
            const wifiSSID = document.getElementById('wifiSSID');
            const wifiPassword = document.getElementById('wifiPassword');
            const motorSpeed = document.getElementById('motorSpeed');
            const motorAcceleration = document.getElementById('motorAcceleration');
            const stepsToClose = document.getElementById('stepsToClose');
            const webToken = document.getElementById('webToken');

            if (wifiSSID) wifiSSID.value = settings.wifiSSID || '';
            if (wifiPassword) wifiPassword.value = settings.wifiPassword || '';
            if (motorSpeed) motorSpeed.value = settings.motorSpeed || 0;
            if (motorAcceleration) motorAcceleration.value = settings.motorAcceleration || 0;
            if (stepsToClose) stepsToClose.value = settings.stepsToClose || 0;
            if (webToken) webToken.value = settings.webToken || '';
        } catch (error) {
            console.error('Failed to load settings:', error);
            displayStatus('Failed to load settings.');
        }
    }


    // Fetch and display the IP address
    async function fetchIPAddress() {
        try {
            const response = await fetch('/getIPAddress');
            if (!response.ok) throw new Error('Failed to fetch IP address');
            const data = await response.json();
            document.getElementById('ipAddress').textContent = `IP: ${data.ip}`;
        } catch (error) {
            console.error('Failed to fetch IP address:', error);
        }
    }

    // Send motor command via HTTP POST
    async function sendMotorCommand(command, steps) {
        try {
            const response = await fetch('/motorControl', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ command, steps }),
            });

            const result = await response.json();
            displayStatus(result.status === 'success'
                ? `Motor ${command}ed by ${steps} steps.`
                : `Error: ${result.error}`);
        } catch (error) {
            displayStatus('Failed to communicate with motor.');
            console.error(error);
        }
    }

    // Set the open position
    async function setMotorOpenPosition() {
        try {
            const response = await fetch('/setOpenPosition', { method: 'POST' });
            const result = await response.json();
            return result.status === 'success';
        } catch (error) {
            console.error('Failed to set open position:', error);
            return false;
        }
    }

    // Set the closed position
    async function setMotorClosedPosition() {
        try {
            const response = await fetch('/setClosedPosition', { method: 'POST' });
            const result = await response.json();
            return result.status === 'success';
        } catch (error) {
            console.error('Failed to set closed position:', error);
            return false;
        }
    }

    // Fetch the motor's current position and save it to the settings
    async function saveStepsToClosed() {
        try {
            const response = await fetch('/getMotorPosition', { method: 'GET' });
            if (!response.ok) throw new Error(`HTTP error! Status: ${response.status}`);

            const data = await response.json();
            const currentPosition = data.currentPosition;
            console.log(`Fetched motor position: ${currentPosition}`);

            const saveResponse = await fetch('/saveSettings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ stepsToClose: currentPosition })
            });

            const saveResult = await saveResponse.json();
            displayStatus(saveResult.status === 'success'
                ? 'Steps to closed saved successfully.'
                : `Failed to save steps: ${saveResult.error}`);
        } catch (error) {
            console.error('Error saving steps to closed:', error);
            displayStatus('Failed to save steps to closed.');
        }
    }

    // Helper function to display status messages
    function displayStatus(message) {
        if (statusMessage) {
            statusMessage.textContent = message;
        }
    }
});
