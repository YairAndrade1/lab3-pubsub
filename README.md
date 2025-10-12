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
## Pruebas con Wireshark
1. Instalación de WSL en PowerShell
```
wsl --install
```
2. Entrar a WSL y ejecutar comandos para instalar gcc
```
wsl
sudo apt update
sudo apt install build-essential
gcc --version
```
3. Clonar repositorio
4. Instalar librería para capturar desde WSL
```
sudo apt update
sudo apt install tcpdump -y
```
5. Correr el comando para captura de la información y guardar la información
```
sudo tcpdump -i any port 8080 -w tcp_pubsub.pcap
cp tcp_pubsub.pcap /mnt/c/Users/<USUARIO>/Desktop/
```
Se incluyen los archivos de pruebas obtenidos para la prueba de TCP y UDP en el repositorio.

END.
