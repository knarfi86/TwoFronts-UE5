import importlib.util
import pathlib
import sys
import unittest
from unittest.mock import patch


SCRIPT_PATH = pathlib.Path(__file__).resolve().parents[2] / ".github" / "scripts" / "sync_trello_status.py"
SPEC = importlib.util.spec_from_file_location("sync_trello_status", SCRIPT_PATH)
sync = importlib.util.module_from_spec(SPEC)
assert SPEC.loader is not None
sys.modules[SPEC.name] = sync
SPEC.loader.exec_module(sync)


class TrelloDescriptionTests(unittest.TestCase):
    def setUp(self):
        self.status = f"{sync.STATUS_START}\n## Automatisch synchronisierter GitHub-Status\n{sync.STATUS_END}"

    def test_adds_status_block_without_changing_manual_description(self):
        manual = "# Manuelle Projektinformationen\n\nDiese Notiz bleibt erhalten."

        result = sync.merge_status_block(manual, self.status)

        self.assertEqual(result, f"{manual}\n\n{self.status}")

    def test_replaces_only_existing_status_block_without_duplicates(self):
        manual_before = "# Manuelle Projektinformationen\n\n"
        manual_after = "\n\nManuelle Notiz nach dem Status."
        previous = f"{sync.STATUS_START}\nAlter Inhalt\n{sync.STATUS_END}"

        result = sync.merge_status_block(f"{manual_before}{previous}{manual_after}", self.status)

        self.assertEqual(result, f"{manual_before}{self.status}{manual_after}")
        self.assertEqual(result.count(sync.STATUS_START), 1)
        self.assertEqual(result.count(sync.STATUS_END), 1)

    def test_refuses_incomplete_marker_pair_to_protect_manual_data(self):
        with self.assertRaisesRegex(sync.SyncError, "unvollständig"):
            sync.merge_status_block(f"Manuell\n{sync.STATUS_START}", self.status)

    def test_refuses_multiple_blocks_to_protect_manual_data(self):
        description = f"{self.status}\n\n{self.status}"
        with self.assertRaisesRegex(sync.SyncError, "Mehrere"):
            sync.merge_status_block(description, self.status)


class StatusRenderingTests(unittest.TestCase):
    def test_renders_required_commit_and_optional_next_step_data(self):
        commits = [
            sync.Commit("abcdef0123456789", "Kamera verbessert", "Ada Beispiel", "2026-10-04T10:30:00+02:00"),
            sync.Commit("0123456789abcdef", "Tests ergänzt", "Ben Beispiel", "2026-10-03T09:00:00+02:00"),
        ]

        result = sync.render_status(
            "knarfi86/TwoFronts-UE5",
            commits,
            ["Source/TwoFronts/Private/TFGameMode.cpp"],
            "Erfolgreich abgeschlossen.",
            "Multiplayer-Test vorbereiten",
        )

        self.assertIn("Aktueller GitHub-Stand", result)
        self.assertIn("Kamera verbessert", result)
        self.assertIn("Fünf letzte Commits", result)
        self.assertIn("TFGameMode.cpp", result)
        self.assertIn("Build-Status", result)
        self.assertIn("Multiplayer-Test vorbereiten", result)
        self.assertIn("https://github.com/knarfi86/TwoFronts-UE5/commit/abcdef0123456789", result)


class CiStatusTests(unittest.TestCase):
    def test_reports_only_completed_successful_checks_as_success(self):
        status = sync.ci_status(
            [
                {"name": "Build", "status": "completed", "conclusion": "success"},
                {"name": "Lint", "status": "completed", "conclusion": "neutral"},
            ]
        )
        self.assertEqual(status, "Erfolgreich abgeschlossen.")

    def test_reports_failing_check_name(self):
        status = sync.ci_status(
            [{"name": "Build", "status": "completed", "conclusion": "failure"}]
        )
        self.assertEqual(status, "Nicht erfolgreich: Build.")


class ConfigurationTests(unittest.TestCase):
    def test_missing_configuration_has_a_clear_error(self):
        with patch.dict("os.environ", {}, clear=True):
            with self.assertRaisesRegex(sync.ConfigurationError, "TRELLO_TOKEN"):
                sync.required_env("TRELLO_TOKEN")


if __name__ == "__main__":
    unittest.main()
