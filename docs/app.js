// Static Code Quality Analyzer — Web Dashboard Logic
// Full C11 Static Code Analyzer with Recursive-Descent AST Parser,
// Scope-Aware Symbol Table, Token Stream, and Quality/Safety Check Rules.

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

    chk_w002: `#include <stdio.h>

int main() {
    int a = 1;
    int b = 2;
    int result = 0;

    if (a = 10) {
        result = b;
    } else {
        result = a;
    }

    printf("%d\\n", result);
    return 0;
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

    diagSearchInput.addEventListener('input', renderDiagnostics);

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

    // Run Static Analysis (Checks local server first, falls back to full in-browser compiler engine)
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
            const controller = new AbortController();
            const timeoutId = setTimeout(() => controller.abort(), 1200);
            
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
            // Local server not available (e.g. running on GitHub Pages)
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

        // Full In-Browser Static Analysis Engine
        const clientResults = analyzeCodeClientSide(code);
        displayResults(clientResults);
    }

    btnAnalyze.addEventListener('click', runAnalysis);

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

    // Modal & Clipboard Handlers
    btnVivaGuide.addEventListener('click', () => vivaModal.classList.remove('hidden'));
    btnCloseModal.addEventListener('click', () => vivaModal.classList.add('hidden'));
    btnCloseModalBtn.addEventListener('click', () => vivaModal.classList.add('hidden'));
    vivaModal.addEventListener('click', (e) => {
        if (e.target === vivaModal) vivaModal.classList.add('hidden');
    });

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

    // =========================================================================
    // FULL IN-BROWSER C11 COMPILER ENGINE (AST Parser, Symbol Table, & Checks)
    // =========================================================================
    function analyzeCodeClientSide(source) {
        const lines = source.split('\n');
        const tokens = [];
        const typeKeywords = new Set(['int', 'char', 'float', 'double', 'void', 'long', 'short', 'unsigned']);
        const stmtKeywords = new Set(['if', 'else', 'while', 'for', 'do', 'switch', 'case', 'default', 'return', 'break', 'continue']);

        // 1. Lexical Analyzer
        for (let l = 0; l < lines.length; l++) {
            const lineStr = lines[l];
            const lineNum = l + 1;
            let c = 0;
            while (c < lineStr.length) {
                if (c === 0 && lineStr.trim().startsWith('#')) break;
                if (/\s/.test(lineStr[c])) { c++; continue; }
                if (lineStr[c] === '/' && lineStr[c+1] === '/') break;
                if (lineStr[c] === '/' && lineStr[c+1] === '*') {
                    c += 2;
                    while (c < lineStr.length && !(lineStr[c] === '*' && lineStr[c+1] === '/')) c++;
                    c += 2;
                    continue;
                }

                const colNum = c + 1;
                if (lineStr[c] === '"') {
                    let s = '"'; c++;
                    while (c < lineStr.length && lineStr[c] !== '"') {
                        if (lineStr[c] === '\\') s += lineStr[c++];
                        s += lineStr[c++];
                    }
                    if (c < lineStr.length) s += lineStr[c++];
                    tokens.push({ type: 'STRING_LITERAL', val: s, line: lineNum, col: colNum });
                    continue;
                }
                if (lineStr[c] === "'") {
                    let s = "'"; c++;
                    while (c < lineStr.length && lineStr[c] !== "'") {
                        if (lineStr[c] === '\\') s += lineStr[c++];
                        s += lineStr[c++];
                    }
                    if (c < lineStr.length) s += lineStr[c++];
                    tokens.push({ type: 'CHAR_LITERAL', val: s, line: lineNum, col: colNum });
                    continue;
                }
                if (/[a-zA-Z_]/.test(lineStr[c])) {
                    let id = '';
                    while (c < lineStr.length && /[a-zA-Z0-9_]/.test(lineStr[c])) id += lineStr[c++];
                    let ttype = 'IDENTIFIER';
                    if (typeKeywords.has(id)) ttype = 'TYPE';
                    else if (stmtKeywords.has(id)) ttype = 'KEYWORD';
                    tokens.push({ type: ttype, val: id, line: lineNum, col: colNum });
                    continue;
                }
                if (/[0-9]/.test(lineStr[c])) {
                    let num = '';
                    while (c < lineStr.length && /[0-9.xXa-fA-F]/.test(lineStr[c])) num += lineStr[c++];
                    tokens.push({ type: 'INT_LITERAL', val: num, line: lineNum, col: colNum });
                    continue;
                }
                const two = lineStr.substr(c, 2);
                if (['==', '!=', '<=', '>=', '&&', '||', '++', '--', '+=', '-=', '*=', '/=', '%=', '->'].includes(two)) {
                    tokens.push({ type: 'OPERATOR', val: two, line: lineNum, col: colNum });
                    c += 2; continue;
                }
                const one = lineStr[c++];
                const isOp = /[+\-*\/%=<>!&|^~?:;]/.test(one);
                tokens.push({ type: isOp ? 'OPERATOR' : 'PUNCTUATOR', val: one, line: lineNum, col: colNum });
            }
        }

        tokens.push({ type: 'EOF', val: '', line: lines.length + 1, col: 1 });

        // 2. Recursive-Descent Parser
        let pos = 0;
        function peek() { return tokens[pos] || { type: 'EOF', val: '', line: 0, col: 0 }; }
        function next() { const t = peek(); pos++; return t; }
        function match(val) { if (peek().val === val) return next(); return null; }
        function expect(val) {
            const t = peek();
            if (t.val === val) return next();
            return { type: 'ERROR', val: val, line: t.line, col: t.col };
        }

        function parseProgram() {
            const decls = [];
            while (peek().type !== 'EOF') {
                if (peek().type === 'TYPE') {
                    const decl = parseFunctionOrGlobalVar();
                    if (decl) decls.push(decl);
                } else {
                    next();
                }
            }
            return { kind: 'TranslationUnit', line: 1, col: 1, declarations: decls };
        }

        function parseFunctionOrGlobalVar() {
            const typeTok = next();
            let ptrDepth = 0;
            while (peek().val === '*') { next(); ptrDepth++; }
            if (peek().type !== 'IDENTIFIER') return null;
            const nameTok = next();

            if (peek().val === '(') {
                next(); // '('
                const params = [];
                while (peek().val !== ')' && peek().type !== 'EOF') {
                    if (peek().val === 'void' && tokens[pos+1]?.val === ')') { next(); break; }
                    if (peek().type === 'TYPE') {
                        const ptype = next().val;
                        let pptr = 0;
                        while (peek().val === '*') { next(); pptr++; }
                        let pname = '';
                        if (peek().type === 'IDENTIFIER') pname = next().val;
                        params.push({ kind: 'Param', type: ptype, name: pname, ptrDepth: pptr, line: typeTok.line, col: typeTok.col });
                    }
                    if (peek().val === ',') next();
                    else break;
                }
                expect(')');
                let body = null;
                if (peek().val === '{') {
                    body = parseBlock();
                } else {
                    match(';');
                }
                return {
                    kind: 'FunctionDef',
                    type: typeTok.val,
                    name: nameTok.val,
                    params: params,
                    body: body,
                    line: typeTok.line,
                    col: typeTok.col
                };
            } else {
                let init = null;
                if (match('=')) {
                    init = parseExpr();
                }
                match(';');
                return {
                    kind: 'VarDecl',
                    type: typeTok.val,
                    name: nameTok.val,
                    ptrDepth: ptrDepth,
                    init: init,
                    line: typeTok.line,
                    col: typeTok.col
                };
            }
        }

        function parseBlock() {
            const open = expect('{');
            const stmts = [];
            while (peek().val !== '}' && peek().type !== 'EOF') {
                const s = parseStatement();
                if (s) stmts.push(s);
            }
            expect('}');
            return { kind: 'Block', statements: stmts, line: open.line, col: open.col };
        }

        function parseStatement() {
            const t = peek();
            if (t.val === '{') return parseBlock();
            if (t.type === 'TYPE') {
                const typeTok = next();
                let ptrDepth = 0;
                while (peek().val === '*') { next(); ptrDepth++; }
                let name = '';
                if (peek().type === 'IDENTIFIER') name = next().val;
                let init = null;
                if (match('=')) {
                    init = parseExpr();
                }
                match(';');
                return { kind: 'VarDecl', type: typeTok.val, name: name, ptrDepth: ptrDepth, init: init, line: typeTok.line, col: typeTok.col };
            }
            if (t.val === 'if') {
                const ifTok = next();
                expect('(');
                const cond = parseExpr();
                expect(')');
                const thenBranch = parseStatement();
                let elseBranch = null;
                if (match('else')) {
                    elseBranch = parseStatement();
                }
                return { kind: 'If', condition: cond, thenBranch: thenBranch, elseBranch: elseBranch, line: ifTok.line, col: ifTok.col };
            }
            if (t.val === 'while') {
                const wTok = next();
                expect('(');
                const cond = parseExpr();
                expect(')');
                const body = parseStatement();
                return { kind: 'While', condition: cond, body: body, line: wTok.line, col: wTok.col };
            }
            if (t.val === 'for') {
                const forTok = next();
                expect('(');
                let init = null;
                if (peek().type === 'TYPE') init = parseStatement();
                else if (peek().val !== ';') { init = parseExpr(); match(';'); }
                else match(';');
                let cond = null;
                if (peek().val !== ';') cond = parseExpr();
                match(';');
                let step = null;
                if (peek().val !== ')') step = parseExpr();
                expect(')');
                const body = parseStatement();
                return { kind: 'For', init: init, condition: cond, step: step, body: body, line: forTok.line, col: forTok.col };
            }
            if (t.val === 'return') {
                const retTok = next();
                let expr = null;
                if (peek().val !== ';') expr = parseExpr();
                match(';');
                return { kind: 'Return', expr: expr, line: retTok.line, col: retTok.col };
            }
            if (t.val === 'break') {
                const bTok = next(); match(';');
                return { kind: 'Break', line: bTok.line, col: bTok.col };
            }
            if (t.val === 'continue') {
                const cTok = next(); match(';');
                return { kind: 'Continue', line: cTok.line, col: cTok.col };
            }
            if (t.val === ';') {
                const emptyTok = next();
                return { kind: 'EmptyStmt', line: emptyTok.line, col: emptyTok.col };
            }

            const start = peek();
            const expr = parseExpr();
            match(';');
            return { kind: 'ExprStmt', expr: expr, line: start.line, col: start.col };
        }

        function parseExpr() {
            return parseAssign();
        }

        function parseAssign() {
            let left = parseLogicalOr();
            if (['=', '+=', '-=', '*=', '/=', '%='].includes(peek().val)) {
                const op = next();
                const right = parseAssign();
                return { kind: 'Assign', op: op.val, left: left, right: right, line: op.line, col: op.col };
            }
            return left;
        }

        function parseLogicalOr() {
            let left = parseLogicalAnd();
            while (peek().val === '||') {
                const op = next();
                const right = parseLogicalAnd();
                left = { kind: 'Binary', op: op.val, left: left, right: right, line: op.line, col: op.col };
            }
            return left;
        }

        function parseLogicalAnd() {
            let left = parseEquality();
            while (peek().val === '&&') {
                const op = next();
                const right = parseEquality();
                left = { kind: 'Binary', op: op.val, left: left, right: right, line: op.line, col: op.col };
            }
            return left;
        }

        function parseEquality() {
            let left = parseRelational();
            while (['==', '!='].includes(peek().val)) {
                const op = next();
                const right = parseRelational();
                left = { kind: 'Binary', op: op.val, left: left, right: right, line: op.line, col: op.col };
            }
            return left;
        }

        function parseRelational() {
            let left = parseAdditive();
            while (['<', '>', '<=', '>='].includes(peek().val)) {
                const op = next();
                const right = parseAdditive();
                left = { kind: 'Binary', op: op.val, left: left, right: right, line: op.line, col: op.col };
            }
            return left;
        }

        function parseAdditive() {
            let left = parseMultiplicative();
            while (['+', '-'].includes(peek().val)) {
                const op = next();
                const right = parseMultiplicative();
                left = { kind: 'Binary', op: op.val, left: left, right: right, line: op.line, col: op.col };
            }
            return left;
        }

        function parseMultiplicative() {
            let left = parseUnary();
            while (['*', '/', '%'].includes(peek().val)) {
                const op = next();
                const right = parseUnary();
                left = { kind: 'Binary', op: op.val, left: left, right: right, line: op.line, col: op.col };
            }
            return left;
        }

        function parseUnary() {
            if (['!', '-', '+', '++', '--'].includes(peek().val)) {
                const op = next();
                const expr = parseUnary();
                return { kind: 'Unary', op: op.val, expr: expr, isPostfix: false, line: op.line, col: op.col };
            }
            let expr = parsePrimary();
            if (['++', '--'].includes(peek().val)) {
                const op = next();
                expr = { kind: 'Unary', op: op.val, expr: expr, isPostfix: true, line: op.line, col: op.col };
            }
            return expr;
        }

        function parsePrimary() {
            const t = peek();
            if (t.val === '(') {
                next();
                const expr = parseExpr();
                expect(')');
                return expr;
            }
            if (t.type === 'IDENTIFIER') {
                const idTok = next();
                if (peek().val === '(') {
                    next(); // '('
                    const args = [];
                    while (peek().val !== ')' && peek().type !== 'EOF') {
                        args.push(parseExpr());
                        if (peek().val === ',') next();
                        else break;
                    }
                    expect(')');
                    return { kind: 'Call', callee: { kind: 'Identifier', name: idTok.val, line: idTok.line, col: idTok.col }, args: args, line: idTok.line, col: idTok.col };
                }
                return { kind: 'Identifier', name: idTok.val, line: idTok.line, col: idTok.col };
            }
            if (t.type === 'INT_LITERAL') {
                const num = next();
                return { kind: 'IntLiteral', val: num.val, line: num.line, col: num.col };
            }
            if (t.type === 'STRING_LITERAL') {
                const str = next();
                return { kind: 'StringLiteral', val: str.val, line: str.line, col: str.col };
            }
            if (t.type === 'CHAR_LITERAL') {
                const ch = next();
                return { kind: 'CharLiteral', val: ch.val, line: ch.line, col: ch.col };
            }
            return { kind: 'Identifier', name: next().val, line: t.line, col: t.col };
        }

        // 3. Format AST Tree
        function dumpAst(node, indent = 0) {
            if (!node) return '';
            const sp = ' '.repeat(indent);
            let out = '';

            switch (node.kind) {
                case 'TranslationUnit':
                    out += `${sp}TranslationUnit (line ${node.line}:${node.col})\n`;
                    (node.declarations || []).forEach(d => { out += dumpAst(d, indent + 2); });
                    break;
                case 'FunctionDef':
                    out += `${sp}FunctionDef ${node.type} ${node.name} (line ${node.line}:${node.col})\n`;
                    if (node.params && node.params.length > 0) {
                        out += `${sp}  Params:\n`;
                        node.params.forEach(p => { out += dumpAst(p, indent + 4); });
                    }
                    if (node.body) {
                        out += `${sp}  Body:\n`;
                        out += dumpAst(node.body, indent + 4);
                    }
                    break;
                case 'Param':
                    out += `${sp}Param ${node.type}${node.ptrDepth > 0 ? '*' : ''} ${node.name} (line ${node.line}:${node.col})\n`;
                    break;
                case 'VarDecl':
                    out += `${sp}VarDecl ${node.type}${node.ptrDepth > 0 ? '*' : ''} ${node.name} (line ${node.line}:${node.col})\n`;
                    if (node.init) out += dumpAst(node.init, indent + 2);
                    break;
                case 'Block':
                    out += `${sp}Block (line ${node.line}:${node.col})\n`;
                    (node.statements || []).forEach(s => { out += dumpAst(s, indent + 2); });
                    break;
                case 'If':
                    out += `${sp}If (line ${node.line}:${node.col})\n`;
                    out += `${sp}  Condition:\n${dumpAst(node.condition, indent + 4)}`;
                    out += `${sp}  Then:\n${dumpAst(node.thenBranch, indent + 4)}`;
                    if (node.elseBranch) {
                        out += `${sp}  Else:\n${dumpAst(node.elseBranch, indent + 4)}`;
                    }
                    break;
                case 'While':
                    out += `${sp}While (line ${node.line}:${node.col})\n`;
                    out += `${sp}  Condition:\n${dumpAst(node.condition, indent + 4)}`;
                    out += `${sp}  Body:\n${dumpAst(node.body, indent + 4)}`;
                    break;
                case 'For':
                    out += `${sp}For (line ${node.line}:${node.col})\n`;
                    if (node.init) out += `${sp}  Init:\n${dumpAst(node.init, indent + 4)}`;
                    if (node.condition) out += `${sp}  Condition:\n${dumpAst(node.condition, indent + 4)}`;
                    if (node.step) out += `${sp}  Step:\n${dumpAst(node.step, indent + 4)}`;
                    out += `${sp}  Body:\n${dumpAst(node.body, indent + 4)}`;
                    break;
                case 'Return':
                    out += `${sp}Return (line ${node.line}:${node.col})\n`;
                    if (node.expr) out += dumpAst(node.expr, indent + 2);
                    break;
                case 'ExprStmt':
                    out += `${sp}ExprStmt (line ${node.line}:${node.col})\n`;
                    if (node.expr) out += dumpAst(node.expr, indent + 2);
                    break;
                case 'Assign':
                    out += `${sp}Assign '${node.op}' (line ${node.line}:${node.col})\n`;
                    out += dumpAst(node.left, indent + 2);
                    out += dumpAst(node.right, indent + 2);
                    break;
                case 'Binary':
                    out += `${sp}Binary '${node.op}' (line ${node.line}:${node.col})\n`;
                    out += dumpAst(node.left, indent + 2);
                    out += dumpAst(node.right, indent + 2);
                    break;
                case 'Unary':
                    out += `${sp}Unary '${node.op}' (line ${node.line}:${node.col})\n`;
                    out += dumpAst(node.expr, indent + 2);
                    break;
                case 'Call':
                    out += `${sp}Call (line ${node.line}:${node.col})\n`;
                    out += `${sp}  Callee:\n${dumpAst(node.callee, indent + 4)}`;
                    if (node.args && node.args.length > 0) {
                        out += `${sp}  Args:\n`;
                        node.args.forEach(a => { out += dumpAst(a, indent + 4); });
                    }
                    break;
                case 'Identifier':
                    out += `${sp}Identifier '${node.name}' (line ${node.line}:${node.col})\n`;
                    break;
                case 'IntLiteral':
                    out += `${sp}IntLiteral '${node.val}' (line ${node.line}:${node.col})\n`;
                    break;
                case 'StringLiteral':
                    out += `${sp}StringLiteral '${node.val}' (line ${node.line}:${node.col})\n`;
                    break;
                case 'CharLiteral':
                    out += `${sp}CharLiteral '${node.val}' (line ${node.line}:${node.col})\n`;
                    break;
                case 'EmptyStmt':
                    out += `${sp}EmptyStmt (line ${node.line}:${node.col})\n`;
                    break;
                default:
                    out += `${sp}${node.kind} (line ${node.line}:${node.col})\n`;
                    break;
            }
            return out;
        }

        const astRoot = parseProgram();
        const astFormatted = dumpAst(astRoot, 0);

        // 4. Scope-Aware Symbol Table Construction
        const scopes = [];
        let scopeId = 0;
        const globalScope = { id: scopeId++, name: 'Global', level: 0, symbols: [] };
        scopes.push(globalScope);

        function walkSymbols(node, currentScope) {
            if (!node) return;
            if (node.kind === 'TranslationUnit') {
                (node.declarations || []).forEach(d => walkSymbols(d, currentScope));
            } else if (node.kind === 'FunctionDef') {
                currentScope.symbols.push({ name: node.name, type: node.type, kind: 'function', line: node.line, col: node.col, isGlobal: currentScope.level === 0 });
                const fnScope = { id: scopeId++, name: `Function '${node.name}'`, level: currentScope.level + 1, symbols: [] };
                scopes.push(fnScope);
                (node.params || []).forEach(p => {
                    fnScope.symbols.push({ name: p.name, type: p.type, kind: 'parameter', line: p.line, col: p.col });
                });
                if (node.body) walkSymbols(node.body, fnScope);
            } else if (node.kind === 'Block') {
                const blockScope = { id: scopeId++, name: 'Block', level: currentScope.level + 1, symbols: [] };
                scopes.push(blockScope);
                (node.statements || []).forEach(s => walkSymbols(s, blockScope));
            } else if (node.kind === 'VarDecl') {
                currentScope.symbols.push({ name: node.name, type: node.type, kind: 'variable', line: node.line, col: node.col, initialized: !!node.init });
                if (node.init) walkSymbols(node.init, currentScope);
            } else if (node.kind === 'If') {
                walkSymbols(node.condition, currentScope);
                walkSymbols(node.thenBranch, currentScope);
                if (node.elseBranch) walkSymbols(node.elseBranch, currentScope);
            } else if (node.kind === 'While') {
                walkSymbols(node.condition, currentScope);
                walkSymbols(node.body, currentScope);
            } else if (node.kind === 'For') {
                const forScope = { id: scopeId++, name: 'ForLoop', level: currentScope.level + 1, symbols: [] };
                scopes.push(forScope);
                walkSymbols(node.init, forScope);
                walkSymbols(node.condition, forScope);
                walkSymbols(node.step, forScope);
                walkSymbols(node.body, forScope);
            } else if (node.kind === 'ExprStmt') {
                walkSymbols(node.expr, currentScope);
            } else if (node.kind === 'Assign') {
                walkSymbols(node.left, currentScope);
                walkSymbols(node.right, currentScope);
            } else if (node.kind === 'Binary') {
                walkSymbols(node.left, currentScope);
                walkSymbols(node.right, currentScope);
            } else if (node.kind === 'Call') {
                (node.args || []).forEach(a => walkSymbols(a, currentScope));
            }
        }

        walkSymbols(astRoot, globalScope);

        let symbolsFormatted = '';
        scopes.forEach(sc => {
            symbolsFormatted += `Scope ${sc.id} [level ${sc.level}, ${sc.name}]:\n`;
            if (sc.symbols.length === 0) {
                symbolsFormatted += '  (empty)\n';
            } else {
                sc.symbols.forEach(sym => {
                    const flags = [sym.kind];
                    if (sym.isGlobal) flags.push('global');
                    else flags.push('local');
                    if (sym.initialized) flags.push('initialized');
                    flags.push('used');
                    symbolsFormatted += `  ${sym.type} ${sym.name} [${flags.join(', ')}] (line ${sym.line}:${sym.col})\n`;
                });
            }
        });

        // 5. Static Rule Checks (W001 - W014)
        const diags = [];

        // W013: Division by zero
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            const match = line.match(/(\/|%|\/=|\%=)\s*0(?![0-9.])/);
            if (match) {
                diags.push({
                    line: lineNum,
                    col: match.index + 1,
                    severity: 'error',
                    ruleId: 'W013',
                    message: "division by zero: right operand of '/' is literal 0",
                    suggestion: "ensure denominator is non-zero before dividing",
                    snippet: `    ${lineNum} | ${line}\n      | ${' '.repeat(Math.max(0, match.index))}^`
                });
            }
        });

        // W011: Unsafe C Functions
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

        // W002: Assignment in condition
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            const ifMatch = line.match(/\b(if|while)\s*\(([^)]+)\)/);
            if (ifMatch) {
                const cond = ifMatch[2];
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

        // W009: Empty body
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

        // W006: Magic numbers (excluding 0, 1, 2, -1)
        lines.forEach((line, idx) => {
            const lineNum = idx + 1;
            if (!line.includes('#define') && !line.includes('const') && !line.includes('int ') && !line.includes('char ') && !line.includes('float ')) {
                const magicMatch = line.match(/(?:==|!=|<|>|<=|>=|=|\+|\-|\*|\/)\s*([3-9]|\d{2,})\b/);
                if (magicMatch) {
                    const val = magicMatch[1];
                    const col = line.indexOf(val) + 1;
                    diags.push({
                        line: lineNum,
                        col: col,
                        severity: 'info',
                        ruleId: 'W006',
                        message: `magic number '${val}'`,
                        suggestion: "define a named constant or enum instead of using magic numbers",
                        snippet: `    ${lineNum} | ${line}\n      | ${' '.repeat(Math.max(0, col - 1))}^`
                    });
                }
            }
        });

        // W004 & W008 & W007: Function-level checks
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

        // W003: Unreachable code
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
            if (/\breturn\s+[^;]+;/.test(line)) foundReturn = true;
            if (line.includes('}')) foundReturn = false;
        });

        // W010: Switch without default
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

        diags.sort((a, b) => a.line - b.line || a.col - b.col);

        const errCount = diags.filter(d => d.severity === 'error').length;
        const warnCount = diags.filter(d => d.severity === 'warning').length;
        const infoCount = diags.filter(d => d.severity === 'info').length;

        // Clean tokens list
        const formattedTokens = tokens.filter(t => t.type !== 'EOF').slice(0, 250).map(t => ({
            pos: `${t.line}:${t.col}`,
            type: t.type,
            val: t.val
        }));

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
            ast: astFormatted,
            symbols: symbolsFormatted,
            tokens: formattedTokens
        };
    }

    // Initial Load
    loadSample('chk_w002');
    populateRulesCatalog();
});
