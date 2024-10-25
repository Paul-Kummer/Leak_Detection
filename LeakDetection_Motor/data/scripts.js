document.addEventListener('DOMContentLoaded', () => {
    const tabs = document.querySelectorAll('.tab-link');
    const stepIncrement = document.getElementById('stepIncrement');
    const incrementButton = document.getElementById('incrementButton');
    const decrementButton = document.getElementById('decrementButton');
    const setOpenPositionButton = document.getElementById('setOpenPosition');
    const setClosedPositionButton = document.getElementById('setClosedPosition');
    const statusMessage = document.getElementById('statusMessage');

    // Handle tab switching
    tabs.forEach(tab => {
        tab.addEventListener('click', (event) => {
            event.preventDefault();
            window.location.href = tab.getAttribute('href');
        });
    });

    // Motor control buttons (if present)
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

    // Open and Closed Position Logic (if present)
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

    // Send motor command via HTTP POST
    async function sendMotorCommand(command, steps) {
        try {
            const response = await fetch('/motorControl', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ command, steps })
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

    // Save the steps to closed in settings
    async function saveStepsToClosed() {
        try {
            const response = await fetch('/saveSettings', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ stepsToClose: motor.currentPosition() })
            });

            const result = await response.json();
            if (result.status === 'success') {
                displayStatus('Steps to closed saved successfully.');
            } else {
                displayStatus(`Error saving steps: ${result.error}`);
            }
        } catch (error) {
            displayStatus('Failed to save settings.');
            console.error(error);
        }
    }

    // Helper function to display status messages
    function displayStatus(message) {
        if (statusMessage) {
            statusMessage.textContent = message;
        }
    }
});
