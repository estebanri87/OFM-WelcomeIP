# Changelog OFM-WelcomeIP

## 0.2.0 - 2026-09-28

### Hinzugefügt
- Kanal-Parameter **Suspendiert** (Nein/Ja) im Kanaldefinitions-Block jedes Geräts. Das Gerät
  bleibt vollständig parametriert, wird aber nicht ausgeführt — hilfreich bei der Fehlersuche.
  Suspendierte Kanäle tragen im ETS-Baum ein **⛔** vor der Beschreibung.
- Die Objekte Verbindung, Diagnose, Klingeln (Sammel) und Sperre (alle) sind unter
  *Allgemein → Objekte* einzeln zuschaltbar.
- Die Funktionen des Smart Access Point sind ebenfalls einzeln zuschaltbar (Klingeln,
  Stummschaltung, Tag/Nacht, Binärein-/-ausgang, Alarm, Sabotage) statt nur gemeinsam
  über einen Schalter. Vorbelegt ist nur Klingeln.
- Echte Parameter-Reserve: Der modulweite Parameterblock ist jetzt fest 144 Byte groß
  (Platzhalter im letzten Byte, Muster aus OGM-Common). Die bisher nur per Kommentar
  „freigehaltenen“ Bytes waren keine Reserve, der Producer packt dicht. Künftige globale
  Parameter verschieben die nachfolgenden Module damit nicht mehr.
  **Der Block wächst dadurch einmalig; die Applikation in der ETS aktualisieren und das
  Gerät neu programmieren.**

### Fixed
- In der Kanalauswahl deaktivierte Geräte wurden trotzdem angelegt: `createChannel()` hat
  `Kanalaktivität` bisher nicht ausgewertet.

### Änderungen
- Kein eigener Parameter „Modul aktiv“ mehr: Das Modul ist immer aktiv und wird bei Bedarf in der
  Modulliste von OpenKNX abgeschaltet; die Firmware wertet dieses Häkchen aus. Bit 0 von Byte 0
  bleibt frei, das Speicherlayout ändert sich nicht.
- KO-Namen und Objektfunktionen vereinheitlicht: „Welcome [Kanal]: Eingang/Ausgang, Wert“. Kanalobjekte zeigen die
  Kanalbeschreibung im Namen.

## 0.1.0 - 2026-09-28

Erste Fassung. Anbindung der Busch-Welcome-IP-Türkommunikation an KNX über die
lokale API des Smart Access Point (MQTT). Klingeln, Türöffnen, Türstatus und die
Ausgänge des IP-Schaltaktors. Video und Ton bleiben bewusst außen vor.

### Hinzugefügt
- **ETS-Applikation** mit den Seiten „Allgemein“ und „Kanalauswahl“ sowie 8 Gerätekanälen.
  Kanaltypen: Außenstation, IP-Schaltaktor H8304, Innenstation, generischer Datenpunkt.
- **Kanalauswahl nach OpenKNX-Standard** (eigener Tab mit Übersichtstabelle). Deaktivierte
  Kanäle erscheinen nicht im ETS-Baum.
- **Türöffnen** mit wählbarem Öffnungsweg (IP-Aktor potenzialfreier Ausgang, IP-Aktor
  Türöffner-Kontakt, IP-Aktor Lichtkanal, Außenstation), Sperrobjekt und Rückmeldung.
- **Klingelsignal** als Gruppenobjekt, wahlweise als Impuls oder für die Dauer des Rufs,
  mit Sperrzeit gegen Mehrfachauslösung. Zusätzlich ein Sammelobjekt über alle Kanäle.
- **SmartAP-Funktionen** auf der Seite „Allgemein“: Klingeln, Stummschaltung,
  Tag/Nacht-Umschaltung, Binärein- und -ausgang, Alarm und Sabotage.
- **Experten-Parameter je Kanal** zum Überschreiben von Kanal- und Datenpunktnummer.
  Die Vorgabewerte stammen aus der Referenztabelle des ABB-Entwicklerportals, die nicht
  gegen Firmware-Versionen dokumentiert ist.
- Konsolenbefehle `wip`, `wip connect`, `wip devices`, `wip set`, `wip raw` zur Diagnose.
- MQTT über TLS (nur ESP32), wahlweise ohne Zertifikatsprüfung oder gegen ein Zertifikat.

### Bekannte Einschränkungen
- **Die MQTT-Topics und die Nutzdaten sind noch nicht bestätigt.** Die Seite
  `wip_local/definition` des ABB-Entwicklerportals benennt GetAll, SetDataPoint und
  Notification, liefert die Beispielblöcke aber leer aus. Alle betroffenen Stellen sind in
  `WipProtocol.cpp` mit `TODO(Stufe2)` markiert und werden am installierten Gerät ermittelt.
- **Nur ESP32.** Der MQTT-Client von OFM-Network hält auf RP2040 einen statischen
  Instanzzeiger für seine lwIP-Callbacks, sodass eine zweite Instanz den geräteeigenen
  Client stören würde. TLS gibt es dort ebenfalls nicht. Das Modul kapselt sich deshalb
  über `OPENKNX_WELCOMEIP` (siehe `WelcomeIPConfig.h`) selbst aus; RP2040-Ziele übersetzen
  weiterhin, nur ohne dieses Modul.
- Setzt SmartAP-Firmware 6.36 oder neuer voraus, mit aktivierter lokaler API und einem
  API-Benutzer.
