# Branch Synchronization Workflow

## Overview

This repository includes an automated workflow to synchronize the `main` and `develop-main` branches, ensuring they remain identical.

## How It Works

The workflow (`sync-branches.yml`) performs the following steps:

1. **Branch Existence Check**: Verifies that both `main` and `develop-main` branches exist
   - If only one branch exists, the workflow exits with a message and does not create any PRs

2. **First PR (develop-main → main)**: Creates a pull request to merge `develop-main` into `main`
   - Lists all changed files in the PR body
   - Detects merge conflicts and adds them to the PR description
   - If conflicts exist, adds labels `needs-review` and `sync-conflict`
   - If no conflicts and branch protection allows, automatically merges the PR

3. **Identity Verification**: After the first merge, checks if both branches are byte-for-byte identical
   - Compares commit SHAs to verify identity

4. **Second PR (main → develop-main)**: If branches are not identical after the first merge
   - Creates another PR to merge `main` into `develop-main`
   - Again detects conflicts and labels appropriately
   - Automatically merges if possible

5. **Final Verification**: Confirms both branches are identical

## Running the Workflow

### Manual Trigger

The workflow is triggered manually via GitHub Actions:

1. Go to **Actions** tab in the repository
2. Select **Synchronize Main and Develop-Main Branches**
3. Click **Run workflow**
4. Optional: Enable dry run mode to see what would happen without creating PRs

### Dry Run Mode

You can run the workflow in dry run mode to preview the actions without creating or merging PRs:

```yaml
inputs:
  dry_run: 'true'
```

## Conflict Handling

When merge conflicts are detected:

- The PR remains open (not merged)
- Conflicting files are listed in the PR body
- Labels `needs-review` and `sync-conflict` are added
- A clear conflict summary is provided

**Important**: The workflow will NOT force-push or perform destructive operations. Manual conflict resolution is required.

## Branch Protection

If branch protection rules prevent automatic merging:

- The PR remains open
- A note about branch protection is added to the PR body
- Manual approval/review may be required according to the protection rules

## PR Format

All PRs created include:

- **Title**: "Sync: Merge [source] into [target]"
- **Body**: Contains:
  - Standard note: "Sync PR created by GitHub Copilot Chat Assistant at user's request (owner: improvedoutlook) — please review and merge."
  - List of changed files
  - Conflict detection status
  - Branch protection notes (if applicable)

## Requirements

- Both `main` and `develop-main` branches must exist
- Workflow needs `contents: write` and `pull-requests: write` permissions
- GitHub token must have access to create PRs and merge them

## Limitations

- Does not set assignees or reviewers automatically
- Does not force-push or rewrite history
- Does not squash or rebase - only normal merge commits
- Requires manual intervention for conflicts
