# Sistema Publish/Subscribe - Implementaciones TCP, UDP e Híbrida

Sistema completo de comunicación Publish/Subscribe implementado en C con tres enfoques diferentes de protocolos de capa de transporte: TCP confiable, UDP sin conexión e híbrido UDP con confirmaciones.

## Arquitectura del Sistema

El sistema implementa el patrón **Publish/Subscribe** donde:

- **Broker**: Intermediario central que recibe mensajes de publishers y los distribuye a subscribers interesados
- **Publisher**: Cliente que publica mensajes en topics específicos
- **Subscriber**: Cliente que se suscribe a topics para recibir mensajes

### Topics Soportados

El sistema maneja tres topics específicos de eventos deportivos:
1. **goles**: Eventos de anotaciones
2. **tarjetas**: Eventos de tarjetas amarillas/rojas
3. **cambios**: Eventos de sustituciones de jugadores

## Implementación TCP

### Librerías Utilizadas
- **`<sys/socket.h>`**: Funciones principales de sockets (socket(), bind(), listen(), accept())
- **`<netinet/in.h>`**: Estructuras de direcciones de red (sockaddr_in, AF_INET)
- **`<arpa/inet.h>`**: Conversiones de direcciones (inet_ntoa(), htons())
- **`<sys/select.h>`**: Multiplexado de E/S para manejar múltiples conexiones
- **`<errno.h>`**: Códigos de error del sistema

### Funcionamiento del Broker TCP

El broker TCP utiliza **multiplexado de E/S** con la función `select()` para manejar múltiples conexiones simultáneamente en un solo thread. Mantiene un array de estructuras `client_t` (líneas 17-22 en `broker_tcp.c`) que almacena información de cada cliente conectado incluyendo su socket, tipo (publisher/subscriber) y topic de suscripción.

**Proceso de conexión**:
1. El broker escucha en puerto 8080 usando `listen()`
2. `select()` monitorea todos los sockets activos
3. Nuevas conexiones se aceptan con `accept()` generando un socket único
4. Los clientes se registran enviando mensajes `SUBSCRIBE:topic` o `PUBLISH:`
5. La función `forward_message()` distribuye mensajes solo a subscribers del topic correspondiente

**Características clave**:
- **Estado persistente**: Cada conexión TCP mantiene estado hasta desconexión
- **Detección automática**: Desconexiones detectadas cuando `recv()` retorna 0
- **Escalabilidad limitada**: Número máximo de conexiones definido por `MAX_CLIENTS`

### Clientes TCP

**Publisher**: Se conecta al broker, se registra con `PUBLISH:` y envía mensajes con formato `MSG:topic:contenido`. Utiliza menú interactivo para seleccionar topics.

**Subscriber**: Se conecta, selecciona un topic y envía `SUBSCRIBE:topic`. Permanece en bucle `recv()` esperando mensajes del broker. Muestra timestamp con cada mensaje recibido.

## Implementación UDP

### Librerías Utilizadas
- **`<stdio.h>`**: Funciones de entrada/salida estándar
- **`<stdlib.h>`**: Funciones de utilidad general (exit(), malloc())
- **`<string.h>`**: Manipulación de cadenas (strcmp(), strncpy(), memset())
- **`<unistd.h>`**: Funciones del sistema POSIX (close())
- **`<arpa/inet.h>`**: Conversiones de red y estructuras de direcciones

### Funcionamiento del Broker UDP

El broker UDP opera sin estado de conexión persistente. Utiliza un solo socket UDP que escucha en puerto 8080 y mantiene un array de estructuras `subscriber_t` (líneas 12-16 en `broker_udp.c`) que almacena la dirección IP:puerto de cada subscriber junto con su topic de interés.

**Proceso de comunicación**:
1. El broker crea socket UDP con `SOCK_DGRAM`
2. Se vincula al puerto 8080 usando `bind()`
3. Permanece en bucle `recvfrom()` esperando mensajes
4. Los subscribers se registran enviando `SUBSCRIBE:topic`
5. La función `add_subscriber()` almacena la dirección del cliente
6. `forward_message()` usa `sendto()` para distribuir mensajes a direcciones específicas

**Características clave**:
- **Sin conexión**: No hay handshake ni estado de conexión
- **Identificación por dirección**: Clientes identificados por IP:puerto
- **Registro dinámico**: Subscribers se registran al enviar primer mensaje
- **No detección de desconexión**: No hay mecanismo automático para detectar clientes offline

### Clientes UDP

**Publisher**: Envía datagramas UDP directamente al broker con formato `MSG:topic:contenido`. No requiere registro previo, cada mensaje es independiente.

**Subscriber**: Envía `SUBSCRIBE:topic` al broker y permanece en bucle `recvfrom()`. El broker almacena su dirección y le enviará mensajes futuros del topic suscrito.

## Implementación Híbrida

### Librerías Utilizadas
- **`<stdio.h>`**: Funciones de entrada/salida estándar
- **`<stdlib.h>`**: Funciones de utilidad general y manejo de memoria
- **`<string.h>`**: Manipulación de cadenas y comparaciones
- **`<unistd.h>`**: Funciones del sistema POSIX
- **`<arpa/inet.h>`**: Conversiones de red y direcciones
- **`<sys/time.h>`**: Estructuras de tiempo para timeouts
- **`<signal.h>`**: Manejo de señales del sistema

### Funcionamiento del Broker Híbrido

El broker híbrido utiliza UDP como protocolo de transporte base pero implementa un sistema de confirmaciones (ACKs) a nivel de aplicación para garantizar confiabilidad selectiva. Mantiene estructuras `subscriber_t` con campo adicional `last_ack_id` para rastrear confirmaciones.

**Proceso de comunicación**:
1. Utiliza socket UDP (`SOCK_DGRAM`) como base
2. Cada mensaje saliente incluye ID único generado por contador estático
3. La función `forward_message()` (líneas 65-82) envía mensajes con formato `MSGID:N:MSG:topic:data`
4. `process_ack()` maneja confirmaciones de recepción de subscribers
5. Subscribers confirman recepción enviando `ACK:MSGID:N`

**Características clave**:
- **Confirmación bidireccional**: Broker confirma recepción de operaciones, subscribers confirman recepción de mensajes
- **IDs únicos**: Cada mensaje tiene identificador secuencial para rastreo
- **Confiabilidad selectiva**: Solo operaciones críticas requieren ACK

### Clientes Híbridos

**Publisher**: Envía mensajes y espera ACK del broker con timeout configurable. Implementa reintentos automáticos (hasta `MAX_ATTEMPTS`) si no recibe confirmación. Utiliza `setsockopt()` con `SO_RCVTIMEO` para timeouts.

**Subscriber**: Recibe mensajes con ID, procesa contenido y envía ACK de confirmación al broker. Maneja múltiples tipos de mensaje: `MSGID:` para datos, `ACK:SUBSCRIBE:` para confirmación de suscripción.

## Comparación de Implementaciones

| Característica | TCP | UDP | Híbrido |
|----------------|-----|-----|---------|
| **Protocolo de transporte** | TCP (SOCK_STREAM) | UDP (SOCK_DGRAM) | UDP + ACK manual |
| **Garantía de entrega** | Automática | Ninguna | Selectiva por aplicación |
| **Orden de mensajes** | Garantizado | No garantizado | No garantizado |
| **Overhead de red** | Alto (headers TCP) | Mínimo | Medio (UDP + ACKs) |
| **Latencia** | Media-Alta | Muy baja | Baja-Media |
| **Detección de pérdidas** | Automática | Manual/inexistente | Manual con timeouts |
| **Escalabilidad** | Limitada por conexiones | Alta | Alta |
| **Complejidad implementación** | Media (select/poll) | Baja | Media-Alta |
| **Uso de memoria** | Alto (estado/conexión) | Bajo | Medio |
| **Casos de uso ideales** | Sistemas críticos | Streaming tiempo-real | Gaming, IoT con ACKs |

## Compilación

**TCP:**
```bash
gcc -o broker_tcp broker_tcp.c
gcc -o publisher_tcp publisher_tcp.c
gcc -o subscriber_tcp subscriber_tcp.c
```

**UDP:**
```bash
gcc -o broker_udp broker_udp.c
gcc -o publisher_udp publisher_udp.c
gcc -o subscriber_udp subscriber_udp.c
```

**Híbrido:**
```bash
gcc -o broker_hybrid broker_hybrid.c
gcc -o publisher_hybrid publisher_hybrid.c
gcc -o subscriber_hybrid subscriber_hybrid.c
```

## Ejecución

**Importante**: Siempre iniciar el broker primero, luego subscribers, finalmente publishers.

### Implementación TCP

**Terminal 1 (Broker):**
```bash
./broker_tcp
```

**Terminal 2 (Subscriber):**
```bash
./subscriber_tcp
```
Selecciona una opción:
1 = goles
2 = tarjetas
3 = cambios

**Terminal 3 (Publisher):**
```bash
./publisher_tcp
```
En cada ciclo:
1 = goles
2 = tarjetas
3 = cambios
4 = salir
Luego de elegir (1-3) escribe el mensaje y presiona Enter.

### Implementación UDP

**Terminal 1 (Broker):**
```bash
./broker_udp
```

**Terminal 2 (Subscriber):**
```bash
./subscriber_udp
```

**Terminal 3 (Publisher):**
```bash
./publisher_udp
```

### Implementación Híbrida

**Terminal 1 (Broker):**
```bash
./broker_hybrid
```

**Terminal 2 (Subscriber):**
```bash
./subscriber_hybrid
```

**Terminal 3 (Publisher):**
```bash
./publisher_hybrid
```

### Limpieza de procesos
```bash
pkill -f broker_tcp
pkill -f publisher_tcp
pkill -f subscriber_tcp
```

## Análisis de Tráfico con Wireshark

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

## Formato de Mensajes

El publisher envía al broker:
```
MSG:<topic>:<contenido>
```
El subscriber se registra enviando:
```
SUBSCRIBE:<topic>
```
El publisher anuncia su rol al conectar (TCP):
```
PUBLISH:
```
