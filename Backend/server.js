/*
 * =================================================================
 * AERAS Backend Server (Node.js + Express)
 * Hackathon: IOTRIX
 * This server manages ride requests and serves two web UIs.
 * To run:
 * 1. npm init -y
 * 2. npm install express cors
 * 3. node server.js
 * =================================================================
 */

const express = require('express');
const cors = require('cors');
const path = require('path');
const app = express();
const PORT = 3000;

// Middleware
app.use(cors()); // Enable Cross-Origin Resource Sharing
app.use(express.json()); // Enable JSON data parsing
app.use(express.static(path.join(__dirname))); // Serve static files (HTML)

// --- In-Memory Database (for Hackathon) ---
// Instead of a real DB, we'll store rides in an array.
let rides = [];
let pullerPoints = {
    "puller_001": 0 // Demo puller for Test Case 6e
};
let rideCounter = 1;

// Location and Point data (Test Case 7 & 11)
const locations = {
    "CUET Campus": { lat: 22.4633, lon: 91.9714 },
    "Pahartoli": { lat: 22.4725, lon: 91.9845 },
    "Noapara": { lat: 22.4580, lon: 91.9920 },
    "Raojan": { lat: 22.4520, lon: 91.9650 }
};
const BASE_POINTS_PER_RIDE = 10; // Test Case 11a

console.log("AERAS Backend Server starting...");

// --- API Endpoints ---

/**
 * Route: / (GET)
 * Description: A basic route to check if the server is running.
 */
app.get('/', (req, res) => {
    res.send('AERAS Backend Server is active. Go to /rickshaw.html or /admin.html');
});

/**
 * Route: /rickshaw.html (GET)
 * Description: Serves the Rickshaw Puller UI file.
 */
app.get('/rickshaw.html', (req, res) => {
    res.sendFile(path.join(__dirname, 'rickshaw.html'));
});

/**
 * Route: /admin.html (GET)
 * Description: Serves the Admin Dashboard UI file.
 */
app.get('/admin.html', (req, res) => {
    res.sendFile(path.join(__dirname, 'admin.html'));
});

/**
 * Route: /request-ride (POST)
 * Description: Receives a new ride request from the ESP32. (Test Case 13.4)
 * Body: { pickup: "CUET Campus", destination: "Pahartoli", pullerId: "puller_001" }
 */
app.post('/request-ride', (req, res) => {
    const { pickup, destination, pullerId } = req.body;
    
    if (!pickup || !destination || !pullerId) {
        return res.status(400).json({ message: "Missing required info (pickup, destination, pullerId)." });
    }

    const newRide = {
        id: `ride_${rideCounter++}`,
        pickup,
        destination,
        pullerId,
        status: "pending", // 'pending' -> 'accepted' -> 'in_progress' -> 'completed'
        requestTime: new Date().toISOString(),
        points: 0
    };

    rides.push(newRide);
    console.log(`New Ride Request: ${newRide.id} (${pickup} to ${destination})`);
    
    // Send the new ride ID back to the ESP32
    res.status(201).json({ 
        message: "Ride request created successfully.", 
        rideId: newRide.id 
    });
});

/**
 * Route: /rides (GET)
 * Description: Sends the list of all rides to the Admin and Rickshaw UIs.
 */
app.get('/rides', (req, res) => {
    res.status(200).json({
        rides: rides,
        pullerPoints: pullerPoints
    });
});

/**
 * Route: /ride-status/:id (GET)
 * Description: Polled by the ESP32 to get the status of a specific ride.
 */
app.get('/ride-status/:id', (req, res) => {
    const { id } = req.params;
    const ride = rides.find(r => r.id === id);

    if (ride) {
        res.status(200).json({ status: ride.status });
    } else {
        res.status(404).json({ message: "Ride not found." });
    }
});

/**
 * Route: /accept-ride/:id (POST)
 * Description: Rickshaw puller accepts a ride. (Test Case 6b, 13.6)
 */
app.post('/accept-ride/:id', (req, res) => {
    const { id } = req.params;
    const ride = rides.find(r => r.id === id);

    if (ride && ride.status === 'pending') {
        ride.status = 'accepted'; // User-side Yellow LED ON (Test Case 4b)
        ride.acceptTime = new Date().toISOString();
        console.log(`Ride ${id} accepted.`);
        res.status(200).json(ride);
    } else {
        res.status(404).json({ message: "Unable to accept ride." });
    }
});

/**
 * Route: /confirm-pickup/:id (POST)
 * Description: Rickshaw puller confirms passenger pickup. (Test Case 6c)
 */
app.post('/confirm-pickup/:id', (req, res) => {
    const { id } = req.params;
    const ride = rides.find(r => r.id === id);

    if (ride && ride.status === 'accepted') {
        ride.status = 'in_progress'; // User-side Green LED ON (Test Case 4d)
        ride.pickupTime = new Date().toISOString();
        console.log(`Ride ${id} pickup confirmed.`);
        res.status(200).json(ride);
    } else {
        res.status(404).json({ message: "Unable to confirm pickup." });
    }
});

/**
 * Route: /confirm-dropoff/:id (POST)
 * Description: Rickshaw puller confirms passenger drop-off. (Test Case 6d, 7, 11)
 */
app.post('/confirm-dropoff/:id', (req, res) => {
    const { id } = req.params;
    const ride = rides.find(r => r.id === id);

    if (ride && ride.status === 'in_progress') {
        ride.status = 'completed';
        ride.dropoffTime = new Date().toISOString();
        
        // Point Calculation (Test Case 7 & 11)
        // Simulation: Assuming drop-off is correct (Test Case 7a)
        const awardedPoints = BASE_POINTS_PER_RIDE; 
        ride.points = awardedPoints;

        // Update puller's total points
        if (pullerPoints[ride.pullerId] !== undefined) {
            pullerPoints[ride.pullerId] += awardedPoints;
        } else {
            pullerPoints[ride.pullerId] = awardedPoints;
        }

        console.log(`Ride ${id} completed. ${awardedPoints} points awarded.`);
        res.status(200).json(ride);
    } else {
        res.status(404).json({ message: "Unable to confirm drop-off." });
    }
});

// Start the server
app.listen(PORT, '0.0.0.0', () => {
    // Using '0.0.0.0' makes the server accessible from other devices (like your ESP32) on the same network.
    console.log(`AERAS Backend Server listening on http://0.0.0.0:${PORT}`);
    console.log(`Rickshaw UI: http://<YOUR_SERVER_IP>:${PORT}/rickshaw.html`);
    console.log(`Admin UI: http://<YOUR_SERVER_IP>:${PORT}/admin.html`);
});