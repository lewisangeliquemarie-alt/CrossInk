#pragma once

#include <Arduino.h>
#include <functional>
#include <string>
#include <vector>
#include "../Activity.h"

struct PetData {
  String name = "Inch";
  int hunger = 60;
  int happiness = 60;
  int cleanliness = 70;
  long read_seconds = 0;
  int care_points = 3;
};

class InchReaderActivity : public Activity {
 public:
  explicit InchReaderActivity(GfxRenderer& renderer, MappedInputManager& mappedInput,
                              const std::function<void()>& onGoHome)
      : Activity("InchReader", renderer, mappedInput), onGoHome_(onGoHome) {}

  void onEnter() override;
  void onExit() override;
  void loop() override;

 private:
  enum class ScreenMode { BOOKSHELF, PET_CARE, READING };
  ScreenMode mode_ = ScreenMode::BOOKSHELF;

  std::function<void()> onGoHome_;
  PetData pet_;
  std::vector<std::string> books_;
  int selected_ = 0;
  int actionIdx_ = 0;
  String statusMsg_ = "Read books to evolve your worm & earn points!";

  // Reader state
  std::string currentBookFile_;
  std::vector<std::vector<std::string>> pages_;
  int currentPage_ = 0;
  unsigned long sessionStartMs_ = 0;

  // Data persistence
  void loadPet();
  void savePet();
  void scanBooks();
  void paginateBook(const std::string& filename);

  // Drawing helpers
  void getWormStage(int& stage, String& title, String& nextGoal);
  void drawCircle(int cx, int cy, int r, bool fillBlack, bool outline = true);
  void drawRoundBox(int x, int y, int w, int h, int r, bool fillBlack, bool outline = true);
  void drawCuteWorm(int cx, int cy, int stage, int happiness, int hunger, float scale);
  void drawAppleHome(int cx, int cy);
  void drawStatBar(const char* label, int value, int x, int y);
  void drawBookPaperFrame();
  void drawWalkingBookworm(int wx, int wy, bool stepToggle);

  // Screens
  void renderBookshelf();
  void renderPetScreen();
  void renderBookPage();
};
