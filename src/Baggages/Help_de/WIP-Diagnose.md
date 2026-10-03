# Diagnose

## Gruppenobjekte

Jedes der vier Objekte laesst sich unter *Objekte* einzeln abwaehlen, wenn es nicht gebraucht wird.

* **Verbindung** - 1, solange die lokale API erreichbar ist. Bei 0 gehen Klingelereignisse
  verloren; das Objekt eignet sich fuer eine Stoermeldung.
* **Diagnose** - Klartextmeldung zur letzten Stoerung.
* **Klingeln (Sammel)** - sendet, wenn eine beliebige Aussenstation klingelt.
* **Sperre (alle)** - sperrt die Tueroeffnung aller Kanaele gemeinsam.

## Konsolenbefehle

| Befehl | Wirkung |
|---|---|
| wip | Status, Seriennummer des Smart Access Point, Kanalzahl |
| wip devices | Geraeteliste anfordern und ausgeben |
| wip set <sn> <ch> <dp> <wert> | Einen Eingangsdatenpunkt schreiben |
| wip raw on | Alle empfangenen Meldungen protokollieren |
