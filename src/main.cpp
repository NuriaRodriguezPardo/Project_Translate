#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "Audio.h"
#include <SD.h>
#include <SPI.h>
#include <stdio.h>
#include <driver/i2s.h>
#include <AudioGeneratorWAV.h>
#include <AudioOutputI2S.h>
#include <AudioFileSourceSD.h>
#include <ESPAsyncWebServer.h>

#include "base64.hpp"
#include "PaginaWeb.hpp"
#include "TalkBase64.hpp"
#include "SpeechToText.hpp"
#include "Translate.hpp"
#include "TextToSpeech.hpp"
#include "Listen.hpp"

// MICROFONO
#define I2S_WS 16
#define I2S_SCK 17
#define I2S_SD 15

// ALTAVOZ
#define I2S_BCLK 10
#define I2S_LRC 11
#define I2S_DOUT 12

// SD
#define SD_CS 39
#define SD_SCK 36
#define SD_MOSI 35
#define SD_MISO 37

const char* ssid = "RedmiNuria";
const char* password = "Patata123";
const char* apiKey1 = "AIzaSyCVmhPyvDX0R8hTu6W8b4gBVqsRVIMYZOI";
const char* apiKey2 = "AIzaSyCz4Pb-7OIi3Gs6LGgJ-XHZ2Xy__hRAeZQ";
const char* apiKey3 = "AIzaSyAK2DlXI83cBLEFxhvFsrUNnMU5m51c_Ms";

void TalkToBase64(const char* name1);
void SpeechToText(const char* name1, String language, String& transcribedText);
void Translate(const String &inputText, const String &targetLanguage, String &translatedText);
void TextToSpeech(const String &text, const String &languageCode);
void PaginaWeb();

void connectToWiFi() {
    // WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns1, dns2);
    WiFi.begin(ssid, password);
    Serial.print("Conectando a WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Conectado a la red WiFi");
    Serial.print("Dirección IP: ");
    Serial.println(WiFi.localIP());
}

bool initSD() {
  SPI.begin(SD_SCK, SD_MISO, SD_MOSI); 
  if (!SD.begin(SD_CS)) {
    Serial.println("Error al inicializar la tarjeta SD.");
    return false;
  }
  Serial.println("Tarjeta SD inicializada.");
  return true;
}

String getLanguageCode(String languageName) {
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
            //if (String(language["name"]).equalsIgnoreCase(languageName))
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

void setup() {
    Serial.begin(115200);
    
    connectToWiFi();
    
    if (!initSD()) {
        return;
    }

    PaginaWeb();
}

// Define the TalkToBase64 function
void TalkToBase64(const char* name1) {
    const char* name2 = "/grabacion64.txt";
    setupI2S();
    recordAudioToFile(name1, 2000);
    Serial.println("Grabación completada y guardada en la tarjeta SD");
    encodeAudioToBase64ToFile(name1, name2);
}

void SpeechToText(const char* name1, String language, String& transcribedText) {
    char* audioBase64;

    if (obtenerAudioDesdeSD(audioBase64)) {
        Serial.println("Archivo de audio leído correctamente");
        String idioma_target = getLanguageCode(language);
        const char* texto_transcrito = transcribeSpeech(audioBase64, apiKey1, idioma_target);
        if (texto_transcrito) {
            transcribedText = texto_transcrito;
            // No necesitamos liberar texto_transcrito aquí, ya que no lo asignamos con malloc/calloc
        } else {
            Serial.println("Error al transcribir el audio");
        }
        free(audioBase64);  // Liberar memoria asignada por obtenerAudioDesdeSD
    } else {
        Serial.println("Error al leer el archivo de audio");
    }
}

// Define the Translate function
void Translate(const String &inputText, const String &targetLanguage, String &translatedText) {
    translatedText = translateText(inputText, apiKey2, targetLanguage.c_str());
    Serial.println("Texto traducido:");
    Serial.println(translatedText);
}

// Define the TextToSpeech function
void TextToSpeech(const String &text, const String &languageCode) {
    const char* base64Audio = speakText(text.c_str(), apiKey3, languageCode.c_str());
    if (strlen(base64Audio) > 0) {
        const char* output_file = "/Final.wav";
        decode_base64_to_file(base64Audio, output_file);
        Serial.println("Archivo de audio decodificado guardado como AudioDecodificado.wav");
    } else {
        Serial.println("Error al obtener el audio desde Google Cloud Text-to-Speech.");
    }
}

void PaginaWeb() {
    const char* name1 = "/grabacion.wav";
    const char* name2 = "/grabacion64.txt";
    char idioma_entrada[20]; // Tamaño ajustado según tus necesidades
    char idioma_salida[20];  // Tamaño ajustado según tus necesidades
    String textoTranscrito;

    server.on("/", HTTP_GET, [name1, idioma_entrada, idioma_salida](AsyncWebServerRequest *request) {
        request->send_P(200, "text/html", index_html);
    });

   server.on("/grabar", HTTP_GET, [](AsyncWebServerRequest *request) {
        const char* name1 = "/grabacion.wav";
        TalkToBase64(name1);
        request->send(200, "text/plain", "Grabación completada");
    });

    server.on("/transcribir", HTTP_GET, [](AsyncWebServerRequest *request) {
        const char* name1 = "/grabacion.wav";
        String textoTranscrito =""; 
        String idioma_string = "Spanish";  // Captura una copia de idioma_entrada como String
        Serial.println(idioma_string); // Para depurar: asegúrate de que idioma_string tenga el valor esperado
        SpeechToText(name1, idioma_string, textoTranscrito); // Pasa el idioma como String

        File file = SD.open("/transcripcion.txt", FILE_WRITE);
        if (!file) {
            Serial.println("Error al abrir el archivo para escritura");
            delay(1000);
            return;
        }
        if (file.print(textoTranscrito)) {
            Serial.println("Transcripción escrita correctamente en la tarjeta SD");
        } else {
            Serial.println("Error al escribir en el archivo en la tarjeta SD");
        }

        file.close();
    });
    

    server.on("/traducir", HTTP_GET, [](AsyncWebServerRequest *request) {
        // Abrir el archivo de transcripción en modo lectura
        File file = SD.open("/transcripcion.txt", FILE_READ);
        if (!file) {
            Serial.println("Error al abrir el archivo de transcripción");
            request->send(500, "text/plain", "Error interno del servidor");
            return;
        }

        // Leer el contenido del archivo de transcripción
        String textoTranscrito = file.readString();
        file.close();

        // Obtener el idioma de salida
        String Ltarget2 = getLanguageCode("English");

        // Traducir el texto
        String translatedText;
        Translate(textoTranscrito, Ltarget2, translatedText);

        // Abrir un nuevo archivo para guardar la traducción en la tarjeta SD
        File fileTraduccion = SD.open("/traduccion.txt", FILE_WRITE);
        if (!fileTraduccion) {
            Serial.println("Error al abrir el archivo de traducción");
            request->send(500, "text/plain", "Error interno del servidor al guardar la traducción");
            return;
        }

        // Escribir la traducción en el archivo de traducción
        if (fileTraduccion.print(translatedText)) {
            Serial.println("Traducción guardada correctamente en traduccion.txt en la tarjeta SD");
            request->send(200, "text/plain", "Traducción y síntesis completadas: " + translatedText);
        } else {
            Serial.println("Error al escribir en el archivo de traducción en la tarjeta SD");
            request->send(500, "text/plain", "Error interno del servidor al guardar la traducción");
        }

        // Cerrar el archivo de traducción
        fileTraduccion.close();
    });
    
    server.on("/textToSpeech", HTTP_GET, [](AsyncWebServerRequest *request) {
        String file = "/Speech.wav";
        // Abrir el archivo de traducción en modo lectura desde la tarjeta SD
        File fileTraduccion = SD.open("/traduccion.txt", FILE_READ);
        if (!fileTraduccion) {
            Serial.println("Error al abrir el archivo de traducción en la tarjeta SD");
            request->send(500, "text/plain", "Error interno del servidor al abrir el archivo de traducción");
            return;
        }

        // Leer el contenido del archivo de traducción
        String translatedText = fileTraduccion.readString();
        fileTraduccion.close();

        String Ltarget3 = getLanguageCode("English");

        // Convertir el texto a voz y generar el archivo de audio
        TextToSpeech(translatedText, Ltarget3);

        request->send(200, "text/plain", "Texto convertido a voz y guardado en audio/output.wav");
    });
    server.on("/playAudio", HTTP_GET, [](AsyncWebServerRequest *request) {
        File file = SD.open("/Final.wav", FILE_READ);
        if (!file) {
            Serial.println("Error al abrir el archivo de audio");
            request->send(500, "text/plain", "Error al abrir el archivo de audio");
            return;
        }
        listen();
        // Establece el tipo MIME adecuado para el archivo de audio
        request->send(SD, "/Final.wav", "audio/mpeg");

        // Cierra el archivo después de enviarlo
        file.close();
    });

    server.begin();
}



void loop() {
    // No loop content needed
}