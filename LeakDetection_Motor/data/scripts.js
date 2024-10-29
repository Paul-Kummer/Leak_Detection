document.addEventListener('DOMContentLoaded', async () => {
    const tabs = document.querySelectorAll('.tab-link');
    const stepIncrement = document.getElementById('stepIncrement');
    const incrementButton = document.getElementById('incrementButton');
    const decrementButton = document.getElementById('decrementButton');
    const setOpenPositionButton = document.getElementById('setOpenPosition');
    const setClosedPositionButton = document.getElementById('setClosedPosition');
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

    // Load the settings from the ESP8266
    try {
        const response = await fetch('/getSettings');
        if (!response.ok) {
            throw new Error('Network response was not ok');
        }
        const settings = await response.json();

        // Populate the form fields with the fetched settings
        document.getElementById('wifiSSID').value = settings.wifiSSID;
        document.getElementById('wifiPassword').value = settings.wifiPassword;
        document.getElementById('motorSpeed').value = settings.motorSpeed;
        document.getElementById('motorAcceleration').value = settings.motorAcceleration;
        document.getElementById('stepsToClose').value = settings.stepsToClose;
        document.getElementById('webToken').value = settings.webToken;
    } catch (error) {
        console.error('Failed to load settings:', error);
        displayStatus('Failed to load settings.');
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

    // Fetch and display the IP address
    async function fetchIPAddress() {
        try {
            const response = await fetch('/getIPAddress');
            if (!response.ok) throw new Error('Failed to fetch IP address');
            const data = await response.json();
            ipAddressElement.textContent = `IP: ${data.ip}`; // Display the IP
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
            if (result.status === 'success') {
                displayStatus(`Motor ${command}ed by ${steps} steps.`);
            } else {
                displayStatus(`Error: ${result.error}`);
            }
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
        // Step 1: Fetch the current motor position from the server
        const response = await fetch('/getMotorPosition', { method: 'GET' });

        if (!response.ok) {
            throw new Error(`HTTP error! Status: ${response.status}`);
        }

        const data = await response.json();
        const currentPosition = data.currentPosition;

        console.log(`Fetched motor position: ${currentPosition}`);

        // Step 2: Save the current position to settings
        const saveResponse = await fetch('/saveSettings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ stepsToClose: currentPosition })
        });

        const saveResult = await saveResponse.json();
        if (saveResult.status === 'success') {
            console.log('Steps to closed saved successfully.');
            displayStatus('Steps to closed saved successfully.');
        } else {
            console.error('Failed to save steps:', saveResult.error);
            displayStatus('Failed to save steps.');
        }
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
