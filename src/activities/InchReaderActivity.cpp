#include "InchReaderActivity.h"

#include <Arduino.h>
#include <algorithm>
#include <cstdio>
#include <cstring>

#include "fontIds.h"

static const char* SAVE_FILE = "/pet_save.bin";
static const char* BOOKS_DIR = "/books";

InchReaderActivity::InchReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput)
    : Activity("InchReader", renderer, mappedInput) {
  std::strncpy(pet.name, "Inch", sizeof(pet.name) - 1);
  pet.name[sizeof(pet.name) - 1] = '\0';
  pet.hunger = 60;
  pet.happiness = 60;
  pet.cleanliness = 70;
  pet.read_seconds = 0;
  pet.care_points = 3;
}

void InchReaderActivity::onEnter() {
  Activity::onEnter();
  Storage.ensureDirectoryExists(BOOKS_DIR);
  loadPet();
  scanBooks();
  mode = AppMode::BOOKSHELF;
  requestUpdate();
}

void InchReaderActivity::loop() {
  if (mode == AppMode::BOOKSHELF) {
    handleBookshelfInput();
  } else if (mode == AppMode::PET_CARE) {
    handlePetCareInput();
  } else if (mode == AppMode::READING) {
    handleReadingInput();
  }
}

void InchReaderActivity::render(RenderLock&&) {
  if (mode == AppMode::BOOKSHELF) {
    renderBookshelf();
  } else if (mode == AppMode::PET_CARE) {
    renderPetScreen();
  } else if (mode == AppMode::READING) {
    renderBookPage();
  }
  renderer.displayBuffer(HalDisplay::FAST_REFRESH);
}

void InchReaderActivity::loadPet() {
  HalFile f;
  if (Storage.openFileForRead("INCH", SAVE_FILE, f)) {
    if (f.size() == sizeof(PetData)) {
      f.read(&pet, sizeof(PetData));
    }
    f.close();
  }
}

void InchReaderActivity::savePet() {
  HalFile f;
  if (Storage.openFileForWrite("INCH", SAVE_FILE, f)) {
    f.write(reinterpret_cast<const uint8_t*>(&pet), sizeof(PetData));
    f.close();
  }
}

void InchReaderActivity::scanBooks() {
  books.clear();
  std::vector<String> all = Storage.listFiles(BOOKS_DIR, 200);
  for (const String& name : all) {
    String low = name;
    low.toLowerCase();
    if (low.endsWith(".txt")) {
      books.push_back(name);
    }
  }
  std::sort(books.begin(), books.end());
}

// --- Drawing Helpers ---

void InchReaderActivity::drawCircle(int cx, int cy, int r, int lineWidth, bool black) const {
  if (r <= 0) return;
  renderer.drawRoundedRect(cx - r, cy - r, r * 2, r * 2, lineWidth, r, black);
}

void InchReaderActivity::fillCircle(int cx, int cy, int r, Color color) const {
  if (r <= 0) return;
  renderer.fillRoundedRect(cx - r, cy - r, r * 2, r * 2, r, color);
}

void InchReaderActivity::drawRoundBox(int x, int y, int w, int h, int r, Color fill, int outlineW) const {
  if (fill != Color::Clear) {
    renderer.fillRoundedRect(x, y, w, h, r, fill);
  }
  if (outlineW > 0) {
    renderer.drawRoundedRect(x, y, w, h, outlineW, r, true);
  }
}

void InchReaderActivity::fillPoly3(int x0, int y0, int x1, int y1, int x2, int y2, bool black) const {
  int xs[3] = {x0, x1, x2};
  int ys[3] = {y0, y1, y2};
  renderer.fillPolygon(xs, ys, 3, black);
}

void InchReaderActivity::fillPoly4(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, bool black) const {
  int xs[4] = {x0, x1, x2, x3};
  int ys[4] = {y0, y1, y2, y3};
  renderer.fillPolygon(xs, ys, 4, black);
}

void InchReaderActivity::drawStatBar(const char* label, int value, int x, int y) const {
  char buf[40];
  std::snprintf(buf, sizeof(buf), "%s: %d%%", label, value);
  renderer.drawText(UI_10_FONT_ID, x, y, buf);
  drawRoundBox(x + 140, y - 3, 260, 18, 9, Color::White, 2);
  int fillW = (value * 254) / 100;
  if (fillW > 6) {
    drawRoundBox(x + 143, y, fillW, 12, 6, Color::DarkGray, 0);
  }
}

// --- Inch & Apple Artwork ---

int InchReaderActivity::getWormStage(uint32_t readSec, String& title, String& nextGoal) const {
  float hours = readSec / 3600.0f;
  if (hours >= 100.0f) {
    title = "Grand Librarian";
    nextGoal = "100h MAX!";
    return 4;
  }
  if (hours >= 75.0f) {
    title = "Grand Librarian";
    nextGoal = "Mastery: 100h";
    return 4;
  }
  if (hours >= 50.0f) {
    title = "Chapter Scholar";
    nextGoal = "Next: 75h";
    return 3;
  }
  if (hours >= 25.0f) {
    title = "Page Nibbler";
    nextGoal = "Next: 50h";
    return 2;
  }
  title = "Baby Sprout";
  nextGoal = "Next: 25h";
  return 1;
}

void InchReaderActivity::drawCuteWorm(int cx, int cy, int stage, int happiness, int hunger, float scale) const {
  int numBodyBalls = stage - 1;
  int baseR = static_cast<int>((18 + (hunger / 30)) * scale);
  int stepX = static_cast<int>(28 * scale);
  int totalBalls = numBodyBalls + 1;
  int startX = cx - ((totalBalls - 1) * stepX) / 2;

  for (int i = 0; i < numBodyBalls; i++) {
    int sx = startX + i * stepX;
    int waveY = static_cast<int>(((i % 2 == 1) ? -5 : 5) * scale);
    int sy = cy + waveY;
    int segR = static_cast<int>(baseR * (0.85f + 0.05f * i));

    if (stage >= 2) {
      int footY = sy + segR + static_cast<int>(3 * scale);
      renderer.fillRectDither(sx - static_cast<int>(6 * scale), footY - static_cast<int>(3 * scale),
                              static_cast<int>(14 * scale), static_cast<int>(7 * scale), Color::DarkGray);
    }
    Color segShade = (i % 2 == 0) ? Color::White : Color::LightGray;
    fillCircle(sx, sy, segR, segShade);
    drawCircle(sx, sy, segR, 2, true);
  }

  int hx = startX + numBodyBalls * stepX;
  int hy = cy - ((numBodyBalls > 0) ? static_cast<int>(6 * scale) : 0);
  int headR = baseR + 7;
  fillCircle(hx, hy, headR, Color::White);
  drawCircle(hx, hy, headR, 2, true);

  // Shoes (Stage 1)
  if (stage == 1) {
    int footY = hy + headR + static_cast<int>(2 * scale);
    renderer.fillRectDither(hx - static_cast<int>(11 * scale), footY - static_cast<int>(3 * scale),
                            static_cast<int>(10 * scale), static_cast<int>(6 * scale), Color::DarkGray);
    renderer.fillRectDither(hx + static_cast<int>(3 * scale), footY - static_cast<int>(3 * scale),
                            static_cast<int>(10 * scale), static_cast<int>(6 * scale), Color::DarkGray);
  }

  // Bow Tie (Stage 2+)
  if (stage >= 2) {
    int by = hy + headR + static_cast<int>(2 * scale);
    int bx = hx - static_cast<int>(4 * scale);
    int s = static_cast<int>(8 * scale);
    fillPoly3(bx, by, bx - s, by - s / 2, bx - s, by + s / 2, true);
    fillPoly3(bx, by, bx + s, by - s / 2, bx + s, by + s / 2, true);
    fillCircle(bx, by, std::max(2, static_cast<int>(3 * scale)), Color::Black);
  }

  // Eyes & Glints
  int eyeR = std::max(3, static_cast<int>(5 * scale));
  int ex1 = hx + static_cast<int>(2 * scale);
  int ex2 = hx + static_cast<int>(14 * scale);
  int ey = hy - static_cast<int>(4 * scale);
  fillCircle(ex1, ey, eyeR, Color::Black);
  fillCircle(ex2, ey, eyeR, Color::Black);
  int glintR = std::max(1, static_cast<int>(2 * scale));
  fillCircle(ex1 - 1, ey - 1, glintR, Color::White);
  fillCircle(ex2 - 1, ey - 1, glintR, Color::White);

  // Rosy Cheeks
  int cheekR = std::max(2, static_cast<int>(4 * scale));
  fillCircle(hx - static_cast<int>(3 * scale), hy + static_cast<int>(4 * scale), cheekR, Color::LightGray);
  fillCircle(hx + static_cast<int>(18 * scale), hy + static_cast<int>(4 * scale), cheekR, Color::LightGray);

  // Mouth
  int my = hy + static_cast<int>(6 * scale);
  if (happiness >= 50) {
    renderer.drawLine(hx + static_cast<int>(4 * scale), my, hx + static_cast<int>(7 * scale), my + static_cast<int>(4 * scale), 2, true);
    renderer.drawLine(hx + static_cast<int>(7 * scale), my + static_cast<int>(4 * scale), hx + static_cast<int>(10 * scale), my + static_cast<int>(1 * scale), 2, true);
    renderer.drawLine(hx + static_cast<int>(10 * scale), my + static_cast<int>(1 * scale), hx + static_cast<int>(13 * scale), my + static_cast<int>(4 * scale), 2, true);
    renderer.drawLine(hx + static_cast<int>(13 * scale), my + static_cast<int>(4 * scale), hx + static_cast<int>(16 * scale), my, 2, true);
  } else {
    drawCircle(hx + static_cast<int>(9 * scale), my + static_cast<int>(2 * scale), std::max(2, static_cast<int>(3 * scale)), 2, true);
  }

  // Glasses (Stage 3+)
  if (stage >= 3) {
    int glassR = std::max(4, static_cast<int>(7 * scale));
    drawCircle(ex1, ey, glassR, 2, true);
    drawCircle(ex2, ey, glassR, 2, true);
    renderer.drawLine(ex1 + glassR, ey, ex2 - glassR, ey, 2, true);
  }

  // Headgear
  int topY = hy - headR;
  if (stage == 1) {
    renderer.drawLine(hx, topY, hx, topY - static_cast<int>(14 * scale), 2, true);
    fillPoly3(hx, topY - static_cast<int>(14 * scale),
              hx + static_cast<int>(12 * scale), topY - static_cast<int>(20 * scale),
              hx + static_cast<int>(6 * scale), topY - static_cast<int>(10 * scale), true);
  } else if (stage == 2 || stage == 3) {
    renderer.drawLine(hx - 4, topY, hx - static_cast<int>(12 * scale), topY - static_cast<int>(16 * scale), 2, true);
    renderer.drawLine(hx + 8, topY, hx + static_cast<int>(16 * scale), topY - static_cast<int>(16 * scale), 2, true);
    fillCircle(hx - static_cast<int>(12 * scale), topY - static_cast<int>(16 * scale), std::max(2, static_cast<int>(4 * scale)), Color::DarkGray);
    fillCircle(hx + static_cast<int>(16 * scale), topY - static_cast<int>(16 * scale), std::max(2, static_cast<int>(4 * scale)), Color::DarkGray);
  } else if (stage == 4) {
    int capW = static_cast<int>(24 * scale);
    int capH = static_cast<int>(8 * scale);
    fillPoly4(hx, topY - capH - static_cast<int>(6 * scale),
              hx - capW, topY - static_cast<int>(2 * scale),
              hx, topY + static_cast<int>(4 * scale),
              hx + capW, topY - static_cast<int>(2 * scale), true);
    renderer.drawLine(hx + capW - 2, topY - static_cast<int>(2 * scale),
                      hx + capW + static_cast<int>(4 * scale), topY + static_cast<int>(12 * scale), 2, true);
    fillCircle(hx + capW + static_cast<int>(4 * scale), topY + static_cast<int>(13 * scale), std::max(2, static_cast<int>(3 * scale)), Color::DarkGray);
  }
}

void InchReaderActivity::drawAppleHome(int cx, int cy) const {
  // Stem & Leaf
