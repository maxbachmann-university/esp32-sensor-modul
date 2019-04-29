# Sensor Modul
Mithilfe des Sensor Moduls können die aktuelle Windstärke und Helligkeit über einen ADC ausgelesen werden.
Der Assembler Code des ADC wurde nicht von uns geschrieben.
Diese Daten werden dann als json formatiert über MQTT zum Regler übertragen.
Für eine effiziente Energienutzung  im Stromverbrauch wird der Ultra Low Power Coprocessor verwendet.
