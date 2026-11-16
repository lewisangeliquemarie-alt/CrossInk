#include "../InchReaderActivity.h"
#include "../reader/BookReadingStats.h"
#include "../reader/BookStatsActivity.h"

#include <ArduinoJson.h>
#include <GfxRenderer.h>
#include <HalStorage.h>
#include <MappedInputManager.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

#include "fontIds.h"

namespace {
constexpr const char* PET_SAVE_PATH = "/pet_save.json";
constexpr const char* BOOKS_DIR = "/books";
constexpr int COLUMNS = 3;
constexpr int ROWS = 2;
constexpr int BOOKS_PER_PAGE = COLUMNS * ROWS;
constexpr int TOTAL_PAGES = 3;
constexpr int MAX_SLOTS = BOOKS_PER_PAGE * TOTAL_PAGES;
constexpr int COVER_WIDTH = 136;
constexpr int COVER_HEIGHT = 270;
constexpr int SHELF_THICKNESS = 12;
}  // namespace

void InchReaderActivity::onEnter() {
  Activity::onEnter();
  loadPet();
  scanBooks();
  mode_ = ScreenMode::BOOKSHELF;
  requestUpdate();
}

void InchReaderActivity::onExit() {
  savePet();
  Activity::onExit();
}

void InchReaderActivity::loadPet() {
  HalFile file;
  if (Storage.openFileForRead("INCH", PET_SAVE_PATH, file)) {
    JsonDocument doc;
    if (deserializeJson(doc, file) == DeserializationError::Ok) {
      pet_.name = doc["name"] | "Inch";
      pet_.hunger = doc["hunger"] | 60;
      pet_.happiness = doc["happiness"] | 60;
      pet_.cleanliness = doc["cleanliness"] | 70;
      pet_.read_seconds = doc["read_seconds"] | 0L;
      pet_.care_points = doc["care_points"] | 3;
    }
    file.close();
  }
}

void InchReaderActivity::savePet() {
  HalFile file;
  if (Storage.openFileForWrite("INCH", PET_SAVE_PATH, file)) {
    JsonDocument doc;
    doc["name"] = pet_.name;
    doc["hunger"] = pet_.hunger;
    doc["happiness"] = pet_.happiness;
    doc["cleanliness"] = pet_.cleanliness;
    doc["read_seconds"] = pet_.read_seconds;
    doc["care_points"] = pet_.care_points;
    serializeJsonPretty(doc, file);
    file.close();
  }
}

void InchReaderActivity::scanBooks() {
  books_.clear();
  Storage.ensureDirectoryExists(BOOKS_DIR);
  HalFile dir = Storage.open(BOOKS_DIR, O_RDONLY);
  if (dir && dir.isDirectory()) {
    char nameBuf;
    while (true) {
      HalFile entry = dir.openNextFile();
      if (!entry) break;
      if (!entry.isDirectory()) {
        entry.getName(nameBuf, sizeof(nameBuf));
        std::string fname(nameBuf);
        if (fname.size() > 4 &&
            (fname.substr(fname.size() - 4) == ".txt" || fname.substr(fname.size() - 4) == ".TXT")) {
          books_.push_back(fname);
        }
      }
      entry.close();
    }
    dir.close();
  }
  std::sort(books_.begin(), books_.end());
}

void InchReaderActivity::getWormStage(int& stage, String& title, String& nextGoal) {
  float hours = pet_.read_seconds / 3600.0f;
  if (hours >= 100.0f) {
    stage = 4;
    title = "Grand Librarian";
    nextGoal = "100h MAX!";
  } else if (hours >= 75.0f) {
    stage = 4;
    title = "Grand Librarian";
    nextGoal = "Mastery: 100h";
  } else if (hours >= 50.0f) {
    stage = 3;
    title = "Chapter Scholar";
    nextGoal = "Next: 75h";
  } else if (hours >= 25.0f) {
    stage = 2;
    title = "Page Nibbler";
    nextGoal = "Next: 50h";
  } else {
    stage = 1;
    title = "Baby Sprout";
    nextGoal = "Next: 25h";
  }
}

void InchReaderActivity::drawCircle(int cx, int cy, int r, bool fillBlack, bool outline) {
  for (int dy = -r; dy <= r; dy++) {
    int dx = static_cast<int>(std::sqrt(r * r - dy * dy));
    if (fillBlack) {
      renderer.drawLine(cx - dx, cy + dy, cx + dx, cy + dy, true);
    } else {
      renderer.drawLine(cx - dx, cy + dy, cx + dx, cy + dy, false);
      if (outline) {
        renderer.drawPixel(cx - dx, cy + dy, true);
        renderer.drawPixel(cx + dx, cy + dy, true);
      }
    }
  }
}

void InchReaderActivity::drawRoundBox(int x, int y, int w, int h, int r, bool fillBlack, bool outline) {
  if (fillBlack) {
    renderer.fillRect(x + r, y, w - 2 * r, h, true);
    renderer.fillRect(x, y + r, w, h - 2 * r, true);
  } else {
    renderer.fillRect(x + r, y, w - 2 * r, h, false);
    renderer.fillRect(x, y + r, w, h - 2 * r, false);
  }
  drawCircle(x + r, y + r, r, fillBlack, outline);
  drawCircle(x + w - r, y + r, r, fillBlack, outline);
  drawCircle(x + r, y + h - r, r, fillBlack, outline);
  drawCircle(x + w - r, y + h - r, r, fillBlack, outline);
  if (outline) {
    renderer.drawLine(x + r, y, x + w - r, y, true);
    renderer.drawLine(x + r, y + h, x + w - r, y + h, true);
    renderer.drawLine(x, y + r, x, y + h - r, true);
    renderer.drawLine(x + w, y + r, x + w, y + h - r, true);
  }
}

void InchReaderActivity::drawCuteWorm(int cx, int cy, int stage, int happiness, int hunger, float scale) {
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
      renderer.fillRect(sx - static_cast<int>(6 * scale), footY - static_cast<int>(3 * scale),
                        static_cast<int>(14 * scale), static_cast<int>(7 * scale), true);
    }
    drawCircle(sx, sy, segR, false, true);
  }

  int hx = startX + numBodyBalls * stepX;
  int hy = cy - ((numBodyBalls > 0) ? static_cast<int>(6 * scale) : 0);
  int headR = baseR + 7;
  drawCircle(hx, hy, headR, false, true);

  if (stage == 1) {
    int footY = hy + headR + static_cast<int>(2 * scale);
    renderer.fillRect(hx - static_cast<int>(11 * scale), footY - static_cast<int>(3 * scale),
                      static_cast<int>(10 * scale), static_cast<int>(6 * scale), true);
    renderer.fillRect(hx + static_cast<int>(3 * scale), footY - static_cast<int>(3 * scale),
                      static_cast<int>(10 * scale), static_cast<int>(6 * scale), true);
  }

  if (stage >= 2) {
    int by = hy + headR + static_cast<int>(2 * scale);
    int bx = hx - static_cast<int>(4 * scale);
    int s = static_cast<int>(8 * scale);
    renderer.fillRect(bx - s, by - s / 2, s * 2, s, true);
  }

  int eyeR = std::max(3, static_cast<int>(5 * scale));
  int ex1 = hx + static_cast<int>(2 * scale);
  int ex2 = hx + static_cast<int>(14 * scale);
  int ey = hy - static_cast<int>(4 * scale);
  drawCircle(ex1, ey, eyeR, true, false);
  drawCircle(ex2, ey, eyeR, true, false);
  drawCircle(ex1 - 1, ey - 1, std::max(1, static_cast<int>(2 * scale)), false, false);
  drawCircle(ex2 - 1, ey - 1, std::max(1, static_cast<int>(2 * scale)), false, false);

  int my = hy + static_cast<int>(6 * scale);
  if (happiness >= 50) {
    renderer.drawLine(hx + static_cast<int>(4 * scale), my, hx + static_cast<int>(7 * scale), my + static_cast<int>(4 * scale), true);
    renderer.drawLine(hx + static_cast<int>(7 * scale), my + static_cast<int>(4 * scale), hx + static_cast<int>(10 * scale), my + static_cast<int>(1 * scale), true);
    renderer.drawLine(hx + static_cast<int>(10 * scale), my + static_cast<int>(1 * scale), hx + static_cast<int>(13 * scale), my + static_cast<int>(4 * scale), true);
    renderer.drawLine(hx + static_cast<int>(13 * scale), my + static_cast<int>(4 * scale), hx + static_cast<int>(16 * scale), my, true);
  } else {
    drawCircle(hx + static_cast<int>(9 * scale), my + static_cast<int>(2 * scale), std::max(2, static_cast<int>(3 * scale)), false, true);
  }

  if (stage >= 3) {
    int glassR = std::max(4, static_cast<int>(7 * scale));
    drawCircle(ex1, ey, glassR, false, true);
    drawCircle(ex2, ey, glassR, false, true);
    drawCircle(ex1, ey, std::max(2, static_cast<int>(3 * scale)), true, false);
    drawCircle(ex2, ey, std::max(2, static_cast<int>(3 * scale)), true, false);
    renderer.drawLine(ex1 + glassR, ey, ex2 - glassR, ey, true);
  }

  int topY = hy - headR;
  if (stage == 1) {
    renderer.drawLine(hx, topY, hx, topY - static_cast<int>(14 * scale), true);
    drawCircle(hx + static_cast<int>(6 * scale), topY - static_cast<int>(15 * scale), std::max(3, static_cast<int>(5 * scale)), true, false);
  } else if (stage == 2 || stage == 3) {
    renderer.drawLine(hx - 4, topY, hx - static_cast<int>(12 * scale), topY - static_cast<int>(16 * scale), true);
    renderer.drawLine(hx + 8, topY, hx + static_cast<int>(16 * scale), topY - static_cast<int>(16 * scale), true);
    drawCircle(hx - static_cast<int>(12 * scale), topY - static_cast<int>(16 * scale), std::max(2, static_cast<int>(4 * scale)), true, false);
    drawCircle(hx + static_cast<int>(16 * scale), topY - static_cast<int>(16 * scale), std::max(2, static_cast<int>(4 * scale)), true, false);
  } else if (stage == 4) {
    int capW = static_cast<int>(24 * scale);
    renderer.fillRect(hx - capW, topY - static_cast<int>(6 * scale), capW * 2, static_cast<int>(6 * scale), true);
    renderer.drawLine(hx + capW - 2, topY - static_cast<int>(2 * scale), hx + capW + static_cast<int>(4 * scale), topY + static_cast<int>(12 * scale), true);
  }
}

void InchReaderActivity::drawAppleHome(int cx, int cy) {
  renderer.fillRect(cx - 3, cy - 122, 8, 28, true);
  drawCircle(cx + 26, cy - 112, 14, true, false);
  drawCircle(cx - 52, cy + 5, 102, false, true);
  drawCircle(cx + 52, cy + 5, 102, false, true);
  drawRoundBox(cx - 85, cy - 82, 170, 175, 55, false, false);
  drawRoundBox(cx - 108, cy - 58, 216, 136, 48, false, true);
  renderer.drawLine(cx - 92, cy + 52, cx + 92, cy + 52, true);
}

void InchReaderActivity::drawStatBar(const char* label, int value, int x, int y) {
  char buf;
  snprintf(buf, sizeof(buf), "%s: %d%%", label, value);
  renderer.drawText(UI_10_FONT_ID, x, y, buf);
  drawRoundBox(x + 140, y - 3, 260, 18, 9, false, true);
  int fillW = static_cast<int>((value / 100.0f) * 254);
  if (fillW > 6) {
    drawRoundBox(x + 143, y, fillW, 12, 6, true, false);
  }
}

void InchReaderActivity::render(RenderLock&&) {
  if (mode_ == ScreenMode::BOOKSHELF) {
