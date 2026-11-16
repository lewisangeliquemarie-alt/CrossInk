#pragma once

#include <HalStorage.h>
#include <cstdint>
#include <string>
#include <vector>

#include "Activity.h"

struct PetData {
  char name[16];
  int hunger;
  int happiness;
  int cleanliness;
  uint32_t read_seconds;
  int care_points;
};

enum class AppMode : uint8_t {
  BOOKSHELF,
  PET_CARE,
  READING
};

class InchReaderActivity final : public Activity {
 public:
  explicit InchReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput);
  void onEnter() override;
  void loop() override;
  void render(RenderLock&&) override;

 private:
  AppMode mode = AppMode::BOOKSHELF;
  PetData pet{};

  // Bookshelf state
  std::vector<String> books;
  int selectedSlot = 0;
  static constexpr int COLUMNS = 3;
  static constexpr int ROWS = 2;
  static constexpr int BOOKS_PER_PAGE = 6;
  static constexpr int TOTAL_PAGES = 3;
  static constexpr int MAX_SLOTS = 18;

  // Pet Care state
  int actionIdx = 0;
  String statusMsg = "Read books to evolve your worm & earn points!";

  // Book Reader state (Byte-offset indexed for low RAM)
  String currentBookFilename;
  std::vector<uint32_t> pageOffsets;
  int currentPage = 0;
  uint32_t sessionStartMs = 0;

  // Persistence & Storage Helpers
  void loadPet();
  void savePet();
  void scanBooks();

  // Drawing Helpers using CrossInk GfxRenderer
  void drawCircle(int cx, int cy, int r, int lineWidth = 2, bool black = true) const;
  void fillCircle(int cx, int cy, int r, Color color) const;
  void drawRoundBox(int x, int y, int w, int h, int r, Color fill, int outlineW) const;
  void fillPoly4(int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3, bool black = true) const;
  void fillPoly3(int x0, int y0, int x1, int y1, int x2, int y2, bool black = true) const;
  void drawStatBar(const char* label, int value, int x, int y) const;

  // Inch & Apple Artwork
  int getWormStage(uint32_t readSec, String& title, String& nextGoal) const;
  void drawCuteWorm(int cx, int cy, int stage, int happiness, int hunger, float scale) const;
  void drawAppleHome(int cx, int cy) const;
  void drawWalkingBookworm(int wx, int wy, bool stepToggle) const;
  void drawBookPaperFrame() const;

  // Screens & Pagination
  void renderBookshelf() const;
  void renderPetScreen() const;
  void renderBookPage() const;
  void buildPageIndex(const String& filename);
  std::vector<String> loadSinglePageLines(int pageIndex) const;

  // Input Handling
  void handleBookshelfInput();
  void handlePetCareInput();
  void handleReadingInput();
};
