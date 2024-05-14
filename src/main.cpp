
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

const char* ssid = "AndroidAP8342";
const char* password = "Montse1704";
const char* apiKey = "AIzaSyCz4Pb-70Ii3Gs6LGgJ-XHZ2Xy__hRAeZQ";

// Definir el pin del micrófono analógico
const int pinMicrophone = A0;

// Función para conectar a WiFi
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

// Función para inicializar
void setup() {
    Serial.begin(115200);
    Serial.println("Inicio");
    connectToWiFi();
    pinMode(pinMicrophone, INPUT);
}

// Función para capturar audio
String captureAudio() {
    const int bufferSize = 1024;
    String audioData = "";

    for (int i = 0; i < bufferSize; i++) {
        // Leer valor del pin analógico
        int audioValue = analogRead(pinMicrophone);
        // Convertir el valor a un rango de 0 a 255
        byte audioByte = map(audioValue, 0, 1023, 0, 255);
        // Convertir el byte a una cadena hexadecimal
        String hexValue = String(audioByte, HEX);
        // Añadir el valor hexadecimal a los datos de audio
        audioData += hexValue;
        // Añadir un separador
        audioData += " ";
    }

    return audioData;
}

// Función para enviar audio a Google Cloud Speech-to-Text
String transcribeSpeech(String audioData, const char* apiKey) {
    HTTPClient http;

    // URL de la API de Google Cloud Speech-to-Text
    String url = "https://speech.googleapis.com/v1/speech:recognize?key=";
    url += apiKey;

    // Configurar la solicitud HTTP
    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    // Crear el cuerpo de la solicitud JSON
    String jsonBody = "{\"config\": {\"encoding\":\"LINEAR16\",\"sampleRateHertz\":16000,\"languageCode\":\"es-ES\"},\"audio\": {\"content\":\"";
    jsonBody += audioData;
    jsonBody += "\"}}";

    // Enviar la solicitud POST con el cuerpo JSON
    int httpResponseCode = http.POST(jsonBody);

    String transcribedText = "";

    // Si la solicitud fue exitosa, obtener la transcripción
    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();

        // Analizar la respuesta JSON para obtener la transcripción
        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);
        if (doc.containsKey("results")) {
            JsonObject result = doc["results"][0];
            if (result.containsKey("alternatives")) {
                JsonObject alternative = result["alternatives"][0];
                if (alternative.containsKey("transcript")) {
                    transcribedText = alternative["transcript"].as<String>();
                }
            }
        } else {
            Serial.println("No se encontró el campo 'results' en la respuesta JSON");
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    // Liberar los recursos
    http.end();

    return transcribedText;
}

String translateText(String text, const char* apiKey, const char* targetLanguage) {
    HTTPClient http;

    String url = "https://translation.googleapis.com/language/translate/v2?key=";
    url += apiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonBody = "{\"q\":\"";
    jsonBody += text;
    jsonBody += "\",\"source\":\"en\",\"target\":\"";
    jsonBody += targetLanguage;
    jsonBody += "\",\"format\":\"text\"}";

    int httpResponseCode = http.POST(jsonBody);

    String translatedText = "";

    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();

        // Parsear la respuesta JSON para obtener la traducción
        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, response);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
        } else {
            translatedText = doc["data"]["translations"][0]["translatedText"].as<String>();
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();

    return translatedText;
}

// Función para sintetizar texto a voz
void speakText(String text, const char* apiKey, const char* targetLanguage) {
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
        // Leer el audio generado
        String audio = http.getString();

        // Aquí puedes manejar el audio según tus necesidades
        // Por ejemplo, podrías guardarlo en un archivo, reproducirlo, etc.
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void loop() {
    // Capturar audio
    String audioData = captureAudio();

    // Transcribir discurso
    String transcribedText = transcribeSpeech(audioData, apiKey);

    // Imprimir la transcripción en el Monitor Serie
    Serial.println("Transcripción:");
    Serial.println(transcribedText);

    String translatedText = translateText(transcribedText, apiKey, "es"); // Traducir al español

    // Imprimir la traducción en el Monitor Serie
    Serial.println("Traducción:");
    Serial.println(translatedText);

    // Sintetizar el texto traducido a voz
    speakText(translatedText, apiKey, "es-ES"); // Sintetizar en español de España


    delay(5000); // Esperar un tiempo antes de la próxima transcripción
}
/*
#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <DFPlayerMini_Fast.h>

const char* ssid = "AndroidAP8342";
const char* password = "Montse1704";
const char* apiKey = "AIzaSyCz4Pb-70Ii3Gs6LGgJ-XHZ2Xy__hRAeZQ";

const int pinMicrophone = A0;

DFPlayerMini_Fast dfplayer;

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

void setup() {
    Serial.begin(115200);
    Serial.println("Inicio");
    connectToWiFi();
    pinMode(pinMicrophone, INPUT);

    dfplayer.begin(Serial1); // Conexión serie al DFPlayer Mini
}

String captureAudio() {
    const int bufferSize = 1024;
    String audioData = "";

    for (int i = 0; i < bufferSize; i++) {
        int audioValue = analogRead(pinMicrophone);
        byte audioByte = map(audioValue, 0, 1023, 0, 255);
        String hexValue = String(audioByte, HEX);
        audioData += hexValue;
        audioData += " ";
    }

    return audioData;
}

String transcribeSpeech(String audioData, const char* apiKey) {
    HTTPClient http;

    String url = "https://speech.googleapis.com/v1/speech:recognize?key=";
    url += apiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonBody = "{\"config\": {\"encoding\":\"LINEAR16\",\"sampleRateHertz\":16000,\"languageCode\":\"es-ES\"},\"audio\": {\"content\":\"";
    jsonBody += audioData;
    jsonBody += "\"}}";

    int httpResponseCode = http.POST(jsonBody);

    String transcribedText = "";

    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();

        DynamicJsonDocument doc(1024);
        deserializeJson(doc, response);
        if (doc.containsKey("results")) {
            JsonObject result = doc["results"][0];
            if (result.containsKey("alternatives")) {
                JsonObject alternative = result["alternatives"][0];
                if (alternative.containsKey("transcript")) {
                    transcribedText = alternative["transcript"].as<String>();
                }
            }
        } else {
            Serial.println("No se encontró el campo 'results' en la respuesta JSON");
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();

    return transcribedText;
}

String translateText(String text, const char* apiKey, const char* targetLanguage) {
    HTTPClient http;

    String url = "https://translation.googleapis.com/language/translate/v2?key=";
    url += apiKey;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");

    String jsonBody = "{\"q\":\"";
    jsonBody += text;
    jsonBody += "\",\"source\":\"en\"#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <driver/i2s.h>

const char* ssid = "TuSSID";
const char* password = "TuContraseña";
const char* apiKey = "TuAPIKey";

// Configuración para la captura de audio
#define SAMPLE_RATE     (16000) // Frecuencia de muestreo en Hz
#define SAMPLE_BITS     (16)    // Profundidad de bits
#define CHANNELS        (1)     // Número de canales (mono)
#define BUF_SIZE        (1024)  // Tamaño del búfer de entrada

// Buffer para almacenar datos de audio capturados
int16_t i2sBuf[BUF_SIZE];

// Función para conectar a WiFi
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

// Función para configurar I2S para la captura de audio
void setupI2S() {
    // Configurar I2S para la captura de audio
    i2s_config_t i2sConfig = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = (i2s_bits_per_sample_t)SAMPLE_BITS,
        .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S, 
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = BUF_SIZE
    };
    i2s_driver_install(I2S_NUM_0, &i2sConfig, 0, NULL);
    i2s_set_pin(I2S_NUM_0, NULL);
}

// Función para capturar audio
String captureAudio() {
    // Leer datos de audio desde I2S
    i2s_read(I2S_NUM_0, i2sBuf, BUF_SIZE * sizeof(int16_t), NULL, portMAX_DELAY);
  
    
    // Convertir datos de audio a una cadena hexadecimal
    String audioData = "";
    for (int i = 0; i < BUF_SIZE; i++) {
        // Convertir el valor a un rango de 0 a 255
        byte audioByte = map(i2sBuf[i], -32768, 32767, 0, 255);
        // Convertir el byte a una cadena hexadecimal
        String hexValue = String(audioByte, HEX);
        // Añadir el valor hexadecimal a los datos de audio
        audioData += hexValue;
        // Añadir un separador
        audioData += " ";
    }

    return audioData;
}

// Función para enviar audio a Google Cloud Speech-to-Text
String transcribeSpeech(String audioData, const char* apiKey) {
    // Código de transcripción como antes...
}

void setup() {
    Serial.begin(115200);
    connectToWiFi();
    setupI2S();
}

void loop() {
    // Capturar audio
    String audioData = captureAudio();

    // Transcribir discurso
    String transcribedText = transcribeSpeech(audioData, apiKey);

    // Imprimir la transcripción en el Monitor Serie
    Serial.println("Transcripción:");
    Serial.println(transcribedText);

    delay(5000); // Esperar un tiempo antes de la próxima transcripción
},\"target\":\"";
    jsonBody += targetLanguage;
    jsonBody += "\",\"format\":\"text\"}";

    int httpResponseCode = http.POST(jsonBody);

    String translatedText = "";

    if (httpResponseCode == HTTP_CODE_OK) {
        String response = http.getString();

        DynamicJsonDocument doc(1024);
        DeserializationError error = deserializeJson(doc, response);

        if (error) {
            Serial.print("deserializeJson() failed: ");
            Serial.println(error.c_str());
        } else {
            translatedText = doc["data"]["translations"][0]["translatedText"].as<String>();
        }
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();

    return translatedText;
}

void speakText(String text, const char* apiKey, const char* targetLanguage) {
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
        String audio = http.getString();
        dfplayer.playByteArray(audio.c_str(), audio.length()); // Reproducir el audio generado a través del DFPlayer Mini
    } else {
        Serial.print("Error en la solicitud: ");
        Serial.println(httpResponseCode);
    }

    http.end();
}

void loop() {
    String audioData = captureAudio();
    String transcribedText = transcribeSpeech(audioData, apiKey);
    Serial.println("Transcripción:");
    Serial.println(transcribedText);

    String translatedText = translateText(transcribedText, apiKey, "es");
    Serial.println("Traducción:");
    Serial.println(translatedText);

    speakText(translatedText, apiKey, "es-ES");

    delay(5000);
}
*/
