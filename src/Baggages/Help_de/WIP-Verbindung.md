# Verbindung

Die lokale API arbeitet mit MQTT. Der Smart Access Point bietet den Dienst laut
Produkthandbuch auf **Port 8883** an.

## TLS

TLS wird empfohlen und laesst sich im Smart Access Point ein- und ausschalten. Ist es
eingeschaltet, erzeugt das Geraet ein Zertifikat, das dort heruntergeladen werden kann.

* **Keine Pruefung** - die Verbindung ist verschluesselt, die Gegenstelle wird aber nicht
  geprueft. Fuer die Inbetriebnahme geeignet.
* **Zertifikat aus Datei** - das heruntergeladene Zertifikat wird als /wip_ca.pem auf das
  Geraet geladen und geprueft.

TLS steht nur auf ESP32-Geraeten zur Verfuegung.

## Zugangsdaten

Benutzername und Passwort des API-Benutzers. Sie werden im ETS-Projekt im Klartext
gespeichert.

## Verbindungsaufbau

Bricht die Verbindung ab, wird sie im eingestellten Abstand erneut aufgebaut. Waehrend
einer Unterbrechung gehen Klingelereignisse verloren; das Objekt *Verbindung* eignet sich
daher fuer eine Stoermeldung.
