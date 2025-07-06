# MPF-1B SD Card Loader

Este proyecto para Arduino permite cargar un archivo binario desde una tarjeta SD y transferirlo byte a byte a un sistema Z80 (como el MPF-1B) a través de un chip Z80-PIO, utilizando un protocolo de handshake por hardware.



# 🗂️ FICHA DEL PROYECTO

 $${\color{red}Este \space documento \space puede \space tener \space errores \space en \space algunas \space conexiones }$$
 
* Sistema MPF-1b, con expansión adicional de 2K RAM ( 6116 ).
* Placa con Arduino nano y módulo de SDCard para tarjetas microSD

Se debe generar un conexionado de elementos de acuerdo al siguiente esquema eléctrico


- Se recomienda una fuente externa de 5V para alimentar el sistema de forma estable.


![Circuito](/CIRCUITO.png)


### 🛠️ Pines utilizados en Arduino nano

| Función        | Pin         |
|----------------|-------------|
| CS SD Card     | 10          |
| STB (Strobe)   | A0 (salida )|
| -- Sin uso --  | A1 (entrada)|
| Data Pins      | 2 a 9       |
| Modo Operacion | A2          |
| MOSI           | 11          |
| MISO           | 12          |
| SCK            | 13          |

### 🛠️ Pines utilizados del MPF-1B

| Pin            | Función                |
|----------------|------------------------|
| J2 21..28      | PIO PB (entrada)       |
| J2 15          | PIO BITO - PA (entrada)|
| J2 16          | PIO /ASTB     (entrada)|
| J2 30          | GND COMÚN              |




## ⚔️ Operación

Una vez conectados los sistemas el proceso de operación es el siguiente:

1. Encender MPF-1B y escribir en la dirección $1800 el siguiente código máquina
2. Posicionar el jumper A2 de la placa arduino en estado HIGH
3. Ejecutar el programa cargado cuando la placa se inicie
4. En la posición HIGH, el sistema volcará el firmware definitivo en la dirección $2000
   dejando así liberados los 2K de RAM en la zona por defecto del MPF-1B
5. Cambiar el jumper A2 a la posción LOW y reiniciar la placa
6. Ejecutar el firmware en la posición $2000
7. El archivo residente en la tarjeta se volcará a partir de la dirección $1800

Dado lo limitado del sistema, el programa que hayamos editado en nuestro PC se deberá
ensamblar y escribir en la memoria de la SD previamente al proceso.
- Archivo a cargar: `file.bin`
- Tamaño máximo: 2048 bytes (2 KiB)

Lo importante de la carga desde fichero es que en el proceso de generación del mismo
se añade una cabecera de 6 bytes, que lleva información sobre la dirección de memoria
de carga, el tamaño y la dirección de ejecución. Si es disinta de cero el programa
se ejecutará, sino el sistema de para.

## 🔥 Mejoras y ampliaciones

A última hora se ha añadido un jumper posicional que no está implementado, aunque
el código si está preparado ( !pero no probado!)
Me gustaría añadir las siguientes funcionalidades

1. Trasmitir desde el PC via usb el programa a subir y escribirlo en la SD
2. Hacer un pequeño programa en JavaSwing para implementar el intrefaz gráfico del punto 1
3. Implementar un emulador del micropofesor y poder ensamblar y depurar en él.

## 🧑‍💻 Dedicatoria

Este proyecto esta dedicado a mi compañero Alberto, que por su juventud, nunca había interactuado
con sistemas tan viejunos. Me ha aportado mucha satisfacción, ver su asombro en las fases iniciales 
del proyecto y su determinación a continuarlo hasta el final.
Te llevas al menos saber que significa 3E 0A , que es la puntación que te mereces chaval !! 



# 📖 HISTORIA DEL DESARROLLO 

## 🎴 La motivación

Realizar un viaje al pasado, donde los sistemas eran realmente limitados, pero no por ello menos fascinantes.

El microProfessor es un sistema de entrenamiento basado en el microprocesador Z80 de Zilog.
El sistema obliga a programar en código máquina, debiendo el usuario generar manualmente todo
el proceso de ensamblado del programa.
Una de las partes más atrayentes es la posibilida de expandir el sistema. Tenemos acceso a todos
los buses de datos, direcciones y señales de control del micro además de contar con un CTC, un 8255 y
un Z80 PIO en otra batería de conectores.

Se me ocurrió la posibilidad de codificar directamente un un sistema actual y generar el archivo binario
que posteriormente pasaríamos al MPF. Para ello decidí utilizar un arduino nano y un lector de SD.

## ✈️ Teoría de la comnunicación entre los sistemas

Para interactuar entre el Z80 y el Arduino, utilizaremos los puertos del Z80 PIO. El esquema conceptual de la comunicación
se presenta en la siguiente figura:

![Protocolo](/PROTOCOLO.png)

El Z80 espera mientras no haya un flanco de 0->1 en el Bit 0 del puerto A. Cuando este se produce lee el valor que 
se encuentra en el puerto B.
Cuando se inicia el protocolo el Bit 0 debe de estar a 1 , indicando dato no disponible.
El Arduino escribe el valor en el puerto B, espera un tiempo determinado y envía la señal de 0 a 1 para que lea el Z80.

#### 🛰️ Inicio y fin de la transmisión

En ambas partes se conoce el tamaño de la transmisión y ambas partes dejan de transmitir/recibir cuando se han enviado
todos los bytes requeridos. Esto es así , porque se define una cabecera de 6  bytes con la siguiente estructura:

| Bytes       | Descripcion         |
|----------------|-------------|
|0 - 1      |Dirección de carga del bloque|
|2 - 3      |Número de bytes a transmitir|
|4 -5       |Dirección de ejecución una vez terminada la transmisión|

#### Velocidad de transmisión y limitaciones

La velocidad de transmisión depende del tiempo de procesamiento del Z80, se ha elegido un valor empírico de 25 msec
entre transiciones de 1 ->0 y de 0-> 1 en el arduino, excesivamente conservador. No se han hecho pruebas para determina la tasas máxima posible.
En la versión actual del firmware, no se pueden trasnmitir más de 2Kbytes. Sería preciso descontar el tamaño ocupado por
el stack en la memoria que comienza en $1800. Si el cargador se ubica en $1800 , si se podrían aplicar los 2Kbytes
comenzado en $2000

## 🐛 Incidentes y curiosidades

Cuando se acomete un desarrollo vinculado al hardware y se programa a bajo nivel ocurren cosas como las siguientes:

- El programa que maneja la SD funciona perfectamente y de repente deja de funcionar al añadir unos cuandos leds para hacer mas visual el proceso
- La señal de strobe parece no tener efecto aunque lo dice la documentación
- Solucionados todos los problemas, la transmisión es perfecta, salvo para algunos valores determinados

Esto son cosas que no suceden en plataformas de alto nivel y pueden generar algún que otro dolor de cabeza, o incluso pensar que algo sobrenatural nos acompaña.
Nada más lejos de la realidad, observen:

- La SD deja de funcionar porque no recibe suficiente tensión del arduino. Añadimos una fuente estabiliza adicional y resuelto.
- La señal de strobe si funciona, lo que ocurría, es que no estaba conectada a su pin correcto, sino una posición más a la izquierda. Efectos de la vista cansada.
- El protocolo trata por igual a todos los valores. Enviando una serie de patrones, descubrí que el bit 1 del puerto A del PIO en mi placa MPF1B esta estropeado.
  Cambié el protocolo para utilizar el puerto B para recibir y el bit 0 del puerto A como control y se terminaron los problemas.

# 📹 Videos de pruebas

Adjunto la parte más agradable, un par de vídeos de algunas pruebas realizadas

https://github.com/user-attachments/assets/f23a9b72-4138-4dd3-8b18-afe6aad5939e

https://github.com/user-attachments/assets/fcb6d47e-9f2a-44cc-a080-19289955b77e






