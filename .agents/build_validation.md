# Build and Validation Instructions

## Primary Compile Check

- Compile validation for this project should be done by running `./fastCheck.sh` from the project root.
- In normal validation runs, use `./fastCheck.sh` without `-r`. Only add `-r` when the user explicitly asks for it or the current task specifically requires the script's run/runtime behavior.
- Treat `./fastCheck.sh` success as the primary build verification signal. A successful run exits with status 0; only the compile result needs to be judged unless runtime validation was explicitly requested.
- The final target name depends on the current project. Do not assume it is `kk_frame` when these instructions are reused from the template.
- The script may build outside the app workspace under `/home/ricken/cdroid/outX64-Debug`, so running it can require filesystem escalation in sandboxed sessions.

## When to Run

- Run `./fastCheck.sh` after changes to C/C++ source, headers, CMake/build configuration, resources that affect packaging/runtime behavior, or any change that could affect compiled output.
- It is acceptable to skip `./fastCheck.sh` for script-only, documentation-only, `.gitignore`, or agent-memory updates that do not touch application logic.
- If validation is skipped, mention the reason explicitly in the final response.
