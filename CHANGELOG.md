# Changelog OFM-WelcomeIP

## 0.1.0 - 2026-09-28

Erste Fassung. Anbindung der Busch-Welcome-IP-Türkommunikation an KNX über die
lokale API des Smart Access Point (MQTT). Klingeln, Türöffnen, Türstatus und die
Ausgänge des IP-Schaltaktors. Video und Ton bleiben bewusst außen vor.

### Added
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
  Client stören würde. TLS gibt es dort ebenfalls nicht.
- Setzt SmartAP-Firmware 6.36 oder neuer voraus, mit aktivierter lokaler API und einem
  API-Benutzer.
