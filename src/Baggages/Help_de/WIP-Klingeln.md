# Klingeln

Das Objekt sendet, sobald die Anlage einen Ruf von dieser Aussenstation meldet. Es eignet
sich, um ueber eine externe Logik einen Gong auszuloesen.

## Signalform

* **Impuls** - das Objekt geht auf 1 und nach der eingestellten Dauer wieder auf 0.
* **Solange der Ruf laeuft** - das Objekt bleibt 1, bis der Ruf endet.

## Sperrzeit

Verhindert, dass mehrfaches Druecken der Klingeltaste den Gong mehrfach startet. Innerhalb
der Sperrzeit wird nur das erste Ereignis weitergegeben. Der Wert 0 gibt jedes Ereignis
weiter.

## Grenze

Ob die Anlage einen zweiten Tastendruck waehrend eines laufenden Rufs ueberhaupt meldet,
haengt vom Smart Access Point ab. Meldet sie ihn nicht, gibt es pro Ruf nur ein Signal -
unabhaengig von der hier eingestellten Sperrzeit.
