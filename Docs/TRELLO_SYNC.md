# GitHub–Trello-Entwicklungsberichte per E-Mail einrichten

Nach jedem Push auf `main` sendet GitHub Actions einen Entwicklungsbericht an die bestehende Trello-Kartenadresse von **„Aktueller Projektstand“**. Trello fügt jede eingehende Nachricht dort als neuen Kommentar hinzu. Der Workflow verwendet keine Trello-REST-API und verändert weder Kartenbeschreibung noch Checklisten, Labels, Mitglieder oder vorhandene Kommentare.

## Einmalig einrichten

1. Öffne in Trello die vorhandene Karte **„Aktueller Projektstand“** und kopiere ihre persönliche Karten-E-Mail-Adresse. Der E-Mail-Empfang wurde bereits erfolgreich getestet.
2. Öffne im GitHub-Repository **Settings → Secrets and variables → Actions → Secrets** und hinterlege diese drei Repository-Secrets:
   - `SMTP_USER`: vollständige Absender-E-Mail-Adresse des SMTP-Kontos
   - `SMTP_PASSWORD`: SMTP-Passwort; bei Gmail ein App-Passwort, nicht das normale Google-Passwort
   - `TRELLO_CARD_EMAIL`: kopierte E-Mail-Adresse der vorhandenen Trello-Karte
3. Für Gmail ist keine weitere Konfiguration nötig: Der Workflow nutzt `smtp.gmail.com`, Port `465` und SSL. Für einen anderen Anbieter lege unter **Settings → Secrets and variables → Actions → Variables** bei Bedarf diese nicht-sensitiven Repository-Variablen an:
   - `SMTP_HOST`, zum Beispiel `mail.example.de`
   - `SMTP_PORT`, zum Beispiel `587`
   - `SMTP_USE_SSL`: `true` für direktes SSL (Port 465) oder `false` für STARTTLS (typisch Port 587)
4. Starte den Erstlauf über **Actions → Trello-Status synchronisieren → Run workflow**, wähle `main` und bestätige den Lauf. Zuerst laufen die Offline-Tests; danach wird genau eine E-Mail an die konfigurierte Kartenadresse gesendet.

Jeder Bericht enthält den aktuellen Commit, verantwortlichen Entwickler, Datum/Uhrzeit, die bis zu fünf letzten Commits, die im neuesten Commit geänderten Dateien und GitHub-Links zu jedem aufgeführten Commit.

## Sicherheit und Fehlerbehebung

- Die drei Zugangswerte stehen ausschließlich in GitHub Secrets. Das Skript und der Workflow geben weder Passwörter noch Kartenadresse aus.
- Fehlende oder ungültige Konfiguration sowie SMTP-Fehler führen zu einer verständlichen Fehlermeldung und einem fehlgeschlagenen Workflow-Lauf. Die konkrete SMTP-Fehlerantwort wird nicht geloggt.
- Für Gmail muss das SMTP-Konto SMTP-Zugriff erlauben; bei aktivierter Zwei-Faktor-Anmeldung wird ein [Google App-Passwort](https://support.google.com/accounts/answer/185833) benötigt.
- Ein Push erzeugt absichtlich einen neuen Trello-Kommentar. Das ist die gewünschte, nicht-destruktive Historie; bestehende Trello-Inhalte werden nicht gelesen, geändert oder gelöscht.
