# Project Instructions for AI Agents

These instructions apply to every AI model or coding agent that reads this file, regardless of vendor or product.

## Scope

- This file defines the baseline instructions for working in this repository.
- More specific instructions may live under `.agents/`. Those files are task-specific and should be loaded only when relevant.

## Instruction Loading

- Do not read every file under `.agents/` by default.
- Before starting an operation, identify whether that operation has a matching instruction file listed below.
- If a matching file exists, read it before performing the operation and follow its rules.
- If `.agents/` or the matching file does not exist, continue with this file and the repository context that is directly relevant to the task.

## Task-Specific Instructions

- Git workflow: for committing, merging, rebasing, branch synchronization, pushing, or other git history operations, read `.agents/git_workflow.md`.
- Build and validation: before changing C/C++ source, headers, CMake/build configuration, resources that may affect compiled output, or before doing any build, compile, test, or validation work, read `.agents/build_validation.md`.

## Adding Instructions

- Add new `.agents/*.md` files for distinct operational areas instead of expanding this file with detailed process rules.
- Keep this file as the entry point and index for when those task-specific files should be read.
