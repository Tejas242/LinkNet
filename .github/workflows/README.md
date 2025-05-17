# GitHub Actions Workflows

This document provides an overview of the GitHub Actions workflows configured for the LinkNet project.

## Available Workflows

### 1. CI (Continuous Integration)

**File**: `.github/workflows/ci.yml`

This workflow runs on every push to main, master, and develop branches, as well as on pull requests to these branches.

**Jobs**:
- **Build and Test**: Builds the project and runs tests on multiple configurations (Release/Debug)
- **Code Analysis**: Runs clang-tidy for static code analysis
- **Sanitizers**: Builds and runs tests with Address Sanitizer to detect memory issues

### 2. Release

**File**: `.github/workflows/release.yml`

This workflow runs when a new tag with format `v*.*.*` is pushed to the repository.

**Jobs**:
- **Create Release**: Creates a new GitHub release
- **Build Linux**: Builds the Linux binary and uploads it as a release asset

### 3. Code Quality & Security

**File**: `.github/workflows/code-quality.yml`

This workflow runs on pushes to main, master, and develop branches, on pull requests to main/master, weekly, and can be triggered manually.

**Jobs**:
- **CodeQL Analysis**: Runs GitHub's CodeQL security scanning
- **C++ Linting**: Checks code formatting with clang-format
- **Cppcheck Analysis**: Runs cppcheck for additional static analysis
- **Dependency Review**: Scans dependencies for security vulnerabilities on pull requests

## How to Use

### Automatic Runs

Most workflows run automatically on specific triggers like pushing code or creating pull requests.

### Manual Runs

You can manually trigger the Deploy and Code Quality workflows from the Actions tab in your GitHub repository.

### Creating Releases

To create a new release:
1. Tag your commit with a version number: `git tag v1.0.0`
2. Push the tag: `git push origin v1.0.0`
3. The Release workflow will automatically create a GitHub release and build assets

### Customizing Workflows

Each workflow file is heavily commented to make customization easier. Common customizations include:
- Adding additional build configurations
- Modifying test environments
- Changing deployment targets
- Adding notification services

## Troubleshooting

If a workflow fails, check the following:
1. Look at the workflow run logs in the GitHub Actions tab
2. Verify your code builds locally
3. Check that all dependencies are properly declared
4. Ensure secrets are properly configured for workflows that need them

## Security Notes

- The workflows use `GITHUB_TOKEN` which is automatically provided by GitHub
- For deployment to servers, you'll need to add appropriate secrets
- CodeQL scanning helps identify security vulnerabilities in your code
