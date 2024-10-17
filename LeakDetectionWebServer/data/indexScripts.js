let pollInterval = null;
let timeoutTimer = null;
let lastPosition = -1;  // Track the previous valve position
let timeoutDuration = 5000;  // 5 seconds timeout

document.addEventListener("DOMContentLoaded", function () {
    const slider = document.getElementById("valveSlider");

    slider.addEventListener("input", updateSliderValue);

    document.querySelector(".close-button").addEventListener("click", () => setSliderValue(0));
    document.querySelector(".open-button").addEventListener("click", () => setSliderValue(100));
    document.querySelector(".set-button").addEventListener("click", () => {
        const sliderValue = slider.value;
        sendDesiredPosition(sliderValue);
        startPolling();  // Start polling for updates
    });

    // Stop button logic: Set desired position to current valve position
    document.querySelector(".stop-button").addEventListener("click", () => {
        fetch('/getValvePosition')
            .then(response => response.json())
            .then(data => {
                console.log(`Setting desired position to current: ${data.valvePosition}%`);
                sendDesiredPosition(data.valvePosition);  // Set desired to current
            })
            .catch(error => console.error('Error fetching valve position:', error));
    });
});

function updateSliderValue() {
    const slider = document.getElementById("valveSlider");
    const display = document.getElementById("sliderValueDisplay");
    display.innerHTML = slider.value + "%";
}

function setSliderValue(value) {
    const slider = document.getElementById("valveSlider");
    slider.value = value;
    updateSliderValue();
    sendDesiredPosition(value);
    startPolling();  // Start polling for updates
}

function sendDesiredPosition(value) {
    console.log(`Sending desired position: ${value}%`);

    fetch('/setDesiredPosition', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ desiredPosition: value })
    })
        .then(response => response.json())
        .then(data => console.log('Desired position set:', data))
        .catch(error => console.error('Error:', error));
}

function startPolling() {
    if (pollInterval) clearInterval(pollInterval);  // Clear any existing polling
    if (timeoutTimer) clearTimeout(timeoutTimer);  // Clear any existing timeout

    lastPosition = -1;  // Reset the last known position

    // Start polling every 1 second
    pollInterval = setInterval(() => {
        fetch('/getValvePosition')
            .then(response => response.json())
            .then(data => {
                console.log('Current valve position:', data.valvePosition);
                updateValvePositionDisplay(data.valvePosition);

                // Stop polling if the valve has reached the desired position
                if (data.valvePosition === parseInt(document.getElementById("valveSlider").value)) {
                    console.log('Valve reached the desired position');
                    clearInterval(pollInterval);  // Stop polling
                    clearTimeout(timeoutTimer);  // Stop timeout
                } else if (data.valvePosition === lastPosition) {
                    // If the position hasn't changed, start/restart the timeout
                    restartTimeout();
                } else {
                    // If the position has changed, clear the timeout
                    clearTimeout(timeoutTimer);
                }

                lastPosition = data.valvePosition;  // Update the last known position
            })
            .catch(error => console.error('Error fetching valve position:', error));
    }, 1000);  // Poll every 1 second
}

function updateValvePositionDisplay(position) {
    const retroDisplay = document.getElementById("currentValvePosition");
    retroDisplay.innerHTML = `${position}%`;
}

function restartTimeout() {
    // Clear any existing timeout
    if (timeoutTimer) clearTimeout(timeoutTimer);

    // Set a new timeout for 5 seconds
    timeoutTimer = setTimeout(() => {
        console.log('Timeout: Valve position did not change within 5 seconds');
        clearInterval(pollInterval);  // Stop polling
    }, timeoutDuration);
}
