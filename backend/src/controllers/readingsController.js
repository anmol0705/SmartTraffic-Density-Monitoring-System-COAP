const scrapper = require("../scrapper");

exports.getLatest = (req, res) => {
  const latest = scrapper.getLatest();
  if (!latest) return res.status(204).send();
  res.json(latest);
};
