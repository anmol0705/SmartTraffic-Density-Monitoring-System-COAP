const express = require('express');
const router = express.Router();
const controller = require('../controllers/readingsController');

router.get('/latest', controller.getLatest);

module.exports = router;
