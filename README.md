# MPF-1B SD Card Loader

Este proyecto para Arduino permite cargar un archivo binario desde una tarjeta SD y transferirlo byte a byte a un sistema Z80 (como el MPF-1B) a través de un chip Z80-PIO, utilizando un protocolo de handshake por hardware.

## 🧩 Descripción

El programa:

- Lee un archivo `file.bin` desde la tarjeta SD.
- Usa pines digitales para enviar cada byte al bus de datos del sistema Z80.
- Controla el proceso mediante un protocolo de "handshake" utilizando los pines `STB` (Strobe) y `RDY` (Ready).
- Permite pruebas de comunicación mediante una función que recorre todos los valores de byte (`count256()`).

## 📐 Protocolo de Handshake

**Arduino → Z80-PIO**

1. Arduino espera que el pin `RDY` esté en `LOW` (listo para recibir).
2. Coloca los 8 bits del dato en los pines `DATA_PINS`.
3. Genera un pulso en `STB` (`HIGH → LOW → HIGH`) para indicar que el dato está disponible.
4. El PIO lee el dato y el proceso se repite.

## ⚙️ Configuración

- Archivo a cargar: `file.bin`
- Tamaño máximo: 2048 bytes (2 KiB)
- Timeout serie: 5000 ms

## 🛠️ Pines utilizados

| Función       | Pin         |
|---------------|-------------|
| CS SD Card    | 10          |
| STB (Strobe)  | A0 (salida )|
| RDY (Ready)   | A1 (entrada)|
| Data Pins     | 2 a 9       |
| Extra Pin     | A2          |

## 🧪 Función de Prueba

La función `count256()` envía los 256 valores posibles (0 a 255) para verificar el funcionamiento completo del bus de datos y la lógica de control.

## ⚠️ Notas de Hardware

- Se recomienda una fuente externa de 5V para alimentar el sistema de forma estable.

## 🧑‍💻 Autores

- Alberto Alegre  
- Ramón Merchán  
- Juan Carlos Redondo  

## 📅 Registro de Cambios

- **29/06/2025**: Se crea función `pushByte()` para escribir 8 bits en el puerto.
- **01/07/2025**: Añadida función `count256()` para pruebas de comunicación. Requiere FA externa.

---

Este programa es útil para emular la carga de programas en sistemas retro como el MPF-1B a través de una interfaz moderna.
