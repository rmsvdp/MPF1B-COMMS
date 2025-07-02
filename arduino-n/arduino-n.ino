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
const int STB_PIN   = A0; // Strobe (OUTPUT) bit0 PUERTO B Z80PIO
const int AUX_PIN1  = A1;  // Ready (INPUT)   -- sin usuar --
const int AUX_PIN2  = A2;  //                   -- reservado --
const int DATA_PINS[] = { 2, 3, 4, 5, 6, 7, 8, 9 };


//--- GLOBALES ---
File file;

uint8_t mc0[] = { 0x38,0x39,0x38,0x39};
/* Programa ejemplo para test sin el uso de la SD mc1
 * ORG     $1900
;       Cabecera de 6 bytes
SADDR   EQU     $
TAM     EQU     FIN-INICIO+1
START   DEFW    SADDR
LENGTH  DEFW    TAM
RUN     DEFW    INICIO
INICIO  LD      A,3
        LD      B,5
        ADD     A,B
        LD      (RESULT),A
        HALT
RESULT  DEFB    0        
FIN     NOP
 */
uint8_t mc1[] = { 0x00,0x19,0x0B,0x00,0x06,0x19,0x3E,0x06,0x05,0x80,0x32,0x0F,0x19,0X76,0x00};

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


  // 1. Bit de control a 1 para que espere el z80
        digitalWrite(STB_PIN, HIGH);
      //  waitKey('0',"STB HIGH EN BIT0 PUERTO B");
  // 2. Escribir dato en puerto A
        pushByte(data);
     //   waitKey('0',"ENVIADO DATO A PUERTO B");
  // 3. Generar pulso para que lea el z80
        digitalWrite(STB_PIN, LOW);
       // waitKey('0',"FLANCO STB A LOW: ATENTO Z80");
        delay(500); // 50 msec Pausa para detección de flanco
        digitalWrite(STB_PIN, HIGH);  //activar flanco
       // waitKey('0',"STB A HIGH : LEE!");
  // 4. Retraso adicional
        delay(1000); // 50 msec .Pausa para asegurar lectura
     //   waitKey('0',"VOY POR EL SIGUIENTE!");
}

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

//--------------------------------------------------------------
//-- FUNCIONES DE PRUEBA
//-------------------------------------------------------------- 
void txFake() {

  Serial.println(F("[INFO] Comenzando transferencia de datos..."));
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

/* waitKey : espera a que se pulse la tecla _key antes
 *  de devolver el control al programa
 *  CE: Puerto serie inicializado
 *      _key tecla que se espera
 */
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

/*
 * testPIO :  Envia una secuencia de bytes fija definidos en un
 *            array de memoria. Hace uso de la función waitKey
 *            para trazar el proceso desde el lado del arduino
 */
void testPIO(byte dato[], int tam) {
  Serial.println("[testPIO] Inicio ... ");
  int bytesCnt = 0;


  while (bytesCnt< tam) {
    byte data = dato[bytesCnt];

    bytesCnt++;
    // Imprime el valor en el monitor
    Serial.print(data);
    Serial.print(" > ");
    sendByteToPIO(data);
  } // while bytesCnt < tam)

  Serial.println(); // Salto de línea después de los puntos de progreso
  Serial.print("[testPIO] Fin.- Total de bytes: ");
  Serial.println(bytesCnt);

}


/**
 * @brief Realiza la transferencia completa del archivo, byte por byte.
 */
void performTransfer(String _fich) {
  Serial.println("[INFO] Comenzando transferencia de datos...");
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
  Serial.print("[ÉXITO] Transferencia de archivo completada. Total de bytes: ");
  Serial.println(bytesTransferred);
  /*
  // Opcional pero recomendado: enviar un byte NULO (0x00)
  // para indicar al Z80 que la transmisión ha terminado.
  Serial.println("[INFO] Enviando byte de fin de transmisión (EOT)...");
  sendByteToPIO(0x00);
  */

  file.close();
  Serial.println("[INFO] Archivo cerrado.");
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
  pinMode(AUX_PIN1, INPUT);
  // Estado inicial del Strobe: BAJO (inactivo para un pulso bajo-alto12º          )
  digitalWrite(STB_PIN, HIGH);       // bit0 del puerto B HIGH para detener Z80
  Serial.println(F("[ÉXITO] Pines de control configurados."));

}

void initializeDataPins() {
  Serial.println(F("[INFO] Configurando pines de datos (PA0-PA7)..."));
  for (int i = 0; i < 8; i++) {
    pinMode(DATA_PINS[i], OUTPUT);
    digitalWrite(DATA_PINS[i], LOW); // Valor 0x00 por defecto
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
  initializeSerial();                             // Inicializa puerto serie
  initializeControlPins();                        // Inicializa gpios de control
  initializeDataPins();                           // Inicializa gpios para puerto A del Z80 PIO
  //initializeSDCard();                             // Inicializa SD card
  //if (!checkFile(FILENAME)) while(1);             // Comprueba la validez del archivo
  Serial.println(F("-------------------------------------------------"));
  Serial.println(F("[INFO] La configuración ha finalizado."));
  Serial.println(F("[INFO] La transferencia comenzará en 3 segundos..."));
  delay(3000);
}

//======================================================================
//  LOOP PRINCIPAL (se ejecuta una sola vez)
//======================================================================
void loop() {

  testPIO(mc0, sizeof(mc0));
  // count256(); // Prueba de activación de leds (comprobar asignación correcta de pines)
  // txFake();   // Prueba de envío de datos con salida por puerto serie
  //performTransfer(FILENAME);
  // La transferencia ha terminado, detenemos el programa.
  Serial.println(F("[INFO] Programa finalizado. Reinicia para volver a ejecutar."));
  while(1);
}
