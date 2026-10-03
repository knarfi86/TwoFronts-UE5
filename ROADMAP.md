# Roadmap

## V0.1 — implementierter Prototyp

- [x] Unabhängiges UE5-C++-Projekt, Git-Ignore und Dokumentation
- [x] Erweiterbare Fraktionen Humans/Synth und acht Unit-Definitionen
- [x] Zwei Fabriken, Queue, Bauzeiten, Spawn
- [x] Kamera, Auswahl, Befehle, Schaden und Reparatur
- [x] Dynamische technische Testarena und funktionales Debug-HUD
- [x] UE-Automationstests für Unit-Katalog und Health/Reparaturregeln
- [ ] Lokaler Editor-Compile und Gameplay-Durchlauf — durch fehlende UE5/MSVC blockiert

## Als Nächstes nach Werkzeug- und Assetverfügbarkeit

1. Die installierte UE5-Version und Toolchain erfassen, Projektdateien generieren, bauen und die Automationstests ausführen.
2. Einen reproduzierbaren Editor-PIE-Test für Auswahl, Bewegung, Kampf, Reparatur, Produktion und Fraktionswechsel aufnehmen.
3. Runtime-Katalog in echte `UPrimaryDataAsset`-Instanzen unter `Content/TwoFronts` überführen.
4. Externe Unit-/Fabrikassets nach `ASSET_PIPELINE.md` integrieren und die Platzhalter per Data Asset austauschen.
5. Finale Map integrieren und dann `/Engine/Maps/Entry` als Default Map ablösen.
6. UMG-/CommonUI-Design, Sound Cues und explizite Fehler-/Statusrückmeldung ausbauen.
7. Besitzmodell, Lobby/Session und dedizierte Server-Tests vor einem LAN-Multiplayer-Meilenstein ergänzen.

## Nicht Bestandteil vor expliziter Beauftragung

Finale Karte, finale Modelle, finale Audio-/Sprachproduktion, Ressourcenwirtschaft, Forschung, dritte Fraktion, komplexe KI, Projektilsimulation, Panzerungsmodell und fertiger LAN-Multiplayer.
