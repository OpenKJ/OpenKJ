# Security Policy

## Reporting a vulnerability

**Please do not report security vulnerabilities through public GitHub issues,
discussions, or pull requests.** A public report makes the issue exploitable
against every current user before a fix is available.

Report privately through either channel:

1. **GitHub private vulnerability reporting** — go to the
   [Security tab](https://github.com/OpenKJ/OpenKJ/security) and choose
   *Report a vulnerability*. This is preferred: it keeps the report, the
   discussion, and the resulting advisory in one place.
2. **Email** — <support@openkj.org>, with `SECURITY` in the subject line.

Please include:

- the version of OpenKJ and the operating system it runs on,
- what an attacker can achieve, not only what the code does,
- the steps to reproduce, and
- any proof-of-concept material you have.

## What to expect

- Acknowledgement of your report within **7 days**.
- An assessment, including whether we agree on severity, within **30 days**.
- Credit in the advisory and release notes, unless you would rather stay
  anonymous.

We ask that you give us a reasonable opportunity to publish a fix before
disclosing publicly. We will keep you informed of progress and will tell you
when the fix ships.

## Supported versions

Security fixes land on `master` and go out in the next release. Older releases
are not patched separately; please update to the current version before
reporting.

## Scope

In scope:

- The OpenKJ application and anything under this repository.
- The build and release pipeline in `.github/workflows/`.
- The container images published to `ghcr.io/openkj/openkj`.

Out of scope:

- Third-party services OpenKJ talks to, including the song shop provider and
  the remote request server. Report those to their respective operators.
- Attacks that require an already-compromised machine or physical access to a
  running installation.
- Findings from automated scanners with no demonstrated impact.

## Automated security tooling

This repository runs the following on every push and pull request, with results
in the [Security tab](https://github.com/OpenKJ/OpenKJ/security/code-scanning):

| Tool | Covers |
| --- | --- |
| CodeQL | C++ dataflow and taint analysis |
| flawfinder | CWE-mapped unsafe C/C++ API usage |
| OWASP Dependency-Check | Known vulnerabilities in dependencies |
| OSV-Scanner | Advisories against the OSV database, including the spdlog submodule |
| Trivy | Filesystem, secret and container misconfiguration scanning |
| gitleaks | Secrets across the full git history |
| zizmor + actionlint | GitHub Actions misconfiguration and workflow injection |
| OSSF Scorecard | Overall supply-chain posture |
| checksec | Asserts hardening flags survive into the shipped binary |

A CycloneDX SBOM is produced for every build and attached to the workflow run.
