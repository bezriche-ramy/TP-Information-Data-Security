# Quick Brute Force Usage Guide

## ✅ Updated Brute Force - Now Searches Full 8 Characters!

The brute force attack module has been enhanced to search up to **full 8-character passwords**.

### How to Use

1. **Run the program**:

   ```bash
   ./auth_system
   ```

2. **Select option 3** (Brute Force Attack Module)

3. **Choose Targeted Attack** (option 2)

4. **Enter the username** to crack (e.g., `ramiy`)

5. **Choose maximum length to try** (1-8):
   - For quick test: try `5` or `6` first
   - For full search: enter `8` (will search ALL passwords up to 8 chars)

### Example Session

```
=== TARGETED ATTACK ===
Enter target username: ramiy
✅ Target found: ramiy (Salt: 56359, Hash: 10489111675542845833)

Enter maximum password length to try (1-8, recommended start with 6): 8

⚠️  WARNING: Searching up to length 8 means up to 360,040,606,269,696 combinations!
   This may take a VERY long time. Continue? (y/n): y

🚀 Starting brute force attack on password...
   Charset: abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789@#$&
   Charset size: 66 characters

[Brute Force] Trying length 1 (66 combinations)...
  [Progress] Attempts: 1000, Current: aB1
  Length 1 complete. Total attempts so far: 66

[Brute Force] Trying length 2 (4,356 combinations)...
  [Progress] Attempts: 2000, Current: cd2
  Length 2 complete. Total attempts so far: 4,422
...
```

### Performance Notes

| Length | Combinations  | Estimated Time |
| ------ | ------------- | -------------- |
| 1      | 66            | ~instant       |
| 2      | 4,356         | ~instant       |
| 3      | 287,496       | seconds        |
| 4      | 18,974,736    | minutes        |
| 5      | 1.25 billion  | hours          |
| 6      | 82.7 billion  | days           |
| 7      | 5.46 trillion | months         |
| 8      | 360 trillion  | years          |

### Tips

- **Start small**: Try lengths 5-6 first to see if the password is shorter or simpler
- **Progress updates**: The program shows progress every 1,000 attempts
- **Time tracking**: Shows elapsed time when complete or stopped
- **Be patient**: Longer passwords take exponentially longer!

### What Changed

✅ Now searches up to full 8 characters (configurable)
✅ Shows estimated combinations for each length
✅ Displays progress every 1,000 attempts (not 10,000)
✅ Shows time elapsed during attack
✅ Warns user about long searches (7-8 chars)
✅ Allows user to cancel if too long
