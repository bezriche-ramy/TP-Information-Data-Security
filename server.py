#!/usr/bin/env python3
"""
Authentication Server - Python Version
Pure brute force password cracking demonstration
Educational purposes only
"""

import hashlib
import secrets
import json
import csv
import os
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse
import re

PORT = 3000
PASSWORD_FILE = "password"

class User:
    def __init__(self, username, salt, hash_value):
        self.username = username
        self.salt = salt
        self.hash = hash_value

def hash_password(password, salt):
    """Hash password using PBKDF2 with SHA256"""
    return hashlib.pbkdf2_hmac('sha256', password.encode(), salt.encode(), 100000).hex()

def validate_username(username):
    """Username must be exactly 5 alphanumeric characters"""
    return bool(re.match(r'^[a-zA-Z0-9]{5}$', username))

def validate_password(password):
    """Password must be 8+ chars with lowercase, uppercase, digit and special (@#$&)"""
    if len(password) < 8:
        return False
    has_lower = bool(re.search(r'[a-z]', password))
    has_upper = bool(re.search(r'[A-Z]', password))
    has_digit = bool(re.search(r'[0-9]', password))
    has_special = bool(re.search(r'[@#$&]', password))
    return has_lower and has_upper and has_digit and has_special

def load_users():
    """Load users from CSV file"""
    users = []
    if os.path.exists(PASSWORD_FILE):
        with open(PASSWORD_FILE, 'r') as f:
            reader = csv.reader(f)
            for row in reader:
                if len(row) >= 3:
                    users.append(User(row[0], row[1], row[2]))
    return users

def save_user(username, password):
    """Save new user to CSV file"""
    salt = str(secrets.randbelow(90000) + 10000)  # 5-digit random salt
    hash_value = hash_password(password, salt)
    
    with open(PASSWORD_FILE, 'a') as f:
        writer = csv.writer(f)
        writer.writerow([username, salt, hash_value])

def find_user(username):
    """Find user by username"""
    users = load_users()
    for user in users:
        if user.username == username:
            return user
    return None

class AuthHandler(BaseHTTPRequestHandler):
    
    def do_OPTIONS(self):
        """Handle CORS preflight"""
        self.send_response(200)
        self.send_header('Access-Control-Allow-Origin', '*')
        self.send_header('Access-Control-Allow-Methods', 'GET, POST, OPTIONS')
        self.send_header('Access-Control-Allow-Headers', 'Content-Type')
        self.end_headers()
    
    def do_POST(self):
        """Handle POST requests"""
        parsed_path = urlparse(self.path)
        
        # Read body
        content_length = int(self.headers.get('Content-Length', 0))
        body = self.rfile.read(content_length).decode('utf-8')
        
        try:
            data = json.loads(body) if body else {}
        except:
            data = {}
        
        if parsed_path.path == '/api/signup':
            self.handle_signup(data)
        elif parsed_path.path == '/api/signin':
            self.handle_signin(data)
        else:
            self.send_json_response(404, {'ok': False, 'msg': 'not found'})
    
    def handle_signup(self, data):
        """Handle signup request"""
        username = data.get('username', '')
        password = data.get('password', '')
        
        if not username or not password:
            self.send_json_response(400, {'ok': False, 'msg': 'username and password required'})
            return
        
        if not validate_username(username):
            self.send_json_response(400, {'ok': False, 'msg': 'username must be 5 alphanumeric characters'})
            return
        
        if not validate_password(password):
            self.send_json_response(400, {'ok': False, 'msg': 'password must be 8+ chars with lowercase, uppercase, digit and special (@#$&)'})
            return
        
        if find_user(username):
            self.send_json_response(400, {'ok': False, 'msg': 'username already exists'})
            return
        
        save_user(username, password)
        self.send_json_response(200, {'ok': True, 'msg': 'signup successful'})
    
    def handle_signin(self, data):
        """Handle signin request"""
        username = data.get('username', '')
        password = data.get('password', '')
        
        if not username or not password:
            self.send_json_response(400, {'ok': False, 'msg': 'username and password required'})
            return
        
        user = find_user(username)
        if not user:
            self.send_json_response(401, {'ok': False, 'msg': 'user not found'})
            return
        
        # Check password
        attempted_hash = hash_password(password, user.salt)
        
        if attempted_hash == user.hash:
            self.send_json_response(200, {'ok': True, 'msg': 'signin successful'})
        else:
            self.send_json_response(401, {'ok': False, 'msg': 'wrong password'})
    
    def send_json_response(self, status, data):
        """Send JSON response"""
        self.send_response(status)
        self.send_header('Content-Type', 'application/json')
        self.send_header('Access-Control-Allow-Origin', '*')
        self.end_headers()
        self.wfile.write(json.dumps(data).encode())
    
    def log_message(self, format, *args):
        """Suppress default logging"""
        pass

def main():
    server_address = ('', PORT)
    httpd = HTTPServer(server_address, AuthHandler)
    print(f'Auth server listening on http://localhost:{PORT}')
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print('\nServer stopped')

if __name__ == '__main__':
    main()
