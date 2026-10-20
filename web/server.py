#!/usr/bin/env python3
"""
Lightweight Web Server for Static Code Quality Analyzer
Serves an interactive web dashboard and bridges the web UI to the compiled C analyzer binary.
"""

import http.server
import socketserver
import json
import subprocess
import os
import sys
import tempfile
import urllib.parse

PORT = 5050
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
PROJECT_ROOT = os.path.dirname(SCRIPT_DIR)
ANALYZER_BIN = os.path.join(PROJECT_ROOT, "analyzer")
if sys.platform == "win32" and not os.path.exists(ANALYZER_BIN):
    ANALYZER_BIN = os.path.join(PROJECT_ROOT, "analyzer.exe")

RULES = [
    {"id": "W001", "name": "Unused Variable", "severity": "warning", "desc": "Local variable declared but never read, or assigned but never read."},
    {"id": "W002", "name": "Assignment in Condition", "severity": "warning", "desc": "Assignment '=' used as condition without extra parentheses."},
    {"id": "W003", "name": "Unreachable Code", "severity": "warning", "desc": "Statements following return, break, or continue in the same block."},
    {"id": "W004", "name": "Missing Return", "severity": "warning", "desc": "Non-void function where control reaches the end of the body without a return statement."},
    {"id": "W005", "name": "Uninitialized Variable", "severity": "warning", "desc": "Local scalar variable read before any prior assignment."},
    {"id": "W006", "name": "Magic Number", "severity": "info", "desc": "Numeric literals other than 0, 1, 2, -1 appearing directly in expressions."},
    {"id": "W007", "name": "Deep Nesting", "severity": "warning", "desc": "Nesting depth of if/loop/switch statements exceeds threshold of 3."},
    {"id": "W008", "name": "Long Function", "severity": "warning", "desc": "Function body length exceeds threshold of 40 lines."},
    {"id": "W009", "name": "Empty Body", "severity": "warning", "desc": "Control flow statement with an empty block or lone semicolon body."},
    {"id": "W010", "name": "Switch Without Default", "severity": "warning", "desc": "Switch statement missing a default: case label."},
    {"id": "W011", "name": "Unsafe Function", "severity": "warning", "desc": "Call to bounded-unsafe C standard library functions (strcpy, gets, sprintf)."},
    {"id": "W012", "name": "Variable Shadowing", "severity": "warning", "desc": "Local variable or parameter shadowing a variable in an enclosing scope."},
    {"id": "W013", "name": "Division by Zero", "severity": "error", "desc": "Division or modulo with a compile-time literal 0 denominator."},
    {"id": "W014", "name": "Fall-Through Case", "severity": "info", "desc": "Switch case label with statements falling through to next case without break/return."}
]

def ensure_analyzer_built():
    """Ensure the analyzer binary is compiled and up-to-date."""
    if not os.path.exists(ANALYZER_BIN):
        subprocess.run(["make"], cwd=PROJECT_ROOT, check=True)

def run_analyzer_tool(args):
    """Executes ./analyzer with given arguments and returns stdout/stderr and exit code."""
    ensure_analyzer_built()
    cmd = [ANALYZER_BIN] + args
    res = subprocess.run(cmd, cwd=PROJECT_ROOT, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    return res.returncode, res.stdout

def parse_diagnostics(raw_output):
    """Parses standard analyzer console output into structured diagnostic objects."""
    diags = []
    lines = raw_output.splitlines()
    i = 0
    while i < len(lines):
        line = lines[i]
        # Format: file:line:col: severity [RULE]: message
        if ": warning [" in line or ": error [" in line or ": info [" in line:
            parts = line.split(":", 3)
            if len(parts) >= 4:
                file_part = parts[0]
                try:
                    line_num = int(parts[1])
                    col_num = int(parts[2])
                except ValueError:
                    line_num, col_num = 1, 1
                
                rest = parts[3].strip()
                # e.g. "warning [W004]: control reaches end of non-void function 'foo'"
                severity = "info"
                rule_id = "UNKNOWN"
                msg = rest
                if " [" in rest and "]: " in rest:
                    sev_str = rest.split(" [")[0].strip()
                    rule_id = rest.split(" [")[1].split("]: ")[0].strip()
                    msg = rest.split("]: ")[1].strip()
                    severity = sev_str.lower()
                
                # Check for source snippet and suggestion on following lines
                snippet_lines = []
                suggestion = ""
                i += 1
                while i < len(lines):
                    next_line = lines[i]
                    if next_line.strip().startswith("= suggestion:"):
                        suggestion = next_line.strip()[len("= suggestion:"):].strip()
                        i += 1
                        break
                    elif ": warning [" in next_line or ": error [" in next_line or ": info [" in next_line:
                        break
                    elif "in " in next_line and ("error" in next_line or "warning" in next_line):
                        break
                    elif "Analysis completed:" in next_line:
                        break
                    else:
                        snippet_lines.append(next_line)
                        i += 1
                
                diags.append({
                    "line": line_num,
                    "col": col_num,
                    "severity": severity,
                    "ruleId": rule_id,
                    "message": msg,
                    "suggestion": suggestion,
                    "snippet": "\n".join(snippet_lines)
                })
                continue
        i += 1
    return diags

class AnalyzerHandler(http.server.SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=SCRIPT_DIR, **kwargs)

    def do_GET(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/api/rules":
            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(RULES).encode("utf-8"))
            return

        if parsed.path == "/api/sample":
            query = urllib.parse.parse_qs(parsed.query)
            sample_name = query.get("name", ["demo"])[0]
            
            sample_path = os.path.join(PROJECT_ROOT, "samples", f"{sample_name}.c")
            if not os.path.exists(sample_path):
                # Try in tests/cases
                sample_path = os.path.join(PROJECT_ROOT, "tests", "cases", f"{sample_name}.c")
            
            if os.path.exists(sample_path):
                with open(sample_path, "r", encoding="utf-8", errors="replace") as f:
                    code = f.read()
                self.send_response(200)
                self.send_header("Content-Type", "application/json")
                self.end_headers()
                self.wfile.write(json.dumps({"name": sample_name, "code": code}).encode("utf-8"))
            else:
                self.send_response(404)
                self.end_headers()
            return

        if parsed.path == "/api/samples":
            samples = []
            samples_dir = os.path.join(PROJECT_ROOT, "samples")
            if os.path.exists(samples_dir):
                for f in os.listdir(samples_dir):
                    if f.endswith(".c"):
                        samples.append({"name": f[:-2], "path": f"samples/{f}", "category": "Presets"})
            
            # Select key showcase cases from tests/cases
            featured_tests = [
                ("chk_w002", "Assignment in Condition", "Syntax Smell"),
                ("chk_w007", "Deep Nesting Threshold", "Complexity"),
                ("chk_w011", "Unsafe C Library Functions", "Security"),
                ("chk_w013", "Division by Zero", "Safety Error"),
                ("sym_shadowing", "Variable Scope Shadowing", "Symbol Table"),
                ("sym_nested_scopes", "Nested Scope Resolution", "Symbol Table")
            ]
            for test_id, label, cat in featured_tests:
                test_file = os.path.join(PROJECT_ROOT, "tests", "cases", f"{test_id}.c")
                if os.path.exists(test_file):
                    samples.append({"name": test_id, "label": label, "category": cat})

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(samples).encode("utf-8"))
            return

        return super().do_GET()

    def do_POST(self):
        parsed = urllib.parse.urlparse(self.path)
        if parsed.path == "/api/analyze":
            content_length = int(self.headers.get("Content-Length", 0))
            body = self.rfile.read(content_length).decode("utf-8")
            try:
                data = json.loads(body)
                code = data.get("code", "")
            except Exception:
                code = body

            # Write code to a temporary C file for static analysis
            with tempfile.NamedTemporaryFile(suffix=".c", mode="w", delete=False) as tmp:
                tmp_filename = tmp.name
                tmp.write(code)

            try:
                # 1. Main analysis
                ret_code, console_out = run_analyzer_tool([tmp_filename])
                # 2. Dump AST
                _, ast_out = run_analyzer_tool([tmp_filename, "--dump-ast"])
                # 3. Dump Symbols
                _, symbols_out = run_analyzer_tool([tmp_filename, "--dump-symbols"])
                # 4. Dump Tokens
                _, tokens_out = run_analyzer_tool([tmp_filename, "--dump-tokens"])
            finally:
                if os.path.exists(tmp_filename):
                    os.remove(tmp_filename)

            # Strip temporary file path for clean display
            clean_console = console_out.replace(tmp_filename, "input.c")
            clean_ast = ast_out.replace(tmp_filename, "input.c")
            clean_symbols = symbols_out.replace(tmp_filename, "input.c")

            diags = parse_diagnostics(clean_console)
            err_count = sum(1 for d in diags if d["severity"] == "error")
            warn_count = sum(1 for d in diags if d["severity"] == "warning")
            info_count = sum(1 for d in diags if d["severity"] == "info")

            # Parse token lines
            tokens = []
            for tline in tokens_out.splitlines():
                parts = tline.strip().split(" ", 2)
                if len(parts) >= 2:
                    pos = parts[0]
                    ttype = parts[1]
                    val = parts[2] if len(parts) > 2 else ""
                    tokens.append({"pos": pos, "type": ttype, "val": val})

            resp = {
                "success": True,
                "exitCode": ret_code,
                "summary": {
                    "errors": err_count,
                    "warnings": warn_count,
                    "info": info_count,
                    "instructionsExecuted": 0,
                    "safety": "100% Pure Static Analysis (0 Instructions Executed)"
                },
                "diagnostics": diags,
                "rawConsole": clean_console,
                "ast": clean_ast,
                "symbols": clean_symbols,
                "tokens": tokens[:250] # Send up to 250 tokens
            }

            self.send_response(200)
            self.send_header("Content-Type", "application/json")
            self.end_headers()
            self.wfile.write(json.dumps(resp).encode("utf-8"))
            return

        self.send_response(404)
        self.end_headers()

def run_server():
    socketserver.TCPServer.allow_reuse_address = True
    port = PORT
    for attempt_port in [PORT, 8080, 5051, 8081]:
        try:
            httpd = socketserver.TCPServer(("0.0.0.0", attempt_port), AnalyzerHandler)
            port = attempt_port
            break
        except OSError:
            continue

    print(f"STATIC_ANALYZER_SERVER_RUNNING on http://localhost:{port}")
    sys.stdout.flush()
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        httpd.server_close()

if __name__ == "__main__":
    run_server()
