// Static Code Quality Analyzer — Web Dashboard Logic

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
    let currentSample = 'demo';

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

    // Synchronize scrolling between editor and line numbers gutter
    codeEditor.addEventListener('scroll', () => {
        lineNumbers.scrollTop = codeEditor.scrollTop;
    });

    // Handle Editor Input & Tab Indentation
    codeEditor.addEventListener('input', updateLineNumbers);
    codeEditor.addEventListener('keydown', (e) => {
        // Tab key inserts 4 spaces
        if (e.key === 'Tab') {
            e.preventDefault();
            const start = codeEditor.selectionStart;
            const end = codeEditor.selectionEnd;
            codeEditor.value = codeEditor.value.substring(0, start) + '    ' + codeEditor.value.substring(end);
            codeEditor.selectionStart = codeEditor.selectionEnd = start + 4;
            updateLineNumbers();
        }
        // Cmd/Ctrl + Enter runs analysis
        if ((e.metaKey || e.ctrlKey) && e.key === 'Enter') {
            e.preventDefault();
            runAnalysis();
        }
    });

    // Tab Switching
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

    // Filter Chips for Diagnostics
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

    // Fetch and Load Sample Preset
    async function loadSample(sampleName) {
        try {
            const res = await fetch(`/api/sample?name=${sampleName}`);
            if (res.ok) {
                const data = await res.json();
                codeEditor.value = data.code;
                currentSample = sampleName;
                updateLineNumbers();
                runAnalysis();
            }
        } catch (err) {
            console.error('Failed to load sample:', err);
        }
    }

    sampleSelect.addEventListener('change', () => {
        loadSample(sampleSelect.value);
    });

    btnResetCode.addEventListener('click', () => {
        loadSample(sampleSelect.value);
    });

    // Execute Static Code Analysis
    async function runAnalysis() {
        btnAnalyze.disabled = true;
        btnAnalyze.innerHTML = `
            <svg class="spinner" width="16" height="16" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                <circle cx="12" cy="12" r="10" stroke-dasharray="32" stroke-linecap="round"></circle>
            </svg>
            <span>Analyzing...</span>
        `;

        try {
            const res = await fetch('/api/analyze', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ code: codeEditor.value })
            });

            if (res.ok) {
                const data = await res.json();
                displayResults(data);
            }
        } catch (err) {
            console.error('Analysis error:', err);
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
    }

    btnAnalyze.addEventListener('click', runAnalysis);

    // Display Results in Dashboard
    function displayResults(data) {
        currentDiagnostics = data.diagnostics || [];
        const summary = data.summary || { errors: 0, warnings: 0, info: 0 };

        // Calculate Health Score
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

        // Render Diagnostics Cards
        renderDiagnostics();

        // Populate AST
        astOutput.textContent = data.ast || '// No AST generated';

        // Populate Symbols
        symbolsOutput.textContent = data.symbols || '// No symbols generated';

        // Populate Tokens Table
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

    // Render Filtered Diagnostics
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

            // Click to jump to line in editor
            card.addEventListener('click', () => {
                jumpToLine(diag.line);
            });

            diagnosticsList.appendChild(card);
        });
    }

    // Jump to line in editor
    function jumpToLine(lineNumber) {
        const lines = codeEditor.value.split('\n');
        let charIndex = 0;
        for (let i = 0; i < lineNumber - 1 && i < lines.length; i++) {
            charIndex += lines[i].length + 1;
        }
        codeEditor.focus();
        codeEditor.setSelectionRange(charIndex, charIndex + (lines[lineNumber - 1] ? lines[lineNumber - 1].length : 0));
        
        // Scroll line into view
        const lineHeight = 19.5; // approx line height in px
        codeEditor.scrollTop = Math.max(0, (lineNumber - 4) * lineHeight);
    }

    // Load Rules Catalog
    async function loadRules() {
        try {
            const res = await fetch('/api/rules');
            if (res.ok) {
                const rules = await res.json();
                rulesGrid.innerHTML = '';
                rules.forEach(rule => {
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
        } catch (err) {
            console.error('Failed to load rules:', err);
        }
    }

    // Modal Handlers
    btnVivaGuide.addEventListener('click', () => {
        vivaModal.classList.remove('hidden');
    });
    btnCloseModal.addEventListener('click', () => {
        vivaModal.classList.add('hidden');
    });
    btnCloseModalBtn.addEventListener('click', () => {
        vivaModal.classList.add('hidden');
    });
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

    // Initial Load
    loadSample('demo');
    loadRules();
});
