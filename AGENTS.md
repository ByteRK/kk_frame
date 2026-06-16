# Project Notes for Codex

- Compile validation for this project should be done by running `./fastCheck.sh` from the project root.
- Treat `./fastCheck.sh` success as the primary build verification signal. A successful run ends with `[100%] Built target kk_frame`.
- The script may build outside the app workspace under `/home/ricken/cdroid/outX64-Debug`, so running it can require filesystem escalation in sandboxed sessions.
- Commit messages should follow the existing history style: an emoji prefix plus a short English title, for example `✨ Update Some Utils`, `🐛 Fix int`, or `✏️ Adjuest PBase::canAutoRecycle`.
