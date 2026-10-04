# GitHub–Trello-Synchronisierung einrichten

Der Workflow aktualisiert ausschließlich den klar markierten Bereich **„Automatisch synchronisierter GitHub-Status“** in der bestehenden Karte **„Aktueller Projektstand“**. Die restliche Beschreibung sowie Checklisten, Kommentare, Labels und Zuständigkeiten der Karte bleiben unverändert. Bei einer falsch konfigurierten Karte bricht der Workflow vor dem Schreiben ab.

## Einmalig einrichten

1. Melde dich bei Trello an und öffne [Trello Apps](https://trello.com/apps/admin). Lege dort bei Bedarf ein kostenloses Power-Up an, öffne dessen Bereich **Trello Auth/API Key** und wähle **Generate a new API Key**. Kopiere den Schlüssel.
2. Klicke auf derselben Seite neben dem Schlüssel auf **Token**, erteile der App **read**- und **write**-Berechtigung und wähle die dauerhafte Laufzeit (*never*), sofern Trello diese anbietet. Kopiere das erzeugte Token sofort und behandle es wie ein Passwort.
3. Öffne das GitHub-Repository **Settings → Secrets and variables → Actions → Secrets** und lege diese beiden Repository-Secrets an:
   - `TRELLO_API_KEY`: der API-Schlüssel
   - `TRELLO_TOKEN`: das Token
4. Ermittle die ID der vorhandenen Karte. Öffne **Aktueller Projektstand**, kopiere ihren Link und übernimm den Teil direkt nach `https://trello.com/c/` (der Shortlink ist für die Karten-API zulässig). Alternativ liefert der folgende lokale PowerShell-Befehl die vollständige ID und den Kartennamen; Werte nicht in die Shell-Historie oder ein Repository kopieren:

   ```powershell
   $apiKey = Read-Host 'Trello API-Schlüssel'
   $token = Read-Host 'Trello Token'
   $cardUrl = Read-Host 'Link der Karte Aktueller Projektstand'
   $shortLink = ([uri]$cardUrl).Segments[2].TrimEnd('/')
   $card = Invoke-RestMethod -Uri "https://api.trello.com/1/cards/$shortLink?fields=id,name&key=$apiKey&token=$token"
   $card | Select-Object id, name
   ```

   Trage die ausgegebene `id` im Repository unter **Settings → Secrets and variables → Actions → Variables** als `TRELLO_CARD_ID` ein. Der Workflow akzeptiert auch den Shortlink, prüft jedoch in jedem Fall den Kartennamen vor dem Update.
5. Optional: Pflege unter demselben Bereich eine Variable `TRELLO_NEXT_STEP`, wenn ein nächster Entwicklungsschritt angezeigt werden soll. Ohne diese Variable erscheint kein entsprechender Hinweis.
6. Öffne **Actions → Trello-Status synchronisieren → Run workflow**, wähle `main` und starte den Lauf. Der Workflow führt zuerst die Offline-Tests mit Testdaten aus. Erst danach liest er die konfigurierte Karte und aktualisiert nur ihren markierten Statusbereich.

Nach jedem Push auf `main` läuft die Synchronisierung ebenfalls. Der Build-Status wird nur angezeigt, wenn für den Commit bereits abgeschlossene GitHub-Checks existieren; laufende oder fehlende Checks werden ausdrücklich als solche angezeigt.

## Sicherheit und Fehlerbehebung

- Schlüssel und Token stehen ausschließlich in GitHub Secrets, nicht im Repository oder in Workflow-Ausgaben.
- Fehler bei fehlenden Konfigurationswerten, API-Zugriff oder falschem Kartennamen brechen mit einer verständlichen Meldung ab, ohne die Karte zu verändern.
- Falls die Statusmarkierungen in Trello manuell beschädigt oder doppelt angelegt wurden, stoppt der Workflow zum Schutz der manuellen Beschreibung. Stelle dann genau ein Start- und End-Markierungspaar wieder her oder entferne beide vollständig; beim nächsten Lauf wird ein neuer Bereich angelegt.

Die verwendete Karten-Aktualisierung (`PUT /1/cards/{id}`) und die tokenbasierte Autorisierung entsprechen der offiziellen [Trello-API-Einführung](https://developer.atlassian.com/cloud/trello/guides/rest-api/api-introduction/) und der [Cards-API-Referenz](https://developer.atlassian.com/cloud/trello/rest/api-group-cards/).
