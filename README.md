# 🔐 Information Data Security - Authentication System & Password Cracker

A comprehensive C implementation demonstrating authentication security concepts and brute force attack techniques for university coursework.

---

## 📋 Table of Contents

1. [Project Overview](#-project-overview)
2. [How Authentication Works](#-how-authentication-works)
3. [How Password Cracking Works](#-how-password-cracking-works)
4. [Quick Start](#-quick-start)
5. [Detailed Usage](#-detailed-usage)
6. [Technical Details](#-technical-details)
7. [Security Features](#-security-features)
8. [Performance Comparison](#-performance-comparison)

---

## 🎯 Project Overview

This project contains **3 main components**:

| Component       | File                   | Purpose                                   |
| --------------- | ---------------------- | ----------------------------------------- |
| **Auth System** | `auth_system.c`        | User signup/signin with security features |
| **CPU Cracker** | Built into auth_system | Brute force attack (single-threaded)      |
| **GPU Cracker** | `gpu_cracker.cu`       | Massively parallel brute force using CUDA |

### File Structure

```
├── auth_system.c      # Main authentication program
├── gpu_cracker.cu     # GPU-accelerated password cracker
├── Makefile           # Build configuration
├── password.txt       # Stores user credentials (auto-created)
├── banned.txt         # List of banned accounts (auto-created)
└── README.md          # This file
```

---

## 🔑 How Authentication Works

### Step 1: User Signup

```
User enters: Username + Password
                ↓
         Validation Check
     (5 lowercase chars, 8 char password)
                ↓
        Generate Random Salt
          (5 digit number)
                ↓
           Hash Password
     hash = FNV-1a(password + salt)
                ↓
        Store in password.txt
     format: username:salt:hash
```

**Example:**

```
Input:  Username: ramiy, Password: abcabc3H
Salt:   02931 (random)
Hash:   FNV-1a("abcabc3H02931") = 13757215776198851619
Stored: ramiy:02931:13757215776198851619
```

### Step 2: User Signin

```
User enters: Username + Password
                ↓
        Read password.txt
    Find matching username
                ↓
      Get stored salt & hash
                ↓
      Hash input password
    new_hash = FNV-1a(input + salt)
                ↓
        Compare hashes
    new_hash == stored_hash ?
                ↓
       ✅ Success / ❌ Fail
```

### Step 3: Security Throttling

When login fails, the system **slows down** attacks:

| Failed Attempts | Delay      | Action                     |
| --------------- | ---------- | -------------------------- |
| 1-3             | 5 seconds  | Warning                    |
| 4-6             | 10 seconds | Increased delay            |
| 7-9             | 15 seconds | Serious warning            |
| 10-12           | 20 seconds | Final warning              |
| 13+             | **BANNED** | Account locked permanently |

---

## 💀 How Password Cracking Works

### The Concept

Brute force = **Try every possible password** until you find the right one.

```
Password charset: abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$&
Charset size: 66 characters
Password length: 8 characters
Total combinations: 66^8 = 360,040,606,269,696 (360 TRILLION!)
```

### CPU Brute Force (Single-threaded)

```
For each password in search space:
    1. Generate password from index
    2. Concatenate: password + salt
    3. Calculate hash = FNV-1a(combined)
    4. If hash == target_hash: FOUND!
    5. Else: try next password
```

**Speed:** ~18 million passwords/second

### GPU Brute Force (Massively Parallel)

The GPU has **2048 CUDA cores** that work simultaneously:

```
┌─────────────────────────────────────────────────────────────┐
│  GPU with 2048 cores                                        │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐ ... ┌─────┐      │
│  │Core1│ │Core2│ │Core3│ │Core4│ │Core5│     │Core │      │
│  │ aaa │ │ aab │ │ aac │ │ aad │ │ aae │     │ 2048│      │
│  │ ↓   │ │ ↓   │ │ ↓   │ │ ↓   │ │ ↓   │     │ ↓   │      │
│  │Hash │ │Hash │ │Hash │ │Hash │ │Hash │     │Hash │      │
│  │Check│ │Check│ │Check│ │Check│ │Check│     │Check│      │
│  └─────┘ └─────┘ └─────┘ └─────┘ └─────┘ ... └─────┘      │
│           All running AT THE SAME TIME!                     │
└─────────────────────────────────────────────────────────────┘
```

**Speed:** ~3.7 BILLION passwords/second (200x faster than CPU!)

### Recursive Algorithm

The brute force uses recursion to generate all combinations:

```c
brute_force(position, max_length):
    if position == max_length:
        TEST this password
    else:
        for each char in charset:
            password[position] = char
            brute_force(position + 1, max_length)
```

**Visualization (3 chars, charset "ab"):**

```
                    ""
           ┌────────┴────────┐
          "a"               "b"
       ┌───┴───┐         ┌───┴───┐
     "aa"     "ab"     "ba"     "bb"
    ┌─┴─┐   ┌─┴─┐    ┌─┴─┐    ┌─┴─┐
  "aaa""aab""aba""abb""baa""bab""bba""bbb"
```

---

## 🚀 Quick Start

### Compile Everything

```bash
make          # Compile CPU auth system
make gpu      # Compile GPU cracker (requires CUDA)
```

### Create an Account

```bash
./auth_system
# Select: 1 (Signup)
# Username: ramiy
# Password: Test1234
```

### Crack the Password

```bash
./gpu_cracker
# Username: ramiy
# Length: 8
# Watch the magic! 🎯
```

---

## 📖 Detailed Usage

### Authentication System Menu

```
╔════════════════════════════════════╗
║  AUTHENTICATION SYSTEM - MAIN MENU ║
╠════════════════════════════════════╣
║ 1. Signup                          ║  ← Create new account
║ 2. Signin                          ║  ← Login to account
║ 3. Brute Force Attack Module       ║  ← CPU password cracker
║ 4. Exit                            ║
╚════════════════════════════════════╝
```

### Signup Rules

| Field    | Rule                                                     | Example                |
| -------- | -------------------------------------------------------- | ---------------------- |
| Username | Exactly 5 lowercase letters                              | `hello`, `ramiy`       |
| Password | Exactly 8 chars (must have: lowercase, uppercase, digit) | `Test1234`, `abcABC12` |

### Brute Force Modes

**1. Blind Attack** - Find ANY username in the system

```bash
# Tries all possible 5-letter usernames
# Useful when you don't know any usernames
```

**2. Targeted Attack** - Crack a SPECIFIC user's password

```bash
# You provide the username
# System finds the hash and cracks the password
```

---

## 🔧 Technical Details

### Hash Function: FNV-1a 64-bit

We use **FNV-1a** instead of DJB2 because DJB2 has collision problems.

```c
// FNV-1a Algorithm
hash = 14695981039346656037  // Offset basis
for each byte in input:
    hash = hash XOR byte
    hash = hash × 1099511628211  // FNV prime
return hash
```

**Why FNV-1a?**

- 64-bit output (18 quintillion possible values)
- Excellent distribution
- Fast to compute
- Minimal collisions

### Password Storage Format

```
username:salt:hash
ramiy:02931:13757215776198851619
```

| Field    | Description                               |
| -------- | ----------------------------------------- |
| username | 5 lowercase letters                       |
| salt     | 5 random digits (prevents rainbow tables) |
| hash     | FNV-1a hash of password+salt              |

### CUDA Configuration (GPU)

```
GPU: NVIDIA RTX 3050 Laptop
CUDA Cores: 2048
Blocks: 40
Threads per block: 256
Passwords per thread: 1000
Passwords per kernel: 10,000,000
```

---

## 🛡️ Security Features

### 1. Password Salting

```
Without salt: hash("password") = X (same for all users)
With salt:    hash("password" + "12345") = Y
              hash("password" + "67890") = Z (different!)
```

Salts prevent pre-computed rainbow table attacks.

### 2. Progressive Throttling

Each failed login makes the attacker wait longer, slowing brute force attempts.

### 3. Account Banning

After 13 failed attempts, the account is permanently locked.

### 4. Input Validation

- Prevents buffer overflow attacks
- Enforces strong password requirements

---

## 📊 Performance Comparison

| Method              | Speed           | Time to crack 8-char password |
| ------------------- | --------------- | ----------------------------- |
| CPU (single-thread) | 18 million/sec  | ~231 days                     |
| GPU (RTX 3050)      | 3.7 billion/sec | ~1.1 days                     |

**Why GPU is faster:**

- 2048 cores vs 1 core
- Parallel hash computation
- Optimized memory access

---

## 🧪 Example Session

### Creating an Account

```
$ ./auth_system

=== SIGNUP ===
Enter Username: ramiy
Enter Password: abcabc3H
✅ Signup successful! Username: ramiy
   (Salt: 02931, Hash: 13757215776198851619)
```

### Cracking with GPU

```
$ ./gpu_cracker

🖥️  GPU Detected: NVIDIA GeForce RTX 3050 Laptop GPU
   CUDA Cores: 2048
   Memory: 3.7 GB

Enter target username: ramiy
✅ Target found: ramiy
   Salt: 02931
   Target Hash: 13757215776198851619

🎯 Attack Configuration:
   Password Length: 8
   Total Combinations: 360,040,606,269,696
   Hash Algorithm: FNV-1a 64-bit

🚀 Starting GPU Brute Force Attack...
  [GPU] 0.0237% | 85,150,000,000 attempts | Speed: 3,702,173,913/s | ETA: 1.1d

🎯 PASSWORD FOUND: abcabc3H
   Time: 23.0 seconds
   Speed: 3,702,173,913 passwords/second
   ✅ Hash verification: PASSED
```

---

## 🎓 Learning Objectives

This project demonstrates:

1. ✅ **Password Security** - Hashing, salting, storage
2. ✅ **Authentication Systems** - Signup, signin, validation
3. ✅ **Brute Force Attacks** - How attackers crack passwords
4. ✅ **Security Throttling** - Rate limiting, account locking
5. ✅ **Parallel Computing** - GPU acceleration with CUDA
6. ✅ **Recursive Algorithms** - Password enumeration
7. ✅ **C Programming** - Buffer safety, file I/O

---

## 📝 Makefile Commands

```bash
make          # Compile auth_system
make gpu      # Compile gpu_cracker
make run      # Run auth_system
make run-gpu  # Run gpu_cracker
make clean    # Delete executables and data files
```

---

## ⚠️ Disclaimer

This project is for **educational purposes only**. The techniques demonstrated should never be used to attack systems without explicit permission. Understanding attack methods helps build better defenses.

---

**Author:** University Assignment (TP) - Information Data Security  
**Last Updated:** January 2026
