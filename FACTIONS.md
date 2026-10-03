# Fraktionen und V0.1-Werte

Die stabilen IDs sind `Humans` und `Synth`. Weitere `ETFactionId`-/Data-Asset-Einträge können später ergänzt werden; Fabrik, Controller und Unit-Code erzwingen keine Zweifraktionenannahme.

| Rolle | Humans | Werte (HP / Tempo / Schaden / Bauzeit) | Synth | Werte (HP / Tempo / Schaden / Bauzeit) |
|---|---|---:|---|---:|
| Aufklärung | Scout | 90 / 780 / 8 / 4s | Probe | 75 / 850 / 6 / 3.5s |
| Standardkampf | Rifle Unit | 150 / 520 / 16 / 6s | Combat Drone | 130 / 580 / 14 / 5.5s |
| Schwer | Battle Tank | 420 / 310 / 42 / 11s | Walker | 360 / 360 / 35 / 10s |
| Unterstützung | Repair Rig | 190 / 420 / – / 8s | Reconstructor | 160 / 450 / – / 7s |

Repair Rig repariert 28 HP/s; Reconstructor 35 HP/s. Scout/Probe sehen 1800 Unreal Units weit, alle anderen 1200. Die Kampfreichweiten und Cooldowns liegen im C++-Katalog und sind absichtlich unterschiedlich, damit die Unit-Paare keine reinen Umbenennungen sind.

Humans verwenden einfache Kugel/Würfel/Zylinder/Kegel-Platzhalter mit Industriecharakter; Synth erhalten abweichende Formen. Die unterschiedlichen Fabrikformen (Würfel vs. Zylinder) und HUD-Akzentfarben schaffen im aktuellen Asset-freien Zustand klare Identität.
