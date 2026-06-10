#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include "expressions.h"

// ---------- OLED ----------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------- NeoPixel ----------
#define LED_PIN   13
#define LED_COUNT 8
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ---------- eye state ----------
int ref_eye_height = 30, ref_eye_width = 30;
int ref_space_between_eye = 10, ref_corner_radius = 10;
int left_eye_height = ref_eye_height, left_eye_width = ref_eye_width;
int left_eye_x = 32, left_eye_y = 32;
int right_eye_x = 32 + 30 + 10, right_eye_y = 32;
int right_eye_height = ref_eye_height, right_eye_width = ref_eye_width;
int corner_radius = ref_corner_radius;

void draw_eyes(bool update = true) {
    display.clearDisplay();
    int x = left_eye_x - left_eye_width / 2;
    int y = left_eye_y - left_eye_height / 2;
    display.fillRoundRect(x, y, left_eye_width, left_eye_height, corner_radius, SSD1306_WHITE);
    x = right_eye_x - right_eye_width / 2;
    y = right_eye_y - right_eye_height / 2;
    display.fillRoundRect(x, y, right_eye_width, right_eye_height, corner_radius, SSD1306_WHITE);
    if (update) display.display();
}

void reset_eyes(bool update = true) {
    left_eye_height = ref_eye_height; left_eye_width = ref_eye_width;
    right_eye_height = ref_eye_height; right_eye_width = ref_eye_width;
    left_eye_x = SCREEN_WIDTH/2 - ref_eye_width/2 - ref_space_between_eye/2;
    left_eye_y = SCREEN_HEIGHT/2;
    right_eye_x = SCREEN_WIDTH/2 + ref_eye_width/2 + ref_space_between_eye/2;
    right_eye_y = SCREEN_HEIGHT/2;
    corner_radius = ref_corner_radius;
    draw_eyes(update);
}

// angry = slanted eyebrows over normal eyes
void angry_eyes() {
    reset_eyes(false);
    draw_eyes(false);
    // cut top inner corners with black triangles to make an angry slant
    display.fillTriangle(left_eye_x - left_eye_width/2, left_eye_y - left_eye_height/2,
                         left_eye_x + left_eye_width/2, left_eye_y - left_eye_height/2,
                         left_eye_x + left_eye_width/2, left_eye_y - left_eye_height/2 + 12, SSD1306_BLACK);
    display.fillTriangle(right_eye_x - right_eye_width/2, right_eye_y - right_eye_height/2,
                         right_eye_x + right_eye_width/2, right_eye_y - right_eye_height/2,
                         right_eye_x - right_eye_width/2, right_eye_y - right_eye_height/2 + 12, SSD1306_BLACK);
    display.display();
}

void blink(int speed = 12) {
    reset_eyes(false); draw_eyes();
    for (int i = 0; i < 3; i++) {
        left_eye_height -= speed; right_eye_height -= speed;
        left_eye_width += 3; right_eye_width += 3;
        draw_eyes(); delay(50);
    }
    for (int i = 0; i < 3; i++) {
        left_eye_height += speed; right_eye_height += speed;
        left_eye_width -= 3; right_eye_width -= 3;
        draw_eyes(); delay(50);
    }
}

void sleepy_eyes() {
    reset_eyes(false);
    for (int i = 0; i < 5; i++) {
        left_eye_height -= 3; right_eye_height -= 3;
        left_eye_y += 1; right_eye_y += 1;
        draw_eyes(); delay(120);
    }
    delay(600);
    for (int i = 0; i < 5; i++) {
        left_eye_height += 3; right_eye_height += 3;
        left_eye_y -= 1; right_eye_y -= 1;
        draw_eyes(); delay(100);
    }
}

void happy_eye() {
    display.clearDisplay();

    int eyeW = 24;
    int thickness = 6;
    int archHeight = 6;

    int cyL = left_eye_x;
    int cyR = right_eye_x;
    int baseY = left_eye_y + 6;

    for (int t = 0; t < thickness; t++) {
        int prevLx = -1, prevLy = 0, prevRx = -1, prevRy = 0;
        for (int i = 0; i <= eyeW; i++) {
            float frac = (float)i / eyeW;          // 0..1 across the eye
            // upward arc: high at the ends, low in the middle -> ∪ shape
            float curve = sin(frac * PI);          // 0 at ends, 1 in middle
            int yOff = (int)(archHeight * curve);

            int lx = cyL - eyeW/2 + i;
            int ly = baseY - yOff + t;
            int rx = cyR - eyeW/2 + i;
            int ry = baseY - yOff + t;

            if (prevLx >= 0) {
                display.drawLine(prevLx, prevLy, lx, ly, SSD1306_WHITE);
                display.drawLine(prevRx, prevRy, rx, ry, SSD1306_WHITE);
            }
            prevLx = lx; prevLy = ly;
            prevRx = rx; prevRy = ry;
        }
    }

    display.display();
    delay(900);
}

void saccade_anim() {
    reset_eyes(true);
    for (int n = 0; n < 6; n++) {
        int dx = random(-1, 2), dy = random(-1, 2);
        left_eye_x += 8*dx; right_eye_x += 8*dx;
        left_eye_y += 6*dy; right_eye_y += 6*dy;
        draw_eyes(); delay(60);
        left_eye_x -= 8*dx; right_eye_x -= 8*dx;
        left_eye_y -= 6*dy; right_eye_y -= 6*dy;
        draw_eyes(); delay(60);
    }
}

void sad_eyes() {
    reset_eyes(false);
    left_eye_height = ref_eye_height - 5; right_eye_height = ref_eye_height - 5;
    left_eye_y += 3; right_eye_y += 3;
    draw_eyes(false);
    for (int i = 0; i < 3; i++) {
        draw_eyes(false);
        display.fillCircle(left_eye_x - 8, left_eye_y + 15 + i*3, 1, SSD1306_WHITE);
        display.fillCircle(right_eye_x + 8, right_eye_y + 15 + i*3, 1, SSD1306_WHITE);
        display.display(); delay(300);
    }
}

void suspicious_eyes() {
    reset_eyes(false);
    for (int i = 0; i < 3; i++) {
        left_eye_height -= 5; right_eye_height -= 5;
        left_eye_x += 2; right_eye_x += 2;
        draw_eyes(); delay(150);
    }
    delay(600);
    for (int i = 0; i < 2; i++) { left_eye_x -= 6; right_eye_x -= 6; draw_eyes(); delay(120); }
    delay(400);
    for (int i = 0; i < 2; i++) { left_eye_x += 6; right_eye_x += 6; draw_eyes(); delay(120); }
}

// ---------- ring helper ----------
void fillRing(uint8_t r, uint8_t g, uint8_t b) {
    for (int i = 0; i < LED_COUNT; i++) ring.setPixelColor(i, ring.Color(r, g, b));
    ring.show();
}

// ---------- idle animation (non-blocking-ish) ----------
unsigned long lastIdle = 0;
unsigned long idleInterval = 3000;

void expressionsBegin() {
    ring.begin(); ring.setBrightness(60); ring.show();
    display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
    display.clearDisplay(); display.display();
    randomSeed(micros());
    showState(CALM);
}

// idle: random angry/blink/sleepy while CALM
void updateIdle() {
    if (millis() - lastIdle < idleInterval) return;
    lastIdle = millis();
    fillRing(255, 0, 0);                 // calm = red per theme
    int pick = random(0, 5);
    if (pick == 0)      blink();
    else if (pick == 1) sleepy_eyes();
    else if (pick == 2) suspicious_eyes();
    else                angry_eyes();    // mostly angry
    idleInterval = random(2500, 5000);   // vary timing so it's not boring
}

void showState(BotState s) {
    switch (s) {
        case HIT:                        // happy, green
            fillRing(0, 255, 0);
            happy_eye();
            break;
        case CUDDLE:                     // crying, blue
            fillRing(0, 0, 255);
            if (random(0, 2) == 0) sad_eyes();
            else                   saccade_anim();
            break;
        case FALLEN:
        case CALM:
        default:
            fillRing(255, 0, 0);
            angry_eyes();
            break;
    }
}