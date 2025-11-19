const express = require('express');
const bodyParser = require('body-parser');
const fs = require('fs');
const path = require('path');
const crypto = require('crypto');

const app = express();
app.use(bodyParser.json());
app.use(express.static(path.join(__dirname, 'public')));

const USERS_FILE = path.join(__dirname, 'password');

function readUsers() {
  if (!fs.existsSync(USERS_FILE)) return {};
  const data = fs.readFileSync(USERS_FILE, 'utf8').trim();
  if (!data) return {};
  const lines = data.split(/\r?\n/);
  const users = {};
  for (const line of lines) {
    const parts = line.split(',');
    if (parts.length < 7) continue;
    const [username, salt, hash, failedSets, attemptInSet, lockUntil, banned] = parts;
    users[username] = {
      username,
      salt,
      hash,
      failedSets: parseInt(failedSets, 10) || 0,
      attemptInSet: parseInt(attemptInSet, 10) || 0,
      lockUntil: parseInt(lockUntil, 10) || 0,
      banned: banned === 'true'
    };
  }
  return users;
}

function writeUsers(users) {
  const lines = Object.values(users).map(u => [u.username, u.salt, u.hash, u.failedSets, u.attemptInSet, u.lockUntil, u.banned].join(','));
  fs.writeFileSync(USERS_FILE, lines.join('\n'), 'utf8');
}

function hashPassword(password, salt) {
  return crypto.pbkdf2Sync(password, salt, 100000, 64, 'sha256').toString('hex');
}

const lockDurations = [5, 10, 15, 20]; // seconds

function validateUsername(u) {
  return /^[a-z]{5}$/.test(u);
}

function validatePassword(p) {
  if (p.length < 8) return false;
  if (!/[a-z]/.test(p)) return false;
  if (!/[A-Z]/.test(p)) return false;
  if (!/[0-9]/.test(p)) return false;
  return true;
}

app.post('/api/signup', (req, res) => {
  const { username, password } = req.body || {};
  if (!username || !password) return res.status(400).json({ ok: false, msg: 'username and password required' });
  if (!validateUsername(username)) return res.status(400).json({ ok: false, msg: 'username must be 5 lowercase letters' });
  if (!validatePassword(password)) return res.status(400).json({ ok: false, msg: 'password must be >=8 chars and include lowercase, uppercase and digits' });

  const users = readUsers();
  if (users[username]) return res.status(400).json({ ok: false, msg: 'username already exists' });

  const salt = String(Math.floor(Math.random() * 90000) + 10000); // 5 digits
  const hash = hashPassword(password, salt);
  users[username] = { username, salt, hash, failedSets: 0, attemptInSet: 0, lockUntil: 0, banned: false };
  writeUsers(users);
  return res.json({ ok: true, msg: 'signup successful' });
});

app.post('/api/signin', (req, res) => {
  const { username, password } = req.body || {};
  if (!username || !password) return res.status(400).json({ ok: false, msg: 'username and password required' });

  const users = readUsers();
  const user = users[username];
  if (!user) return res.status(400).json({ ok: false, msg: 'user not found' });
  if (user.banned) return res.status(403).json({ ok: false, msg: 'account is banned' });
  const now = Date.now();
  if (user.lockUntil && now < user.lockUntil) {
    const remain = Math.ceil((user.lockUntil - now) / 1000);
    return res.status(429).json({ ok: false, msg: `account locked, try again in ${remain} seconds` });
  }

  const attemptedHash = hashPassword(password, user.salt);
  if (attemptedHash === user.hash) {
    user.failedSets = 0;
    user.attemptInSet = 0;
    user.lockUntil = 0;
    user.banned = false;
    writeUsers(users);
    return res.json({ ok: true, msg: 'signin successful' });
  }

  // wrong password
  user.attemptInSet = (user.attemptInSet || 0) + 1;
  if (user.attemptInSet >= 3) {
    user.attemptInSet = 0;
    user.failedSets = (user.failedSets || 0) + 1;
    if (user.failedSets > lockDurations.length) {
      user.banned = true;
      writeUsers(users);
      return res.status(403).json({ ok: false, msg: 'account banned due to repeated failures' });
    }
    const lockSeconds = lockDurations[Math.min(user.failedSets - 1, lockDurations.length - 1)];
    user.lockUntil = now + lockSeconds * 1000;
    writeUsers(users);
    return res.status(429).json({ ok: false, msg: `wrong password — account locked ${lockSeconds} seconds` });
  }

  writeUsers(users);
  return res.status(401).json({ ok: false, msg: 'wrong password' });
});

app.listen(3000, () => console.log('Auth app listening on http://localhost:3000'));
