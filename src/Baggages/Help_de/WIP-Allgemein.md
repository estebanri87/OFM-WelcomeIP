# Welcome IP

Bindet eine Busch-Welcome-IP-Tuerkommunikation ueber die **lokale API** des Smart Access
Point an KNX an: Klingeln, Tueroeffnen, Tuerstatus und die Ausgaenge des IP-Schaltaktors.
Video und Ton werden bewusst nicht uebertragen.

## Voraussetzungen

* Smart Access Point ab Version **6.36**
* Lokale API aktiviert unter *Einstellungen -> Verbindungen & APIs -> Lokale API & SIP-Konfiguration*
* Ein API-Benutzer ist angelegt. Der Smart Access Point vergibt dafuer einen **generischen**
  Benutzernamen, der nicht dem Kontonamen entspricht; er ist im Geraet einsehbar.
* Das KNX-Geraet muss den Smart Access Point auf der Heimnetzseite erreichen, nicht ueber
  das Welcome-interne Netz (10.x).

## Smart Access Point

Die eigenen Funktionen des Smart Access Point lassen sich zusaetzlich auf den Bus legen:
Klingeln, Stummschaltung, Tag/Nacht-Umschaltung, Binaerein- und -ausgang sowie Alarm- und
Sabotagemeldung. Die Seriennummer des Geraets wird automatisch aus der Geraeteliste
uebernommen und muss nicht eingetragen werden.
