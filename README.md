TP Authentication App
=====================

Simple authentication example for the TP Information Data Security assignment.

Features implemented:
- Signup: username must be 5 lowercase letters; password >=8 chars with lowercase, uppercase and digits.
- Signin: compares password with salted PBKDF2 hash.
- Lockout escalation: after 3 wrong attempts -> 5s lock; next 3 wrong -> 10s; then 15s, then 20s; then account banned.
- Password storage file: `password` (CSV lines `username,salt,hash,failedSets,attemptInSet,lockUntil,banned`).

Run (requires Node.js):

```
cd "$(dirname "$0")"
npm install
npm start
```

Open `http://localhost:3000` in your browser.

Notes:
- Salt is generated as a random 5-digit number (as requested).
- Hashing uses PBKDF2-SHA256 with 100000 iterations (a slow hash function).
# TP-Information-Data-Security
# TP-Information-Data-Security
