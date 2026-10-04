#!/usr/bin/env python3
"""Sendet einen GitHub-Entwicklungsbericht per SMTP an die Trello-Kartenadresse."""

from __future__ import annotations

import os
import smtplib
import ssl
import subprocess
import sys
from dataclasses import dataclass
from datetime import datetime
from email.message import EmailMessage


class SyncError(RuntimeError):
    """A user-facing error that never includes SMTP credentials."""


class ConfigurationError(SyncError):
    """Raised when a required GitHub Actions configuration value is missing."""


@dataclass(frozen=True)
class Commit:
    sha: str
    subject: str
    author: str
    committed_at: str


@dataclass(frozen=True)
class SMTPSettings:
    host: str
    port: int
    use_ssl: bool
    user: str
    password: str
    recipient: str


def required_env(name: str) -> str:
    value = os.environ.get(name, "").strip()
    if not value:
        raise ConfigurationError(
            f"Die GitHub-Actions-Konfiguration '{name}' fehlt oder ist leer. "
            "Bitte die Einrichtungsanleitung in Docs/TRELLO_SYNC.md beachten."
        )
    return value


def smtp_settings_from_environment() -> SMTPSettings:
    host = os.environ.get("SMTP_HOST", "smtp.gmail.com").strip()
    if not host:
        raise ConfigurationError("SMTP_HOST darf nicht leer sein.")
    try:
        port = int(os.environ.get("SMTP_PORT", "465"))
    except ValueError as error:
        raise ConfigurationError("SMTP_PORT muss eine gültige Portnummer sein.") from error
    if not 1 <= port <= 65535:
        raise ConfigurationError("SMTP_PORT muss zwischen 1 und 65535 liegen.")
    use_ssl_value = os.environ.get("SMTP_USE_SSL", "true").strip().lower()
    if use_ssl_value not in {"true", "false"}:
        raise ConfigurationError("SMTP_USE_SSL muss 'true' oder 'false' sein.")
    recipient = required_env("TRELLO_CARD_EMAIL")
    if "@" not in recipient:
        raise ConfigurationError("TRELLO_CARD_EMAIL enthält keine gültige E-Mail-Adresse.")
    return SMTPSettings(
        host=host,
        port=port,
        use_ssl=use_ssl_value == "true",
        user=required_env("SMTP_USER"),
        password=required_env("SMTP_PASSWORD"),
        recipient=recipient,
    )


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
        raise SyncError("Es wurde kein Commit für den Entwicklungsbericht gefunden.")
    return commits


def changed_files() -> list[str]:
    files = run_git("show", "--format=", "--name-only", "HEAD").splitlines()
    return sorted({path for path in files if path.strip()})


def one_line(value: str) -> str:
    return " ".join(value.split())


def commit_url(repository: str, sha: str) -> str:
    server_url = os.environ.get("GITHUB_SERVER_URL", "https://github.com").rstrip("/")
    return f"{server_url}/{repository}/commit/{sha}"


def format_commit_time(value: str) -> str:
    try:
        timestamp = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        return one_line(value)
    return timestamp.strftime("%d.%m.%Y, %H:%M %z")


def create_report(repository: str, commits: list[Commit], files: list[str]) -> str:
    latest = commits[0]
    lines = [
        "Entwicklungsbericht – Two Fronts",
        "",
        f"Repository: {repository}",
        f"Aktueller Commit: {latest.sha[:7]} – {one_line(latest.subject)}",
        f"Link: {commit_url(repository, latest.sha)}",
        f"Verantwortlicher Entwickler: {one_line(latest.author)}",
        f"Letzte Änderung: {format_commit_time(latest.committed_at)}",
        "",
        "Fünf letzte Commits:",
    ]
    for commit in commits:
        lines.append(
            f"- {commit.sha[:7]} – {one_line(commit.subject)} "
            f"({one_line(commit.author)}, {format_commit_time(commit.committed_at)})"
        )
        lines.append(f"  {commit_url(repository, commit.sha)}")
    lines.extend(["", "Zuletzt geänderte Dateien:"])
    if files:
        lines.extend(f"- {one_line(path)}" for path in files)
    else:
        lines.append("- Keine Dateien im letzten Commit gemeldet.")
    return "\n".join(lines)


def create_message(settings: SMTPSettings, repository: str, commits: list[Commit], files: list[str]) -> EmailMessage:
    latest = commits[0]
    message = EmailMessage()
    message["Subject"] = f"Two Fronts – Entwicklungsbericht {latest.sha[:7]}"
    message["From"] = settings.user
    message["To"] = settings.recipient
    message.set_content(create_report(repository, commits, files))
    return message


def send_message(settings: SMTPSettings, message: EmailMessage) -> None:
    try:
        if settings.use_ssl:
            with smtplib.SMTP_SSL(
                settings.host,
                settings.port,
                context=ssl.create_default_context(),
                timeout=30,
            ) as server:
                server.login(settings.user, settings.password)
                server.send_message(message)
        else:
            with smtplib.SMTP(settings.host, settings.port, timeout=30) as server:
                server.ehlo()
                server.starttls(context=ssl.create_default_context())
                server.ehlo()
                server.login(settings.user, settings.password)
                server.send_message(message)
    except (OSError, smtplib.SMTPException) as error:
        raise SyncError(
            "SMTP-Versand fehlgeschlagen. Prüfen Sie SMTP-Host, Port, TLS-Modus sowie die hinterlegten Secrets; Zugangsdaten werden nicht ausgegeben."
        ) from error


def main() -> int:
    try:
        repository = required_env("GITHUB_REPOSITORY")
        settings = smtp_settings_from_environment()
        commits = recent_commits()
        files = changed_files()
        message = create_message(settings, repository, commits, files)
        send_message(settings, message)
        print("Entwicklungsbericht wurde an die konfigurierte Trello-Kartenadresse gesendet.")
        return 0
    except SyncError as error:
        print(f"Trello-E-Mail-Synchronisierung fehlgeschlagen: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
