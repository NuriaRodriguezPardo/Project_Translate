#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"
#include <SD.h>
#include <Audio.h>
#include <SPI.h>
#include <stdio.h>
#include "base64.hpp"

#ifndef TEXTTOSPEECH_HPP  // Guardia de inclusión para evitar la inclusión múltiple
#define TEXTTOSPEECH_HPP

// void connectToWiFi();
// bool initSD();
// String getLanguageCode(const char* languageName);
const char* speakText(const char* text, const char* apiKey, String targetLanguage);
void decode_base64_to_file(const char* base64_input, const char* output_file);

/*
// VARIABLES / CONTRASENYES
const char* ssid = "RedmiNuria";
const char* password = "Patata123";
const char* apiKey = "AIzaSyAK2DlXI83cBLEFxhvFsrUNnMU5m51c_Ms";
const char* apiKey2 = "AIzaSyCz4Pb-7OIi3Gs6LGgJ-XHZ2Xy__hRAeZQ";
*/

/*
void setup() {
    Serial.begin(115200);

    // Conectar a WiFi
    connectToWiFi();

    // Inicializar tarjeta SD
    if (!initSD()) {
        return;
    }

    // Texto a convertir en voz
    const char* text = "Hola, como estas? Yo soy Núria, y tu? Esto es una prueba para saber si google soporta audios grandes, porque con el mio no va.";

    // Idioma de destino
    const char* idioma = "Spanish";
    String targetLanguage = getLanguageCode(idioma);

    // Obtener el audio en Base64 desde Google Cloud Text-to-Speech
    const char* base64Audio = speakText(text, apiKey, targetLanguage);
    // Verificar si se obtuvo el audio correctamente
    if (strlen(base64Audio) > 0)
    {
        // Ejemplo de cadena Base64 (deberías usar tu propia cadena Base64 de audio)
        const char* output_file = "/AudioDecodificado_hola.wav";
        decode_base64_to_file(base64Audio, output_file);
        Serial.println("Archivo de audio decodificado guardado como AudioDecodificado.wav");
    } else {
        Serial.println("Error al obtener el audio desde Google Cloud Text-to-Speech.");
    }
}
*/

/*
void connectToWiFi() {
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Conectado a la red WiFi");
}
*/

/*
const int chipSelect = 39; // Cambia esto según el pin que uses

bool initSD() {
  SPI.begin(36, 37, 35); // void begin(int8_t sck=-1, int8_t miso=-1, int8_t mosi=-1, int8_t ss=-1);
  if (!SD.begin(chipSelect)) {
    Serial.println("Error al inicializar la tarjeta SD.");
    return false;
  }
  Serial.println("Tarjeta SD inicializada.");
  return true;
}
*/

// En const char*
const char* speakText(const char* text, const char* apiKey, String targetLanguage) {
    HTTPClient http;
    String url = "https://texttospeech.googleapis.com/v1/text:synthesize?key=";
    url += apiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonBody = "{\"input\":{\"text\":\"";
    jsonBody += text;
    jsonBody += "\"},\"voice\":{\"languageCode\":\"";
    jsonBody += targetLanguage;
    jsonBody += "\"},\"audioConfig\":{\"audioEncoding\":\"MP3\"}}";

    int httpResponseCode = http.POST(jsonBody);

    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();
        http.end();

        // Usar ArduinoJson para parsear la respuesta JSON
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, response);
        if (error) {
            Serial.print("Error al parsear JSON: ");
            Serial.println(error.f_str());
            return nullptr;
        }

        const char* base64Audio = doc["audioContent"];
        if (!base64Audio) {
            Serial.println("No se encontró audioContent en la respuesta JSON");
            return nullptr;
        }

        // Asignar memoria para el contenido base64 y copiar la cadena
        char* audioBuffer = (char*)malloc(strlen(base64Audio) + 1);
        if (audioBuffer) {
            strcpy(audioBuffer, base64Audio);
        }
        return audioBuffer;
        
    } else {
        Serial.print("Error en la solicitud: ");
        http.end();
        return nullptr;
    }
}

/*
String getLanguageCode(const char* languageName) {
    HTTPClient http;
    String url = "https://translation.googleapis.com/language/translate/v2/languages?key=";
    url += apiKey2;
    url += "&target=en";  // Para obtener los nombres de idiomas en inglés

    http.begin(url);
    int httpResponseCode = http.GET();


    String languageCode = "";
    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();
        StaticJsonDocument<4096> doc;
        deserializeJson(doc, response);
        //DynamicJsonDocument doc(4096);
        //deserializeJson(doc, response);

        JsonArray languages = doc["data"]["languages"];
        for (JsonObject language : languages) {
            if (String(language["name"].as<const char*>()).equalsIgnoreCase(languageName)) {
                languageCode = language["language"].as<String>();
                break;
            }
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return languageCode;
}
*/

// Función para guardar el archivo .mp3 desde una cadena Base64 decodificada
void guardarAudioDesdeBase64(const char* base64String, const char* nombreArchivo) {
  // Decodificar Base64
  unsigned int base64Length = strlen(base64String);
  Serial.print("Longitud de Base64: ");
  Serial.println(base64Length);

  unsigned int decodedLength = decode_base64_length((unsigned char*)base64String, base64Length);
  //Serial.print("Longitud decodificada esperada: ");
  //Serial.println(decodedLength);

  unsigned char decodedBytes[decodedLength];

  // Decodificar
  decodedLength = decode_base64((unsigned char*)base64String, base64Length, decodedBytes);
  //Serial.print("Longitud decodificada real: ");
  //Serial.println(decodedLength);

  // Verificar si la decodificación fue exitosa
  if (decodedLength > 0) {
    // Abrir archivo para escritura en la tarjeta SD
    File archivo = SD.open(nombreArchivo, FILE_WRITE);
    if (!archivo) {
      Serial.println("Error al abrir el archivo en la tarjeta SD.");
      return;
    }

    // Escribir el contenido decodificado en el archivo
    archivo.write(decodedBytes, decodedLength);

    // Cerrar archivo
    archivo.close();
    Serial.println("Archivo de audio guardado correctamente en la tarjeta SD.");
  } else {
    Serial.println("Error en la decodificación o tamaño de datos decodificados igual a 0.");
  }
}

// Escribe el archivo decodificado en la tarjeta SD
void decode_base64_to_file(const char* base64_input, const char* output_file) {
  unsigned int input_length = strlen(base64_input);
  unsigned char* decoded_data = (unsigned char*)malloc(decode_base64_length((unsigned char*)base64_input, input_length));
  unsigned int decoded_length = decode_base64((unsigned char*)base64_input, input_length, decoded_data);

  File file = SD.open(output_file, FILE_WRITE);
  if (!file) {
    Serial.println("No se pudo abrir el archivo de salida.");
    free(decoded_data);
    return;
  }

  file.write(decoded_data, decoded_length);
  file.close();
  free(decoded_data);
}

/*
void loop() {
  // No se necesita hacer nada en el loop
}
*/

#endif 
