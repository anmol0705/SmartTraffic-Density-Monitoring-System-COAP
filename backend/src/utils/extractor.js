function extractReading(text) {
  if (!text) return null;

  // find first JSON object
  for (let start = text.indexOf('{'); start !== -1; start = text.indexOf('{', start + 1)) {
    let depth = 0;
    for (let i = start; i < text.length; i++) {
      if (text[i] === '{') depth++;
      if (text[i] === '}') {
        depth--;
        if (depth === 0) {
          const jsonStr = text.slice(start, i + 1);
          try {
            return JSON.parse(jsonStr);
          } catch {}
          break;
        }
      }
    }
  }

  // fallback: top non-empty line
  return text.split(/\r?\n/).map(l => l.trim()).filter(Boolean)[0] || null;
}

module.exports = { extractReading };
