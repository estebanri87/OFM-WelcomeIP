# Tueroeffner

## Oeffnungsweg

Der IP-Schaltaktor H8304 hat **zwei getrennte Schloss-Ausgaenge**:

* **Potenzialfreier Ausgang** (Klemmen NC/COM/NO) - hier haengt ueblicherweise ein
  Motorschloss. Im Smart Access Point muss er als *Entsperren* konfiguriert sein, nicht
  als *Licht*.
* **Tueroeffner-Kontakt** (Klemmen LOCK+/LOCK-) - fuer 12-V-Tueroeffner.

Welcher Kanal der lokalen API welchen Ausgang schaltet, ist von ABB nicht dokumentiert.
Der Vorgabewert ist der potenzialfreie Ausgang. Zieht das Relais nicht an, sind die
anderen Wege der Reihe nach zu probieren.

Ist der Schaltaktor im Smart Access Point einer Aussenstation als Schloss zugeordnet,
funktioniert der Weg **Ueber die Aussenstation**; dann ist deren Seriennummer einzutragen.

## Schaltdauer

Wie lange der Ausgang anzieht, wird **im Smart Access Point** eingestellt
(*Tuerkommunikation -> IP-Schaltaktor -> Tueroeffner/Licht*), nicht hier. Der Wert muss zum
Schloss passen.

## Rueckmeldung

Das Objekt *Tueroeffner aktiv* meldet jede Entsperrung. Dafuer muss im Smart Access Point
die Funktion *Tueroeffnung melden* aktiviert sein.

## Sperre

Das Sperrobjekt verhindert das Oeffnen, solange es gesetzt ist. Ein Oeffnungsversuch wird
dann abgewiesen und das Objekt *Fehler* gesetzt.

## Hinweis zur Freigabe

Der IP-Schaltaktor gibt seine Sperre nur fuer Geraete frei, die im Smart Access Point als
vertrauenswuerdig eingetragen sind.
