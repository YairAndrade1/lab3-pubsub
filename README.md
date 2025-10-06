# Sistema TCP Publish/Subscriber

Implementación  de un modelo PubSub usando sockets TCP en C.

## Temas disponibles

El sistema trabaja únicamente con tres topics:
1. goles
2. tarjetas
3. cambios

## Archivos principales
- broker_tcp.c
- publisher_tcp.c
- subscriber_tcp.c

## Compilación

Se corre el siguiente comando en una terminal para compilar los 3 archivos correspondientes a la implementación en TCP. 
```bash
gcc -o broker_tcp broker_tcp.c && gcc -o publisher_tcp publisher_tcp.c && gcc -o subscriber_tcp subscriber_tcp.c
```

## Ejecución

Para ejecutarlo es necesario tener una terminal por instancia (Pub,Sub,Broker)
Supongamos que queremos 1 broker, 1 subscriber y 1 publisher. Es importante ejecutar en el siguiente orden, de lo contrario tendremos errores.
Terminal 1 (Broker):
```bash
./broker_tcp
```

Terminal 2 (Subscriber):
```bash
./subscriber_tcp
```
Selecciona una opción:
1 = goles
2 = tarjetas
3 = cambios

Terminal 3 (Publisher):
```bash
./publisher_tcp
```
En cada ciclo:
1 = goles
2 = tarjetas
3 = cambios
4 = salir
Luego de elegir (1-3) escribe el mensaje y presiona Enter.

## Formato interno de mensajes

El publisher envía al broker:
```
MSG:<topic>:<contenido>
```
El subscriber se registra enviando:
```
SUBSCRIBE:<topic>
```
El publisher anuncia su rol al conectar:
```
PUBLISH:
```

## Limpieza de procesos (opcional pero puede ser util cuando se hagan muchas pruebas)
```bash
pkill -f broker_tcp
pkill -f publisher_tcp
pkill -f subscriber_tcp
```

END.