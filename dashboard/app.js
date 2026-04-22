// MQTT Config
const broker = 'wss://broker.hivemq.com:8884/mqtt'; // Secure wss is best for browsers
const dataTopic = 'water/quality/data';
const controlTopic = 'water/quality/control';

let client;

// UI Elements
const statusIndicator = document.getElementById('status-indicator');
const tempVal = document.getElementById('temp-val');
const phVal = document.getElementById('ph-val');
const tdsVal = document.getElementById('tds-val');
const phCard = document.getElementById('ph-card');
const tdsCard = document.getElementById('tds-card');
const tempCard = document.getElementById('temp-card');
const tdsLimitVal = document.getElementById('tds-limit-val');
const displayTdsLimit = document.getElementById('display-tds-limit');
const tdsSlider = document.getElementById('tds-slider');

const btnBuzzer = document.getElementById('btn-buzzer');
const btnLED = document.getElementById('btn-led');

// Chart Setup
const ctx = document.getElementById('historyChart').getContext('2d');
const MAX_DATA_POINTS = 20;

const chart = new Chart(ctx, {
    type: 'line',
    data: {
        labels: [],
        datasets: [
            { label: 'Temp (°C)', borderColor: '#3b82f6', backgroundColor: '#3b82f633', tension: 0.4, data: [], fill: true},
            { label: 'pH', borderColor: '#22c55e', backgroundColor: '#22c55e33', tension: 0.4, data: [], fill: true},
            { label: 'TDS (ppm / 100)', borderColor: '#eab308', backgroundColor: '#eab30833', tension: 0.4, data: [], fill: true}
        ]
    },
    options: {
        responsive: true,
        maintainAspectRatio: false,
        plugins: { legend: { labels: { color: '#f8fafc' } } },
        scales: {
            x: { ticks: { color: '#94a3b8' }, grid: { color: 'rgba(255,255,255,0.05)' } },
            y: { ticks: { color: '#94a3b8' }, grid: { color: 'rgba(255,255,255,0.05)' } }
        }
    }
});

// Initialize MQTT Connection
function initMQTT() {
    console.log("Connecting to MQTT broker...");
    client = mqtt.connect(broker, {
        clientId: 'CollegeProjectClient_' + Math.random().toString(16).substr(2, 8)
    });

    client.on('connect', () => {
        console.log("Connected to HiveMQ via WebSockets!");
        client.subscribe(dataTopic);
        statusIndicator.innerText = "CONNECTED";
        statusIndicator.style.background = "rgba(59, 130, 246, 0.2)";
        statusIndicator.style.color = "#3b82f6";
    });

    client.on('message', (topic, message) => {
        if (topic === dataTopic) {
            try {
                const data = JSON.parse(message.toString());
                updateDashboard(data);
            } catch (e) {
                console.error("Failed to parse JSON:", e);
            }
        }
    });
}

function updateDashboard(data) {
    // Update Values
    tempVal.innerText = `${data.temperature.toFixed(1)} °C`;
    phVal.innerText = `${data.ph.toFixed(1)}`;
    tdsVal.innerText = `${data.tds} ppm`;

    // Process Status and Colors
    statusIndicator.innerText = data.status;
    statusIndicator.className = '';
    
    // Alerts Reset
    phCard.classList.remove('alert-card');
    tdsCard.classList.remove('alert-card');
    tempCard.classList.remove('alert-card');

    if (data.status === 'SAFE') {
        statusIndicator.classList.add('status-safe');
    } else if (data.status === 'WARNING') {
        statusIndicator.classList.add('status-warn');
    } else {
        statusIndicator.classList.add('status-danger');
    }

    // Rules checking based on project standard
    if (data.ph < 6.5 || data.ph > 8.5) phCard.classList.add('alert-card');
    if (data.tds > document.getElementById('tds-slider').value) tdsCard.classList.add('alert-card');
    if (data.temperature > 35.0) tempCard.classList.add('alert-card');

    // Chart Update
    const timeNow = new Date().toLocaleTimeString();
    chart.data.labels.push(timeNow);
    chart.data.datasets[0].data.push(data.temperature);
    chart.data.datasets[1].data.push(data.ph);
    chart.data.datasets[2].data.push(data.tds / 100);

    if (chart.data.labels.length > MAX_DATA_POINTS) {
        chart.data.labels.shift();
        chart.data.datasets.forEach(dataset => dataset.data.shift());
    }
    chart.update();
}

// Control Panel Functions
let buzzerState = 'off';
let ledState = 'off';

function toggleBuzzer() {
    buzzerState = buzzerState === 'off' ? 'on' : 'off';
    if(buzzerState === 'on') {
        btnBuzzer.classList.add('active');
        btnBuzzer.innerText = "Force Buzzer: ON";
    } else {
        btnBuzzer.classList.remove('active');
        btnBuzzer.innerText = "Force Buzzer: OFF";
    }
    sendCommand({ buzzer: buzzerState });
}

function toggleLED() {
    ledState = ledState === 'off' ? 'on' : 'off';
    if(ledState === 'on') {
        btnLED.classList.add('active');
        btnLED.innerText = "Force Alert LED: ON";
    } else {
        btnLED.classList.remove('active');
        btnLED.innerText = "Force Alert LED: OFF";
    }
    sendCommand({ led: ledState });
}

function updateThreshold() {
    const limit = tdsSlider.value;
    tdsLimitVal.innerText = limit;
    displayTdsLimit.innerText = limit;
    sendCommand({ tds_limit: parseInt(limit) });
}

function sendCommand(payload) {
    if (client && client.connected) {
        const msg = JSON.stringify(payload);
        client.publish(controlTopic, msg);
        console.log("Sent command:", msg);
    } else {
        alert("MQTT Not Connected!");
    }
}

// Start
window.onload = initMQTT;
