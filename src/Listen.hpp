/*
#include <SD.h>
#include <Audio.h>
#include <SPI.h>

// Configuraciones de la tarjeta SD
#define SD_CS_PIN 5 // Pin CS de la tarjeta SD
#define FILE_NAME "/audio.wav" // Nombre del archivo de audio en la tarjeta SD

// Configuración del I2S para altavoz
#define I2S_BCLK 10
#define I2S_LRC 11
#define I2S_DOUT 12

// Configuración de la tarjeta SD
#define SD_CS 39

void setupSD();

Audio audio;

bool isAudioPlaying = false;
unsigned long startTime = 0;
unsigned long audioDuration = 0;

void setup() {
  Serial.begin(115200);

  setupSD();

  // Inicializa I2S y configura los pines
  audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
  
  // Reproduce el archivo de audio
  if (!audio.connecttoFS(SD, FILE_NAME)) {
    Serial.println("No se pudo abrir el archivo de audio");
    while (true);
  } else {
    Serial.println("Reproduciendo archivo de audio...");
    isAudioPlaying = true;
    startTime = millis();
    audioDuration = 30000; // Establece la duración del archivo de audio en milisegundos (ejemplo: 30 segundos)
  }
}

void setupSD() {
  // Inicializar la tarjeta SD
  SPI.begin(36, 37, 35);
  if (!SD.begin(SD_CS)) {
    Serial.println("Error al montar la tarjeta SD");
    while (true);
  }
}

void loop() {
  // Verifica si la reproducción está en curso basada en el tiempo transcurrido
  if (isAudioPlaying && millis() - startTime >= audioDuration) {
    Serial.println("Reproducción de audio finalizada.");
    isAudioPlaying = false;
    // Aquí puedes realizar alguna acción una vez que el audio ha terminado, como reiniciar la reproducción o cualquier otra lógica que necesites.
    delay(1000); // Ejemplo: Espera 1 segundo antes de reiniciar la reproducción
    //audio.stop(); // Detiene la reproducción del archivo de audio
    audio.connecttoFS(SD, FILE_NAME); // Reinicia la reproducción del archivo de audio
    startTime = millis(); // Reinicia el contador de tiempo
  }

  // Importante: asegúrate de llamar a audio.loop() para mantener la reproducción continua mientras sea necesario
  audio.loop();
}
*/
#include <Arduino.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSD.h> // Incluimos la nueva clase para leer desde SD
#include <SD.h>
#include <SPI.h>

void listen();

#ifndef LISTEN_HPP  // Guardia de inclusión para evitar la inclusión múltiple
#define LISTEN_HPP

// Configuración del I2S para altavoz
#define I2S_BCLK 10
#define I2S_LRC 11
#define I2S_DOUT 12

// Configuración de la tarjeta SD
#define SD_CS_PIN 39   // Pin CS de la tarjeta SD
#define FILE_NAME "/AudioDecodificado_hola.wav" // Nombre del archivo de audio en la tarjeta SD

AudioFileSourceSD *sd;
AudioGeneratorWAV *wav;
AudioOutputI2S *out;
/*
void setupSD() {
  // Inicializar la tarjeta SD
  SPI.begin(36, 37, 35); // Configura los pines SPI para la comunicación con la SD
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("Error al montar la tarjeta SD");
    while (true);
  }
}
*/
/*
void setup() {
  Serial.begin(115200);
  
  setupSD(); // Inicializa la tarjeta SD
  
  listen();

  // Inicializa las instancias de las clases
  sd = new AudioFileSourceSD(FILE_NAME); // Inicializa desde SD utilizando el nombre del archivo
  wav = new AudioGeneratorWAV();
  out = new AudioOutputI2S();

  // Configura el protocolo I2S
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  // Inicia la reproducción del archivo AAC desde SD
  if (!wav->begin(sd, out)) {
    Serial.println("Error al iniciar la reproducción de WAV desde SD");
    while (true);
  }
}

void loop() {
  // Verifica si la reproducción del archivo AAC está en curso
  if (wav->isRunning()) {
    wav->loop();  // Continúa la reproducción del archivo AAC
  } else {
    Serial.println("AAC done");
    delay(10000);
    // Aquí podrías agregar lógica adicional después de la reproducción
  }
}
*/
/*
void listen()
{
    // Inicializa las instancias de las clases
  sd = new AudioFileSourceSD(FILE_NAME); // Inicializa desde SD utilizando el nombre del archivo
  wav = new AudioGeneratorWAV();
  out = new AudioOutputI2S();

  // Configura el protocolo I2S
  out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

  // Inicia la reproducción del archivo AAC desde SD
  if (!wav->begin(sd, out)) {
    Serial.println("Error al iniciar la reproducción de WAV desde SD");
    while (true);
  }
}
*/
void listen() {
    // Inicializa las instancias de las clases
    AudioFileSourceSD *sd = new AudioFileSourceSD(FILE_NAME); // Inicializa desde SD utilizando el nombre del archivo
    AudioGeneratorWAV *wav = new AudioGeneratorWAV();
    AudioOutputI2S *out = new AudioOutputI2S();

    // Configura el protocolo I2S
    out->SetPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);

    // Inicia la reproducción del archivo WAV desde SD
    if (!wav->begin(sd, out)) {
        Serial.println("Error al iniciar la reproducción de WAV desde SD");
        return;
    }

    // Espera a que la reproducción termine
    while (wav->isRunning()) {
        delay(100);
    }

    // Limpia y libera los recursos
    wav->stop();
    delete wav;
    delete out;
    delete sd;
}

#endif 
