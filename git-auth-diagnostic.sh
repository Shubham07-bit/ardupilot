#!/bin/bash

# Git Authentication Diagnostic Script
# This script helps diagnose common Git authentication issues

echo "🔍 Git Authentication Diagnostic Tool"
echo "====================================="
echo

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Function to print colored output
print_status() {
    local status=$1
    local message=$2
    case $status in
        "OK")
            echo -e "${GREEN}✓${NC} $message"
            ;;
        "WARN")
            echo -e "${YELLOW}⚠${NC} $message"
            ;;
        "ERROR")
            echo -e "${RED}✗${NC} $message"
            ;;
        "INFO")
            echo -e "${BLUE}ℹ${NC} $message"
            ;;
    esac
}

echo "1. Checking Git Configuration"
echo "----------------------------"

# Check if git is installed
if command -v git &> /dev/null; then
    print_status "OK" "Git is installed: $(git --version)"
else
    print_status "ERROR" "Git is not installed"
    exit 1
fi

# Check user configuration
if git config user.name &> /dev/null && git config user.email &> /dev/null; then
    print_status "OK" "Git user configured: $(git config user.name) <$(git config user.email)>"
else
    print_status "WARN" "Git user not configured. Run:"
    echo "  git config --global user.name \"Your Name\""
    echo "  git config --global user.email \"your.email@example.com\""
fi

echo
echo "2. Checking Repository Configuration"
echo "-----------------------------------"

# Check if we're in a git repository
if git rev-parse --git-dir &> /dev/null; then
    print_status "OK" "Inside a Git repository"
    
    # Check remote configuration
    if git remote get-url origin &> /dev/null; then
        remote_url=$(git remote get-url origin)
        print_status "INFO" "Remote origin: $remote_url"
        
        if [[ $remote_url == git@github.com:* ]]; then
            auth_method="SSH"
            print_status "INFO" "Using SSH authentication"
        elif [[ $remote_url == https://github.com/* ]]; then
            auth_method="HTTPS"
            print_status "INFO" "Using HTTPS authentication"
        else
            auth_method="OTHER"
            print_status "WARN" "Unknown authentication method"
        fi
    else
        print_status "WARN" "No remote 'origin' configured"
        auth_method="NONE"
    fi
else
    print_status "WARN" "Not inside a Git repository"
    auth_method="NONE"
fi

echo
echo "3. SSH Configuration Check"
echo "-------------------------"

# Check SSH key existence
ssh_keys_found=false
if [ -f ~/.ssh/id_ed25519 ]; then
    print_status "OK" "Ed25519 SSH key found: ~/.ssh/id_ed25519"
    ssh_keys_found=true
elif [ -f ~/.ssh/id_rsa ]; then
    print_status "OK" "RSA SSH key found: ~/.ssh/id_rsa"
    ssh_keys_found=true
else
    print_status "WARN" "No SSH keys found in ~/.ssh/"
fi

if $ssh_keys_found; then
    # Check if SSH agent is running
    if ssh-add -l &> /dev/null; then
        print_status "OK" "SSH agent is running with keys loaded"
        echo "Loaded keys:"
        ssh-add -l | sed 's/^/  /'
    else
        print_status "WARN" "SSH agent not running or no keys loaded"
        echo "To fix: eval \"\$(ssh-agent -s)\" && ssh-add ~/.ssh/id_ed25519"
    fi
    
    # Test GitHub SSH connection
    echo
    print_status "INFO" "Testing SSH connection to GitHub..."
    if ssh -T git@github.com 2>&1 | grep -q "successfully authenticated"; then
        print_status "OK" "SSH connection to GitHub successful"
    else
        print_status "ERROR" "SSH connection to GitHub failed"
        echo "Make sure your SSH key is added to your GitHub account"
    fi
fi

echo
echo "4. HTTPS Configuration Check"
echo "---------------------------"

# Check credential helper
credential_helper=$(git config --get credential.helper 2>/dev/null)
if [ -n "$credential_helper" ]; then
    print_status "OK" "Credential helper configured: $credential_helper"
else
    print_status "WARN" "No credential helper configured"
    echo "To fix: git config --global credential.helper 'cache --timeout=3600'"
fi

echo
echo "5. Connectivity Test"
echo "-------------------"

# Test internet connectivity to GitHub
if ping -c 1 github.com &> /dev/null; then
    print_status "OK" "Can reach GitHub"
else
    print_status "ERROR" "Cannot reach GitHub (network issue?)"
fi

# Test HTTPS connectivity
if curl -s https://api.github.com/user &> /dev/null; then
    print_status "OK" "HTTPS connection to GitHub API works"
else
    print_status "WARN" "HTTPS connection to GitHub API failed"
fi

echo
echo "6. Recommendations"
echo "-----------------"

if [ "$auth_method" = "SSH" ]; then
    if $ssh_keys_found && ssh -T git@github.com 2>&1 | grep -q "successfully authenticated"; then
        print_status "OK" "SSH authentication is properly configured"
    else
        print_status "WARN" "SSH authentication needs attention"
        echo "Recommendations:"
        echo "1. Ensure SSH key is added to GitHub: https://github.com/settings/keys"
        echo "2. Run: ssh-add ~/.ssh/id_ed25519"
        echo "3. Test: ssh -T git@github.com"
    fi
elif [ "$auth_method" = "HTTPS" ]; then
    print_status "INFO" "Using HTTPS authentication"
    echo "Make sure to use Personal Access Token when prompted for password"
    echo "Create token at: https://github.com/settings/tokens"
elif [ "$auth_method" = "NONE" ]; then
    print_status "INFO" "No remote configured or not in a Git repository"
    echo "If working with GitHub, consider using SSH for convenience:"
    echo "git remote add origin git@github.com:username/repository.git"
fi

echo
echo "7. Quick Fixes"
echo "-------------"

echo "Switch to SSH authentication:"
echo "  git remote set-url origin git@github.com:username/repository.git"
echo
echo "Switch to HTTPS authentication:"
echo "  git remote set-url origin https://github.com/username/repository.git"
echo
echo "Generate new SSH key:"
echo "  ssh-keygen -t ed25519 -C \"your.email@example.com\""
echo
echo "Configure credential helper for HTTPS:"
echo "  git config --global credential.helper 'cache --timeout=3600'"

echo
echo "🎯 Diagnostic complete!"
