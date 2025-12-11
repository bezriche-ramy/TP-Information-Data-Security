# 🔐 Authentication Server & Password Cracker (Pure C - Brute Force Only)

## 📋 Description

Pure brute force password cracking system in C for Information Data Security coursework.

**Components:**
- HTTP Authentication Server (C) - Port 3000
- Pure Brute Force Password Cracker (2 modes)

## 🎯 Requirements Implemented

✅ **Backend in C**  
✅ **Mode 1**: Crack username (5 chars: a-z,A-Z,0-9) + password (8 chars: a-z,A-Z,0-9,@#$&)  
✅ **Mode 2**: Crack password only with known username  
✅ **Security**: PBKDF2 hashing, progressive account locking, banning  
✅ **Credentials Export**: Saves cracked credentials to shell script  
✅ **Pure Brute Force**: No dictionary attacks or common passwords

## ⚙️ Installation

### Prerequisites
```bash
sudo apt-get update
sudo apt-get install -y build-essential libssl-dev
```

### Compilation
```bash
make all
```

## 🚀 Usage

### 1. Start the Server
```bash
./server &
```

Server runs on port 3000.

### 2. Create Test User via API

```bash
# Create user with simple password (easy to crack for demo)
curl -X POST http://localhost:3000/api/signup \
  -H "Content-Type: application/json" \
  -d '{"username":"demo1","password":"Aaaa0000@"}'
```

**Username Requirements:**
- Exactly 5 alphanumeric characters (a-z, A-Z, 0-9)

**Password Requirements:**
- Minimum 8 characters
- At least 1 lowercase (a-z)
- At least 1 uppercase (A-Z)
- At least 1 digit (0-9)
- At least 1 special character (@, #, $, &)

### 3. Run Pure Brute Force Cracker

#### Mode 1: Crack Both Username and Password
```bash
./cracker 1
```
**Warning**: Tests ALL combinations (~3.3×10¹⁷). Impractical - for demonstration only.

#### Mode 2: Crack Password (Username Known) - RECOMMENDED
```bash
./cracker 2 <username>
```

Example:
```bash
./cracker 2 demo1
```

This mode only tests passwords (~3.6×10¹⁴ combinations).

### 4. View Cracked Credentials

When the cracker finds the password, it creates `cracked_credentials.sh`:

```bash
# View the credentials
cat cracked_credentials.sh

# Or run the script
./cracked_credentials.sh
```

Output:
```
Cracked Credentials:
Username: demo1
Password: Aaaa0000@
```

## 📊 Complete Test Example

```bash
# 1. Clean and compile
make clean
make all

# 2. Start server
./server &
sleep 2

# 3. Create simple user (password starts early in brute force space)
curl -X POST http://localhost:3000/api/signup \
  -H "Content-Type: application/json" \
  -d '{"username":"aaaaa","password":"Aaaa0000@"}'

# 4. Run brute force cracker Mode 2
./cracker 2 aaaaa

# 5. View saved credentials
cat cracked_credentials.sh
./cracked_credentials.sh

# 6. Verify credentials work
curl -X POST http://localhost:3000/api/signin \
  -H "Content-Type: application/json" \
  -d '{"username":"aaaaa","password":"Aaaa0000@"}'
```

## 🔐 Security Features

### Server
- **PBKDF2 Hashing**: 100,000 iterations with SHA-256
- **Unique Salt**: Random 5-digit salt per user
- **Progressive Locking**: 5s → 10s → 15s → 20s after 3 failed attempts
- **Auto-Ban**: Permanent ban after 4 lock cycles
- **Password Validation**: Enforces complexity requirements

### Cracker
- **Pure Brute Force**: Tests every possible combination systematically
- **No Dictionary**: No common passwords or patterns
- **Auto Lock Handling**: Waits 25 seconds when account locks
- **Real-time Stats**: Shows attempts/second and progress
- **Credential Export**: Saves to executable shell script

## 📈 Brute Force Complexity

| Mode | Combinations | Time @ 200 req/s |
|------|--------------|------------------|
| Mode 1 | 62⁵ × 66⁸ ≈ 3.3×10¹⁷ | ~52 million years |
| Mode 2 | 66⁸ ≈ 3.6×10¹⁴ | ~57,000 years |

**Note**: Mode 2 is 916,000x faster than Mode 1.

### Character Sets

**Username (5 chars):**
- a-z (26) + A-Z (26) + 0-9 (10) = 62 characters
- Total: 62⁵ = 916,132,832 combinations

**Password (8 chars minimum):**
- a-z (26) + A-Z (26) + 0-9 (10) + @#$& (4) = 66 characters
- Total: 66⁸ = 360,040,606,269,696 combinations

## 📁 Project Structure

```
├── server.c              # C authentication server
├── cracker.c             # Pure brute force cracker
├── server                # Compiled server
├── cracker               # Compiled cracker
├── Makefile              # Build automation
├── password              # User database (CSV)
├── cracked_credentials.sh # Generated credentials file
└── README.md             # This file
```

## 🛠️ Makefile Commands

```bash
make all      # Compile server and cracker
make server   # Compile only server
make cracker  # Compile only cracker
make clean    # Remove compiled files
```

## 🔬 How Brute Force Works

### Mode 1: Full Brute Force
1. Start with username "aaaaa" and password "aaaaaaaa"
2. Increment password through all 66⁸ combinations
3. Move to next username "aaaab" and repeat
4. Continue until match found or all combinations tested

### Mode 2: Password-Only Brute Force
1. Start with password "aaaaaaaa"
2. Increment through charset: a→b→...→z→A→...→Z→0→...→9→@→#→$→&
3. Test each combination: "aaaaaaaa", "aaaaaaab", "aaaaaaac", etc.
4. Continue until match found

Example progression:
```
aaaaaaaa
aaaaaaab
aaaaaaac
...
aaaaaaba
aaaaaabb
...
Aaaa0000@  ← Found!
```

## 📞 API Endpoints

### POST /api/signup
Create new user account.

**Request:**
```json
{
  "username": "user1",
  "password": "Pass1234@"
}
```

**Response:**
```json
{
  "ok": true,
  "msg": "signup successful"
}
```

### POST /api/signin
Authenticate user.

**Request:**
```json
{
  "username": "user1",
  "password": "Pass1234@"
}
```

**Response (Success):**
```json
{
  "ok": true,
  "msg": "signin successful"
}
```

**Response (Locked):**
```json
{
  "ok": false,
  "msg": "account locked, try again in 5 seconds"
}
```

**Response (Banned):**
```json
{
  "ok": false,
  "msg": "account is banned"
}
```

## 🐛 Troubleshooting

### Server won't start
```bash
# Kill existing processes
killall server
# Or
lsof -ti:3000 | xargs kill -9

# Restart
./server &
```

### Compilation errors
```bash
# Install dependencies
sudo apt-get install build-essential libssl-dev

# Clean and rebuild
make clean
make all
```

### Cracker stuck
- Account may be locked (waits 25s automatically)
- Check server is running: `curl -s http://localhost:3000/api/signin`

### No credentials file created
- File only created when password is found
- Check terminal for "✓ SUCCESS FOUND!" message

## 🎓 For Presentation

### Demo Flow

1. **Compile**
   ```bash
   make all
   ```

2. **Start Server**
   ```bash
   ./server &
   ```

3. **Create User**
   ```bash
   curl -X POST http://localhost:3000/api/signup \
     -H "Content-Type: application/json" \
     -d '{"username":"demo1","password":"Aaaa0000@"}'
   ```

4. **Run Brute Force (Mode 2)**
   ```bash
   ./cracker 2 demo1
   ```

5. **Show Results**
   ```bash
   ./cracked_credentials.sh
   ```

6. **Verify**
   ```bash
   curl -X POST http://localhost:3000/api/signin \
     -H "Content-Type: application/json" \
     -d '{"username":"demo1","password":"Aaaa0000@"}'
   ```

### Key Points to Explain

1. **Pure Brute Force**: No shortcuts, tests every combination
2. **Mode 1 Impractical**: Demonstrates why username+password brute force is impossible
3. **Mode 2 Faster**: Knowing username reduces space by 916,000x
4. **Still Too Slow**: Even Mode 2 takes 57,000 years for full search
5. **PBKDF2 Impact**: 100k iterations make each attempt slow
6. **Account Locking**: Additional protection against brute force
7. **Strong Passwords**: Long, random passwords are effectively uncrackable

## 📝 Password Storage Format

File: `password` (CSV format)
```
username,salt,hash,failed_sets,attempt_in_set,lock_until,banned
```

Example:
```
demo1,54321,a3f5e9b2c1d4...,0,0,0,false
```

Fields:
- **username**: 5 alphanumeric characters
- **salt**: Random 5-digit number (10000-99999)
- **hash**: PBKDF2-SHA256 hash (128 hex characters)
- **failed_sets**: Number of 3-failure sets
- **attempt_in_set**: Current failures in set (0-2)
- **lock_until**: Unix timestamp for unlock
- **banned**: true if permanently banned

## ⚠️ Important Notes

1. **Educational Only**: For coursework only. Unauthorized use is illegal.

2. **Brute Force Reality**: Demonstrates why brute force is impractical against strong passwords.

3. **Account Locking**: Cracker waits automatically when accounts lock.

4. **Simple Passwords**: Use passwords starting with "Aaaa0000@" for quick demo (found in ~2500 attempts).

5. **No Web Interface**: Pure C implementation, API only.

## 🔢 Brute Force Mathematics

### Why Mode 1 is Impractical

```
Username space: 62^5 = 916,132,832
Password space: 66^8 = 360,040,606,269,696
Total: 3.3 × 10^17 combinations

At 200 requests/second:
Time = 3.3×10^17 / 200 / 60 / 60 / 24 / 365
     ≈ 52,417,708 years
```

### Why Mode 2 is Still Hard

```
Password space: 66^8 = 360,040,606,269,696

At 200 requests/second:
Time = 3.6×10^14 / 200 / 60 / 60 / 24 / 365
     ≈ 57,163 years
```

### With Account Locking

```
3 attempts per 5 seconds = 0.6 attempts/second
Time ≈ 19 million years
```

---

**Pure brute force implementation - No shortcuts, no dictionaries!** 🔐

For technical details, see code comments in `server.c` and `cracker.c`.
