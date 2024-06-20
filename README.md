# Idioma - bridge 
## Objetivo: 
El objetivo de este proyecto es utilizar un ESP32 para realizar diversas tareas de procesamiento de audio y texto mediante servicios en la nube de Google. Esto incluye la grabación de audio desde un micrófono, su conversión a texto mediante reconocimiento de voz, la traducción de este texto a otro idioma, la síntesis de voz del texto traducido y la reproducción del audio resultante. Todo esto se controla y monitorea a través de una interfaz web alojada en el ESP32.
## Materiales: 
- ESP32-S3
- Micrófono y altavoz compatible con ESP32 I2S
- Tarjeta SD para almacenamiento y lector
- Librerias: 
```.ini
    lib_deps = 
        me-no-dev/AsyncTCP@^1.1.1
        ottowinter/ESPAsyncWebServer-esphome@^3.1.0
        bblanchon/ArduinoJson@^7.0.4
        esphome/ESP32-audioI2S@^2.0.7
        earlephilhower/ESP8266Audio@^1.9.7
        base64.h
        densaugeo/base64@^1.4.0
```
## Procedimiento: 
### General: 
**Codigo:**<br>
```cpp
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
```
**Descripción:**<br>
El proyecto utiliza un ESP32 conectado a una red WiFi para interactuar con servicios de Google Cloud mediante APIs. Aquí está la funcionalidad principal del sistema:
Conexión WiFi: El ESP32 se conecta automáticamente a una red WiFi especificada al encenderse, utilizando las credenciales proporcionadas.
Grabación de Audio: Utilizando pines configurados para entrada y salida de audio (I2S), el ESP32 captura audio desde un micrófono y lo guarda en una tarjeta SD como un archivo WAV.
Conversión de Audio a Texto: El archivo WAV grabado se convierte en texto utilizando la API de Google Cloud Speech-to-Text. El idioma del texto de salida puede configurarse dinámicamente a través de la interfaz web.
Traducción de Texto: El texto transcribido se traduce a otro idioma utilizando la API de Google Cloud Translation. Esta traducción se guarda en un archivo de texto en la tarjeta SD.
Síntesis de Voz: El texto traducido se convierte de nuevo en voz utilizando la API de Google Cloud Text-to-Speech. El archivo de audio resultante se guarda en la tarjeta SD.
Interfaz Web: Se implementa una interfaz web utilizando AsyncWebServer en el ESP32. Esta interfaz permite iniciar la grabación de audio, realizar la transcripción, traducción, síntesis de voz y reproducir el audio generado.
**Diagrama de flujos:**
```mermaid 
    graph TD
        A[ESP32] --> B[Conexión WiFi]
        B --> C[Inicialización SD]
        C --> D[Configuración I2S]
        D --> E[Grabación de Audio]
        E --> F[Guardar Audio en SD]
        F --> G[Interfaz Web]
        G --> H[Iniciar Grabación]
        H --> I[Convertir a Base64]
        I --> J[Google STT]
        J --> K[Obtener Texto Transcrito]
        K --> L[Guardar Transcripción en SD]
        L --> M[Obtener Idioma de Salida]
        M --> N[Google Translate]
        N --> O[Guardar Traducción en SD]
        O --> P[Obtener Idioma de Síntesis]
        P --> Q[Google TTS]
        Q --> R[Guardar Audio en SD]
        R --> S[Reproducir Audio]

        subgraph Google
            J[Google STT]
            N[Google Translate]
            Q[Google TTS]
        end
```
### Pagina web: 
**Codigo:**
```h
    #ifndef PAGINA_WEB_HPP
    #define PAGINA_WEB_HPP

    #include <Arduino.h>
    #include <WiFi.h>
    #include <ESPAsyncWebServer.h>

    void PaginaWeb(String &idioma1, String &idioma2);

    AsyncWebServer server(80);

    String idioma1 = "";
    String idioma2 = "";
    String texto1 = "Texto de ejemplo 1";
    String texto2 = "Texto de ejemplo 2";

    const char* index_html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="es">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>IDIOMA-BRIDGE</title>
        <style>
            body {
                background-color: #f0f0f0; /* Color de fondo principal */
                font-family: Arial, sans-serif;
                display: flex;
                justify-content: center;
                align-items: center;
                height: 100vh;
                margin: 0;
            }
            #container {
                background-color: #ffffff; /* Color de fondo del contenedor */
                border-radius: 15px; /* Bordes redondeados del contenedor */
                box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.1); /* Sombra suave */
                padding: 20px;
                width: 80%;
                max-width: 600px; /* Ancho máximo del contenedor */
                text-align: center;
            }
            h1 {
                color: #2e7d32; /* Color del título */
                font-size: 2.5em; /* Tamaño del título */
                margin-bottom: 20px;
            }
            .button-container {
                display: flex;
                justify-content: space-around;
                flex-wrap: wrap; /* Ajusta los botones en varias líneas si es necesario */
                margin-top: 20px;
            }
            .button {
                padding: 15px 30px; /* Tamaño de padding para hacer los botones más grandes */
                margin: 10px; /* Espacio entre botones */
                border-radius: 8px; /* Bordes redondeados de los botones */
                border: none;
                background-color: #4caf50; /* Color de fondo de los botones */
                color: white; /* Color del texto */
                font-size: 18px; /* Tamaño del texto */
                cursor: pointer;
                transition: background-color 0.3s ease; /* Transición suave del color de fondo */
            }
            .button:hover {
                background-color: #45a049; /* Color de fondo al pasar el mouse */
            }
        </style>
    </head>
    <body>
        <div id="container">
            <h1>IDIOMA-BRIDGE</h1>
            <div class="button-container">
                <button class="button" onclick="startRecording()">Hablar</button>
                <button class="button" onclick="speechToText()">Pasar a Texto</button>
                <button class="button" onclick="translateText()">Traducir</button>
                <button class="button" onclick="textToSpeech()">Pasar a Voz</button>
                <button class="button" onclick="playAudio()">Reproducir Audio</button>
            </div>
        </div>

        <script>
            function startRecording() {
                fetch('/grabar')
                    .then(response => {
                        if (response.ok) {
                            console.log("Grabación completada");
                        } else {
                            console.error("Error al iniciar la grabación");
                        }
                    });
            }

            function speechToText() {
                fetch('/transcribir')
                    .then(response => response.text())
                    .then(data => {
                        console.log("Transcripción completada: " + data);
                        // Aquí puedes manipular la transcripción como lo necesites
                    });
            }

            function translateText() {
                fetch('/traducir')
                    .then(response => response.text())
                    .then(data => {
                        console.log("Traducción y síntesis completadas: " + data);
                        // Aquí puedes manipular la traducción como lo necesites
                    });
            }

            function textToSpeech() {
                fetch('/textToSpeech')
                    .then(response => {
                        if (response.ok) {
                            console.log("Texto convertido a voz");
                        } else {
                            console.error("Error al convertir texto a voz");
                        }
                    });
            }

            function playAudio() {
                const audio = new Audio('/audio/output.mp3');
                audio.play();
            }
        </script>
    </body>
    </html>


    )rawliteral";

    #endif // PAGINA_WEB_HPP
```
**Descripción:**<br>
El archivo PaginaWeb.hpp define una página web estática que proporciona una interfaz para controlar diversas funciones de un dispositivo ESP32 conectado. Esta interfaz permite iniciar la grabación de audio, convertir el audio a texto mediante reconocimiento de voz, traducir el texto a otro idioma, convertir el texto traducido a voz y reproducir el audio generado. Está diseñada para integrarse con el sistema de hardware y software que incluye conexiones WiFi, tarjeta SD, y servicios de Google Cloud para el procesamiento de audio y texto.
**Foto de la página Web:**
<img src="PaginaWeb.jpg" width="800" height="400"> <br>

### Only talk and Base64:
**Codigo:**
```h
    #include <Arduino.h>
    #include "Audio.h"
    #include <SD.h>
    #include <SPI.h>
    #include <driver/i2s.h>
    #include "base64.hpp"

    #ifndef TALKBASE64_HPP  // Guardia de inclusión para evitar la inclusión múltiple
    #define TALKBASE64_HPP

    void setupI2S();
    void recordAudioToFile(const char* filename, int duration);
    void encodeAudioToBase64ToFile(const char* inputFilename, const char* outputFilename);

    // Configuración del I2S
    #define I2S_WS 16
    #define I2S_SCK 17
    #define I2S_SD 15

    // Configuración de la tarjeta SD
    #define SD_CS 39

    #define SAMPLE_RATE 8000  // Reducir la frecuencia de muestreo a 8000 Hz
    #define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
    #define CHANNEL_FORMAT I2S_CHANNEL_FMT_ONLY_LEFT
    #define BUFFER_SIZE 512  // Reducir el tamaño del buffer a 512 bytes

    void setupI2S() {
    // Configurar el pinout del I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = CHANNEL_FORMAT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    // Inicializar el I2S
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
    }
    void writeWavHeader(File file, int sampleRate, int bitsPerSample, int channels, int dataSize) {
    byte header[44];
    
    // Chunk ID "RIFF"
    header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
    
    // Chunk size
    int chunkSize = dataSize + 36;
    header[4] = (byte)(chunkSize & 0xFF);
    header[5] = (byte)((chunkSize >> 8) & 0xFF);
    header[6] = (byte)((chunkSize >> 16) & 0xFF);
    header[7] = (byte)((chunkSize >> 24) & 0xFF);
    
    // Format "WAVE"
    header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
    
    // Subchunk1 ID "fmt "
    header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
    
    // Subchunk1 size (16 for PCM)
    header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
    
    // Audio format (1 for PCM)
    header[20] = 1; header[21] = 0;
    
    // Number of channels
    header[22] = channels; header[23] = 0;
    
    // Sample rate
    header[24] = (byte)(sampleRate & 0xFF);
    header[25] = (byte)((sampleRate >> 8) & 0xFF);
    header[26] = (byte)((sampleRate >> 16) & 0xFF);
    header[27] = (byte)((sampleRate >> 24) & 0xFF);
    
    // Byte rate
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    header[28] = (byte)(byteRate & 0xFF);
    header[29] = (byte)((byteRate >> 8) & 0xFF);
    header[30] = (byte)((byteRate >> 16) & 0xFF);
    header[31] = (byte)((byteRate >> 24) & 0xFF);
    
    // Block align
    int blockAlign = channels * bitsPerSample / 8;
    header[32] = (byte)(blockAlign & 0xFF);
    header[33] = (byte)((blockAlign >> 8) & 0xFF);
    
    // Bits per sample
    header[34] = bitsPerSample; header[35] = 0;
    
    // Subchunk2 ID "data"
    header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
    
    // Subchunk2 size
    header[40] = (byte)(dataSize & 0xFF);
    header[41] = (byte)((dataSize >> 8) & 0xFF);
    header[42] = (byte)((dataSize >> 16) & 0xFF);
    header[43] = (byte)((dataSize >> 24) & 0xFF);
    
    // Write the header to the file
    file.write(header, 44);
    }

    void recordAudioToFile(const char* filename, int duration) {
    // Abrir el archivo para escribir
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Error al abrir el archivo para escribir");
        return;
    }

    // Escribir el encabezado WAV (con un tamaño de datos de 0 inicialmente)
    writeWavHeader(file, SAMPLE_RATE, 16, 1, 0);

    // Capturar y guardar audio
    int64_t endTime = millis() + duration;
    size_t bytesRead;
    uint8_t buffer[BUFFER_SIZE];
    int totalBytesWritten = 0;

    while (millis() < endTime) {
        i2s_read(I2S_NUM_0, buffer, BUFFER_SIZE, &bytesRead, portMAX_DELAY);
        file.write(buffer, bytesRead);
        totalBytesWritten += bytesRead;
    }

    // Actualizar el encabezado WAV con el tamaño correcto de datos
    file.seek(0);
    writeWavHeader(file, SAMPLE_RATE, 16, 1, totalBytesWritten);

    // Cerrar el archivo
    file.close();
    }
    void encodeAudioToBase64ToFile(const char* filename, const char* outputFilename) {
    // Abrir el archivo de audio
    File file = SD.open(filename);
    if (!file) {
        Serial.println("Error al abrir el archivo de audio");
        return;
    }

    // Crear el archivo de salida para guardar el base64
    File outputFile = SD.open(outputFilename, FILE_WRITE);
    if (!outputFile) {
        Serial.println("Error al crear el archivo de salida");
        file.close();
        return;
    }

    // Obtener el tamaño del archivo
    size_t fileSize = file.size();
    Serial.print("Tamaño del archivo WAV: ");
    Serial.println(fileSize);

    // Crear un buffer para almacenar el contenido del archivo
    uint8_t* buffer = (uint8_t*)malloc(fileSize);
    if (!buffer) {
        Serial.println("Error al asignar memoria para el buffer");
        file.close();
        outputFile.close();
        return;
    }

    // Leer el contenido del archivo en el buffer
    size_t bytesRead = file.read(buffer, fileSize);
    file.close();
    if (bytesRead != fileSize) {
        Serial.println("Error al leer el archivo de audio");
        free(buffer);
        outputFile.close();
        return;
    }

    // Calcular el tamaño necesario para la codificación base64
    unsigned int base64Length = encode_base64_length(fileSize);

    // Crear un buffer para la cadena codificada base64
    char* base64Buffer = (char*)malloc(base64Length + 1);
    if (!base64Buffer) {
        Serial.println("Error al asignar memoria para el buffer base64");
        free(buffer);
        outputFile.close();
        return;
    }

    // Codificar los datos del archivo WAV a base64
    encode_base64(buffer, fileSize, (unsigned char*)base64Buffer);
    base64Buffer[base64Length] = '\0'; // Agregar terminador nulo al final de la cadena base64

    // Escribir la cadena base64 en el archivo de salida
    outputFile.print(base64Buffer);

    // Liberar la memoria del buffer original y del buffer base64
    free(buffer);
    free(base64Buffer);

    // Cerrar el archivo de salida
    outputFile.close();

    Serial.println("Archivo de audio codificado en base64 y guardado en el archivo:");
    Serial.println(outputFilename);
    }
    #endif  // TALKBASE64_HPP

```
**Descripción:**<br>
El archivo TalkBase64.hpp contiene funciones esenciales para la captura de audio utilizando el ESP32, almacenamiento en una tarjeta SD, y codificación de archivos de audio en formato WAV a base64. También define la configuración del bus I2S necesario para la captura de audio.
En resumen, las funciones principales son ``setupI2S()``, ``recordAudioToFile(const char* filename, int duration)`` y ``encodeAudioToBase64ToFile(const char* filename, const char* outputFilename)``. Además, se incluyen definiciones y configuraciones relacionadas con el I2S y la gestión de archivos.
El archivo utiliza una guardia de inclusión ``#ifndef TALKBASE64_HPP`` para evitar la inclusión múltiple durante la compilación.

### Only speech to text: 
**Código:**
```h
    #include <Arduino.h>
    #include <WiFi.h>
    #include <WiFiClientSecure.h>
    #include <HTTPClient.h>
    #include <ArduinoJson.h>
    #include "Audio.h"
    #include <SD.h>
    #include <Audio.h>
    #include <SPI.h>
    #include <Base64.h>

    #ifndef SPEECHTOTEXT_HPP  // Guardia de inclusión para evitar la inclusión múltiple
    #define SPEECHTOTEXT_HPP

    bool obtenerAudioDesdeSD(char* &audioBase64);
    const char* transcribeSpeech(char* audioBase64, const char* apiKey, String idioma_target);
    bool obtenerAudioDesdeSD(char* &audioBase64) {
        // Abrir el archivo de texto con el contenido Base64
        File archivo = SD.open("/grabacion64.txt", FILE_READ);
        if (!archivo) {
            Serial.println("Error al abrir el archivo de texto");
            return false;
        }

        // Leer el contenido del archivo de texto y almacenar en un buffer dinámico
        int fileSize = archivo.size();
        audioBase64 = (char*)malloc(fileSize + 1);
        if (audioBase64 == nullptr) {
            Serial.println("Error al asignar memoria");
            archivo.close();
            return false;
        }

        int index = 0;
        while (archivo.available()) {
            char c = archivo.read();
            // Solo agregar caracteres válidos de Base64
            if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '+' || c == '/' || c == '=') {
                audioBase64[index++] = c;
            }
        }
        audioBase64[index] = '\0'; // Asegurar que la cadena está terminada en null

        // Cerrar el archivo
        archivo.close();
        // Serial.println(audioBase64);
        return true;
    }
    const char* transcribeSpeech(char* audioBase64, const char* apiKey, String idioma_target) {
        HTTPClient http;

        // URL de la API de Google Cloud Speech-to-Text
        String url = "https://speech.googleapis.com/v1/speech:recognize?key=";
        url += apiKey;

        //Serial.println(url);

        // Configurar la solicitud HTTP
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        String transcribedText;

        // Crear el cuerpo de la solicitud JSON
        String jsonBody = "{\"config\": {\"encoding\":\"MP3\",\"sampleRateHertz\":16000, \"languageCode\": \"";
        jsonBody += idioma_target;
        jsonBody += "\"},\"audio\": {\"content\":\"";
        jsonBody += audioBase64;
        jsonBody += "\"}}";
        
        //Serial.println(jsonBody);

        // Enviar la solicitud POST con el cuerpo JSON
        int httpResponseCode = http.POST(jsonBody);

        // Si la solicitud fue exitosa, obtener la transcripción
        if (httpResponseCode == HTTP_CODE_OK) {
            String response = http.getString();

            // Analizar la respuesta JSON para obtener la transcripción
            DynamicJsonDocument doc(4096); // Aumentar el tamaño del documento si es necesario
            DeserializationError error = deserializeJson(doc, response);

            if (error) {
                Serial.print("deserializeJson() failed: ");
                Serial.println(error.c_str());
            } else {
                if (doc.containsKey("results")) {
                    JsonObject result = doc["results"][0];
                    if (result.containsKey("alternatives")) {
                        JsonObject alternative = result["alternatives"][0];
                        if (alternative.containsKey("transcript")) {
                            transcribedText += alternative["transcript"].as<String>();
                        }
                    }
                } else {
                    Serial.println("No se encontró el campo 'results' en la respuesta JSON");
                }
            }
            Serial.println(response);
        } else {
            Serial.print("Error en la solicitud: ");
            Serial.println(httpResponseCode);
        }

        // Liberar los recursos
        http.end();

        // Asignar memoria para el texto transcrito y copiar la cadena
        char* transcriptionBuffer = (char*)malloc(transcribedText.length() + 1);
        if (transcriptionBuffer) {
            strcpy(transcriptionBuffer, transcribedText.c_str());
        }

        free(audioBase64);
        audioBase64 = nullptr;

        return transcriptionBuffer;
    }

    /*
    void loop() {
    // put your main code here, to run repeatedly:
    }
    */

    #endif 
```
**Descripción:**<br> 
El archivo SpeechToText.hpp proporciona funciones para interactuar con la API de Google Cloud Speech-to-Text. Está diseñado para obtener un archivo de audio codificado en base64 desde una tarjeta SD, enviarlo a la API de Google Cloud para la transcripción del habla, y devolver el texto transcrito como resultado.
Las funciones principales son ``obtenerAudioDesdeSD(char* &audioBase64)`` y ``transcribeSpeech(char* audioBase64, const char* apiKey, String idioma_target)``. La primera función lee un archivo de texto de la tarjeta SD que contiene el audio en formato base64. La segunda función realiza una solicitud HTTP POST a la API de Google Cloud Speech-to-Text con el audio codificado y recibe el texto transcrito como respuesta.
Se utiliza la biblioteca HTTPClient.h para manejar las solicitudes HTTP y ArduinoJson.h para analizar las respuestas JSON de la API. Además, se define una guardia de inclusión ``#ifndef SPEECHTOTEXT_HPP`` para evitar la inclusión múltiple durante la compilación.
Estas funciones son esenciales para integrar la transcripción de habla en proyectos con ESP32 que requieren la conversión de audio en texto utilizando servicios de reconocimiento de voz en la nube como Google Cloud.

### Only translate: 
**Código:**
```h
    #include <Arduino.h>
    #include <WiFi.h>
    #include <WiFiClientSecure.h>
    #include <HTTPClient.h>
    #include <ArduinoJson.h>
    #include "Audio.h"
    #include <SD.h>
    #include <Audio.h>
    #include <SPI.h>
    #include <Base64.h>

    #ifndef TRANSLATE_HPP  // Guardia de inclusión para evitar la inclusión múltiple
    #define TRANSLATE_HPP

    // String getLanguageCode(const char* languageName);
    String translateText(String text, const char* apiKey, const char* targetLanguage);

    
    // TRADUCTOR

    String translateText(String text, const char* apiKey, const char* targetLanguage) {
        HTTPClient http;

        String url = "https://translation.googleapis.com/language/translate/v2?key=";
        url += apiKey;
        http.begin(url);
        http.addHeader("Content-Type", "application/json");

        String jsonBody = "{\"q\":\"";
        jsonBody += text;
        jsonBody += "\",\"target\":\"";
        jsonBody += targetLanguage;
        jsonBody += "\",\"format\":\"text\"}";  // No especificar "source" para detección automática

        int httpResponseCode = http.POST(jsonBody);

        String translatedText = "";

        if (httpResponseCode == HTTP_CODE_OK) {
            String response = http.getString();
            // Parsear la respuesta JSON para obtener la traducción
            StaticJsonDocument<1024> doc;
            DeserializationError error = deserializeJson(doc, response);
            //DynamicJsonDocument doc(1024);
            //DeserializationError error = deserializeJson(doc, response);

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

    #endif 
```
**Descripción:**<br>
El archivo Translate.hpp proporciona funciones para realizar traducciones de texto utilizando la API de Google Cloud Translation. Está diseñado para enviar solicitudes HTTP POST con el texto a traducir y recibir la traducción correspondiente como respuesta en formato JSON.
La función principal es ``translateText(String text, const char* apiKey, const char* targetLanguage)``, que acepta el texto a traducir, la clave API de Google Cloud Translation y el código de idioma de destino como parámetros. Esta función construye un cuerpo JSON para la solicitud, envía la solicitud a la API y luego procesa la respuesta JSON para extraer y devolver el texto traducido.

### Only text to speech and Base64: 
**Código:**
```h
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

    const char* speakText(const char* text, const char* apiKey, String targetLanguage);
    void decode_base64_to_file(const char* base64_input, const char* output_file);


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

    #endif 

```
**Descripción:**<br>
El archivo TextToSpeech.hpp proporciona funciones para interactuar con la API de Google Cloud Text-to-Speech desde un dispositivo Arduino o ESP32. Su objetivo principal es convertir texto en voz y manejar el resultado de audio utilizando la tarjeta SD.
La función ``speakText`` se encarga de enviar una solicitud HTTP POST a la API de Google Cloud Text-to-Speech con el texto que se desea convertir en voz. Utiliza la clave API proporcionada para autenticarse en el servicio y especifica el idioma de destino para la voz generada. La respuesta de la API contiene el audio codificado en formato base64, el cual se procesa y devuelve como un buffer de caracteres.
La función ``decode_base64_to_file`` decodifica una cadena base64 que representa audio y guarda el resultado como un archivo de audio en la tarjeta SD. Esta función es crucial para convertir y almacenar el audio generado por la API en un formato reproducible.
Además de estas funciones, el archivo incluye guardias de inclusión para evitar la inclusión múltiple durante la compilación, así como algunas funciones y variables adicionales que podrían haber sido parte de una implementación más amplia o estar comentadas para uso futuro.
En conjunto, este archivo facilita la integración de capacidades de conversión de texto a voz basadas en la nube en proyectos de hardware que utilizan Arduino o ESP32, proporcionando una forma eficiente de manejar la generación y almacenamiento de audio a partir de texto.

### Only listen: 
**Código:**
```h
    #include <Arduino.h>
    #include "Audio.h"
    #include <SD.h>
    #include <SPI.h>
    #include <driver/i2s.h>
    #include "base64.hpp"

    #ifndef TALKBASE64_HPP  // Guardia de inclusión para evitar la inclusión múltiple
    #define TALKBASE64_HPP

    void setupI2S();
    void recordAudioToFile(const char* filename, int duration);
    void encodeAudioToBase64ToFile(const char* inputFilename, const char* outputFilename);

    // Configuración del I2S
    #define I2S_WS 16
    #define I2S_SCK 17
    #define I2S_SD 15

    // Configuración de la tarjeta SD
    #define SD_CS 39

    #define SAMPLE_RATE 8000  // Reducir la frecuencia de muestreo a 8000 Hz
    #define BITS_PER_SAMPLE I2S_BITS_PER_SAMPLE_16BIT
    #define CHANNEL_FORMAT I2S_CHANNEL_FMT_ONLY_LEFT
    #define BUFFER_SIZE 512  // Reducir el tamaño del buffer a 512 bytes

    void setupI2S() {
    // Configurar el pinout del I2S
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = BITS_PER_SAMPLE,
        .channel_format = CHANNEL_FORMAT,
        .communication_format = I2S_COMM_FORMAT_I2S_MSB,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 4,
        .dma_buf_len = BUFFER_SIZE,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_SCK,
        .ws_io_num = I2S_WS,
        .data_out_num = I2S_PIN_NO_CHANGE,
        .data_in_num = I2S_SD
    };

    // Inicializar el I2S
    i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL);
    i2s_set_pin(I2S_NUM_0, &pin_config);
    i2s_zero_dma_buffer(I2S_NUM_0);
    }

    void writeWavHeader(File file, int sampleRate, int bitsPerSample, int channels, int dataSize) {
    byte header[44];
    
    // Chunk ID "RIFF"
    header[0] = 'R'; header[1] = 'I'; header[2] = 'F'; header[3] = 'F';
    
    // Chunk size
    int chunkSize = dataSize + 36;
    header[4] = (byte)(chunkSize & 0xFF);
    header[5] = (byte)((chunkSize >> 8) & 0xFF);
    header[6] = (byte)((chunkSize >> 16) & 0xFF);
    header[7] = (byte)((chunkSize >> 24) & 0xFF);
    
    // Format "WAVE"
    header[8] = 'W'; header[9] = 'A'; header[10] = 'V'; header[11] = 'E';
    
    // Subchunk1 ID "fmt "
    header[12] = 'f'; header[13] = 'm'; header[14] = 't'; header[15] = ' ';
    
    // Subchunk1 size (16 for PCM)
    header[16] = 16; header[17] = 0; header[18] = 0; header[19] = 0;
    
    // Audio format (1 for PCM)
    header[20] = 1; header[21] = 0;
    
    // Number of channels
    header[22] = channels; header[23] = 0;
    
    // Sample rate
    header[24] = (byte)(sampleRate & 0xFF);
    header[25] = (byte)((sampleRate >> 8) & 0xFF);
    header[26] = (byte)((sampleRate >> 16) & 0xFF);
    header[27] = (byte)((sampleRate >> 24) & 0xFF);
    
    // Byte rate
    int byteRate = sampleRate * channels * bitsPerSample / 8;
    header[28] = (byte)(byteRate & 0xFF);
    header[29] = (byte)((byteRate >> 8) & 0xFF);
    header[30] = (byte)((byteRate >> 16) & 0xFF);
    header[31] = (byte)((byteRate >> 24) & 0xFF);
    
    // Block align
    int blockAlign = channels * bitsPerSample / 8;
    header[32] = (byte)(blockAlign & 0xFF);
    header[33] = (byte)((blockAlign >> 8) & 0xFF);
    
    // Bits per sample
    header[34] = bitsPerSample; header[35] = 0;
    
    // Subchunk2 ID "data"
    header[36] = 'd'; header[37] = 'a'; header[38] = 't'; header[39] = 'a';
    
    // Subchunk2 size
    header[40] = (byte)(dataSize & 0xFF);
    header[41] = (byte)((dataSize >> 8) & 0xFF);
    header[42] = (byte)((dataSize >> 16) & 0xFF);
    header[43] = (byte)((dataSize >> 24) & 0xFF);
    
    // Write the header to the file
    file.write(header, 44);
    }

    void recordAudioToFile(const char* filename, int duration) {
    // Abrir el archivo para escribir
    File file = SD.open(filename, FILE_WRITE);
    if (!file) {
        Serial.println("Error al abrir el archivo para escribir");
        return;
    }

    // Escribir el encabezado WAV (con un tamaño de datos de 0 inicialmente)
    writeWavHeader(file, SAMPLE_RATE, 16, 1, 0);

    // Capturar y guardar audio
    int64_t endTime = millis() + duration;
    size_t bytesRead;
    uint8_t buffer[BUFFER_SIZE];
    int totalBytesWritten = 0;

    while (millis() < endTime) {
        i2s_read(I2S_NUM_0, buffer, BUFFER_SIZE, &bytesRead, portMAX_DELAY);
        file.write(buffer, bytesRead);
        totalBytesWritten += bytesRead;
    }

    // Actualizar el encabezado WAV con el tamaño correcto de datos
    file.seek(0);
    writeWavHeader(file, SAMPLE_RATE, 16, 1, totalBytesWritten);

    // Cerrar el archivo
    file.close();
    }

    void encodeAudioToBase64ToFile(const char* filename, const char* outputFilename) {
    // Abrir el archivo de audio
    File file = SD.open(filename);
    if (!file) {
        Serial.println("Error al abrir el archivo de audio");
        return;
    }

    // Crear el archivo de salida para guardar el base64
    File outputFile = SD.open(outputFilename, FILE_WRITE);
    if (!outputFile) {
        Serial.println("Error al crear el archivo de salida");
        file.close();
        return;
    }

    // Obtener el tamaño del archivo
    size_t fileSize = file.size();
    Serial.print("Tamaño del archivo WAV: ");
    Serial.println(fileSize);

    // Crear un buffer para almacenar el contenido del archivo
    uint8_t* buffer = (uint8_t*)malloc(fileSize);
    if (!buffer) {
        Serial.println("Error al asignar memoria para el buffer");
        file.close();
        outputFile.close();
        return;
    }

    // Leer el contenido del archivo en el buffer
    size_t bytesRead = file.read(buffer, fileSize);
    file.close();
    if (bytesRead != fileSize) {
        Serial.println("Error al leer el archivo de audio");
        free(buffer);
        outputFile.close();
        return;
    }

    // Calcular el tamaño necesario para la codificación base64
    unsigned int base64Length = encode_base64_length(fileSize);

    // Crear un buffer para la cadena codificada base64
    char* base64Buffer = (char*)malloc(base64Length + 1);
    if (!base64Buffer) {
        Serial.println("Error al asignar memoria para el buffer base64");
        free(buffer);
        outputFile.close();
        return;
    }

    // Codificar los datos del archivo WAV a base64
    encode_base64(buffer, fileSize, (unsigned char*)base64Buffer);
    base64Buffer[base64Length] = '\0'; // Agregar terminador nulo al final de la cadena base64

    // Escribir la cadena base64 en el archivo de salida
    outputFile.print(base64Buffer);

    // Liberar la memoria del buffer original y del buffer base64
    free(buffer);
    free(base64Buffer);

    // Cerrar el archivo de salida
    outputFile.close();

    Serial.println("Archivo de audio codificado en base64 y guardado en el archivo:");
    Serial.println(outputFilename);
    }

    #endif  // TALKBASE64_HPP

```
**Descripción:**<br>
El archivo TalkBase64.hpp está diseñado para facilitar la captura de audio a través del protocolo I2S, su almacenamiento en la tarjeta SD en formato WAV, y la posterior codificación de estos archivos WAV en formato base64. Inicia incluyendo las bibliotecas esenciales como Arduino.h, Audio.h, SD.h, SPI.h, driver/i2s.h, y "base64.hpp" para asegurar la disponibilidad de las funciones y definiciones necesarias para la manipulación de audio, comunicación SPI, y codificación base64.
Una vez incluidas las bibliotecas, el archivo define una guardia de inclusión ``#ifndef TALKBASE64_HPP`` para evitar la inclusión múltiple durante la compilación.
Las funciones principales del archivo son ``setupI2S()``, que configura y prepara el bus I2S para la captura de audio. Esta función define parámetros críticos como la frecuencia de muestreo, tamaño de bits por muestra, formato de canal, y tamaño del buffer DMA necesario para manejar los datos de audio de manera eficiente.
La función ``recordAudioToFile(const char* filename, int duration)`` permite grabar audio desde el bus I2S durante una duración especificada en milisegundos y almacenarlo en un archivo en la tarjeta SD. Utiliza el formato WAV estándar, donde primero escribe un encabezado WAV con un tamaño de datos inicial de 0, el cual se actualiza después de la grabación.
Otra función clave es ``encodeAudioToBase64ToFile(const char* inputFilename, const char* outputFilename)``, que toma un archivo de audio en formato WAV desde la tarjeta SD, lo codifica en base64, y guarda la cadena resultante en un archivo de salida. Este proceso incluye la lectura del archivo WAV, la asignación de memoria para el buffer de datos, la codificación base64 del contenido, y la escritura de la cadena base64 en el archivo de salida.
El archivo también establece configuraciones específicas como la configuración de pines para el I2S (I2S_WS, I2S_SCK, I2S_SD) y para la tarjeta SD (SD_CS), junto con constantes para la frecuencia de muestreo, tamaño de buffer y otros parámetros necesarios para la manipulación de audio.
En resumen, TalkBase64.hpp proporciona una infraestructura completa para la captura, almacenamiento en la tarjeta SD en formato WAV, y codificación en base64 de audio capturado a través del bus I2S. Es útil en aplicaciones que requieren la manipulación de audio en dispositivos embebidos basados en Arduino o ESP32.

## Conclusión: 
A partir de la página web y las funciones implementadas, podemos hablar a través de un micrófono, pasar ese audio a texto, traducirlo, pasar la traducción a audio y sacarlo a través de un altavoz. 