// LAF Text Library
// Copyright (C) 2019-2024  Igara Studio S.A.
// Copyright (C) 2012-2017  David Capello
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifndef LAF_TEXT_SPRITE_SHEET_FONT_H_INCLUDED
#define LAF_TEXT_SPRITE_SHEET_FONT_H_INCLUDED
#pragma once

#include "base/debug.h"
#include "base/ref.h"
#include "base/string.h"
#include "base/utf8_decode.h"
#include "gfx/rect.h"
#include "os/surface.h"
#include "text/font.h"
#include "text/font_metrics.h"
#include "text/typeface.h"

#include <vector>

namespace text {

class SpriteSheetFont : public Font {
  static constexpr auto kRedColor = gfx::rgba(255, 0, 0);

public:
  SpriteSheetFont() : m_sheet(nullptr) {}
  ~SpriteSheetFont() {}

  FontType type() override { return FontType::SpriteSheet; }

  TypefaceRef typeface() const override
  {
    return nullptr; // TODO impl
  }

  float metrics(FontMetrics* metrics) const override
  {
    // TODO impl

    if (metrics) {
      metrics->ascent = -height();
      metrics->underlineThickness = 1.0f;
      metrics->underlinePosition = 0.5f;
    }

    return height();
  }

  int height() const override { return getCharBounds(' ').h; }

  int textLength(const std::string_view str) const override
  {
    base::utf8_decode decode(str);
    int x = 0;
    while (int chr = decode.next())
      x += getCharBounds(chr).w;
    return x;
  }

  float measureText(const std::string_view str,
                    gfx::RectF* bounds,
                    const os::Paint* paint) const override
  {
    float w = textLength(str);
    if (bounds)
      *bounds = gfx::RectF(0, 0, w, height());
    return w;
  }

  bool isScalable() const override { return false; }

  void setSize(int size) override
  {
    // Do nothing
  }

  bool antialias() const override { return false; }

  void setAntialias(bool antialias) override
  {
    // Do nothing
  }

  glyph_t codePointToGlyph(const codepoint_t codepoint) const override
  {
    glyph_t glyph = codepoint - int(' ') + 2;
    if (glyph >= 0 && glyph < int(m_glyphs.size()) && !m_glyphs[glyph].isEmpty()) {
      return glyph;
    }
    else
      return 0;
  }

  gfx::RectF getGlyphBounds(glyph_t glyph) const override
  {
    if (glyph >= 0 && glyph < (int)m_glyphs.size())
      return gfx::RectF(0, 0, m_glyphs[glyph].w, m_glyphs[glyph].h);

    return getCharBounds(128);
  }

  float getGlyphAdvance(glyph_t glyph) const override { return getGlyphBounds(glyph).w; }

  gfx::RectF getGlyphBoundsOnSheet(glyph_t glyph) const
  {
    if (glyph >= 0 && glyph < (int)m_glyphs.size())
      return m_glyphs[glyph];

    return getCharBounds(128);
  }

  os::Surface* sheetSurface() const { return m_sheet.get(); }

  gfx::Rect getCharBounds(codepoint_t cp) const
  {
    glyph_t glyph = codePointToGlyph(cp);
    if (glyph == 0)
      glyph = codePointToGlyph(128);

    if (glyph != 0)
      return m_glyphs[glyph];
    else
      return gfx::Rect();
  }

  static FontRef FromSurface(const os::SurfaceRef& sur)
  {
    auto font = base::make_ref<SpriteSheetFont>();
    font->m_sheet = sur;
    font->m_glyphs.push_back(gfx::Rect()); // glyph index 0 is MISSING CHARACTER glyph
    font->m_glyphs.push_back(gfx::Rect()); // glyph index 1 is NULL glyph

    os::SurfaceLock lock(sur.get());
    gfx::Rect bounds(0, 0, 1, 1);
    gfx::Rect glyphBounds;

    while (font->findGlyph(sur.get(), sur->width(), sur->height(), bounds, glyphBounds)) {
      font->m_glyphs.push_back(glyphBounds);
      bounds.x += bounds.w;
    }

    return font;
  }

private:
  bool findGlyph(const os::Surface* sur,
                 int width,
                 int height,
                 gfx::Rect& bounds,
                 gfx::Rect& glyphBounds)
  {
    gfx::Color keyColor = sur->getPixel(0, 0);

    while (sur->getPixel(bounds.x, bounds.y) == keyColor) {
      bounds.x++;
      if (bounds.x >= width) {
        bounds.x = 0;
        bounds.y += bounds.h;
        bounds.h = 1;
        if (bounds.y >= height)
          return false;
      }
    }

    gfx::Color firstCharPixel = sur->getPixel(bounds.x, bounds.y);

    bounds.w = 0;
    while ((bounds.x + bounds.w < width) &&
           (sur->getPixel(bounds.x + bounds.w, bounds.y) != keyColor)) {
      bounds.w++;
    }

    bounds.h = 0;
    while ((bounds.y + bounds.h < height) &&
           (sur->getPixel(bounds.x, bounds.y + bounds.h) != keyColor)) {
      bounds.h++;
    }

    // Using red color in the first pixel of the char indicates that
    // this glyph shouldn't be used as a valid one.
    if (firstCharPixel != kRedColor)
      glyphBounds = bounds;
    else
      glyphBounds = gfx::Rect();

    return !bounds.isEmpty();
  }

private:
  os::SurfaceRef m_sheet;
  std::vector<gfx::Rect> m_glyphs;
};

} // namespace text

#endif
