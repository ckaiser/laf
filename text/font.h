// LAF Text Library
// Copyright (c) 2019-2024  Igara Studio S.A.
// Copyright (c) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef LAF_TEXT_FONT_H_INCLUDED
#define LAF_TEXT_FONT_H_INCLUDED
#pragma once

#include "base/ints.h"
#include "base/ref.h"
#include "gfx/fwd.h"
#include "text/fwd.h"

#include <string>

namespace os {
class Paint;
}

namespace text {

enum class FontType {
  Unknown,
  SpriteSheet, // SpriteSheet
  FreeType,    // FreeType
  Native,      // Skia
};

class Font : public base::RefCount {
public:
  Font() : m_fallback(nullptr) {}
  virtual ~Font() {}
  virtual FontType type() = 0;
  virtual TypefaceRef typeface() const = 0;
  virtual float metrics(FontMetrics* metrics) const = 0;
  virtual int height() const = 0;
  virtual int textLength(const std::string_view str) const = 0;
  virtual float measureText(const std::string_view str,
                            gfx::RectF* bounds,
                            const os::Paint* paint = nullptr) const = 0;
  virtual bool isScalable() const = 0;
  virtual void setSize(int size) = 0;
  virtual bool antialias() const = 0;
  virtual void setAntialias(bool antialias) = 0;

  bool hasCodePoint(codepoint_t cp) const { return (codePointToGlyph(cp) != 0); }

  virtual glyph_t codePointToGlyph(codepoint_t cp) const = 0;
  virtual gfx::RectF getGlyphBounds(glyph_t glyph) const = 0;
  virtual float getGlyphAdvance(glyph_t glyph) const = 0;

  Font* fallback() const { return m_fallback; }
  void setFallback(Font* font) { m_fallback = font; }

private:
  Font* m_fallback;
};

} // namespace text

#endif
