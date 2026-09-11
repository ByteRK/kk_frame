// 为调试提供「弹窗选择 X11 DISPLAY」的命令输入（命令 ID: pickX11Display）。
//
// 作为 launch.json 的 inputs(type=command) 使用：每次启动调试时实时探测当前
// 真正可用的显示并弹窗，因此不需要把 DISPLAY 写死，也不会用到过期的显示号。
const vscode = require('vscode');
const cp = require('child_process');
const fs = require('fs');
const path = require('path');

// 探测脚本与状态文件位于项目的 .vscode/x11/ 下。
// 扩展被安装到 ~/.vscode-server/extensions 后 __dirname 已不在项目内，
// 因此只能通过工作区根目录定位（未打开文件夹时退回 __dirname，此时探测会失败）。
function projectPath(name) {
    const folders = vscode.workspace.workspaceFolders;
    if (folders && folders.length > 0) {
        return path.join(folders[0].uri.fsPath, '.vscode', 'x11', name);
    }
    return path.join(__dirname, name);
}

/** 调用探测脚本，返回当前实测可用的 DISPLAY 列表（会话新的在前）。 */
function detectDisplays() {
    const script = projectPath('displays.py');
    if (!fs.existsSync(script)) {
        return [];
    }
    try {
        const out = cp.execFileSync('python3', [script], {
            encoding: 'utf8',
            timeout: 15000,
            stdio: ['ignore', 'pipe', 'ignore'],
        });
        return out
            .split('\n')
            .map((s) => s.trim())
            .filter(Boolean);
    } catch (err) {
        return [];
    }
}

/** 上次使用的 DISPLAY（来自 display.env），用作默认选中项。 */
function readLastUsed() {
    try {
        const m = fs
            .readFileSync(projectPath('display.env'), 'utf8')
            .match(/^\s*DISPLAY\s*=\s*(.+?)\s*$/m);
        return m ? m[1] : '';
    } catch (err) {
        return '';
    }
}

/** 记住本次选择，作为下次的默认值。 */
function remember(display) {
    try {
        fs.writeFileSync(projectPath('display.env'), `DISPLAY=${display}\n`);
    } catch (err) {
        // 记录失败不影响调试
    }
}

/** 命令实现：返回选中的 DISPLAY 字符串，供 ${input:x11Display} 使用。 */
async function pickDisplay() {
    const last = readLastUsed();
    const current = process.env.DISPLAY || '';

    // 候选顺序：当前环境变量 > 上次使用 > 实测可用
    const candidates = [];
    for (const d of [current, last, ...detectDisplays()]) {
        if (d && !candidates.includes(d)) {
            candidates.push(d);
        }
    }

    if (candidates.length === 0) {
        vscode.window.showWarningMessage(
            '未探测到可用的 X11 DISPLAY，请确认 MobaXterm 的 X11 转发已开启。'
        );
        return last;
    }

    if (candidates.length === 1) {
        remember(candidates[0]);
        return candidates[0];
    }

    const items = candidates.map((d) => ({
        label: d,
        description: d === last ? '上次使用' : '',
        detail: d === current ? '来自当前环境变量' : undefined,
        picked: d === last,
    }));

    const selected = await vscode.window.showQuickPick(items, {
        title: '选择本次调试使用的 X11 DISPLAY',
        placeHolder: `探测到 ${candidates.length} 个可用显示（MobaXterm X11 转发）`,
        ignoreFocusOut: true,
    });

    // 取消时退回「上次使用 / 首个」，避免 DISPLAY 为空导致启动失败
    const result = (selected && selected.label) || last || candidates[0];
    remember(result);
    return result;
}

function activate(context) {
    context.subscriptions.push(
        vscode.commands.registerCommand('pickX11Display', pickDisplay)
    );
}

function deactivate() {}

module.exports = { activate, deactivate };
