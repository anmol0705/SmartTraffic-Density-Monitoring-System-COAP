const express = require('express');
const cors = require('cors');
const dotenv = require('dotenv');

dotenv.config();

const PORT = Number(process.env.PORT || 3000);
const scrapper = require('./scrapper');
const readingsRouter = require('./routes/readings');

const app = express();
app.use(cors());
app.use(express.json());

app.use(express.static('public'));

app.use('/', readingsRouter);

const server = app.listen(PORT, () => {
  console.log(`Server running at http://localhost:${PORT}`);
  scrapper.start();
});

process.on('SIGINT', () => {
  console.log('Shutting down...');
  scrapper.stop();
  server.close(() => process.exit(0));
});
