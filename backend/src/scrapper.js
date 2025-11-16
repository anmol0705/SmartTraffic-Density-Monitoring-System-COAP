const axios = require('axios');
const cheerio = require('cheerio');
const extractor = require('./utils/extractor');

const POLL_URL = process.env.POLL_URL || 'https://coap.me/crawl/coap://coap.me:5683';
const POLL_INTERVAL_SEC = Number(process.env.POLL_INTERVAL_SEC || 2);
const FETCH_TIMEOUT_MS = Number(process.env.FETCH_TIMEOUT_MS || 8000);

let latest = null;
let lastKey = null;
let intervalHandle = null;

async function fetchPage(url) {
  try {
    const r = await axios.get(url, {
      timeout: FETCH_TIMEOUT_MS,
      headers: { 'User-Agent': 'Poller/1.0' },
      responseType: 'text',
      validateStatus: () => true
    });
    return r.data || null;
  } catch (err) {
    console.warn('Fetch failed:', err.message);
    return null;
  }
}

function makeKey(raw) {
  if (!raw) return null;
  if (typeof raw === 'object') {
    const sorted = {};
    Object.keys(raw).sort().forEach(k => sorted[k] = raw[k]);
    return JSON.stringify(sorted);
  }
  return raw.toString().trim();
}

async function pollOnce() {
  const body = await fetchPage(POLL_URL);
  if (!body) return;

  const $ = cheerio.load(body);
  const raw = extractor.extractReading($('body').text());

  const key = makeKey(raw);
  if (!key || key === lastKey) return;

  lastKey = key;
  latest = typeof raw === 'object' ? raw : { value: String(raw) };
  console.log('Updated:', latest);
}

function start() {
  console.log('Poller started...');
  pollOnce();
  intervalHandle = setInterval(pollOnce, POLL_INTERVAL_SEC * 1000);
}

function stop() {
  clearInterval(intervalHandle);
  console.log('Poller stopped.');
}

function getLatest() {
  return latest;
}

module.exports = { start, stop, getLatest };
