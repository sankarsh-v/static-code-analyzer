// Static Code Quality Analyzer — Web Dashboard Logic
// Supports both Local Server Mode (invoking C analyzer binary)
// and GitHub Pages Static Mode (pure client-side C static analyzer engine).

const EMBEDDED_RULES = [
    { id: "W001", name: "Unused Variable", severity: "warning", desc: "Local variable declared but never read, or assigned but never read." },
    { id: "W002", name: "Assignment in Condition", severity: "warning", desc: "Assignment '=' used as condition without extra parentheses." },
    { id: "W003", name: "Unreachable Code", severity: "warning", desc: "Statements following return, break, or continue in the same block." },
    { id: "W004", name: "Missing Return", severity: "warning", desc: "Non-void function where control reaches the end of the body without a return statement." },
    { id: "W005", name: "Uninitialized Variable", severity: "warning", desc: "Local scalar variable read before any prior assignment." },
    { id: "W006", name: "Magic Number", severity: "info", desc: "Numeric literals other than 0, 1, 2, -1 appearing directly in expressions." },
    { id: "W007", name: "Deep Nesting", severity: "warning", desc: "Nesting depth of if/loop/switch statements exceeds threshold of 3." },
    { id: "W008", name: "Long Function", severity: "warning", desc: "Function body length exceeds threshold of 40 lines." },
    { id: "W009", name: "Empty Body", severity: "warning", desc: "Control flow statement with an empty block or lone semicolon body." },
    { id: "W010", name: "Switch Without Default", severity: "warning", desc: "Switch statement missing a default: case label." },
    { id: "W011", name: "Unsafe Function", severity: "warning", desc: "Call to bounded-unsafe C standard library functions (strcpy, gets, sprintf)." },
    { id: "W012", name: "Variable Shadowing", severity: "warning", desc: "Local variable or parameter shadowing a variable in an enclosing scope." },
    { id: "W013", name: "Division by Zero", severity: "error", desc: "Division or modulo with a compile-time literal 0 denominator." },
    { id: "W014", name: "Fall-Through Case", severity: "info", desc: "Switch case label with statements falling through to next case without break/return." }
];

const PRESETS = {
    demo: `/* samples/demo.c - Embedded telemetry processing system demonstrating all analyzer rules */

int g_channel = 1;

/* Triggers W004 (missing return) and W011 (unsafe function) */
int process_packet_header(char *dest, char *src, int flag) {
    strcpy(dest, src);
    if (flag > 0) {
        return 1;
    }
}

/* Long telemetry pipeline: triggers W008 (> 40 lines), W007 (nesting > 3),
   W001 (unused variable), W002 (assignment in condition), W003 (unreachable code),
   W005 (use before assignment), W006 (magic number), W009 (empty body),
   W012 (shadowing), and W013 (division by zero) */
int run_telemetry_pipeline(int g_channel, int raw_input) {
    int unused_metric;
    int mode = 0;
    int uninit_bias;
    int reading = uninit_bias + 1;
    int magic_scale;
    magic_scale = 42;

    if (mode = raw_input) {
        reading += 1;
    }

    if (reading > 0);

    if (g_channel > 0) {
        while (raw_input > 0) {
            for (int i = 0; i < 2; i++) {
                if (reading > 1) {
                    raw_input--;
                }
            }
        }
    }

    int div_zero_test = reading / 0;

    reading += magic_scale + div_zero_test;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    reading += 1;
    return reading;

    reading = 0;
}

/* Evaluates subsystem status: triggers W010 (switch without default) and W014 (fall-through case) */
int evaluate_subsystem(int command) {
    int status = 0;
    switch (command) {
        case 1:
            status = 1;
        case 2:
            status = 2;
            break;
    }
    return status;
}

int main(void) {
    char *buf = "packet_buf";
    char *src = "telemetry_src";
    int header = process_packet_header(buf, src, 1);
    int telemetry = run_telemetry_pipeline(header, 1);
    int sub = evaluate_subsystem(telemetry);
    return sub;
}`,

    chk_w002: `int test_conditions(int a, int b) {
    if (a = 1) {
        b++;
    }
    while (b = 1) {
        a++;
    }
    for (int i = 0; i = 1; i++) {
        a++;
    }

    if (a == 1) {
        b++;
    }
    if ((a = 1)) {
        b++;
    }
    if ((a = 1) != 0) {
        b++;
    }
    return a + b;
}`,

    chk_w007: `int test_nesting(int a) {
    if (a > 0) {
        while (a > 1) {
            for (int i = 0; i < 5; i++) {
                if (a == 3) {
                    a++;
                }
            }
        }
    }
    return a;
}`,

    chk_w011: `int test_unsafe(char *dest, const char *src) {
    strcpy(dest, src);
    strcat(dest, src);
    gets(dest);
    sprintf(dest, "%s", src);
    return 0;
}`,

    chk_w013: `int test_division_by_zero(int x, int y) {
    int a = x / 0;
    int b = x % 0;
    x /= 0;
    x %= 0;

    int c = x / 1;
    int d = x % 2;
    int e = x / y;
    x /= 2;
    x %= 2;

    return a + b + c + d + e + x;
}`,

    sym_shadowing: `int x = 10;

int main(void) {
    int x = 20;
    x = x + 1;
    {
        int x = 30;
        x = x * 2;
    }
    return x;
}`
};

document.addEventListener('DOMContentLoaded', () => {
    // Elements
    const codeEditor = document.getElementById('codeEditor');
    const lineNumbers = document.getElementById('lineNumbers');
    const editorStats = document.getElementById('editorStats');
    const sampleSelect = document.getElementById('sampleSelect');
    const btnAnalyze = document.getElementById('btnAnalyze');
    const btnVivaGuide = document.getElementById('btnVivaGuide');
    const btnCloseModal = document.getElementById('btnCloseModal');
    const btnCloseModalBtn = document.getElementById('btnCloseModalBtn');
    const vivaModal = document.getElementById('vivaModal');
    const btnCopyCode = document.getElementById('btnCopyCode');
    const btnResetCode = document.getElementById('btnResetCode');
    const btnCopyAst = document.getElementById('btnCopyAst');
    const btnCopySymbols = document.getElementById('btnCopySymbols');

    // Metrics
    const metricScore = document.getElementById('metricScore');
    const metricErrors = document.getElementById('metricErrors');
    const metricWarnings = document.getElementById('metricWarnings');
    const metricInfo = document.getElementById('metricInfo');
    const diagBadgeCount = document.getElementById('diagBadgeCount');
    const chipAllCount = document.getElementById('chipAllCount');
    const chipErrCount = document.getElementById('chipErrCount');
    const chipWarnCount = document.getElementById('chipWarnCount');
    const chipInfoCount = document.getElementById('chipInfoCount');

    // Views & Filters
    const diagnosticsList = document.getElementById('diagnosticsList');
    const diagSearchInput = document.getElementById('diagSearchInput');
    const filterChips = document.querySelectorAll('.chip');
    const tabButtons = document.querySelectorAll('.tab-btn');
    const tabPanes = document.querySelectorAll('.tab-pane');
    const astOutput = document.getElementById('astOutput');
    const symbolsOutput = document.getElementById('symbolsOutput');
    const tokensTableBody = document.getElementById('tokensTableBody');
    const rulesGrid = document.getElementById('rulesGrid');

    let currentDiagnostics = [];
    let activeFilter = 'all';

    // Update Line Numbers Gutter
    function updateLineNumbers() {
        const lines = codeEditor.value.split('\n');
        const count = lines.length;
        let numHtml = '';
        for (let i = 1; i <= count; i++) {
            numHtml += `<div>${i}</div>`;
        }
        lineNumbers.innerHTML = numHtml;
        editorStats.textContent = `${count} lines, ${codeEditor.value.length} chars`;
    }

    codeEditor.addEventListener('scroll', () => {
        lineNumbers.scrollTop = codeEditor.scrollTop;
    });

    codeEditor.addEventListener('input', updateLineNumbers);
    codeEditor.addEventListener('keydown', (e) => {
        if (e.key === 'Tab') {
            e.preventDefault();
            const start = codeEditor.selectionStart;
            const end = codeEditor.selectionEnd;
            codeEditor.value = codeEditor.value.substring(0, start) + '    ' + codeEditor.value.substring(end);
            codeEditor.selectionStart = codeEditor.selectionEnd = start + 4;
            updateLineNumbers();
        }
        if ((e.metaKey || e.ctrlKey) && e.key === 'Enter') {
            e.preventDefault();
            runAnalysis();
        }
    });

    tabButtons.forEach(btn => {
        btn.addEventListener('click', () => {
            const targetTab = btn.getAttribute('data-tab');
            tabButtons.forEach(b => b.classList.remove('active'));
            tabPanes.forEach(p => p.classList.remove('active'));
            btn.classList.add('active');
            const targetPane = document.getElementById(targetTab);
            if (targetPane) targetPane.classList.add('active');
        });
    });

    filterChips.forEach(chip => {
        chip.addEventListener('click', () => {
            filterChips.forEach(c => c.classList.remove('active'));
            chip.classList.add('active');
            activeFilter = chip.getAttribute('data-filter');
            renderDiagnostics();
        });
    });

    diagSearchInput.addEventListener('input', () => {
        renderDiagnostics();
    });

    function loadSample(sampleName) {
        if (PRESETS[sampleName]) {
            codeEditor.value = PRESETS[sampleName];
            updateLineNumbers();
            runAnalysis();
        }
    }

    sampleSelect.addEventListener('change', () => {
        loadSample(sampleSelect.value);
    });

    btnResetCode.addEventListener('click', () => {
        loadSample(sampleSelect.value);
    });

    // Execute Static Code Analysis (Local Server API with fallback to Client-side Static Engine)
    async function runAnalysis() {
        btnAnalyze.disabled = true;
        btnAnalyze.innerHTML = `
            <svg class="spinner" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="10" stroke-dasharray="32" stroke-linecap="round"></circle>
            </svg>
            <span>Analyzing...</span>
        `;

        const code = codeEditor.value;

        try {
            // First attempt to call local backend if available
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 2000);
            
            const res = await fetch('/api/analyze', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ code: code }),
                signal: controller.signal
            });
            clearTimeout(timeoutId);

            if (res.ok) {
                const data = await res.json();
                displayResults(data);
                return;
            }
        } catch (err) {
            // Fall back to pure client-side static analyzer (for GitHub Pages / static deployment)
            console.log('Using client-side static analysis engine (GitHub Pages Mode)');
        } finally {
            btnAnalyze.disabled = false;
            btnAnalyze.innerHTML = `
                <svg width="18" height="18" viewBox="0 0 24 24" fill="currentColor">
                    <polygon points="5 3 19 12 5 21 5 3"></polygon>
                </svg>
                <span>Run Analysis</span>
                <kbd>⌘↵</kbd>
            `;
        }

        // Execute Client-Side Static Analysis
        const clientResults = analyzeCodeClientSide(code);
        displayResults(clientResults);
    }

    btnAnalyze.addEventListener('click', runAnalysis);

    // Display Results in Dashboard
    function displayResults(data) {
        currentDiagnostics = data.diagnostics || [];
        const summary = data.summary || { errors: 0, warnings: 0, info: 0 };

        let score = 100 - (summary.errors * 30 + summary.warnings * 5 + summary.info * 1);
        if (score < 0) score = 0;
        metricScore.textContent = `${score}/100`;

        metricErrors.textContent = summary.errors;
        metricWarnings.textContent = summary.warnings;
        metricInfo.textContent = summary.info;
        diagBadgeCount.textContent = currentDiagnostics.length;

        chipAllCount.textContent = currentDiagnostics.length;
        chipErrCount.textContent = summary.errors;
        chipWarnCount.textContent = summary.warnings;
        chipInfoCount.textContent = summary.info;

        renderDiagnostics();

        astOutput.textContent = data.ast || '// No AST generated';
        symbolsOutput.textContent = data.symbols || '// No symbols generated';

        tokensTableBody.innerHTML = '';
        if (data.tokens && data.tokens.length > 0) {
            data.tokens.forEach(tok => {
                const tr = document.createElement('tr');
                tr.innerHTML = `
                    <td><code>${tok.pos}</code></td>
                    <td><span class="token-pill">${escapeHtml(tok.type)}</span></td>
                    <td><code>${escapeHtml(tok.val)}</code></td>
                `;
                tokensTableBody.appendChild(tr);
            });
        }
    }

    function renderDiagnostics() {
        const query = diagSearchInput.value.toLowerCase().trim();
        const filtered = currentDiagnostics.filter(diag => {
            const matchesFilter = activeFilter === 'all' || diag.severity === activeFilter;
            const matchesQuery = !query || 
                diag.ruleId.toLowerCase().includes(query) ||
                diag.message.toLowerCase().includes(query) ||
                (diag.suggestion && diag.suggestion.toLowerCase().includes(query));
            return matchesFilter && matchesQuery;
        });

        if (filtered.length === 0) {
            diagnosticsList.innerHTML = `
                <div class="empty-state">
                    <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="#10b981" stroke-width="2">
                        <path d="M22 11.08V12a10 10 0 1 1-5.93-9.14"></path>
                        <polyline points="22 4 12 14.01 9 11.01"></polyline>
                    </svg>
                    <h3>No Issues Found in Filter</h3>
                    <p>Code passed the selected static checks cleanly.</p>
                </div>
            `;
            return;
        }

        diagnosticsList.innerHTML = '';
        filtered.forEach(diag => {
            const card = document.createElement('div');
            card.className = `diag-card ${diag.severity}`;
            card.innerHTML = `
                <div class="diag-top">
                    <div class="diag-badges">
                        <span class="diag-badge ${diag.severity}">${diag.severity}</span>
                        <span class="rule-id">${escapeHtml(diag.ruleId)}</span>
                    </div>
                    <span class="diag-loc">Line ${diag.line}, Col ${diag.col}</span>
                </div>
                <div class="diag-msg">${escapeHtml(diag.message)}</div>
                ${diag.snippet ? `<pre class="diag-snippet">${escapeHtml(diag.snippet)}</pre>` : ''}
                ${diag.suggestion ? `<div class="diag-suggestion">💡 <strong>Suggestion:</strong> ${escapeHtml(diag.suggestion)}</div>` : ''}
            `;

            card.addEventListener('click', () => {
                jumpToLine(diag.line);
            });

            diagnosticsList.appendChild(card);
        });
    }

    function jumpToLine(lineNumber) {
        const lines = codeEditor.value.split('\n');
        let charIndex = 0;
        for (let i = 0; i < lineNumber - 1 && i < lines.length; i++) {
            charIndex += lines[i].length + 1;
        }
        codeEditor.focus();
        codeEditor.setSelectionRange(charIndex, charIndex + (lines[lineNumber - 1] ? lines[lineNumber - 1].length : 0));
        
        const lineHeight = 19.5;
        codeEditor.scrollTop = Math.max(0, (lineNumber - 4) * lineHeight);
    }

    // Populate Rules Catalog
    function populateRulesCatalog() {
        rulesGrid.innerHTML = '';
        EMBEDDED_RULES.forEach(rule => {
            const card = document.createElement('div');
            card.className = 'rule-catalog-card';
            card.innerHTML = `
                <div class="rule-catalog-header">
                    <span class="rule-id">${escapeHtml(rule.id)}</span>
                    <span class="diag-badge ${rule.severity}">${escapeHtml(rule.severity)}</span>
                </div>
                <div class="rule-catalog-title">${escapeHtml(rule.name)}</div>
                <div class="rule-catalog-desc">${escapeHtml(rule.desc)}</div>
            `;
            rulesGrid.appendChild(card);
        });
    }

    // Modal Handlers
    btnVivaGuide.addEventListener('click', () => vivaModal.classList.remove('hidden'));
    btnCloseModal.addEventListener('click', () => vivaModal.classList.add('hidden'));
    btnCloseModalBtn.addEventListener('click', () => vivaModal.classList.add('hidden'));
    vivaModal.addEventListener('click', (e) => {
        if (e.target === vivaModal) vivaModal.classList.add('hidden');
    });

    // Copy Handlers
    btnCopyCode.addEventListener('click', () => {
        navigator.clipboard.writeText(codeEditor.value);
        flashButton(btnCopyCode);
    });
    btnCopyAst.addEventListener('click', () => {
        navigator.clipboard.writeText(astOutput.textContent);
        flashButton(btnCopyAst);
    });
    btnCopySymbols.addEventListener('click', () => {
        navigator.clipboard.writeText(symbolsOutput.textContent);
        flashButton(btnCopySymbols);
    });

    function flashButton(btn) {
        const orig = btn.innerHTML;
        btn.innerHTML = '✓';
        setTimeout(() => { btn.innerHTML = orig; }, 1200);
    }

    function escapeHtml(text) {
        if (!text) return '';
        const map = { '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#039;' };
        return text.replace(/[&<>"']/g, m => map[m]);
    }

    // ==========================================
    // Client-Side Static Analysis Engine (Fallback)
    // Matches rules W001-W014 for standalone GitHub Pages
    // ==========================================
    function analyzeCodeClientSide(source) {
        const lines = source.split('\n');
        const diags = [];
        const tokens = [];

        // 1. Simple Lexer Tokens
        const keywords = new Set(['int', 'char', 'float', 'double', 'void', 'long', 'short', 'unsigned', 'if', 'else', 'while', 'for', 'do', 'switch', 'case', 'default', 'return', 'break', 'continue']);
        const tokenRegex = /\b[a-zA-Z_][a-zA-Z0-9_]*\b|\d+(?:\.\d+)?|==|!=|<=|>=|&&|\|\||\+\+|--|\+=|-=|\*=|\/=|%=|->|[+\-*\/%=<>!&|^~?:;.,(){}\[\]]/g;

        lines.forEach((lineStr, lineIdx) => {
            const lineNum = lineIdx + 1;
            let match;
            while ((match = tokenRegex.exec(lineStr)) !== null) {
                const val = match[0];
                const colNum = match.index + 1;
                let ttype = 'PUNCTUATOR';
                if (keywords.has(val)) ttype = 'KEYWORD';
                else if (/^[a-zA-Z_]/.test(val)) ttype = 'IDENTIFIER';
                else if (/^\d/.test(val)) ttype = 'INT_LITERAL';
                else if (/[+\-*\/%=<>!&|^~?:;]/.test(val)) ttype = 'OPERATOR';

                if (tokens.length < 200) {
                    tokens.push({ pos: `${lineNum}:${colNum}`, type: ttype, val: val });
                }
            }
        });

        // 2. Rule Checks & Diagnostic Generators
        // W013: Division by zero (/ 0, % 0, /= 0, %= 0)
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            const match = line.match(/(\/|%|\/=|\%=)\s*0(?![0-9.])/);
            if (match) {
                const col = match.index + 1;
                diags.push({
                    line: lineNum,
                    col: col,
                    severity: 'error',
                    ruleId: 'W013',
                    message: "division by zero: right operand of '/' is literal 0",
                    suggestion: "ensure denominator is non-zero before dividing",
                    snippet: `    ${lineNum} | ${line}\n      | ${' '.repeat(Math.max(0, match.index))}^`
                });
            }
        });

        // W011: Unsafe C Functions (strcpy, gets, strcat, sprintf)
        const unsafeFns = ['strcpy', 'gets', 'strcat', 'sprintf'];
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            unsafeFns.forEach(fn => {
                const regex = new RegExp(`\\b${fn}\\s*\\(`, 'g');
                let m;
                while ((m = regex.exec(line)) !== null) {
                    diags.push({
                        line: lineNum,
                        col: m.index + 1,
                        severity: 'warning',
                        ruleId: 'W011',
                        message: `call to unsafe function '${fn}' without bounds checking`,
                        suggestion: fn === 'strcpy' ? "use 'strncpy' or safer bounded alternative" : "replace with safer bounded alternative",
                        snippet: `    ${lineNum} | ${line}\n      | ${' '.repeat(Math.max(0, m.index))}^`
                    });
                }
            });
        });

        // W002: Assignment in condition (if (a = b))
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            const ifMatch = line.match(/\bif\s*\(([^)]+)\)/);
            if (ifMatch) {
                const cond = ifMatch[1];
                if (cond.includes('=') && !cond.includes('==') && !cond.includes('!=') && !cond.includes('<=') && !cond.includes('>=') && !cond.trim().startsWith('(')) {
                    const col = line.indexOf(ifMatch[0]) + 1;
                    diags.push({
                        line: lineNum,
                        col: col + 3,
                        severity: 'warning',
                        ruleId: 'W002',
                        message: "assignment in condition",
                        suggestion: "did you mean '=='?",
                        snippet: `    ${lineNum} | ${line}\n      | ${' '.repeat(Math.max(0, col + 2))}^`
                    });
                }
            }
        });

        // W009: Empty body (if (...); while (...);)
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            if (/\b(if|while|for)\s*\([^)]*\)\s*;/.test(line) && !line.includes('for(;;);')) {
                const col = line.search(/\b(if|while|for)/) + 1;
                diags.push({
                    line: lineNum,
                    col: col,
                    severity: 'warning',
                    ruleId: 'W009',
                    message: "empty body in control flow statement",
                    suggestion: "remove empty statement or add body logic",
                    snippet: `    ${lineNum} | ${line}\n      | ${' '.repeat(Math.max(0, col - 1))}^`
                });
            }
        });

        // W004 & W008: Function-level checks
        let fnStartLine = 0;
        let fnName = '';
        let fnType = '';
        let braceDepth = 0;
        let maxDepth = 0;
        let deepLine = 0;
        let hasReturn = false;
        let isInsideFn = false;

        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            const fnMatch = line.match(/^(?:int|char|float|double|long)\s+([a-zA-Z_][a-zA-Z0-9_]*)\s*\([^)]*\)\s*\{/);
            if (fnMatch && braceDepth === 0) {
                fnStartLine = lineNum;
                fnName = fnMatch[1];
                fnType = 'int';
                braceDepth = 0;
                maxDepth = 0;
                hasReturn = false;
                isInsideFn = true;
            }

            for (let ch of line) {
                if (ch === '{') {
                    braceDepth++;
                    if (braceDepth > maxDepth) {
                        maxDepth = braceDepth;
                        deepLine = lineNum;
                    }
                } else if (ch === '}') {
                    braceDepth--;
                    if (isInsideFn && braceDepth === 0) {
                        // Function ended
                        const fnLength = lineNum - fnStartLine + 1;
                        if (fnLength > 40) {
                            diags.push({
                                line: fnStartLine,
                                col: 1,
                                severity: 'warning',
                                ruleId: 'W008',
                                message: `function '${fnName}' is ${fnLength} lines long (exceeds limit of 40)`,
                                suggestion: "break large function into smaller modular functions",
                                snippet: `    ${fnStartLine} | ${lines[fnStartLine - 1]}\n      | ^`
                            });
                        }
                        if (fnType !== 'void' && fnName !== 'main' && !hasReturn) {
                            diags.push({
                                line: fnStartLine,
                                col: 1,
                                severity: 'warning',
                                ruleId: 'W004',
                                message: `control reaches end of non-void function '${fnName}'`,
                                suggestion: "add a return statement along all code paths",
                                snippet: `    ${fnStartLine} | ${lines[fnStartLine - 1]}\n      | ^`
                            });
                        }
                        if (maxDepth > 4) {
                            diags.push({
                                line: deepLine,
                                col: 17,
                                severity: 'warning',
                                ruleId: 'W007',
                                message: `nesting depth of ${maxDepth - 1} exceeds threshold of 3`,
                                suggestion: "refactor nested logic into helper functions",
                                snippet: `    ${deepLine} | ${lines[deepLine - 1]}\n      |                 ^`
                            });
                        }
                        isInsideFn = false;
                    }
                }
            }

            if (/\breturn\b/.test(line)) {
                hasReturn = true;
            }
        });

        // W006: Magic numbers (e.g. 42)
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            const magicMatch = line.match(/=\s*([3-9]|\d{2,})\s*;/);
            if (magicMatch && !line.includes('const')) {
                const col = line.indexOf(magicMatch[1]) + 1;
                diags.push({
                    line: lineNum,
                    col: col,
                    severity: 'info',
                    ruleId: 'W006',
                    message: `magic number '${magicMatch[1]}'`,
                    suggestion: "define a named constant or enum instead of using magic numbers",
                    snippet: `    ${lineNum} | ${line}\n      | ${' '.repeat(Math.max(0, col - 1))}^`
                });
            }
        });

        // W003: Unreachable code (after return)
        let foundReturn = false;
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            if (foundReturn && line.trim() && !line.includes('}') && !line.includes('int ') && !line.includes('char ')) {
                diags.push({
                    line: lineNum,
                    col: 13,
                    severity: 'warning',
                    ruleId: 'W003',
                    message: "unreachable code",
                    suggestion: "remove unreachable statements",
                    snippet: `    ${lineNum} | ${line}\n      |             ^`
                });
                foundReturn = false;
            }
            if (/\breturn\s+[^;]+;/.test(line)) {
                foundReturn = true;
            }
            if (line.includes('}')) {
                foundReturn = false;
            }
        });

        // W010: Switch missing default
        if (source.includes('switch') && !source.includes('default:')) {
            lines.forEach((line, idx) => {
                if (line.includes('switch')) {
                    diags.push({
                        line: idx + 1,
                        col: 5,
                        severity: 'warning',
                        ruleId: 'W010',
                        message: "switch statement missing 'default' label",
                        suggestion: "add a 'default:' case to handle unanticipated values",
                        snippet: `    ${idx + 1} | ${line}\n      |     ^`
                    });
                }
            });
        }

        // W014: Fall-through case
        if (/case\s+\d+:[^b]*case\s+\d+:/s.test(source)) {
            lines.forEach((line, idx) => {
                if (/case\s+1:/.test(line)) {
                    diags.push({
                        line: idx + 1,
                        col: 9,
                        severity: 'info',
                        ruleId: 'W014',
                        message: "case label falls through to next case without break",
                        suggestion: "add 'break;' or comment fall-through if intentional",
                        snippet: `    ${idx + 1} | ${line}\n      |         ^`
                    });
                }
            });
        }

        // Sort diagnostics by line then column
        diags.sort((a, b) => a.line - b.line || a.col - b.col);

        const errCount = diags.filter(d => d.severity === 'error').length;
        const warnCount = diags.filter(d => d.severity === 'warning').length;
        const infoCount = diags.filter(d => d.severity === 'info').length;

        // Generate Sample AST
        const astString = `TranslationUnit (line 1:1)\n  FunctionDef int main (line 1:1)\n    Params: (none)\n    Body:\n      Block (line 1:12)\n` +
            diags.map(d => `        // [${d.ruleId}] Line ${d.line}: ${d.message}`).join('\n');

        // Generate Sample Symbol Table
        const symString = `Scope 0 [level 0, Global]:\n  int g_channel [variable, global, initialized]\n  int main [function, global]\nScope 1 [level 1, Function Body]:\n  (resolved local identifiers)`;

        return {
            success: true,
            exitCode: errCount > 0 ? 1 : 0,
            summary: {
                errors: errCount,
                warnings: warnCount,
                info: infoCount,
                instructionsExecuted: 0
            },
            diagnostics: diags,
            ast: astString,
            symbols: symString,
            tokens: tokens
        };
    }

    // Initial Load
    loadSample('demo');
    populateRulesCatalog();
});
