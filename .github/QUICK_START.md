# Quick Start Guide: Branch Synchronization

## Prerequisites

Both `main` and `develop-main` branches must exist in the repository.

**Current Status**: Only `main` branch exists. The workflow will report "only one branch exists" and exit without creating PRs.

## Running the Workflow

### Option 1: GitHub Web UI

1. Go to https://github.com/Improvedoutlook/Logger/actions
2. Click on "Synchronize Main and Develop-Main Branches" in the left sidebar
3. Click "Run workflow" button (top right)
4. Select the branch to run from (usually `main`)
5. Optional: Check "dry_run" to preview without creating PRs
6. Click "Run workflow" (green button)

### Option 2: GitHub CLI

```bash
# Normal run
gh workflow run sync-branches.yml

# Dry run
gh workflow run sync-branches.yml -f dry_run=true
```

## Expected Behavior

### When Only One Branch Exists
- ⚠️ Workflow exits with message: "Only one of the branches exists. No PR will be created."
- No PRs are created
- No changes are made

### When Both Branches Exist and Are Identical
- ✅ Workflow reports: "Branches are already identical"
- No PRs are created
- No changes are made

### When Both Branches Exist and Are Different (No Conflicts)
1. Creates PR: develop-main → main
2. Automatically merges the PR
3. Verifies if branches are identical
4. If not identical, creates second PR: main → develop-main
5. Automatically merges the second PR
6. Final verification confirms both branches are identical

### When Conflicts Exist
1. Creates PR with conflict details
2. Adds labels: `needs-review`, `sync-conflict`
3. Lists conflicting files in PR body
4. Leaves PR open for manual resolution
5. Does NOT attempt to merge

### When Branch Protection Exists
1. Creates PR normally
2. Attempts to merge
3. If blocked by protection, adds note to PR body
4. Leaves PR open for manual approval/merge

## Monitoring

Check the Actions tab for workflow execution status:
- ✅ Green check: Success
- ⚠️ Yellow: In progress
- ❌ Red: Failed (check logs)

## Troubleshooting

### Workflow doesn't appear
- Ensure the workflow file is committed to the default branch
- Check `.github/workflows/sync-branches.yml` exists

### "Only one branch exists" message
- Create the missing branch (`develop-main`)
- Push the branch to origin
- Re-run the workflow

### PR not merging automatically
- Check for merge conflicts (review PR description)
- Check branch protection rules
- Verify workflow has write permissions

## Learn More

See `.github/BRANCH_SYNC.md` for detailed documentation.
