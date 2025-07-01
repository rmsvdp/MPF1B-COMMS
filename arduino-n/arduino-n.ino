/*
  MPF-1B SD Card Loader
  ----------------------
  Lee un archivo binario desde una tarjeta SD y lo transfiere
  byte a byte a un sistema Z80 (como el MPF-1B) a través
  de un chip Z80-PIO, usando un protocolo de handshake.

  Lógica del Handshake (Arduino -> PIO):
  1. Arduino espera a que el pin RDY del PIO esté en LOW (listo para recibir).
  2. Arduino pone los 8 bits del dato en los pines DATA_PINS.
  3. Arduino genera un pulso activo-bajo en el pin STB (HIGH -> LOW -> HIGH)
  para indicarle al PIO que el dato está disponible para ser leído.
  4. El PIO captura el dato y el ciclo se repite.

  Realizado por Alberto Alegre, Ramón Merchán, Juan Carlos Redondo

  log de revisiones: 29/06/2025
        29:06/2025  Se crea función para escribir los 8 bits en el puerto
        01/07/2025  Añadida función de demostración ( count256() ) para probar todos los bits de comunicaciones
                    NOTA HW: Se precisa alimentación estable externa de 5V. Añadida FA auxiliar
          
*/

#include <SPI.h>
#include <SD.h>

//--- CONFIGURACIÓN ---
const String FILENAME = "file.bin";              // Nombre del fichero a cargar
const unsigned long MAX_RAM_SIZE_BYTES = 2048;  // Límite de la RAM del MPF-1 (2 KiB)
const unsigned long SERIAL_TIMEOUT = 5000;      // Timeout para el monitor serie

//--- PINES ---
const int SD_CS_PIN = 10;
const int STB_PIN   = A0; // Strobe (OUTPUT)
const int RDY_PIN   = A1; // Ready (INPUT)
const int DATA_PINS[] = { 2, 3, 4, 5, 6, 7, 8, 9 };
const int EXTRA_PIN =  A2;

//--- GLOBALES ---
File file;

//======================================================================
//  LÓGICA DE TRANSFERENCIA
//======================================================================

/*
 * pushByte : Envia a los 8 pines asignados, los bits que componen
 *            el byte que se pasa como parámetro
 * CE:        _data : byte a exponer en los 8 pines
 * Fecha:     29/06/2025
 */
void pushByte(byte _data) {
  
    // Recorre los 8 bits del byte y los pone en los pines de datos.
    for (int i = 0; i < 8; i++) {
    // (_data >> i) & 1  -> Extrae el i-ésimo bit (0 o 1) y lo usa para HIGH/LOW
    digitalWrite(DATA_PINS[i], (_data >> i) & 1);
    } // end for
  } // en pushByte


/*
 * count256() : prueba sobre los 8 gpios asignados para la transferencia. 
 *              Se encuentra a nivel hw conectados a leds para verificación
 *              visual. Se muestras las 256 combinaciones posibles
 * Fecha:     01/07/2025
 */
void count256() {

  Serial.println("[INFO] Contando de 0 a 255 ...");
  byte data = 0;
  int count = 0;
  
while (count < 256)
{
    pushByte(data);
    delay(125);
    Serial.print(data);
    Serial.print(".");
    data++;
    count++;
}   
data = 0;
pushByte(data);
delay(1000);
data = 255;
pushByte(data);
delay(1000);
data = 0;
pushByte(data);
delay(1000);

Serial.print("\n");

}

void txFake() {

  Serial.println(F("[INFO] Comenzando transferencia de datos..."));
  unsigned long bytesTransferred = 0;
  
  // Lee el archivo mientras haya bytes disponibles
  while (file.available()) {

    byte data = file.read();
    char c = (char) (data+65);
    delay (5);              // esperar 5 msec
    bytesTransferred++;
    //digitalWrite(DATA_PINS[0],(data & 1));
    pushByte(data);
    delay(1500);
    Serial.print(c);
    Serial.print(".");
    

  }
Serial.print("\n");


}
/**
 * @brief Realiza la transferencia completa del archivo, byte por byte.
 */
void performTransfer() {
  Serial.println("[INFO] Comenzando transferencia de datos...");
  unsigned long bytesTransferred = 0;
  
  // Lee el archivo mientras haya bytes disponibles
  while (file.available()) {
    byte data = file.read();
    sendByteToPIO(data);
    bytesTransferred++;

    // Imprime un punto cada 64 bytes para mostrar progreso sin inundar el monitor
    if (bytesTransferred % 64 == 0) {
      Serial.print(F("."));
    }
  }

  Serial.println(); // Salto de línea después de los puntos de progreso
  Serial.print("[ÉXITO] Transferencia de archivo completada. Total de bytes: ");
  Serial.println(bytesTransferred);
  
  // Opcional pero recomendado: enviar un byte NULO (0x00)
  // para indicar al Z80 que la transmisión ha terminado.
  Serial.println("[INFO] Enviando byte de fin de transmisión (EOT)...");
  sendByteToPIO(0x00);

  file.close();
  Serial.println("[INFO] Archivo cerrado.");
}


/*
 *  sendByteToPIO() Escribe en el puerto A del pio el byte a transferir
 *  El protocolo consiste en :
 *      1. esperar a que el PIO esté listo para recibir: señal RDY_PortA a Low
 *      2. escribir en el puerto A
 *      3. Activar strobe del puerto B. Esto se consigue escribiendo en el bit 0 del 
 *      puerto B y en el bit de strobe. Como el PIO no tendrá habilitada la interrupción
 *      del Z80, del lado de uPF1 nos enteraremos por el cambio del valor de este bit en
 *      el puerto B ( El strobe asegura una lectura correcta del bit)
 *      4. Retraso intencionado del lado del Arduino para estabilizar la comunicación
 */
void sendByteToPIO(byte data) {
  // 1. ESPERAR A QUE EL PIO ESTÉ LISTO
  // El Z80/PIO pone RDY en LOW cuando está listo para recibir un nuevo byte.
  // Nos quedamos esperando mientras esté en HIGH (ocupado).
  while (digitalRead(RDY_PIN) == HIGH) {
    Serial.println(F("[INFO] Esperando a que el Z80 procese el byte anterior..."));
    delay(1000);
  }

  // 2. PONER EL DATO EN LOS PINES
  pushByte(data);
  
  // 3. GENERAR EL PULSO STROBE (activo-bajo)
  // Este pulso le dice al PIO: "¡Ey, el dato está listo, captúralo!"
  digitalWrite(STB_PIN, LOW);
  delayMicroseconds(100); // 100 usec .Pequeña pausa para asegurar la detección del flanco
  digitalWrite(STB_PIN, HIGH);

  // 4. Retraso intencionado para depuración / trazas / estabilización de las comunicaciones

  delay(50); // 50 msec 
}


//======================================================================
//  MÉTODOS DE INICIALIZACIÓN
//======================================================================

void initializeSerial(unsigned long baudRate = 9600) {
  Serial.begin(baudRate);
  unsigned long startTime = millis();
  Serial.println(F("\n--- Cargador SD a Z80-PIO ---"));
  Serial.println(F("\n--- 29/06/2025  ---"));
  Serial.println(F("[INFO] Esperando conexión del monitor serie..."));
  while (millis() - startTime < SERIAL_TIMEOUT) {
    if (Serial) {
      Serial.println(F("[ÉXITO] Monitor serie conectado"));
      return;
    }
    delay(10);
  }
  Serial.println(F("[ERROR] Timeout esperando monitor serie"));
}

void initializeControlPins() {
  Serial.println(F("[INFO] Configurando pines de control (STB/RDY)..."));
  pinMode(STB_PIN, OUTPUT);
  pinMode(RDY_PIN, INPUT);
  // Estado inicial del Strobe: ALTO (inactivo para un pulso activo-bajo)
  digitalWrite(STB_PIN, HIGH);
  Serial.println(F("[ÉXITO] Pines de control configurados."));

}

void initializeDataPins() {
  Serial.println(F("[INFO] Configurando pines de datos (PA0-PA7)..."));
  for (int i = 0; i < 8; i++) {
    pinMode(DATA_PINS[i], OUTPUT);
    digitalWrite(DATA_PINS[i], LOW);
  }

    
  Serial.println(F("[ÉXITO] Pines de datos configurados."));
}

/*
 * checkFile()    Comprueba si el archivo solicitado es válido para la transferencia
 * Las condiciones en las que un archivo es válido son las siguientes:
 *                - El archivo existe en la SD
 *                - El archivo tiene un tamaño no superior a 2K ( máxima RAM del upF1b)
 * CE:            Nombre del archivo
 * CS:            false (0) : archivo no válido
 *                true  (1) : archivo válido
 * fecha:         01/07/2025
 */
boolean checkFile(String _fich) {
  
    file = SD.open(_fich, FILE_READ);

  if (!file) {
    Serial.print(F("[ERROR] No se pudo abrir el archivo '"));
    Serial.print(_fich);
    Serial.println(F("'"));
    return false;
  }

  unsigned long fileSize = file.size();
  Serial.print(F("[INFO] Archivo abierto. Tamaño detectado: "));
  Serial.print(fileSize);
  Serial.println(F(" bytes."));
  
  if (fileSize == 0) {
    Serial.println(F("[ERROR] El archivo está vacío."));
    file.close();
    return false;
  }

  if (fileSize > MAX_RAM_SIZE_BYTES) {
    Serial.println(F("[ERROR] El archivo es demasiado grande para la RAM del MPF-1."));
    Serial.print(F("        - Tamaño máximo de la RAM (2 KiB): "));
    Serial.println(MAX_RAM_SIZE_BYTES);
    Serial.print(F("        - Tamaño del archivo encontrado: "));
    Serial.println(fileSize);
    file.close();
    return false;
  }
    file.close();
    return true;
  }

/*
 * initializeSDCard . Compueba el funcionamiento de la SD
 * CS:        El programa se bloquea si no se puede inicializar la SD
 */
 
void initializeSDCard() {
  Serial.println(F("[INFO] Inicializando tarjeta SD..."));
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("[ERROR] No se pudo inicializar la tarjeta SD. Verifica conexiones."));
    while (1);
  }
  Serial.println(F("[ÉXITO] Tarjeta SD inicializada."));
/*
  Serial.print(F("[INFO] Abriendo archivo para cargar en RAM: "));
  Serial.println(FILENAME);
  file = SD.open(FILENAME, FILE_READ);
*/
}




//======================================================================
//  SETUP
//======================================================================
void setup() {
  initializeSerial();
  //initializeControlPins();
  initializeDataPins();
  initializeSDCard();
  if (!checkFile(FILENAME)) while(1);          // Comprueba la validez del archivo
  Serial.println(F("-------------------------------------------------"));
  Serial.println(F("[INFO] La configuración ha finalizado."));
  Serial.println(F("[INFO] La transferencia comenzará en 3 segundos..."));
  delay(3000);
}

//======================================================================
//  LOOP PRINCIPAL (se ejecuta una sola vez)
//======================================================================
void loop() {
   // while(1);
  //performTransfer();
  // La transferencia ha terminado, detenemos el programa.
  count256();
  // txFake();
  Serial.println(F("[INFO] Programa finalizado. Reinicia para volver a ejecutar."));
  while(1);
}
