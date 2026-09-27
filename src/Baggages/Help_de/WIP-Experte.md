# Experteneinstellungen

Kanal- und Datenpunktnummern lassen sich hier ueberschreiben.

Die Vorgabewerte stammen aus der Referenztabelle des ABB-Entwicklerportals:

| Geraet | Kanal 0 | Kanal 1 | Kanal 2 |
|---|---|---|---|
| Aussenstation | Trigger | Schloss | Eingehender Ruf |
| IP-Schaltaktor | Trigger | Licht | Schloss |
| Innenstation | Trigger | - | - |

Diese Tabelle ist **nicht gegen Firmware-Staende dokumentiert**. Weicht die Anlage davon
ab, laesst sie sich hier anpassen, ohne dass die Firmware geaendert werden muss.

Zum Ermitteln der richtigen Werte eignet sich der Konsolenbefehl wip devices: er listet
alle Geraete mit Seriennummer, Typ und Kanalzahl. Mit wip raw on werden zusaetzlich alle
empfangenen Meldungen protokolliert.
