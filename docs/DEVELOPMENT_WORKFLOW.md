# Development Workflow - Bedrock

**Version:** 2.0.0  
**Last Updated:** 2025-01-21  
**Environment:** dev-01 (Primary) + Local Machines (Secondary)

---

## 🎯 **Core Development Policy**

### **ALL CODE CHANGES MUST BE DONE ON DEV-01 FIRST**

**Rationale:**
- ✅ **Consistent build environment** - Linux-based development
- ✅ **Automated testing** - CI/CD pipelines run on dev-01
- ✅ **Resource debugging** - Can test with proper environment
- ✅ **Version control** - All changes tracked in Git
- ✅ **Collaboration** - Team can access and review changes
- ✅ **Quality assurance** - Automated tests and validation

---

## 🚀 **Development Workflow**

### **Phase 1: Development on dev-01**

#### **1.1 Environment Setup**
```bash
# Connect to dev-01 via Tailscale
ssh -i ~/.ssh/github_phoenix -o StrictHostKeyChecking=no mark@100.97.54.75

# Navigate to project
cd /home/ec2-user/workspace/bedrock
```

#### **1.2 Code Changes**
- ✅ **Make all code changes on dev-01**
- ✅ **Test builds on dev-01**
- ✅ **Run automated tests**
- ✅ **Debug issues on dev-01**
- ✅ **Validate functionality**

#### **1.3 Testing & Validation**
```bash
# Build and test on dev-01
cd /home/ec2-user/workspace/bedrock
mkdir -p build && cd build
cmake .. -G Ninja
ninja -k0

# Run tests
ctest --output-on-failure
```

#### **1.4 Commit & Push**
```bash
# Commit changes to dev-01
git add .
git commit -m "feat(core): add new feature"
git push origin feature-branch

# Create PR for review
```

### **Phase 2: Local Machine Testing**

#### **2.1 Sync Changes**
```bash
# On local machine (Mac/Windows)
git pull origin feature-branch
```

#### **2.2 Local Build & Test**
```bash
# Build on local machine
mkdir -p build && cd build
cmake .. -G Ninja
ninja -k0

# Test local-specific behavior
./bedrock_app
```

#### **2.3 Platform-Specific Testing**
- ✅ **macOS**: Native integration, performance
- ✅ **Windows**: Windows-specific features
- ✅ **Linux**: Desktop integration, system calls

---

## 🔧 **Tailscale Integration**

### **Connection Setup**
```bash
# Install Tailscale on dev-01 (already configured)
# Install Tailscale on local machine
# Join same Tailscale network

# Connect via Tailscale IP
ssh -i ~/.ssh/github_phoenix mark@100.97.54.75
```

### **Benefits of Tailscale**
- ✅ **Direct connection** - No NAT issues
- ✅ **Secure** - Encrypted tunnel
- ✅ **Reliable** - No port forwarding needed
- ✅ **Fast** - Low latency connection
- ✅ **Persistent** - Always available

---

## 📋 **Development Standards**

### **Code Quality Requirements**
- ✅ **All code changes on dev-01 first**
- ✅ **Build and test on dev-01**
- ✅ **Commit with descriptive messages**
- ✅ **Create PR for review**
- ✅ **Test on local machine after sync**

### **Testing Requirements**
- ✅ **Build success on dev-01**
- ✅ **Automated tests pass**
- ✅ **Functionality verified**
- ✅ **Local platform testing**

### **Resource Management**
- ✅ **Test resource loading on dev-01**
- ✅ **Verify dependencies work**
- ✅ **Validate configuration**
- ✅ **Test performance**

---

## 🚫 **Anti-Patterns to Avoid**

### **❌ Direct Local Development**
```bash
# DON'T DO THIS
# Making changes directly on local machine
# Without testing on dev-01 first
```

### **❌ Skip Testing**
```bash
# DON'T DO THIS
# Committing without testing on dev-01
# Pushing untested changes
```

### **❌ Environment Issues**
```bash
# DON'T DO THIS
# Hardcoding paths
# Not testing cross-platform compatibility
```

---

## 🔄 **Workflow Examples**

### **Example 1: Adding New Core Feature**

#### **Step 1: Development on dev-01**
```bash
# Connect to dev-01
ssh -i ~/.ssh/github_phoenix mark@100.97.54.75
cd /home/ec2-user/workspace/bedrock

# Create new feature
mkdir -p src/core/features
# Edit feature files
# Test build
mkdir -p build && cd build
cmake .. -G Ninja
ninja -k0

# Run tests
ctest --output-on-failure
```

#### **Step 2: Commit & Push**
```bash
git add .
git commit -m "feat(core): add new feature"
git push origin feature-branch
```

#### **Step 3: Local Testing**
```bash
# On local machine
git pull origin feature-branch
# Build and test locally
# Verify platform-specific behavior
```

### **Example 2: Fixing Build Issues**

#### **Step 1: Debug on dev-01**
```bash
# Connect to dev-01
ssh -i ~/.ssh/github_phoenix mark@100.97.54.75
cd /home/ec2-user/workspace/bedrock

# Debug build issues
# Test with proper environment
# Fix dependencies
# Verify build works
```

#### **Step 2: Test & Commit**
```bash
# Test build and functionality
# Commit fixes
git add .
git commit -m "fix(build): resolve dependency issues"
git push origin feature-branch
```

#### **Step 3: Local Verification**
```bash
# On local machine
git pull origin feature-branch
# Build and test
# Verify fixes work locally
```

---

## 📊 **Quality Metrics**

### **Success Criteria**
- ✅ **Build success on dev-01**
- ✅ **Automated tests pass**
- ✅ **Functionality verified**
- ✅ **Local platform testing**
- ✅ **Code review approved**
- ✅ **CI/CD pipeline passes**

### **Failure Scenarios**
- ❌ **Build fails on dev-01** → Fix on dev-01
- ❌ **Tests fail** → Debug on dev-01
- ❌ **Local build fails** → Sync latest changes
- ❌ **Platform-specific issues** → Test on local machine

---

## 🛠 **Tools & Commands**

### **Dev-01 Development**
```bash
# Connect to dev-01
ssh -i ~/.ssh/github_phoenix mark@100.97.54.75

# Build project
cd /home/ec2-user/workspace/bedrock
mkdir -p build && cd build
cmake .. -G Ninja
ninja -k0

# Run tests
ctest --output-on-failure

# Debug with gdb
gdb ./bedrock_app
```

### **Local Machine Testing**
```bash
# Sync changes
git pull origin feature-branch

# Build locally
mkdir -p build && cd build
cmake .. -G Ninja
ninja -k0

# Test locally
./bedrock_app
```

---

## 📚 **Documentation Updates**

### **Updated Documents**
- ✅ **DEVELOPMENT_WORKFLOW.md** - This document
- ✅ **CODING_STANDARDS.md** - Updated with dev-01 policy
- ✅ **dev-setup.md** - Updated with Tailscale approach

### **Key Changes**
- ✅ **All development on dev-01 first**
- ✅ **Tailscale for secure connection**
- ✅ **Automated testing on dev-01**
- ✅ **Resource debugging on dev-01**
- ✅ **Local testing after sync**

---

## 🎯 **Success Metrics**

### **Development Efficiency**
- ✅ **Faster iteration** - No SSH overhead for every change
- ✅ **Better debugging** - Proper environment on dev-01
- ✅ **Consistent builds** - Same environment for all developers
- ✅ **Quality assurance** - Automated testing on dev-01

### **Code Quality**
- ✅ **All changes tested** - Build and functionality testing on dev-01
- ✅ **Resource management** - Proper dependency handling
- ✅ **Platform compatibility** - Tested on multiple platforms
- ✅ **Version control** - All changes tracked in Git

---

**Maintained By:** Engineering Team  
**Questions:** See `.underlord/docs/GOTCHAS.md` for common issues  
**Last Updated:** 2025-01-21
