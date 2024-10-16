// Function to update the displayed slider value
function updateSliderValue() {
    const slider = document.getElementById("valveSlider");
    const display = document.getElementById("sliderValueDisplay");
    display.innerHTML = slider.value + "%";

    // Also update the retro display
    const retroDisplay = document.getElementById("currentValvePosition");
    retroDisplay.innerHTML = slider.value + "%";
    updateRetroDisplayColor(slider.value);
}

// Function to handle button clicks
document.addEventListener("DOMContentLoaded", function () {
    document.querySelector(".close-button").addEventListener("click", function () {
        setSliderValue(0);
    });

    document.querySelector(".open-button").addEventListener("click", function () {
        setSliderValue(100);
    });

    document.querySelector(".set-button").addEventListener("click", function () {
        const sliderValue = document.getElementById("valveSlider").value;
        sendValvePosition(sliderValue);
    });
});

// Function to set slider value programmatically
function setSliderValue(value) {
    const slider = document.getElementById("valveSlider");
    slider.value = value;
    updateSliderValue();  // Update the displayed value
}

// Function to update the retro display color based on the percentage
function updateRetroDisplayColor(value) {
    const retroDisplay = document.getElementById("currentValvePosition");
    const percentage = parseInt(value, 10);
    // Gradient from red to green
    const red = Math.min(255, Math.round((100 - percentage) * 2.55));
    const green = Math.min(255, Math.round(percentage * 2.55));
    retroDisplay.style.color = `rgb(${red}, ${green}, 0)`;
}

// Function to send the slider value to the microcontroller
function sendValvePosition(value) {
    // You could add your code to send the value to the microcontroller here
    console.log(`Sending valve position: ${value}%`);
}
