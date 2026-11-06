# Git and Commit Instructions

## Commit Message Style

- Commit messages should follow the existing history style: start with an emoji that describes the change type, then an English summary in title-style wording.
- Keep simple maintenance changes concise, but use a more descriptive phrase when the behavior or scope needs it.
- Common prefixes include `✨` for additions/updates/support, `🐋` for adjustments, `🐛` for fixes, `✏️` for small edits, `🗑️` for removals, `👓` for tooling/scripts, and `⚒️` for follow-up work.
- Examples: `✨ Update Some Utils`, `🐋 Adjust Page Registration and Pop Behavior`, `🐛 Clean up wild pointers`, `🗑️ Turn off unnecessary click sound effects.`, `👓 Add Python Script :  Audio Gain Tool`.

## Branch Model

- `master` — stable release branch, always deployable
- `dev` — main development branch, all feature work happens here
- Feature branches (e.g. `feature/xxx`) — optional, for complex/long-running work

## Core Rules

1. **Never commit directly to `master`.** All development happens on `dev` or feature branches.
2. **Use rebase, not merge**, to keep history linear and avoid meaningless merge commits.
3. Use `--ff-only` when advancing a long-lived branch without rebasing it.
4. **`git config pull.rebase true`** is already set globally, so `git pull` rebases by default.

## Index and Commit Authorization

- **Never run `git add`, `git rm --cached`, `git reset`, `git stash`, or `git commit` unless the user's current request explicitly asks for it.** Finishing code edits is not permission to stage or commit them.
- Leaving changes unstaged is the default end state for every task: report what changed and stop.
- Do not stage files "to keep a clean working tree", to prepare a later commit, or because an earlier turn in the same conversation approved staging once.
- Authorization is per-operation and per-request: approval to commit does not imply approval to stage unrelated files; approval to stage does not imply approval to commit.
- Read-only git usage (`git status`, `git diff`, `git log`, `git show`, `git blame`) is always allowed.

## Remote Write Authorization

- **Never push unless the user's current request explicitly asks for a remote push.** A request to edit, commit, sync branches, merge `dev` into `master`, or prepare a release authorizes local operations only.
- Do not treat workflow examples, earlier push approval, or a local branch being ahead as permission to push.
- Before pushing, inspect the remote's push URLs. If one remote name targets multiple repositories, state all destinations and obtain explicit confirmation unless the user already requested all of them.
- Push only the branch(es) explicitly requested. Do not automatically push `dev` before pushing `master`, or vice versa.
- Never force-push `master`. Force-pushing any other shared branch requires explicit approval and coordination.

## Local Development Workflow

The commands below are executed **only when the user explicitly requests** the corresponding operation (`git add` / `git commit` must be requested; see "Index and Commit Authorization").

```bash
git checkout dev
git pull --rebase origin dev

git add <files>
git commit -m "✨ Your change description"
```

Only when the user explicitly requests pushing `dev`:

```bash
git push origin dev
```

## Syncing dev to master Locally

```bash
git checkout master
git fetch origin
git merge --ff-only dev
git checkout dev
```

Only when the user explicitly requests pushing `master`:

```bash
git push origin master
```

If fast-forward fails because the branches diverged, inspect the history before choosing the rebase direction. Do not guess or create a merge commit.

## Feature Branches

```bash
git checkout -b feature/my-feature dev

git fetch origin
git rebase origin/dev

git checkout dev
git merge --ff-only feature/my-feature
git branch -d feature/my-feature
```

## Prohibited Operations

- Do not stage or commit without an explicit request (no `git add`, `git rm --cached`, `git reset`, `git stash`, or `git commit` on your own initiative).
- Do not merge long-lived branches with a merge commit.
- Do not commit directly to `master`.
- Do not run an unrequested `git push`.
- Do not force-push `master`.
- Do not mix merge and rebase strategies on the same branch.
