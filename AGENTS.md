# Project Instructions for AI Agents

These instructions apply to every AI model or coding agent that reads this file, regardless of vendor or product.

## Additional Instructions

- Before working in this repository, read every regular file under `.agents/` and treat its contents as additional project instructions.
- If `.agents/` does not exist (for example, in a project generated from this template), continue using this file alone.

## Build & Validation

- Compile validation for this project should be done by running `./fastCheck.sh` from the project root.
- Treat `./fastCheck.sh` success as the primary build verification signal. A successful run ends with `[100%] Built target kk_frame`.
- The script may build outside the app workspace under `/home/ricken/cdroid/outX64-Debug`, so running it can require filesystem escalation in sandboxed sessions.
- Not every change requires running `./fastCheck.sh`. Run it for C/C++ source, headers, CMake/build configuration, resources that affect packaging/runtime behavior, or any change that could affect compiled output. It is acceptable to skip it for script-only, documentation-only, `.gitignore`, or agent-memory updates that do not touch application logic; mention the skip explicitly in the final response.
