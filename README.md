**Smart Traffic Density Control System — COAP**

- **Description:** A small project that polls a COAP endpoint, extracts ultrasonic sensor readings for multiple lanes, and serves a live dashboard showing lane densities/ distances. Includes a simple Express backend that scrapes/polls the COAP source and a static frontend in `backend/public`.

**Repository Layout**
- **`backend/`**: Node.js Express backend and frontend static files
  - `package.json` — backend scripts & dependencies
  - `src/index.js` — Express server entry
  - `src/scrapper.js` — poller that fetches the COAP source and extracts readings
  - `src/routes/readings.js` — exposes `/latest` endpoint
  - `src/controllers/readingsController.js` — returns the latest reading
  - `src/utils/extractor.js` — helper to parse JSON from scraped text
  - `public/index.html` — dashboard UI (Tailwind CSS via CDN)
- **`worwi/`**: hardware sketch and diagram (`sketch.ino`, `diagram.json`)

**Quick Start (backend + UI)**
Prerequisites: Node.js (16+ recommended), npm

1. Open a terminal and go to the backend folder:

```
cd backend
```

2. Install dependencies (first time only):

```
npm install
```

3. Start the server (production):

```
npm start
```

Or start with auto-reload (development):

```
npm run dev
```

4. Open the dashboard in a browser:

```
http://localhost:3000/
```

**Environment variables (optional)**
Create or edit `backend/src/.env` to change defaults. Supported variables:
- `PORT` — port for the Express server (default: `3000`)
- `POLL_URL` — the URL the poller scrapes (default set in `scrapper.js`)
- `POLL_INTERVAL_SEC` — poll interval in seconds (default: `2`)
- `FETCH_TIMEOUT_MS` — HTTP fetch timeout (ms)

**API**
- `GET /latest` — returns the most recent reading (JSON) or `204 No Content` if none.

Example typical payloads the scraper extracts (two common shapes):
- Per-lane numeric keys (counts):

```
{ "lane1": 5, "lane2": 3, "lane3": 2, "lane4": 0 }
```

- Keys with suffixes (distance/density):

```
{ "lane1_density": 0, "lane2_density": 6, "lane3_density": 3, "lane4_density": 0 }
```

The frontend auto-maps keys that start with `lane1`, `lane2`, etc., and displays counts or distance (cm) using small heuristics.

**Frontend**
- The static app is `backend/public/index.html` and uses Tailwind via CDN. It polls `GET /latest` every 1s and shows a fixed 4-lane layout (responsive: 1 / 2x2 / 4-column).
- To modify visuals, edit `public/index.html` directly.

**Hardware (worwi folder)**
- `worwi/sketch.ino` — an Arduino/ESP sketch for ultrasonic sensor readings. Open in Arduino IDE, set correct board and port, then upload.
- `worwi/diagram.json` — circuit diagram reference.

**Testing / Debugging**
- If the UI shows no data:
  - Check backend logs (run `npm start` in `backend` and watch console). The poller logs "Updated:" events when it detects changes.
  - Visit `/latest` directly in the browser or curl to see raw output:

```
curl http://localhost:3000/latest
```

- If you want to test without the COAP source, you can temporarily add a small debug route that returns a sample payload, or I can add a dev button to the UI that injects mock data for visual testing.

**Common fixes**
- Server not starting: ensure Node.js is installed and run `npm install` in `backend`.
- Poller not updating: verify `POLL_URL` is reachable (network, firewall) and check `scrapper.js` console output.

**Contributions & Next Steps**
- Convert the static frontend to a React app (Vite) if you prefer componentization.
- Add authentication, persistence (DB) for historical data, or WebSocket push updates for lower latency.

**License & Notes**
- No license file included. Treat as a personal/experimental project.

---
If you'd like, I can also add a small developer-only mock button to `public/index.html` to inject sample `/latest` payloads for UI verification — would you like that?
