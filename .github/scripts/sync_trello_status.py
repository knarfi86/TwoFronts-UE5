#!/usr/bin/env python3
"""Aktualisiert ausschließlich den automatisch verwalteten GitHub-Status einer Trello-Karte."""

from __future__ import annotations

import json
import os
import re
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime
from typing import Any, Iterable
from urllib.error import HTTPError, URLError
from urllib.parse import urlencode
from urllib.request import Request, urlopen


EXPECTED_CARD_NAME = "Aktueller Projektstand"
STATUS_START = "<!-- TWO-FRONTS-GITHUB-STATUS:START -->"
STATUS_END = "<!-- TWO-FRONTS-GITHUB-STATUS:END -->"
STATUS_PATTERN = re.compile(
    rf"{re.escape(STATUS_START)}.*?{re.escape(STATUS_END)}", re.DOTALL
)


class SyncError(RuntimeError):
    """A user-facing synchronization error that never includes credentials."""


class ConfigurationError(SyncError):
    """Raised when a required GitHub Actions configuration value is missing."""


@dataclass(frozen=True)
class Commit:
    sha: str
    subject: str
    author: str
    committed_at: str


def required_env(name: str) -> str:
    value = os.environ.get(name, "").strip()
    if not value:
        raise ConfigurationError(
            f"Die GitHub-Actions-Konfiguration '{name}' fehlt oder ist leer. "
            "Bitte die Einrichtungsanleitung in Docs/TRELLO_SYNC.md beachten."
        )
    return value


def run_git(*args: str) -> str:
    try:
        result = subprocess.run(
            ["git", *args],
            check=True,
            capture_output=True,
            text=True,
            encoding="utf-8",
        )
    except FileNotFoundError as error:
        raise SyncError("Git ist im GitHub-Actions-Runner nicht verfügbar.") from error
    except subprocess.CalledProcessError as error:
        detail = error.stderr.strip() or error.stdout.strip()
        raise SyncError(f"Git-Daten konnten nicht gelesen werden: {detail}") from error
    return result.stdout


def recent_commits() -> list[Commit]:
    separator = "\x1f"
    records = run_git("log", "-5", f"--format=%H{separator}%s{separator}%an{separator}%aI").splitlines()
    commits: list[Commit] = []
    for record in records:
        parts = record.split(separator)
        if len(parts) != 4:
            raise SyncError("Das Git-Log hat ein unerwartetes Format.")
        commits.append(Commit(*parts))
    if not commits:
        raise SyncError("Es wurde kein Commit für die Synchronisierung gefunden.")
    return commits


def changed_files() -> list[str]:
    files = run_git("show", "--format=", "--name-only", "HEAD").splitlines()
    return sorted({path for path in files if path.strip()})


def one_line(value: str) -> str:
    return " ".join(value.split()).replace("[", "\\[").replace("]", "\\]")


def commit_url(repository: str, sha: str) -> str:
    server_url = os.environ.get("GITHUB_SERVER_URL", "https://github.com").rstrip("/")
    return f"{server_url}/{repository}/commit/{sha}"


def format_commit_time(value: str) -> str:
    try:
        timestamp = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return one_line(value)
    return timestamp.strftime("%d.%m.%Y, %H:%M %z")


def ci_status(check_runs: Iterable[dict[str, Any]]) -> str:
    relevant = [
        check
        for check in check_runs
        if not str(check.get("name", "")).startswith("Trello-Status synchronisieren")
    ]
    if not relevant:
        return "Keine CI-Prüfung für diesen Commit verfügbar."
    completed = [check for check in relevant if check.get("status") == "completed"]
    if not completed:
        return "CI-Prüfung läuft noch; es liegt kein abgeschlossenes Ergebnis vor."
    failed = [
        check
        for check in completed
        if check.get("conclusion") not in {"success", "neutral", "skipped"}
    ]
    if failed:
        names = ", ".join(one_line(str(check.get("name", "Unbenannte Prüfung"))) for check in failed)
        return f"Nicht erfolgreich: {names}."
    return "Erfolgreich abgeschlossen."


def render_status(
    repository: str,
    commits: list[Commit],
    files: list[str],
    build_status: str,
    next_step: str | None,
) -> str:
    latest = commits[0]
    latest_link = commit_url(repository, latest.sha)
    lines = [
        STATUS_START,
        "## Automatisch synchronisierter GitHub-Status",
        "_Dieser Bereich wird durch GitHub Actions verwaltet. Inhalte außerhalb bleiben unverändert._",
        "",
        f"- **Aktueller GitHub-Stand:** [{repository} @ {latest.sha[:7]}]({latest_link})",
        f"- **Letzter Commit:** [{latest.sha[:7]}]({latest_link}) — {one_line(latest.subject)}",
        f"- **Letzte Änderung:** {format_commit_time(latest.committed_at)}",
        f"- **Verantwortlicher Entwickler:** {one_line(latest.author)}",
        f"- **Build-Status:** {build_status}",
        "",
        "### Fünf letzte Commits",
    ]
    for commit in commits:
        lines.append(
            f"- [{commit.sha[:7]}]({commit_url(repository, commit.sha)}) — {one_line(commit.subject)} "
            f"({one_line(commit.author)}, {format_commit_time(commit.committed_at)})"
        )
    lines.extend(["", "### Zuletzt geänderte Dateien"])
    if files:
        lines.extend(f"- `{one_line(path)}`" for path in files)
    else:
        lines.append("- Keine Dateien im letzten Commit gemeldet.")
    if next_step and next_step.strip():
        lines.extend(["", "### Nächster Entwicklungsschritt", one_line(next_step)])
    lines.append(STATUS_END)
    return "\n".join(lines)


def merge_status_block(description: str, status_block: str) -> str:
    starts = description.count(STATUS_START)
    ends = description.count(STATUS_END)
    if starts != ends:
        raise SyncError(
            "Der automatische Trello-Statusbereich ist unvollständig. Die Karte wird zum Schutz manueller Inhalte nicht geändert."
        )
    if starts > 1:
        raise SyncError(
            "Mehrere automatische Trello-Statusbereiche wurden gefunden. Die Karte wird nicht geändert, um keine Inhalte zu verlieren."
        )
    if starts == 1:
        return STATUS_PATTERN.sub(status_block, description, count=1)
    return f"{description.rstrip()}\n\n{status_block}" if description.strip() else status_block


class TrelloClient:
    def __init__(self, api_key: str, token: str, card_id: str) -> None:
        self.api_key = api_key
        self.token = token
        self.card_id = card_id

    def _request(self, method: str, path: str, data: dict[str, str] | None = None) -> dict[str, Any]:
        query = urlencode({"key": self.api_key, "token": self.token})
        payload = urlencode(data).encode("utf-8") if data else None
        request = Request(
            f"https://api.trello.com/1/{path}?{query}",
            data=payload,
            method=method,
            headers={"Accept": "application/json"},
        )
        try:
            with urlopen(request, timeout=20) as response:
                return json.loads(response.read().decode("utf-8"))
        except HTTPError as error:
            raise SyncError(
                f"Trello-API-Fehler (HTTP {error.code}). Prüfen Sie Karten-ID sowie Trello-Key und -Token; Zugangsdaten werden nicht ausgegeben."
            ) from error
        except URLError as error:
            raise SyncError(
                "Trello konnte nicht erreicht werden. Prüfen Sie die Netzwerkverbindung und versuchen Sie den Workflow erneut."
            ) from error
        except json.JSONDecodeError as error:
            raise SyncError("Trello lieferte keine erwartete JSON-Antwort.") from error

    def get_card(self) -> dict[str, Any]:
        return self._request("GET", f"cards/{self.card_id}")

    def update_description(self, description: str) -> None:
        self._request("PUT", f"cards/{self.card_id}", {"desc": description})


def github_check_runs(repository: str, sha: str) -> list[dict[str, Any]]:
    token = os.environ.get("GITHUB_TOKEN", "").strip()
    if not token:
        return []
    request = Request(
        f"https://api.github.com/repos/{repository}/commits/{sha}/check-runs",
        headers={
            "Accept": "application/vnd.github+json",
            "Authorization": f"Bearer {token}",
            "X-GitHub-Api-Version": "2022-11-28",
        },
    )
    try:
        with urlopen(request, timeout=20) as response:
            payload = json.loads(response.read().decode("utf-8"))
    except (HTTPError, URLError, json.JSONDecodeError):
        return []
    return payload.get("check_runs", []) if isinstance(payload, dict) else []


def main() -> int:
    try:
        api_key = required_env("TRELLO_API_KEY")
        token = required_env("TRELLO_TOKEN")
        card_id = required_env("TRELLO_CARD_ID")
        repository = required_env("GITHUB_REPOSITORY")
        commits = recent_commits()
        files = changed_files()
        build = ci_status(github_check_runs(repository, commits[0].sha))
        status_block = render_status(
            repository, commits, files, build, os.environ.get("TRELLO_NEXT_STEP")
        )
        client = TrelloClient(api_key, token, card_id)
        card = client.get_card()
        if card.get("name") != EXPECTED_CARD_NAME:
            raise SyncError(
                f"Die konfigurierte Trello-Karte heißt nicht '{EXPECTED_CARD_NAME}'. Es wurden keine Änderungen vorgenommen."
            )
        updated_description = merge_status_block(str(card.get("desc", "")), status_block)
        client.update_description(updated_description)
        print("Trello-Karte 'Aktueller Projektstand' wurde aktualisiert.")
        return 0
    except SyncError as error:
        print(f"Trello-Synchronisierung fehlgeschlagen: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
