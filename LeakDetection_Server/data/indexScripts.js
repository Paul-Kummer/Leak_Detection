let pollInterval = null;
let fastPolling = false;  // Track if fast polling is active
let lastPosition = -1;  // Track the previous valve position
const timeoutDuration = 5000;  // 5 seconds timeout

document.addEventListener("DOMContentLoaded", function () {
    const slider = document.getElementById("valveSlider");

    slider.addEventListener("input", updateSliderValue);

    document.querySelector(".close-button").addEventListener("click", () => setSliderValue(0));
    document.querySelector(".open-button").addEventListener("click", () => setSliderValue(100));
    document.querySelector(".set-button").addEventListener("click", () => {
        const sliderValue = slider.value;
        sendDesiredPosition(sliderValue);
        startPolling(true);  // Start fast polling
    });

    document.querySelector(".stop-button").addEventListener("click", () => {
        fetch('/getValvePosition')
            .then(response => response.json())
            .then(data => {
                console.log(`Setting desired position to current: ${data.valvePosition}%`);
                sendDesiredPosition(data.valvePosition);
            })
            .catch(error => console.error('Error fetching valve position:', error));
    });

    // Start regular polling on page load
    startPolling(false);
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
    startPolling(true);  // Start fast polling
}

function sendDesiredPosition(value) {
    console.log(`Sending desired position: ${value}%`);

    fetch('/controlValve', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: new URLSearchParams({ desiredPosition: value })
    })
        .then(response => {
            if (!response.ok) {
                throw new Error(`Server responded with status: ${response.status}`);
            }
            return response.json();
        })
        .then(data => console.log('Desired position set:', data))
        .catch(error => console.error('Error sending desired position:', error));
}

function startPolling(fast = false) {
    if (pollInterval) clearInterval(pollInterval);  // Clear any existing polling

    fastPolling = fast;  // Set polling mode
    const pollingRate = fast ? 500 : 10000;  // 500ms for fast, 10s for regular

    pollInterval = setInterval(() => {
        fetch('/getValvePosition')
            .then(response => response.json())
            .then(data => {
                console.log('Current valve position:', data.valvePosition);
                updateValvePositionDisplay(data.valvePosition);

                if (fastPolling && data.valvePosition === parseInt(document.getElementById("valveSlider").value)) {
                    console.log('Valve reached the desired position');
                    stopFastPolling();  // Stop fast polling if goal reached
                }

                lastPosition = data.valvePosition;
            })
            .catch(error => console.error('Error fetching valve position:', error));
    }, pollingRate);
}

function updateValvePositionDisplay(position) {
    const retroDisplay = document.getElementById("currentValvePosition");
    retroDisplay.innerHTML = `${position}%`;
}

function stopFastPolling() {
    fastPolling = false;
    startPolling(false);  // Switch to regular polling
}
