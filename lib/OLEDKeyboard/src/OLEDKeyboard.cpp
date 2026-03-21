/*
  OLEDKeyboard.cpp - On-screen keyboard library for OLED displays
  
  Created by Sk Raihan, SKR Electronics Lab
  https://github.com/skr-electronics-lab
*/

#include "OLEDKeyboard.h"

// Keyboard layouts
const char* const OLEDKeyboard::_keysUpper[KEY_COUNT] = {
  "A","B","C","D","E","F","G","H",
  "I","J","K","L","M","N","O","P",
  "Q","R","S","T","U","V","W","X",
  "Aa","?#","<","_",".","Y","Z",">"
};

const char* const OLEDKeyboard::_keysLower[KEY_COUNT] = {
  "a","b","c","d","e","f","g","h",
  "i","j","k","l","m","n","o","p",
  "q","r","s","t","u","v","w","x",
  "Aa","?#","<","_",".","y","z",">"
};

const char* const OLEDKeyboard::_keysSymbols[KEY_COUNT] = {
  "1","2","3","4","5","6","7","8",
  "9","0","@","#","$","%","&","*",
  "-","+","=","/","\\","(",")","!",
  "Aa","?#","<","_",".","?",",",">"
};

OLEDKeyboard::OLEDKeyboard(U8G2* display): _display(display) {
  
  // Default settings
  _screenWidth = 128;
  _screenHeight = 64;
  _inputAreaHeight = 14;
  _keyWidth = 13;
  _keyHeight = 11;
  _hSpacing = 2;
  _vSpacing = 2;
  _maxInputLength = 20;
  _debounceDelay = 200;
  _cursorBlinkInterval = 500;
  
  // Initial state
  _currentState = STATE_UPPERCASE;
  _inputText = "";
  _inputComplete = false;
  _cursorVisible = true;
  _selectedKeyIndex = 0;
  
  // Timing
  _lastCursorBlink = 0;
}

void OLEDKeyboard::begin() {
  // Get actual display dimensions
  _screenWidth = _display->getDisplayWidth();
  _screenHeight = _display->getDisplayHeight();
  
  // Calculate layout
  _calculateLayout();
  
  // Set default font
  _display->setFont(u8g2_font_6x10_tr);
}

bool OLEDKeyboard::update() {
  // Handle cursor blinking
  if (millis() - _lastCursorBlink > _cursorBlinkInterval) {
    _cursorVisible = !_cursorVisible;
    _lastCursorBlink = millis();
  }
  draw();
  return _inputComplete;
}

void OLEDKeyboard::handleInput(KeyBoardInput input) {
  int actualRow = _selectedKeyIndex / KEY_COLS;
  int actualCol = _selectedKeyIndex % KEY_COLS;
  switch (input) {
  case PRESSED_NONE:
    return;
  case PRESSED_UP:
    actualRow = (actualRow - 1 + KEY_ROWS) % KEY_ROWS;
    break;
  case PRESSED_DOWN:
    actualRow = (actualRow + 1) % KEY_ROWS;
    break;
  case PRESSED_LEFT:
    actualCol = (actualCol - 1 + KEY_COLS) % KEY_COLS;
    break;
  case PRESSED_RIGHT:
    actualCol = (actualCol + 1) % KEY_COLS;
    break;
  case PRESSED_OK:
      const char* const* currentKeys = _getCurrentKeys();
      const char* selectedKey = currentKeys[_selectedKeyIndex];
      _processKeyPress(selectedKey);
  }
  _selectedKeyIndex = actualRow * KEY_COLS + actualCol;
}

void OLEDKeyboard::draw() {
  _display->clearBuffer();
  _drawInputArea();
  _drawKeyboard();
  _display->sendBuffer();
}

void OLEDKeyboard::_drawInputArea() {
  // Draw input frame
  _display->drawFrame(0, 0, _screenWidth, _inputAreaHeight);
  
  // Prepare text to display with scrolling
  String displayText = _inputText;
  int fontWidth = 6;
  int maxChars = (_screenWidth - 4) / fontWidth;
  
  if (displayText.length() > maxChars) {
    displayText = "..." + displayText.substring(displayText.length() - maxChars + 3);
  }
  
  // Draw text
  _display->drawStr(2, 11, displayText.c_str());
  
  // Draw cursor
  if (_cursorVisible && !_inputComplete) {
    int textWidth = _display->getStrWidth(displayText.c_str());
    if (textWidth < _screenWidth - 8) {
      _display->drawStr(2 + textWidth, 11, "_");
    }
  }
}

void OLEDKeyboard::_drawKeyboard() {
  const char* const* currentKeys = _getCurrentKeys();
  
  for (int i = 0; i < KEY_COUNT; i++) {
    int row = i / KEY_COLS;
    int col = i % KEY_COLS;
    int keyX = _keyboardX + col * (_keyWidth + _hSpacing);
    int keyY = _keyboardY + row * (_keyHeight + _vSpacing);
    
    const char* keyLabel = currentKeys[i];
    int labelWidth = _display->getStrWidth(keyLabel);
    int labelX = keyX + (_keyWidth - labelWidth) / 2;
    int labelY = keyY + _keyHeight - 2;
    
    if (i == _selectedKeyIndex) {
      // Draw selected key (inverted)
      _display->setDrawColor(1);
      _display->drawBox(keyX, keyY, _keyWidth, _keyHeight);
      _display->setDrawColor(0);
      _display->drawStr(labelX, labelY, keyLabel);
      _display->setDrawColor(1);
    } else {
      // Draw normal key
      _display->drawFrame(keyX, keyY, _keyWidth, _keyHeight);
      _display->drawStr(labelX, labelY, keyLabel);
    }
  }
}

const char* const* OLEDKeyboard::_getCurrentKeys() const {
  switch (_currentState) {
    case STATE_LOWERCASE:
      return _keysLower;
    case STATE_SYMBOLS:
      return _keysSymbols;
    default:
      return _keysUpper;
  }
}

void OLEDKeyboard::_processKeyPress(const char* key) {
  if (_isSpecialKey(key)) {
    _handleSpecialKey(key);
  } else {
    // Regular character
    if (_inputText.length() < _maxInputLength) {
      _inputText += key;
    }
  }
}

bool OLEDKeyboard::_isSpecialKey(const char* key) const {
  return (strcmp(key, ">") == 0 ||    // Enter/Go
          strcmp(key, "<") == 0 ||    // Backspace
          strcmp(key, "_") == 0 ||    // Space
          strcmp(key, "Aa") == 0 ||   // Shift
          strcmp(key, "?#") == 0);    // Symbols
}

void OLEDKeyboard::_handleSpecialKey(const char* key) {
  if (strcmp(key, ">") == 0) {
    // Enter/Go
    _inputComplete = true;
  } else if (strcmp(key, "<") == 0) {
    // Backspace
    if (_inputText.length() > 0) {
      _inputText.remove(_inputText.length() - 1);
    }
  } else if (strcmp(key, "_") == 0) {
    // Space
    if (_inputText.length() < _maxInputLength) {
      _inputText += " ";
    }
  } else if (strcmp(key, "Aa") == 0) {
    // Shift (toggle between uppercase and lowercase)
    _currentState = (_currentState == STATE_UPPERCASE) ? STATE_LOWERCASE : STATE_UPPERCASE;
  } else if (strcmp(key, "?#") == 0) {
    // Symbols toggle
    _currentState = (_currentState == STATE_SYMBOLS) ? STATE_LOWERCASE : STATE_SYMBOLS;
  }
}

void OLEDKeyboard::_calculateLayout() {
  _keyboardX = (_screenWidth - (KEY_COLS * _keyWidth + (KEY_COLS - 1) * _hSpacing)) / 2;
  _keyboardY = _inputAreaHeight;
}

// Public interface methods
bool OLEDKeyboard::isInputComplete() const {
  return _inputComplete;
}

String OLEDKeyboard::getInputText() const {
  return _inputText;
}

void OLEDKeyboard::clearInput() {
  _inputText = "";
  _inputComplete = false;
}

void OLEDKeyboard::reset() {
  _currentState = STATE_UPPERCASE;
  _selectedKeyIndex = 0;
  _inputText = "";
  _inputComplete = false;
  _cursorVisible = true;
  _lastCursorBlink = 0;
}

void OLEDKeyboard::setMaxLength(int maxLen) {
  if (maxLen > 0) {
    _maxInputLength = maxLen;
  }
}

void OLEDKeyboard::setPosition(int x, int y) {
  _keyboardX = x;
  _keyboardY = y;
}

void OLEDKeyboard::setDebounceDelay(unsigned long delay) {
  _debounceDelay = delay;
}

void OLEDKeyboard::setCursorBlinkInterval(unsigned long interval) {
  _cursorBlinkInterval = interval;
}

void OLEDKeyboard::setInputAreaHeight(int height) {
  if (height > 0) {
    _inputAreaHeight = height;
    _calculateLayout();
  }
}

void OLEDKeyboard::setKeySize(int width, int height) {
  if (width > 0 && height > 0) {
    _keyWidth = width;
    _keyHeight = height;
    _calculateLayout();
  }
}

void OLEDKeyboard::setKeySpacing(int horizontal, int vertical) {
  if (horizontal >= 0 && vertical >= 0) {
    _hSpacing = horizontal;
    _vSpacing = vertical;
    _calculateLayout();
  }
}