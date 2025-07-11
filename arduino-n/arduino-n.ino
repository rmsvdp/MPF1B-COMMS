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
const int MAX_RAM_SIZE_BYTES = 2048;  // Límite de la RAM del MPF-1 (2 KiB)
const int SERIAL_TIMEOUT = 5000;      // Timeout para el monitor serie


//--- PINES ---
const int SD_CS_PIN = 10;
const int STB_PIN   = A0; // Strobe (OUTPUT) bit0 PUERTO B Z80PIO y /ASTB
const int AUX_PIN1  = A1; // Ready (INPUT)   -- sin usuar --
const int MODE_PIN  = A2; // Modo de operación: (INPUT)
                          //   LOW   -- Envía archivo de la SD --
                          //   HIGH  -- Carga loader en MPF-I  --
const int DATA_PINS[] = { 2, 3, 4, 5, 6, 7, 8, 9 };
const int _DELAY = 25;    // msec a esperar para sincronizar con Z80
int _MODE = 0;       // Cargar desde SD
  const String msg[]  = {  "Listo para transferir archivo de SD a MPF-1B",
                    "Listo para transferir Loader a MPF-1B"};

//--- GLOBALES ---
File file;

/*  Loader final que gestiona cargas posicionadas
 *  con autoejecución
 */

 uint8_t loader[] = {  0x3e,0x4f,0xd3,0x83,0x3e,0xcf,0xd3,0x82,
                      0x3e,0xff,0xd3,0x82,0xcd,0x37,0x20,0x57,
                      0xcd,0x37,0x20,0x5f,0xcd,0x37,0x20,0x47,
                      0xcd,0x37,0x20,0x4f,0xcd,0x37,0x20,0x67,
                      0xcd,0x37,0x20,0x6f,0xcd,0x37,0x20,0xcd,
                      0x46,0x20,0x12,0x13,0x0b,0x78,0xb1,0x20,
                      0xf3,0x7c,0xb5,0x28,0x01,0x76,0xe9,0xdb,
                      0x80,0xcb,0x47,0x20,0xfa,0xdb,0x80,0xcb,
                      0x47,0x28,0xfa,0xdb,0x81,0xc9,0x00,0xc9};

// Secuencia de test para todos los bits del puerto del Z80 PIO
//const uint8_t mc0[] = { 0x00,0x0f,0xf0,0xff};

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

  
  // 1. Bit de control debería estar a 1 para que espere el z80
        digitalWrite(STB_PIN, HIGH);
  // 2. Escribir dato en puerto B
        pushByte(data);
  // 3. Generar pulso para que lea el z80
        digitalWrite(STB_PIN, LOW);
        delay(_DELAY); // 25 msec Pausa para detección de flanco
        digitalWrite(STB_PIN, HIGH);  //activar flanco
  // 4. Retraso adicional
        delay(_DELAY); // 25 msec .Pausa para asegurar lectura
}

/*
 * count256() : prueba sobre los 8 gpios asignados para la transferencia. 
 *              Se encuentra a nivel hw conectados a leds para verificación
 *              visual. Se muestras las 256 combinaciones posibles
 * Fecha:     01/07/2025
 */

/* 
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
  data = 0;     pushByte(data);  delay(1000);
  data = 255;   pushByte(data);  delay(1000);
  data = 0;     pushByte(data);  delay(1000);
  
  Serial.print("\n");

} // count256()
*/

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
    Serial.print(F("No se pudo abrir el archivo '"));
    Serial.print(_fich);
    Serial.println(F("'"));
    return false;
  }

  unsigned long fileSize = file.size();
  Serial.print(F("[INFO] Archivo abierto. Tamaño detectado: "));
  Serial.print(fileSize);
  Serial.println(" bytes.");
  
  if (fileSize == 0) {
    Serial.println(F("El archivo está vacío."));
    file.close();
    return false;
  }

  if (fileSize > MAX_RAM_SIZE_BYTES) {
    Serial.println(F("El archivo es demasiado grande para la RAM del MPF-1."));
    /*Serial.print("        - Tamaño del archivo encontrado: ");
    Serial.println(fileSize);
    */
    file.close();
    return false;
  }
    file.close();
    return true;
  }




//--------------------------------------------------------------
//-- FUNCIONES DE PRUEBA
//-------------------------------------------------------------- 
/*
void txFake() {

  Serial.println("Comenzando transferencia de datos...");
  unsigned long bytesTransferred = 0;
  
  // Lee el archivo mientras haya bytes disponibles
  while (file.available()) {

    byte data = file.read();
    char c = (char) (data+65);
    delay (5);              // esperar 5 msec
    bytesTransferred++;
    pushByte(data);
    delay(1500);
    Serial.print(c);
    Serial.print(".");
    

  }
Serial.print("\n");

}
*/
/* waitKey : espera a que se pulse la tecla _key antes
 *  de devolver el control al programa
 *  CE: Puerto serie inicializado
 *      _key tecla que se espera
 */
/*
void waitKey(char _key,String msg) {

boolean salir = false;

  Serial.print(msg);
  Serial.print("\n");
  while (!salir) {
  
    if (Serial.available() > 0) {
     char recibido = Serial.read(); // Lee el carácter recibido   
    if (recibido == _key) salir = true;
    } // if serialAvailable  
  
  } // while !salir


} // waitKey
*/

/*
 * flash_mpf1b :  Envia una secuencia de bytes fija definidos en un
 *            array de memoria. 
 *            void flash_mpf1b(byte dato[], int tam) {
 */

void flash_mpf1b(byte dato[], int tam) {
  
  Serial.println("Enviando Firmware...");
 
  int bytesCnt = 0;

                  

  while (bytesCnt< tam) {
    byte data = dato[bytesCnt];

    bytesCnt++;
    // Imprime el valor en el monitor
    Serial.print(data);
    Serial.print(F(" > "));
    sendByteToPIO(data);
  } // while bytesCnt < tam)

  Serial.println(); // Salto de línea después de los puntos de progreso
  Serial.println(F("Firmware enviado."));
  //Serial.println(bytesCnt);

}


/**
 * @brief Realiza la transferencia completa del archivo, byte por byte.
 */
void performTransfer(String _fich) {
  Serial.println(F("[INFO] Comenzando transferencia de datos..."));
  unsigned long bytesTransferred = 0;

  file = SD.open(_fich, FILE_READ);
  // Lee el archivo mientras haya bytes disponibles
  while (file.available()) {
    byte data = file.read();
    sendByteToPIO(data);
    bytesTransferred++;

    // Imprime un punto cada 64 bytes para mostrar progreso sin inundar el monitor
    if (bytesTransferred % 64 == 0) {
      Serial.print(F("."));
    }
  } // while file.available()

  Serial.println(); // Salto de línea después de los puntos de progreso
  Serial.print(F("[ÉXITO] Transferencia de archivo completada. Total de bytes: "));
  Serial.println(bytesTransferred);
  file.close();
  Serial.println(F("[INFO] Archivo cerrado."));
}





//======================================================================
//  MÉTODOS DE INICIALIZACIÓN
//======================================================================

/*
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
*/

void initializePins() {
  // --- Pines de control de comunicación y modo de funcionamiento
  Serial.println(F("[INFO] Configurando pines de control (STB/RDY)..."));
  pinMode(STB_PIN, OUTPUT);
  pinMode(AUX_PIN1, INPUT);
  pinMode(MODE_PIN, INPUT);           // Modo de funcionamiento LOW = SD , HIGH = LOADER
  // Estado inicial del Strobe: BAJO (inactivo para un pulso bajo-alto12º          )
  digitalWrite(STB_PIN, HIGH);       // bit0 del puerto B HIGH para detener Z80
  // --- Inicializa a 0 el bus datos con el PIO
  //  Serial.println("[INFO] Configurando pines de datos (PB0-PB7)...");
  for (int i = 0; i < 8; i++) {
    pinMode(DATA_PINS[i], OUTPUT);
    digitalWrite(DATA_PINS[i], LOW); // Valor 0x00 por defecto
  }    
  //Serial.println(F("[ÉXITO] Pines de datos configurados."));
  Serial.println(F("[ÉXITO] Pines de control configurados."));

}

/*
 * initializeSDCard . Compueba el funcionamiento de la SD
 * CS:        El programa se bloquea si no se puede inicializar la SD
 */
 
void initializeSDCard() {
  
  Serial.println(F("[INFO] Inicializando tarjeta SD..."));
  if (SD.begin(SD_CS_PIN)) {
    if (!checkFile(FILENAME)) while(1);            // Comprueba la validez del archivo
    else { 
         Serial.println(F("[ERROR] No se pudo inicializar la tarjeta SD. Verifica conexiones."));
         while (1);
         }
  Serial.println(F("[ÉXITO] Tarjeta SD inicializada."));

   } 
}// initializeSDCard()




//======================================================================
//  SETUP
//======================================================================
void setup() {

  //initializeSerial();                                   // Inicializa puerto serie
  Serial.begin(9600);                                   // Inicializa puerto serie
  initializePins();                                     // Inicializa gpios del Arduino
  int res;

  Serial.println(F("-------------------------------------------------"));               

  res = digitalRead(MODE_PIN);
  if (res ==HIGH) _MODE=1;                              // Carga del firmware al MPF-1B
  else  initializeSDCard();                             // Inicializa SD card
  Serial.println(msg[_MODE]);
  Serial.println(F("esperando 3 segundos..."));
  delay(3000);  
}

//======================================================================
//  LOOP PRINCIPAL (se ejecuta una sola vez)
//======================================================================
void loop() {

  if (_MODE == 1) flash_mpf1b(loader, sizeof(loader));
  else            performTransfer(FILENAME);
  Serial.println(F("[INFO] Operación Terminada. Reinicia para volver a ejecutar."));
  while(1);
}
