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

  Última revisión: 29/06/2025
        Se crea función para escribir los 8 bits en el puerto
*/

#include <SPI.h>
#include <SD.h>

//--- CONFIGURACIÓN ---
const char* FILENAME = "file.bin";            // Nombre del fichero a cargar
const unsigned long MAX_RAM_SIZE_BYTES = 2048;  // Límite de la RAM del MPF-1 (2 KiB)
const unsigned long SERIAL_TIMEOUT = 5000;      // Timeout para el monitor serie

//--- PINES ---
const int SD_CS_PIN = 10;
const int STB_PIN   = A0; // Strobe (OUTPUT)
const int RDY_PIN   = A1; // Ready (INPUT)
const int DATA_PINS[] = { 2, 3, 4, 5, 6, 7, 8, 9 };
const int KEY =  A2;

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


void txFake() {

  Serial.println(F("[INFO] Comenzando transferencia de datos..."));
  unsigned long bytesTransferred = 0;
  
  // Lee el archivo mientras haya bytes disponibles
  while (file.available()) {

    byte data = file.read();
    char c = (char) (data+65);
    delay (5);              // esperar 500 msec
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
  Serial.println(F("[INFO] Comenzando transferencia de datos..."));
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
  Serial.print(F("[ÉXITO] Transferencia de archivo completada. Total de bytes: "));
  Serial.println(bytesTransferred);
  
  // Opcional pero recomendado: enviar un byte NULO (0x00)
  // para indicar al Z80 que la transmisión ha terminado.
  Serial.println(F("[INFO] Enviando byte de fin de transmisión (EOT)..."));
  sendByteToPIO(0x00);

  file.close();
  Serial.println(F("[INFO] Archivo cerrado."));
}


/**
 * @brief Envía un solo byte al PIO usando el protocolo de handshake.
 * @param data El byte de 8 bits a enviar.
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
  // Recorre los 8 bits del byte y los pone en los pines de datos.
  for (int i = 0; i < 8; i++) {
    // (data >> i) & 1  -> Extrae el i-ésimo bit (0 o 1) y lo usa para HIGH/LOW
    digitalWrite(DATA_PINS[i], (data >> i) & 1);
  }

  // 3. GENERAR EL PULSO STROBE (activo-bajo)
  // Este pulso le dice al PIO: "¡Ey, el dato está listo, captúralo!"
  digitalWrite(STB_PIN, LOW);
  delayMicroseconds(5); // Pequeña pausa para asegurar la detección del flanco
  digitalWrite(STB_PIN, HIGH);
}


//======================================================================
//  MÉTODOS DE INICIALIZACIÓN
//======================================================================

void initializeSerial(unsigned long baudRate = 9600) {
  Serial.begin(baudRate);
  unsigned long startTime = millis();
  Serial.println(F("\n--- Cargador SD a Z80-PIO ---"));

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
    pinMode(KEY,INPUT);
    
  Serial.println(F("[ÉXITO] Pines de datos configurados."));
}

void initializeSDCard() {
  Serial.println(F("[INFO] Inicializando tarjeta SD..."));
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println(F("[ERROR] No se pudo inicializar la tarjeta SD. Verifica conexiones."));
    while (1);
  }
  Serial.println(F("[ÉXITO] Tarjeta SD inicializada."));

  Serial.print(F("[INFO] Abriendo archivo para cargar en RAM: "));
  Serial.println(FILENAME);
  file = SD.open(FILENAME, FILE_READ);

  if (!file) {
    Serial.print(F("[ERROR] No se pudo abrir el archivo '"));
    Serial.print(FILENAME);
    Serial.println(F("'"));
    while (1);
  }

  unsigned long fileSize = file.size();
  Serial.print(F("[INFO] Archivo abierto. Tamaño detectado: "));
  Serial.print(fileSize);
  Serial.println(F(" bytes."));
  
  if (fileSize == 0) {
    Serial.println(F("[ERROR] El archivo está vacío."));
    file.close();
    while (1);
  }

  if (fileSize > MAX_RAM_SIZE_BYTES) {
    Serial.println(F("[ERROR] El archivo es demasiado grande para la RAM del MPF-1."));
    Serial.print(F("        - Tamaño máximo de la RAM (2 KiB): "));
    Serial.println(MAX_RAM_SIZE_BYTES);
    Serial.print(F("        - Tamaño del archivo encontrado: "));
    Serial.println(fileSize);
    file.close();
    while (1);
  }
  Serial.println(F("[ÉXITO] El tamaño del archivo es válido para la RAM."));
}




//======================================================================
//  SETUP
//======================================================================
void setup() {
  initializeSerial();
  initializeControlPins();
  initializeDataPins();
  initializeSDCard();

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
  txFake();
  Serial.println(F("[INFO] Programa finalizado. Reinicia para volver a ejecutar."));
  while(1);
}
