# Welcome IP

Bindet eine Busch-Welcome-IP-Tuerkommunikation ueber die **lokale API** des Smart Access
Point an KNX an: Klingeln, Tueroeffnen, Tuerstatus und die Ausgaenge des IP-Schaltaktors.
Video und Ton werden bewusst nicht uebertragen.

Das Modul ist standardmaessig aktiv. Wird es nicht gebraucht, laesst es sich unter
*OpenKNX -> Module* abschalten; dann blendet die ETS seine Seiten aus und die Firmware baut
keine Verbindung auf.

## Voraussetzungen

* Smart Access Point ab Version **6.36**
* Lokale API aktiviert unter *Einstellungen -> Verbindungen & APIs -> Lokale API & SIP-Konfiguration*
* Ein API-Benutzer ist angelegt. Der Smart Access Point vergibt dafuer einen **generischen**
  Benutzernamen, der nicht dem Kontonamen entspricht; er ist im Geraet einsehbar.
* Das KNX-Geraet muss den Smart Access Point auf der Heimnetzseite erreichen, nicht ueber
  das Welcome-interne Netz (10.x).

## Smart Access Point

Die eigenen Funktionen des Smart Access Point lassen sich zusaetzlich auf den Bus legen.
Jede Funktion hat einen eigenen Haken; nur angehakte Funktionen erscheinen als
Kommunikationsobjekt:

* **Klingeln** - Ruf am Smart Access Point (vorbelegt)
* **Stummschaltung** - Schalten und Statusrueckmeldung, beide Objekte zusammen
* **Tag/Nacht-Umschaltung**
* **Binaereingang** und **Binaerausgang**
* **Alarm** und **Sabotage**

Die Seriennummer des Geraets wird automatisch aus der Geraeteliste uebernommen und muss
nicht eingetragen werden.
