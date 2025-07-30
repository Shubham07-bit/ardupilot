# Git Authentication Quick Reference Card

## 🔑 SSH Authentication (Recommended)

### Setup (One-time)
```bash
# 1. Generate SSH key
ssh-keygen -t ed25519 -C "your.email@example.com"

# 2. Start SSH agent and add key
eval "$(ssh-agent -s)"
ssh-add ~/.ssh/id_ed25519

# 3. Copy public key to clipboard
cat ~/.ssh/id_ed25519.pub

# 4. Add to GitHub: Settings → SSH and GPG keys → New SSH key

# 5. Test connection
ssh -T git@github.com
```

### Configure Repository
```bash
# Switch existing repo to SSH
git remote set-url origin git@github.com:username/repository.git

# Clone new repo with SSH
git clone git@github.com:username/repository.git
```

---

## 🌐 HTTPS with Personal Access Token

### Setup (One-time)
```bash
# 1. Create PAT: GitHub → Settings → Developer settings → Personal access tokens
# 2. Configure credential helper
git config --global credential.helper 'cache --timeout=3600'
```

### Configure Repository
```bash
# Switch existing repo to HTTPS
git remote set-url origin https://github.com/username/repository.git

# Clone new repo with HTTPS
git clone https://github.com/username/repository.git
```

### When prompted for credentials:
- **Username**: Your GitHub username
- **Password**: Your Personal Access Token (NOT your GitHub password)

---

## 🔧 Troubleshooting

### Check Current Setup
```bash
git remote -v                    # Check remote URLs
git config --list | grep user   # Check user config
ssh -T git@github.com           # Test SSH connection
```

### Fix Authentication Issues
```bash
# Clear cached credentials
git credential-cache exit

# Re-add SSH key
ssh-add ~/.ssh/id_ed25519

# Debug SSH connection
ssh -vT git@github.com
```

### Common Error Solutions

| Error | Solution |
|-------|----------|
| `Permission denied (publickey)` | Add SSH key to agent: `ssh-add ~/.ssh/id_ed25519` |
| `Authentication failed` | Use Personal Access Token, not GitHub password |
| `Invalid username or token` | Check username and verify token hasn't expired |

---

## ⚡ Quick Commands

```bash
# Check what authentication method you're using
git remote get-url origin

# SSH format: git@github.com:username/repo.git
# HTTPS format: https://github.com/username/repo.git

# Switch between methods
git remote set-url origin git@github.com:username/repo.git       # To SSH
git remote set-url origin https://github.com/username/repo.git   # To HTTPS
```

---

**💡 Pro Tip**: SSH is more convenient for daily development work, while HTTPS with PAT is better for automation and CI/CD systems.
