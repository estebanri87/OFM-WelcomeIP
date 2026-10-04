# OFM-WelcomeIP

OpenKNX-Modul zur Anbindung der **Busch-Welcome-IP**-Türkommunikation an den KNX-Bus über die
**lokale API** des Smart Access Point (MQTT). Klingeln, Türöffnen, Türstatus und die Ausgänge
des IP-Schaltaktors. Video und Ton bleiben bewusst außen vor.

> **Status: Beta.** ETS-Applikation, Kanalmodell und Transport sind fertig und gegen einen
> beliebigen MQTT-Broker testbar. Offen ist das **ABB-Datenformat**: Topics und Nutzdaten sind
> noch Platzhalter, siehe [Offene Punkte](#offene-punkte).

## Wofür

| Anwendungsfall | Weg |
|---|---|
| Haustür per KNX-Taster öffnen; das Motorschloss hängt am potenzialfreien Ausgang des H8304-03 | KNX → `setDataPoint` am IP-Schaltaktor |
| Klingelsignal als Gruppenobjekt, z.B. für einen Gong über externe Logik | Außenstation `ch0002` „Incoming call" → KNX |

## Voraussetzungen

- Smart Access Point mit Firmware **6.36 oder neuer**
- Lokale API eingeschaltet: *Einstellungen → Verbindungen & APIs → Local API & SIP configuration*
- Ein API-Benutzer. Der Smart Access Point vergibt dafür einen **generischen** Benutzernamen,
  nicht den Kontonamen.
- Das Gerät muss den Smart Access Point auf der Heimnetz-Seite erreichen, nicht auf der
  10.x-Welcome-Seite.
- **ESP32.** Auf RP2040 ist das Modul nicht enthalten: `MQTT::Client` hält einen statischen
  Instanzzeiger für seine lwIP-Callbacks, ein zweiter Client würde den geräteeigenen stören,
  und TLS gibt es dort nicht.

## Funktionen

Ein Kanal entspricht einem Gerät. Kanäle werden nach OpenKNX-Standard über die **Kanalauswahl**
aktiviert (Typ-Variante mit „Deaktiviert"). Kanaltypen: Außenstation, IP-Schaltaktor H8304,
Innenstation und generischer Datenpunkt.

- **Türöffnen** mit wählbarem Öffnungsweg (potenzialfreier Ausgang, Türöffner-Kontakt oder
  Lichtkanal des IP-Aktors, oder Außenstation), mit Sperrobjekt und Rückmeldung
- **Klingelsignal** wahlweise als Impuls oder für die Dauer des Rufs, mit Sperrzeit gegen
  Mehrfachauslösung, zusätzlich ein Sammelobjekt über alle Kanäle
- **SmartAP-Funktionen** auf der Seite „Allgemein": Klingeln, Stummschaltung,
  Tag/Nacht-Umschaltung, Binärein- und -ausgang, Alarm und Sabotage
- **Experten-Parameter je Kanal** zum Überschreiben von Kanal- und Datenpunktnummer

## Aufbau

```
KNX ── WelcomeIPModule ── WelcomeIPChannel[]      Kanäle kennen nur WipAddress
             │
             ├─ WipMqttLink     Verbindung, 30-s-Heartbeat, Ereigniswarteschlange
             │       └─ MQTT::Module (zweite Instanz, OFM-Network)
             └─ WipProtocol     einzige Stelle, die das ABB-Format kennt
```

**`WipProtocol` ist die Nahtstelle.** Topics, Aufbau und Auswertung der Nutzdaten liegen dort
und nirgends sonst. Das Format an echter Hardware zu bestätigen ändert damit eine Datei statt
des ganzen Moduls.

**Eingehende Datenpunkte werden eingereiht, nicht sofort zugestellt.** Auf dem ESP32 läuft der
MQTT-Client in einer eigenen FreeRTOS-Task; ein Subscription-Callback läuft also nicht im
KNX-Loop. Ein Gruppenobjekt von dort zu schreiben würde mit dem Stack kollidieren.
`WipMqttLink` wertet deshalb im Callback aus und übergibt das Ergebnis über einen Ringpuffer
an `loop()` — dieselbe Regel, der auch der Webserver von OFM-Network folgt.

**Nur aktive Kanäle werden angelegt.** `createChannel()` liefert `nullptr` für ein Gerät, das
in der Kanalauswahl deaktiviert oder auf seinem Kanal-Tab *suspendiert* ist. Ein geparktes
Gerät kostet damit weder RAM noch MQTT-Verkehr, behält aber seine vollständige Konfiguration.
Suspendierte Kanäle tragen im ETS-Baum ein ⛔.

**Ein zweiter MQTT-Client, nicht der geräteeigene.** Der Smart Access Point ist ein anderer
Broker mit eigenen Zugangsdaten und eigenem Topic-Schema, deshalb richtet
`MQTT::Module::configure()` eine getrennte Instanz darauf. Status-Veröffentlichungen und Last
Will sind aus: ein fremder Broker lehnt `<prefix>status` per ACL ab.

**Der Empfangspuffer ist 64 KB groß.** Eine `getAll`-Antwort umfasst 17 bis 50 KB JSON. Die
voreingestellten 1 KB reichen nicht, und da der Broker erneut sendet, führt ein zu kleiner
Puffer nicht zu einer verlorenen Nachricht, sondern zu einer Endlosschleife aus Neuverbindungen.
Ausgewertet wird mit einem ArduinoJson-Filter, der nur Seriennummer, Gerätetyp, Kanalindex und
Ausgangswerte behält.

## Konsole

```
wip                                Status, SmartAP-Seriennummer, Kanalzahl
wip devices                        Gerätemodell anfordern (getAll) und Geräte auflisten
wip set <sn> <ch> <dp> <val>       Eingangs-Datenpunkt schreiben
wip raw on|off                     jede empfangene Nutzlast protokollieren
```

Die Verbindungsdaten kommen aus den ETS-Parametern.

## Offene Punkte

Im Quelltext mit `TODO(Stufe2)` (Datenformat) und `TODO(Stufe3)` (ETS) markiert.

1. **Topics und Nutzdaten sind Platzhalter.** Die ABB-Seite `wip_local/definition` nennt
   GetAll, SetDataPoint und Notification, liefert aber **leere Codeblöcke**. Die einzigen
   konkreten veröffentlichten Daten sind die Referenztabellen für DeviceTypeId und ChannelID.
   Die echten Zeichenketten bekommt man, indem man am Gerät `#` abonniert und den
   WebSocket-Verkehr der Weboberfläche mitliest — der `getAll`-Umschlag (`method`, `queryid`,
   `jid`, `sessionjwt`) legt nahe, dass die Oberfläche dieselbe Sprache spricht.
2. **Transport unbestätigt.** Port 8883 ist dokumentiert (SmartAP-Produkthandbuch, „Ports and
   services"), nicht aber, ob darauf einfaches MQTT oder MQTT über WebSocket läuft. WebSocket
   bräuchte zusätzliches Framing in `MQTT::Client` und wird deshalb zuerst geprüft.
3. **Klingel-Datenpunkt ungeprüft.** Die Referenztabelle führt Außenstation `ch0002`
   „Incoming call" und SmartAP `ch0001` „Doorbell ring"; MQTT ist damit die wahrscheinliche
   Quelle. Löst das nicht aus, bleibt als Rückfallebene die Anmeldung als dritte SIP-Station
   am Smart Access Point.
4. **Vertrauenswürdige Geräte.** Der H8304 öffnet sein Schloss nur für signierte,
   vertrauenswürdige Geräte. Ob ein Befehl über die lokale API als vertrauenswürdig gilt, ist
   ungetestet.

## Dokumentation

- [CHANGELOG](CHANGELOG.md)
