# Automated API Tests with Postman CLI

The GitHub Action workflow runs end-to-end API tests for our webserv server using Postman CLI. It builds and starts a local webserv instance with the dedicated `postman-test.conf` configuration, executes a full Postman collection, and reports results via JSON parsing with `jq`.

It runs on every **pull request** to any branch (`*`) or **manually** via `workflow_dispatch`
This ensures API functionality is validated before merging, complementing checks on config parsing.

## What It Tests

Uses Postman collection **Webserv Tests - Enhanced** covering:

- **HTTP methods:** GET, POST, DELETE, HEAD, OPTIONS
- **Static files:** HTML, images...
- **File uploads**
- **Redirections:** 301, 302
- **CGI:** Python `.py`, PHP `.php`, crash cases
- **Custom error pages:** 400, 403, 404, 405, 408, 500, 505...
- **Body size limits:** 1K, 1M, 10M, 50M
- **Multi-ports:** 8080, 8081 and virtual hosting (`localhost`, `localhost2`, `example.com`, etc.)
- **Autoindex:** on/off
- **Basic security and edge cases**

### Environment: `webserv-local`

| Variable | Value |
|---|---|
| `BASE_URL` | `http://localhost:8080` |
| `fileName` | `403.png` (for uploads) |

The config `postman-test.conf` defines routes like `/api/`, `/cgi-bin-py/`, `/upload/`, redirections, and multi-server blocks.

---

## Workflow Steps Overview

| Step | Description |
|---|---|
| **Checkout** | Clones repo with `actions/checkout@v4`. |
| **Install Dependencies** | `build-essential` (for `make`), `jq` (JSON parsing). |
| **Build webserv** | Runs `make`. |
| **Make Executable** | `chmod +x ./webserv`. |
| **Install Postman CLI** | Downloads standalone Linux script via curl. |
| **Login** | Uses `${{ secrets.POSTMAN_API_KEY }}`. |
| **Start Server** | `./webserv config-files/valid/postman-test.conf &` (PID saved). |
| **Run Tests** | Postman collection run → `report.json` (ignores redirects). `continue-on-error: true`. |
| **Show Results** | `jq` extracts stats and failed assertions. |
| **Validate** | Re-runs dry test, greps failures → exits 1 if any. |
| **Cleanup** | Kills server via PID (always runs). |

---

## Related Workflows

This integrates with the config parsing suite:

- **`check-build-parsing.yml`:** Tests ~180 config files for parsing errors via `./scripts/test_parser.sh --valgrind`.
  - **Triggers:** PR, push to `main`.
  - **Jobs:** `test-parser` (build + script, `continue-on-error`), `test-summary` (fail if parser fails).
  - Ensures configs like `postman-test.conf` parse correctly before API tests.

---

## Troubleshooting

- **Server bind fail:** Port 8080/8081 in use (should never happen in Github Actions environment)? Cleanup kills ensure no orphans.
- **Postman auth:** Verify API key permissions (collection read).
- **Valgrind in parser:** Pairs well for memory leaks in build deps.
