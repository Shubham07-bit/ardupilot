# Git Authentication Tutorial: SSH vs HTTPS with Personal Access Tokens

## Table of Contents
1. [Overview](#overview)
2. [Why Authentication Changed](#why-authentication-changed)
3. [Method 1: SSH Authentication (Recommended)](#method-1-ssh-authentication-recommended)
4. [Method 2: HTTPS with Personal Access Token](#method-2-https-with-personal-access-token)
5. [Troubleshooting Common Issues](#troubleshooting-common-issues)
6. [Best Practices](#best-practices)

## Overview

GitHub authentication for Git operations has evolved significantly. This tutorial covers two primary methods for authenticating with GitHub repositories:

- **SSH Authentication**: Uses public/private key pairs (most secure and convenient)
- **HTTPS with Personal Access Token (PAT)**: Uses tokens instead of passwords

## Why Authentication Changed

In August 2021, GitHub discontinued password authentication for Git operations due to security concerns. Passwords are:
- Often weak or reused
- Transmitted in plain text
- Vulnerable to brute force attacks
- Not easily revokable

## Method 1: SSH Authentication (Recommended)

SSH authentication uses cryptographic key pairs and is the most secure and convenient method.

### Step 1: Check for Existing SSH Keys

```bash
ls -la ~/.ssh/
```

Look for files like:
- `id_ed25519` and `id_ed25519.pub` (Ed25519 - recommended)
- `id_rsa` and `id_rsa.pub` (RSA - older but still secure)

### Step 2: Generate SSH Key (if needed)

If you don't have SSH keys, generate them:

```bash
# Generate Ed25519 key (recommended)
ssh-keygen -t ed25519 -C "your.email@example.com"

# Or generate RSA key (if Ed25519 is not supported)
ssh-keygen -t rsa -b 4096 -C "your.email@example.com"
```

**During key generation:**
- Press Enter to accept default file location
- Enter a secure passphrase (optional but recommended)
- Confirm the passphrase

### Step 3: Add SSH Key to SSH Agent

```bash
# Start the SSH agent
eval "$(ssh-agent -s)"

# Add your SSH private key to the agent
ssh-add ~/.ssh/id_ed25519  # or ~/.ssh/id_rsa for RSA keys
```

### Step 4: Add SSH Key to GitHub

1. Copy your public key:
   ```bash
   cat ~/.ssh/id_ed25519.pub  # or ~/.ssh/id_rsa.pub
   ```

2. Go to GitHub.com → Settings → SSH and GPG keys → New SSH key

3. Give it a descriptive title (e.g., "Development Laptop - Ubuntu")

4. Paste your public key content

5. Click "Add SSH key"

### Step 5: Test SSH Connection

```bash
ssh -T git@github.com
```

Expected output:
```
Hi username! You've successfully authenticated, but GitHub does not provide shell access.
```

### Step 6: Configure Repository for SSH

For existing repositories using HTTPS:
```bash
# Check current remote
git remote -v

# Change to SSH
git remote set-url origin git@github.com:username/repository.git
```

For new repositories:
```bash
git clone git@github.com:username/repository.git
```

### Step 7: Configure SSH (Optional)

Create `~/.ssh/config` for easier management:

```bash
# GitHub
Host github.com
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519
    IdentitiesOnly yes
```

## Method 2: HTTPS with Personal Access Token

If you prefer HTTPS or SSH is not available in your environment.

### Step 1: Create Personal Access Token

1. Go to GitHub.com → Settings → Developer settings → Personal access tokens → Tokens (classic)

2. Click "Generate new token (classic)"

3. Configure the token:
   - **Note**: Give it a descriptive name (e.g., "ArduPilot Development")
   - **Expiration**: Choose appropriate duration
   - **Scopes**: Select required permissions:
     - `repo` (for private repositories)
     - `public_repo` (for public repositories)
     - `workflow` (if using GitHub Actions)

4. Click "Generate token"

5. **Important**: Copy the token immediately (you won't see it again!)

### Step 2: Configure Git Credential Helper

```bash
# Cache credentials for 1 hour (3600 seconds)
git config --global credential.helper 'cache --timeout=3600'

# Or store credentials permanently (less secure)
git config --global credential.helper store
```

### Step 3: Use Token for Authentication

When Git prompts for credentials:
- **Username**: Your GitHub username
- **Password**: Your Personal Access Token (not your GitHub password)

### Step 4: Configure Repository for HTTPS

```bash
# For existing repositories
git remote set-url origin https://github.com/username/repository.git

# For new repositories
git clone https://github.com/username/repository.git
```

### Step 5: Alternative - Embed Token in URL (Not Recommended)

You can embed the token in the URL, but this is less secure:

```bash
git remote set-url origin https://username:TOKEN@github.com/username/repository.git
```

**Warning**: This method stores the token in plain text in your Git configuration.

## Troubleshooting Common Issues

### Issue 1: "Permission denied (publickey)"

**Cause**: SSH key not properly configured or not added to GitHub.

**Solutions**:
```bash
# Check if SSH agent is running
ssh-add -l

# Add key to agent if missing
ssh-add ~/.ssh/id_ed25519

# Test connection
ssh -T git@github.com

# Debug connection
ssh -vT git@github.com
```

### Issue 2: "Authentication failed" with HTTPS

**Cause**: Using GitHub password instead of Personal Access Token.

**Solutions**:
- Ensure you're using a Personal Access Token as the password
- Check if token has expired
- Verify token has correct scopes/permissions

### Issue 3: "remote: Invalid username or token"

**Cause**: Incorrect username or expired/invalid token.

**Solutions**:
```bash
# Clear cached credentials
git config --global --unset credential.helper
git config --unset credential.helper

# Or clear credential cache
git credential-cache exit
```

### Issue 4: SSH key passphrase prompts

**Solutions**:
```bash
# Add key to SSH agent with passphrase caching
ssh-add ~/.ssh/id_ed25519

# Configure SSH agent to start automatically (add to ~/.bashrc or ~/.zshrc)
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519
```

## Best Practices

### Security Best Practices

1. **Use SSH when possible**: More secure and convenient
2. **Use strong passphrases**: For SSH keys and GitHub account
3. **Rotate tokens regularly**: Set expiration dates on Personal Access Tokens
4. **Minimize token scope**: Only grant necessary permissions
5. **Never share private keys**: Keep SSH private keys secure
6. **Use different keys for different purposes**: Separate keys for different projects/environments

### Workflow Best Practices

1. **Test authentication before working**:
   ```bash
   # For SSH
   ssh -T git@github.com
   
   # For HTTPS
   git ls-remote origin
   ```

2. **Keep backups of SSH keys**: Store them securely

3. **Document your setup**: Keep track of which keys/tokens are used where

4. **Use descriptive names**: For SSH keys and tokens to identify their purpose

### Multi-Account Setup

If you work with multiple GitHub accounts:

```bash
# ~/.ssh/config
Host github-work
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519_work

Host github-personal
    HostName github.com
    User git
    IdentityFile ~/.ssh/id_ed25519_personal
```

Then clone with:
```bash
git clone git@github-work:company/repository.git
git clone git@github-personal:username/repository.git
```

## Quick Reference Commands

### SSH Setup
```bash
# Generate key
ssh-keygen -t ed25519 -C "email@example.com"

# Start agent and add key
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519

# Test connection
ssh -T git@github.com

# Set remote to SSH
git remote set-url origin git@github.com:username/repo.git
```

### HTTPS Setup
```bash
# Configure credential helper
git config --global credential.helper 'cache --timeout=3600'

# Set remote to HTTPS
git remote set-url origin https://github.com/username/repo.git
```

### Common Commands
```bash
# Check remote URLs
git remote -v

# Check Git configuration
git config --list | grep -E "(user|credential|remote)"

# Clear credentials
git credential-cache exit
```

## Conclusion

SSH authentication is generally recommended for development work due to its security and convenience. Personal Access Tokens with HTTPS are useful for automated systems or when SSH is not available.

Choose the method that best fits your workflow and security requirements. Both methods are secure when configured properly.

---

**Created**: July 30, 2025  
**Author**: GitHub Copilot  
**Version**: 1.0
