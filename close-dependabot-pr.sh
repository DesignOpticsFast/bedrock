#!/bin/bash

# Script to close Dependabot PR #52 by merging it
# This will trigger the auto-merge workflow

echo "🔧 Closing Dependabot PR #52..."

# Check if we're in the right repository
if [ ! -d ".git" ]; then
    echo "❌ Not in a git repository"
    exit 1
fi

# Get the current branch
CURRENT_BRANCH=$(git branch --show-current)
echo "📍 Current branch: $CURRENT_BRANCH"

# Check if the Dependabot branch exists
if git show-ref --verify --quiet refs/remotes/origin/dependabot/github_actions/gha-all-d49115c731; then
    echo "✅ Dependabot branch found"
    
    # Create a merge commit to close the PR
    echo "🔄 Creating merge commit..."
    git merge origin/dependabot/github_actions/gha-all-d49115c731 --no-ff -m "merge: Close Dependabot PR #52

- All Dependabot updates already incorporated
- PR #52 can now be closed
- Auto-merge workflow will handle future PRs"

    echo "✅ Merge commit created"
    
    # Push the changes
    echo "🚀 Pushing changes..."
    git push origin $CURRENT_BRANCH
    
    echo "✅ Changes pushed successfully"
    echo "🎉 Dependabot PR #52 should now be closed!"
    
else
    echo "❌ Dependabot branch not found"
    exit 1
fi
