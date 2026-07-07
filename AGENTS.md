# Project Notes for Codex

## Build & Validation

- Compile validation for this project should be done by running `./fastCheck.sh` from the project root.
- Treat `./fastCheck.sh` success as the primary build verification signal. A successful run ends with `[100%] Built target kk_frame`.
- The script may build outside the app workspace under `/home/ricken/cdroid/outX64-Debug`, so running it can require filesystem escalation in sandboxed sessions.
- Not every change requires running `./fastCheck.sh`. Run it for C/C++ source, headers, CMake/build configuration, resources that affect packaging/runtime behavior, or any change that could affect compiled output. It is acceptable to skip it for script-only, documentation-only, `.gitignore`, or agent-memory updates that do not touch application logic; mention the skip explicitly in the final response.

## Commit Message Style

- Commit messages should follow the existing history style: start with an emoji that describes the change type, then an English summary in title-style wording. Keep simple maintenance changes concise, but use a more descriptive phrase when the behavior or scope needs it. Common prefixes include `✨` for additions/updates/support/adjustments, `🐛` for fixes, `✏️` for small edits, `🗑️` for removals, `👓` for tooling/scripts, and `⚒️` for follow-up work. Examples from history: `✨ Update Some Utils`, `✨ The registration tool that adjusts the page and pop`, `🐛 Clean up wild pointers`, `🗑️ Turn off unnecessary click sound effects.`, `👓 Add Python Script :  Audio Gain Tool`.

## Git Workflow (Rebase Strategy)

### Branch Model

- `master` — stable release branch, always deployable
- `dev` — main development branch, all feature work happens here
- Feature branches (e.g. `feature/xxx`) — optional, for complex/long-running work

### Core Rules

1. **Never commit directly to `master`.** All development happens on `dev` or feature branches.
2. **Use rebase, not merge**, to keep history linear and avoid meaningless merge commits.
3. **`git config pull.rebase true`** is already set globally. This ensures `git pull` uses rebase by default.

### Daily Workflow

```bash
# 1. Start from dev, sync with remote
git checkout dev
git pull --rebase origin dev

# 2. Make changes and commit
git add <files>
git commit -m "✨ Your change description"

# 3. Push to dev
git push origin dev
```

### Merging dev → master (Fast-Forward Only)

When `dev` is ready to be released:

```bash
git checkout master
git fetch origin
git merge --ff-only dev          # fast-forward only, no merge commit
git push origin master
git checkout dev                  # switch back to dev
```

If fast-forward fails (e.g. hotfix directly on master), use rebase instead:

```bash
git checkout master
git rebase dev                    # or: git checkout dev && git rebase master
git checkout dev
git merge --ff-only master
```

### Feature Branches (Optional)

```bash
# Create from dev
git checkout -b feature/my-feature dev

# Work and commit as usual...
git commit -m "✨ Partial work"

# Sync with dev periodically via rebase (NOT merge)
git fetch origin
git rebase origin/dev

# When done, rebase onto dev and fast-forward merge
git checkout dev
git merge --ff-only feature/my-feature
git branch -d feature/my-feature
```

### What to Avoid

- ❌ `git merge` between long-lived branches (creates meaningless merge commits)
- ❌ Direct commits to `master`
- ❌ `git push --force` on `master` (force push on `dev` is acceptable only after rebase and with coordination)
- ❌ Mixing merge and rebase on the same branch
