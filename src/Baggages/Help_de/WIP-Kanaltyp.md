# Geraetetyp

Jeder Kanal steht fuer ein Geraet der Welcome-Anlage. Der Typ bestimmt, welche Parameter
und Gruppenobjekte erscheinen.

* **Aussenstation** - Klingeln, Tueroeffner, Tuerstatus
* **IP-Schaltaktor H8304** - Tueroeffner, Licht, Tuerstatus
* **Innenstation** - Ereignis
* **Generischer Datenpunkt** - beliebiger Kanal und Datenpunkt, fuer alles, was die
  obigen Typen nicht abdecken

## Seriennummer

Die 15-stellige Seriennummer des Geraets, zum Beispiel 101807A7F04AAA0. Sie steht im
Smart Access Point auf der Seite des jeweiligen Geraets. Ueber den Konsolenbefehl
wip devices lassen sich alle gefundenen Geraete mit Seriennummer auflisten.

## Tuerstatus

Nur sinnvoll, wenn am IP-Schaltaktor ein Tuerkontakt angeschlossen und im Smart Access
Point die Funktion *Statuserkennung Tuer* aktiviert ist.
