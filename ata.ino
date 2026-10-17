#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// --- PIN CONFIGURATION ---
const int JOY_X      = A0;
const int JOY_Y      = A1;
const int JOY_SW     = 5; 
const int BTN_CYCLE  = 2; 
const int BTN_SHOOT  = 3; 
const int BUZZER_PIN = 6; 

// --- WORLD GRID CHUNKS ---
#define CHUNK_SIZE 64      
#define WORLD_CHUNKS_X 8   
#define WORLD_CHUNKS_Y 8   

const byte WORLD_MAP[WORLD_CHUNKS_Y][WORLD_CHUNKS_X] = {
  {1,1,1,1,1,1,1,1},
  {1,0,0,0,1,0,0,1},
  {1,0,1,0,0,0,1,1},
  {1,0,1,1,0,1,0,1},
  {1,0,0,0,0,1,0,1},
  {1,1,0,1,0,0,0,1},
  {1,0,0,1,0,1,0,1},
  {1,1,1,1,1,1,1,1}
};

// --- RETRO SPRITES (8x8 px) ---
const unsigned char PROGMEM player_car_sprite[] = {
  0x18, 0x3C, 0x5A, 0x7E, 0x7E, 0x5A, 0x3C, 0x18
};
const unsigned char PROGMEM police_sprite[] = {
  0x3C, 0x7E, 0x66, 0xFF, 0xFF, 0x66, 0x7E, 0x3C
};

// Game System States
bool inMenu = false;
bool inShop = false;

// Player Attributes
float worldX = 256.0, worldY = 256.0; 
float carAngle = 0.0;
float carSpeed = 0.0;
const float MAX_SPEED = 2.2;
const float FRICTION = 0.90;
long cash = 1200; 
int health = 100;
int wantedStars = 0; 
int civilKills = 0;
int policeKills = 0;  

// UPDATED WEAPON WHEEL & PRICES
int currentWeaponIndex = 0; 
const char* weaponNames[] = {"FISTS", "GLOCK", "M1911", "AK47", "M16", "ULTRA RPG"};
const int weaponPrices[]  = {0, 150, 250, 700, 1000, 2500};

// Bullet System
bool bulletActive = false;
float bulletX, bulletY, bulletVx, bulletVy;

// Store State
int shopSelection = 1; 

// Dynamic NPCs
float civilianX = 150.0, civilianY = 160.0;
bool civilianAlive = true;
bool civilianAggressive = false; 

float copWorldX = 80.0, copWorldY = 80.0;
bool copAlive = false; 
bool copIsSwat = false; 

// Prototypes
void showPDAMenu();
void showAmmuNationUI();
void renderChinatownGraphics();
void triggerRespawn(bool isBusted);

void setup() {
  pinMode(JOY_SW, INPUT_PULLUP);
  pinMode(BTN_CYCLE, INPUT_PULLUP);
  pinMode(BTN_SHOOT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { for(;;); }
  display.clearDisplay();
  
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(4, 8);    display.println("ATA 1");
  display.setTextSize(1);
  display.setCursor(4, 30);   display.println("ArduTheftAuto 1");
  display.setCursor(4, 42);   display.println("V 1.0 (bugs!)");
  display.display();
  delay(2500);
}

void loop() {
  // --- 1. WEAPON WHEEL SWITCH (BUTTON 1) ---
  if (digitalRead(BTN_CYCLE) == LOW && !inMenu) {
    currentWeaponIndex++;
    if (currentWeaponIndex > 5) currentWeaponIndex = 0; 
    tone(BUZZER_PIN, 700, 40); delay(40); tone(BUZZER_PIN, 900, 30);
    delay(200); 
  }

  // PDA Menu trigger
  if (digitalRead(JOY_SW) == LOW) {
    if (inShop) inShop = false;
    else inMenu = !inMenu;
    tone(BUZZER_PIN, 500, 50);
    delay(250);
  }
  if (inMenu) {
    if (inShop) showAmmuNationUI();
    else showPDAMenu();
    return;
  }

  // --- 2. 360° SCROLLING ENGINE & MAP OVERLAYS ---
  int xVal = analogRead(JOY_X);
  int yVal = analogRead(JOY_Y);
  float dx = (xVal - 512) / 512.0;
  float dy = (yVal - 512) / 512.0;

  if (abs(dx) > 0.15 || abs(dy) > 0.15) {
    carAngle = atan2(dy, dx);
    carSpeed += 0.15;
    if (carSpeed > MAX_SPEED) carSpeed = MAX_SPEED;
    if (millis() % 90 < 20) {
      tone(BUZZER_PIN, 120 + (carSpeed * 35), 8); 
    }
  } else {
    carSpeed *= FRICTION;
  }

  float nextX = worldX + cos(carAngle) * carSpeed;
  float nextY = worldY + sin(carAngle) * carSpeed;

  int chunkIdxX = (int)(nextX / CHUNK_SIZE);
  int chunkIdxY = (int)(nextY / CHUNK_SIZE);

  if (chunkIdxX >= 0 && chunkIdxX < WORLD_CHUNKS_X && chunkIdxY >= 0 && chunkIdxY < WORLD_CHUNKS_Y) {
    if (WORLD_MAP[chunkIdxY][chunkIdxX] == 0) {
      worldX = nextX;
      worldY = nextY;
    } else {
      worldX -= cos(carAngle) * (carSpeed + 2.0);
      worldY -= sin(carAngle) * (carSpeed + 2.0);
      carSpeed = -0.4;
      tone(BUZZER_PIN, 160, 50);
    }
  }

  // --- 3. DYNAMIC COMBAT SYSTEM ---
  if (digitalRead(BTN_SHOOT) == LOW && !bulletActive) {
    bulletActive = true;
    bulletX = worldX;
    bulletY = worldY;
    
    float projectileSpeed = 4.5;
    if (currentWeaponIndex == 0) projectileSpeed = 1.5; 
    if (currentWeaponIndex == 5) projectileSpeed = 3.0; 
    
    bulletVx = cos(carAngle) * projectileSpeed;
    bulletVy = sin(carAngle) * projectileSpeed;
    
    switch(currentWeaponIndex) {
      case 0: tone(BUZZER_PIN, 200, 30); break;   
      case 1: case 2: tone(BUZZER_PIN, 1100, 25); break;  
      case 3: tone(BUZZER_PIN, 1650, 30); break;  
      case 4: tone(BUZZER_PIN, 1900, 20); break;  
      case 5: tone(BUZZER_PIN, 300, 200); break;  
    }
  }

  if (bulletActive) {
    bulletX += bulletVx; bulletY += bulletVy;
    
    int maxRange = 90;
    if (currentWeaponIndex == 0) maxRange = 12; 
    if (abs(bulletX - worldX) > maxRange || abs(bulletY - worldY) > maxRange) bulletActive = false;

    // Hit Registration: Civilian
    if (civilianAlive && abs(bulletX - civilianX) < 10 && abs(bulletY - civilianY) < 10) {
      bulletActive = false;
      civilianAlive = false;
      cash += random(100, 300); 
      civilKills++;
      tone(BUZZER_PIN, 950, 120);
    }

    // Hit Registration: Police / SWAT
    if (copAlive && abs(bulletX - copWorldX) < 10 && abs(bulletY - copWorldY) < 10) {
      bulletActive = false;
      copAlive = false;
      cash += 800; 
      policeKills++;
      tone(BUZZER_PIN, 1250, 180);
    }
  }

  // --- 4. 4-STAR HEAT SCALE LOGIC ---
  if (policeKills > 0) {
    wantedStars = (policeKills == 1) ? 3 : 4;
    copIsSwat = true;
  } else if (civilKills > 0) {
    wantedStars = (civilKills == 1) ? 1 : 2;
    copIsSwat = false;
  } else {
    wantedStars = 0;
    copIsSwat = false;
  }

  // --- 5. ENEMY RETALIATION & ARREST AI ---
  if (civilianAlive) {
    float civDist = sqrt(pow(worldX - civilianX, 2) + pow(worldY - civilianY, 2));
    if (!civilianAggressive && civDist < 35 && wantedStars > 0 && random(0, 100) < 6) {
      civilianAggressive = true; 
    }
    if (civilianAggressive && civDist > 5) {
      civilianX += (worldX > civilianX ? 0.35 : -0.35);
      civilianY += (worldY > civilianY ? 0.35 : -0.35);
    }
    if (civilianAggressive && civDist <= 6.5) {
      health -= 2; 
      tone(BUZZER_PIN, 140, 25);
      if (health <= 0) triggerRespawn(true); 
    }
  } else {
    if (random(0, 220) == 25) {
      civilianX = worldX + random(-60, 60); civilianY = worldY + random(-60, 60);
      civilianAlive = true; civilianAggressive = false;
    }
  }

  if (wantedStars > 0) {
    copAlive = true;
    float copDx = worldX - copWorldX; float copDy = worldY - copWorldY;
    float copDistance = sqrt(copDx*copDx + copDy*copDy);
    float copVelocity = 0.30 + (wantedStars * 0.15); 
    
    if(copDistance > 4) { 
      copWorldX += (copDx / copDistance) * copVelocity; 
      copWorldY += (copDy / copDistance) * copVelocity; 
    }

    if (copDistance < 6.5) {
      if (wantedStars <= 2) {
        triggerRespawn(false); 
      } else {
        health -= (8 * wantedStars); 
        tone(BUZZER_PIN, 135, 60);
        copWorldX = worldX + random(-100, -70); copWorldY = worldY + random(-100, -70);
        if(health <= 0) { 
          triggerRespawn(true); 
        }
      }
    }
  } else {
    copAlive = false;
  }

  renderChinatownGraphics();
}

// --- SYSTEM GAME OVER / RESPAWN HANDLER ---
void triggerRespawn(bool isBusted) {
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  
  if (!isBusted) {
    display.setCursor(25, 24); display.print("JAILED!");
    display.display();
    tone(BUZZER_PIN, 150, 600); delay(150); tone(BUZZER_PIN, 100, 800);
    delay(2500);
    cash = max(0L, cash - 300);
    worldX = 80.0; worldY = 80.0; 
  } else {
    display.setCursor(25, 24); display.print("BUSTED!");
    display.display();
    tone(BUZZER_PIN, 90, 1000);
    delay(2500);
    cash = max(0L, cash - 500);
    worldX = 256.0; worldY = 256.0; 
  }

  health = 100; 
  civilKills = 0; 
  policeKills = 0; 
  wantedStars = 0;
  carSpeed = 0;
  bulletActive = false;
}

// --- INTERFACE HUD & STORES ---

void showPDAMenu() {
  if (digitalRead(BTN_CYCLE) == LOW) {
    inShop = true;
    tone(BUZZER_PIN, 800, 60);
    delay(250);
    return;
  }
  display.clearDisplay();
  display.setTextSize(1); display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);   display.print("= PDA SYSTEM OVERVIEW =");
  display.setCursor(0, 14);  display.print("CASH: $"); display.print(cash);
  display.setCursor(0, 24);  display.print("EQUIP: "); display.print(weaponNames[currentWeaponIndex]);
  display.setCursor(0, 34);  display.print("CIV KILLS: "); display.print(civilKills);
  display.setCursor(0, 44);  display.print("COP KILLS: "); display.print(policeKills);
  display.setCursor(0, 56);  display.print("> PRESS BTN1 FOR SHOP");
  display.display();
}

void showAmmuNationUI() {
  int yVal = analogRead(JOY_Y);
  if (yVal < 200) { shopSelection--; delay(150); }
  if (yVal > 800) { shopSelection++; delay(150); }
  if (shopSelection < 1) shopSelection = 5; 
  if (shopSelection > 5) shopSelection = 1;

  if (digitalRead(BTN_SHOOT) == LOW) {
    if (cash >= weaponPrices[shopSelection]) {
      cash -= weaponPrices[shopSelection];
      currentWeaponIndex = shopSelection; 
      tone(BUZZER_PIN, 1100, 80); delay(80); tone(BUZZER_PIN, 1400, 120);
    } else {
      tone(BUZZER_PIN, 100, 250); 
    }
    delay(250);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(4, 2); display.print("AMMU-NATION ARMORY");
  
  display.drawRect(6, 14, 116, 32, SSD1306_WHITE);
  display.setCursor(12, 19); display.print("WEAPON: "); display.print(weaponNames[shopSelection]);
//Folosește codul cu precauție.
display.setCursor(12, 33); display.print("PRICE:  $"); display.print(weaponPrices[shopSelection]);
display.setCursor(6, 51);  display.print("CASH:$"); display.print(cash);
display.setCursor(76, 51); display.print("[BTN2]=BUY");
display.display();
}
void renderChinatownGraphics() {
display.clearDisplay();
int offsetX = (SCREEN_WIDTH / 2) - (int)worldX;
int offsetY = (SCREEN_HEIGHT / 2) - (int)worldY;
for (int y = 0; y < WORLD_CHUNKS_Y; y++) {
for (int x = 0; x < WORLD_CHUNKS_X; x++) {
int blockX = (x * CHUNK_SIZE) + offsetX;
int blockY = (y * CHUNK_SIZE) + offsetY;
if (WORLD_MAP[y][x] == 1) {
if (blockX + CHUNK_SIZE >= 0 && blockX < SCREEN_WIDTH && blockY + CHUNK_SIZE >= 0 && blockY < SCREEN_HEIGHT) {
display.drawRect(blockX, blockY, CHUNK_SIZE, CHUNK_SIZE, SSD1306_WHITE);
display.drawCircle(blockX + CHUNK_SIZE/2, blockY + CHUNK_SIZE/2, 3, SSD1306_WHITE);
}
}
}
}
if (civilianAlive) {
int civScrX = (int)civilianX + offsetX;
int civScrY = (int)civilianY + offsetY;
if (civScrX >= 0 && civScrX < SCREEN_WIDTH && civScrY >= 0 && civScrY < SCREEN_HEIGHT) {
display.drawCircle(civScrX, civScrY, 2, SSD1306_WHITE);
}
}
if (copAlive) {
int copScrX = (int)copWorldX + offsetX - 4;
int copScrY = (int)copWorldY + offsetY - 4;
if (copScrX + 8 >= 0 && copScrX < SCREEN_WIDTH && copScrY + 8 >= 0 && copScrY < SCREEN_HEIGHT) {
display.drawBitmap(copScrX, copScrY, police_sprite, 8, 8, SSD1306_WHITE);
if (copIsSwat) {
display.drawFastVLine(copScrX + 4, copScrY - 4, 3, SSD1306_WHITE);
}
}
}
if (bulletActive) {
int bulScrX = (int)bulletX + offsetX;
int bulScrY = (int)bulletY + offsetY;
if (bulScrX >= 0 && bulScrX < SCREEN_WIDTH && bulScrY >= 0 && bulScrY < SCREEN_HEIGHT) {
display.drawPixel(bulScrX, bulScrY, SSD1306_WHITE);
}
}
int playerScrX = (SCREEN_WIDTH / 2) - 4;
int playerScrY = (SCREEN_HEIGHT / 2) - 4;
display.drawBitmap(playerScrX, playerScrY, player_car_sprite, 8, 8, SSD1306_WHITE);
display.fillRect(0, 0, SCREEN_WIDTH, 10, SSD1306_BLACK);
display.drawFastHLine(0, 10, SCREEN_WIDTH, SSD1306_WHITE);
display.setTextSize(1);
display.setCursor(2, 1);  display.print("HP:"); display.print(health);
display.setCursor(42, 1); display.print("$:"); display.print(cash);
display.setCursor(96, 1);
for(int i = 0; i < wantedStars; i++) {
display.print("*");
}
int mmX = 102;
int mmY = 38;
display.fillRect(mmX - 2, mmY - 2, 28, 28, SSD1306_BLACK);
display.drawRect(mmX - 2, mmY - 2, 28, 28, SSD1306_WHITE);
for (int y = 0; y < WORLD_CHUNKS_Y; y++) {
for (int x = 0; x < WORLD_CHUNKS_X; x++) {
if (WORLD_MAP[y][x] == 1) {
display.fillRect(mmX + (x * 3), mmY + (y * 3), 3, 3, SSD1306_WHITE);
}
}
}
int pMmX = mmX + (int)(worldX / CHUNK_SIZE * 3);
int pMmY = mmY + (int)(worldY / CHUNK_SIZE * 3);
if (millis() % 400 < 200) {
display.drawPixel(pMmX, pMmY, SSD1306_WHITE);
}
display.display();
}
