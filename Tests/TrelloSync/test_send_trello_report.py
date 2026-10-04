import importlib.util
import os
import pathlib
import sys
import unittest
from unittest.mock import MagicMock, patch


SCRIPT_PATH = pathlib.Path(__file__).resolve().parents[2] / ".github" / "scripts" / "send_trello_report.py"
SPEC = importlib.util.spec_from_file_location("send_trello_report", SCRIPT_PATH)
sync = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = sync
SPEC.loader.exec_module(sync)


class ReportTests(unittest.TestCase):
    def setUp(self):
        self.commits = [
            sync.Commit("abcdef0123456789", "Kamera verbessert", "Ada Beispiel", "2026-10-04T10:30:00+02:00"),
            sync.Commit("0123456789abcdef", "Tests ergänzt", "Ben Beispiel", "2026-10-03T09:00:00+02:00"),
        ]

    def test_report_contains_commit_developer_date_files_and_links(self):
        report = sync.create_report(
            "knarfi86/TwoFronts-UE5",
            self.commits,
            ["Source/TwoFronts/Private/TFGameMode.cpp"],
        )

        self.assertIn("Kamera verbessert", report)
        self.assertIn("Ada Beispiel", report)
        self.assertIn("04.10.2026, 10:30 +0200", report)
        self.assertIn("TFGameMode.cpp", report)
        self.assertIn("https://github.com/knarfi86/TwoFronts-UE5/commit/abcdef0123456789", report)
        self.assertIn("https://github.com/knarfi86/TwoFronts-UE5/commit/0123456789abcdef", report)

    def test_message_addresses_only_the_configured_card_email(self):
        settings = sync.SMTPSettings("smtp.example.test", 465, True, "sender@example.test", "secret", "card@example.test")

        message = sync.create_message(settings, "knarfi86/TwoFronts-UE5", self.commits, [])

        self.assertEqual(message["From"], "sender@example.test")
        self.assertEqual(message["To"], "card@example.test")
        self.assertIn("abcdef0", message["Subject"])


class ConfigurationTests(unittest.TestCase):
    def test_default_configuration_supports_gmail_ssl_port_465(self):
        values = {
            "SMTP_USER": "sender@example.test",
            "SMTP_PASSWORD": "secret",
            "TRELLO_CARD_EMAIL": "card@example.test",
        }
        with patch.dict(os.environ, values, clear=True):
            settings = sync.smtp_settings_from_environment()

        self.assertEqual(settings.host, "smtp.gmail.com")
        self.assertEqual(settings.port, 465)
        self.assertTrue(settings.use_ssl)

    def test_supports_configured_starttls_server(self):
        values = {
            "SMTP_HOST": "mail.example.test",
            "SMTP_PORT": "587",
            "SMTP_USE_SSL": "false",
            "SMTP_USER": "sender@example.test",
            "SMTP_PASSWORD": "secret",
            "TRELLO_CARD_EMAIL": "card@example.test",
        }
        with patch.dict(os.environ, values, clear=True):
            settings = sync.smtp_settings_from_environment()

        self.assertEqual(settings.host, "mail.example.test")
        self.assertEqual(settings.port, 587)
        self.assertFalse(settings.use_ssl)

    def test_missing_secret_has_a_clear_error(self):
        with patch.dict(os.environ, {}, clear=True):
            with self.assertRaisesRegex(sync.ConfigurationError, "SMTP_PASSWORD"):
                sync.required_env("SMTP_PASSWORD")


class SMTPTransportTests(unittest.TestCase):
    def setUp(self):
        self.settings = sync.SMTPSettings("smtp.example.test", 465, True, "sender@example.test", "secret", "card@example.test")
        self.message = sync.create_message(
            self.settings,
            "knarfi86/TwoFronts-UE5",
            [sync.Commit("abcdef0123456789", "Test", "Ada", "2026-10-04T10:30:00+02:00")],
            [],
        )

    @patch.object(sync.smtplib, "SMTP_SSL")
    def test_ssl_transport_logs_in_and_sends_message(self, smtp_ssl):
        server = MagicMock()
        smtp_ssl.return_value.__enter__.return_value = server

        sync.send_message(self.settings, self.message)

        smtp_ssl.assert_called_once()
        server.login.assert_called_once_with("sender@example.test", "secret")
        server.send_message.assert_called_once_with(self.message)

    @patch.object(sync.smtplib, "SMTP")
    def test_starttls_transport_is_supported_when_ssl_is_disabled(self, smtp):
        settings = sync.SMTPSettings("smtp.example.test", 587, False, "sender@example.test", "secret", "card@example.test")
        server = MagicMock()
        smtp.return_value.__enter__.return_value = server

        sync.send_message(settings, self.message)

        server.starttls.assert_called_once()
        server.login.assert_called_once_with("sender@example.test", "secret")
        server.send_message.assert_called_once_with(self.message)

    @patch.object(sync.smtplib, "SMTP_SSL", side_effect=OSError("network unavailable"))
    def test_smtp_error_does_not_expose_credentials(self, smtp_ssl):
        with self.assertRaisesRegex(sync.SyncError, "SMTP-Versand fehlgeschlagen") as captured:
            sync.send_message(self.settings, self.message)

        self.assertNotIn("secret", str(captured.exception))


if __name__ == "__main__":
    unittest.main()
