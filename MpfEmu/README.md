# MPF EMULATOR

Emulador del equipo Microprofessor 1, modelo 1B.

# Antecedentes

Un emulador es un programa que recrea el funcionamiento de un determinado sistema o dispositivo.
En el caso de un entrenador digital basado en el microprocesador Zilog Z80, con un conjunto mínimo
de dispositos de e/s : teclado básico, memorias (RAM/ROM/EPROM) e integrados de e/s (Z80 PIO, Z80 CTC).

Microprofessor 1B es una equipo de la compañia Multitech (Acer en el actualidad), que se fabricó en
los años 90.
Se puede consultar es sitio de internet, que posee un buen conjunto de documentación:

https://electrickery.hosting.philpem.me.uk/comp/mpf1/


# Desarrollo del proyecto

SE puede acometer el proyecto desde 0 o partir de algún proyecto similar. Esta fué la opción elegida, pero antes 
de ellos explicaremos los fundamentos principales para construir un emulador:

El desarrollo de un emulador está basado en las siguientes premisas:

- Interfaz de usuario en entorno gráfico que simula esencialmente la interacción con el usuario : teclado, joystick
, raton y salida visual : leds, tv, monitor. y la orquestación del funcionamiento del resto
de dispositivos (integrados).
- Conjunto de clases que simulan los integrados que conforman el dispositivo: Procesador, memorias, integrados
de entra salida, dispositivos de sonido, etc.

Dentro de los integrados, lo más complicado es simular los micropocesadores y los dispositivos de e/s, 
ya que las memorias no dejan de ser estructuras de memoria, convientemnte modeladas.
Los integrados específicos se construyen también con clases que reproducen los registros, buses , memoria y comportamiento
de dichos integrados.

Afortunadamente existe muchos proyectos de código libre que emulan los procesadores más populares e integrados
más comunes.

También existen dos proyectos que terminaron convergiendo en uno solo : MAME Y MESS.
El proyecto mame, es un emulador de recreativas, que tuvo una deriva para ordenadores y sistemas personales
Actualmente el proyecto es único, pero muy complejo y amplio, está emulada prácticamente la totalidad
de los sistemas comerciales ( máquinas recretativas y ordenadores personales).

La madurez del proyecto es tan grande, que tiene una arquitectura modular que permite incorporar nuevos
sistemas, pero es muy compleja y la compilación y mantenimiento no es un tema trivial.

Microprofessor estuvo presente durante un conjunto de versiones de mess pero posteriomente desapareció.

Además de esta iniciativa, hay centenares de proyectos que tienen como objetivo una única máquina, son sistemas
uni emulador o como mucho varias versiones del mismo fabricante. Pero el diseño es completamente propietario.

## Emulador desde cero o adaptación de uno existente

Disponiendo del core del microprocesador, dada la sencillez del sistema, es asumible un desarrollo desde
cero.
Sin embargo, hace más de 10 años ya, hice una pequeña adaptación de un emulador de ordenadores Spectrum
que estaba desarrollado en C#.

Pensé que podría optar por simplificar este proyecto y reducirlo hasta lo esencial para  emular el 
sistema objetivo.

En la sección de licencias están los enlaces a los dos proyectos que he utilizado.

## Fases del Desarrollo

Las fases de desarrollo planificadas fueron:


* Fase 1: Adaptación del IDE a Windows 10/11
* Fase 2: Comprender más a fondo el funcionamiento del proyecto original
* Fase 3: Limpieza de código : Simplificación de clases y funcionalidad
* Fase 4: Construcción del core de ejecución : procesador + memoria
* Fase 5: Construcción de la entrada por teclado ( Matriz de 36 teclas )
* Fase 6: Construcción de la salida ( Display de 6 leds de 7 segmentos)
* Fase 7: Implementación del sonido.
* Fase 8: Timming y Precision de la emulación

La fase 3 ha continuado intercalándose con el resto de siguientes fases.


## Notas técnicas

### Fase 1

El proyecto de partida es antiguo y utiliza una versión obsoleta de drivers directX.
Hasta el momento se han mantenido y funcionan sin problemas, pero requieren su instalación
para desarrollar y ejecutar el código.
El sistema emulado, pese a ser simple exigía renderizado por pantalla, que se ha dejado 
inhabilitado en el código , pero no se ha eliminado.

### Fase 2

El proyecto es muy complejo, ya que emulaba almcenamiento en disco, carga desde cinta,
depuración de código máquina paso a paso, inspección de memoria y muchas funciones más.
La depuración era uno de los objetivos el almacenamiento se ha dejado simplemnte como carga y salvagurada
de ficheros binarios que se leen / escriben sin alteración de formato en la memoria.
Durante la fase 3 y efectuando sesiones de depuración se ha ido comprendiendo mejor la arquitectura
del proyecto original.

### Fase 3

La simplificación ha consistido esencialemnte en ir aislando secciones de código / variables mediante
comentarios y comprobando su efecto. Los controles de los formularios se modificaban directamente desde
el IDE.
Se han realizado muchas iteraciones, realizando commits, ya que en varias situaciones se han producido
efectos laterales bloqueantes que obligaban a revertir las modificaciones.

### Fase 4

A nivel de modelado de clases, el proyecto está organizado como sigue:

- Clase principal que contiene modelado un microprocesador Z80 : Z80.cs.
- Clase que hereda de la anterior y representa el procesador del sistema Microprofesor: MpfMachine.cs
- Clase que hereda de la anterior y representa el modelo concreto de sitema : Mpf1b.cs

Esto se ha mantenido, ya que el emulador original, disponía de una clase maestra que contenía
la arquitectura común a todos los modelos Sinclair y luego una clase derivada por cada uno de los
modelos.

Esencialmente los modelos difieren en la cantidad memoria y los dispositivos auxiliares de e/s.
La diferencia fundamental se encuentra en las instrucciones de e/s IN OUT que difieren en los modelos.

#### Memoria

El Z80 sólo direcciona 64K  de memoria (ROM + RAM), los sistemas Sinclair/Amstrad aumentaron la cantidad
de memoria mediante la técnica de paginación de bancos de 8K / 16K. 

En el sistema Mpf1, se ha mantenido la filosofía de páginas. Para ello se ha definido un único sistema
de 32 páginas de 2k, que conforman en espacio completo de 64K. En futuras versiones se podría implementar
paginación, pero no se correspondería con ningún modelo real.
Sin redefinir las instrucciones de e/s , utilizando las del core, ya se dispone de un dispositivoque ejecuta
programas. Pero no ofrece ninguna salida ni lee teclado.
Con esto se pudo comprobar la ejecuión de instrucciones de registro y acceso a memoria.
Se valido el módulo monitor, que muestra el desensamblado y permite ejecutar paso a paso las instrucciones.

La memoria ROM se carga desde un archivo que contiene un volcado de los integrados originales de memoria ROM / EPROM

**TO-DO** : Todo el espacio está disponible, falta el código para permitir el acceso a las memorias que realmente
tiene el Mpf-1b.


### Fase 5

#### Matriz de teclas

De acuerdo al manual, la matriz de teclado lo forman 36 teclas, que controla un i8255. Este integrado
tambien se encarga de manejar los leds además del control del altavoz y la e/s de cassette.
Se ha modelado una clase i8255.cs, que implementa esencialmente los registros y su comportamiento ( in/out/control)

El objetivo es capturar las pulsaciones de teclado del ordenador que ejecuta el emulador y convertirlas
en pulsaciones/liberación de las teclas de la matriz.
Esencialmente el diseño electrónico pasa por presentar un 1 - 0 en cada uno de los bits del puerto
asociado a la matriz, que en ciclos de lectura mediante instrucciones IN, permiten al programa monitor
del Mpf-1b detectar si se pulso/liberó una tecla.

Para conseguir esto, el formulario del emulador ( Form1.cs) , instancia un objeto de la clase mpf1.cs que comienza
a ejecutarse en paralelo a la atención a los eventos del formulario.
Se ha definido una matriz de 36 posiciones en el formulario , que guarda el valor del teclado del PC.
El objeto Mpf1b , dispone de un array de 6 posiciones que almacena cada una de las filas que puede leer
el Z80 a través de los puertos del i8255. Escribiendo en ese array de 6 posiciones, cuando se ejecuta
una instrucción IN, se escribe en el registro i8255 el valor de la fila que toca en ese momento.
De esta forma se consigue el mismo efecto que en el sistema real.
La magia ocurre, porque el PC va a mucha más velocidad que el Z80 y para cuando este quiere leer, ya existe
el valor.

**TO-DO**: Faltan las 4 teclas conectadas directamente al hardware que no están aún implementadas porque no van
por instrucciones IN.

La rutina que se encarga de recoger los valores está en la dirección $0624, lee una por una las filas y una Por
una las teclas. Si hay más de una, prevalece la úlitma. 

### Fase 6

#### Array de 6 Leds de 7 segmentos

Un led de 7 segmentos está encendido o apagado, pero no mantiene memoria. En función de los segmentos que 
se iluminan tenemos distintos patrones.

Para dar una sensación de texto estable en los 6 leds, se utiliza la técnica de la multiplexación. 
Se representa el patrón, se mantiene y tiempo, se apaga y se pasa al siguiente. Si se hace suficientemente rápido
la persitencia de la retina del ojo humano hace el resto.
Esto se hace muy complicado de pasar a la pantalla de un monitor, si se quiere reproducir fielmente, por
lo que se ha optado, tras estudiar la rutinas de monitor por otra estrategia.

La rutina que actualiza los leds es la misma que la que lee el teclado, de hecho como son seis filas
de teclas, en cada fila se actualiza un led primero y se leen todas la teclas de esa fila.
Para actualizar el led, el z80 debe ejecutar una instrucción OUT para cada led.

Sólo existe esta rutina en todo el código de la ROM para implemtar la interacción. Esta rutina
tiene como requisito de entrada que el registro IX debe apuntar a la zona donde están los patrones 
para refrescar los Leds.

Como todo ocurre a mucha velocidad, se captura el comando CALL del Z80 y se comprueba si la dirección 
a saltar es la $0624. Si es así, tomando como referencia el valor del registro IX, se rellena una array
con los 6 valores apuntados. El punto decimal se habilita también, sobre el patrón que se desea. Para ello
se ha creado también un array de 6 posiciones para registrar si está encendido o no.

Existe el recurso Action, que permite invocar métodos de un formulario desde un proceso que está corriendo.

Cuando termina la ejecución de la rutina $0624, antes de ejecutar la siguiente instrucción se invoca
al método del formulario que refresca los controles custom de los leds.
No es una reproducción fiel, pero más que suficiente y además efectiva.

Los códigos que se utilizan para refrescar los leds orginales no se corresponden con los que se necesitan 
para el control personalizado utilizado. Esto se ha solventado con un pequeño switch de conversión sin mayor
complicación.
La propiedad del punto , tampoco está integrada en la valor a representar sino que es una propiedad
independiente, por eso se ha utilizado el array adicional.


### Fase 7

**SIN implementar**

El interfaz de usuario lo permite, las clases tienen atributos y métodos, pero no se ha integrado 
en el funcionamiento.
También se debe implementar en las instrucciones OUT de la clase mpf1.cs



### Fase 8

**SIN implementar**

Comprobar que la emulación es fiel en T-estados y tiempo de ejecución
Mejoras en el código.
Incorporar la etiquetas en el Debugger. Está casi en la adaptación que realicé.

**Implementación de los leds TONE y HALT**

Existen dos leds en el equipo original, que se utilizan para enviar información visual sobre el estado del equipo:

* <ins>Led HALT</ins> : Color Rojo. Se enciende en condiciones de error y cuando el microprocesador ejecuta una instrucción Halt.
Esta simulado en el core de emulación, capturando la instrucción HALT ($76). Se utiliza el recurso Action para refrescar
el valor.
* <ins>Led TONE</ins> : Color Verde. Este led se habilita/inhabilita a través del bit 7 del puerto C del i8255. Se optó inicialmente
por capturalo desde la instrucción OUT, pero se ha optado por complementarlo con la captura de la rutina estandar de la 
ROM BEEP. Como el timming no es muy preciso aún, se aprecia el cambio ON /OFF levemente, pero suficiente para esta
versión de desarrollo.


# Licencia & Copyrights

Este proyecto está basado en dos proyectos existentes:
- Emulador de equipos Sinclair ZxSpectrum, varias versiones: Arjun Nair en https://github.com/ArjunNair/Zero-Emulator
- Emulador de leds de 7 segmentos: dbrant en https://github.com/dbrant/SevenSegment.git

