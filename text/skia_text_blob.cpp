// LAF Text Library
// Copyright (c) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#include "text/skia_text_blob.h"

#include "os/skia/skia_helpers.h"
#include "text/skia_font.h"
#include "text/skia_font_mgr.h"

#include "include/core/SkTextBlob.h"

#include <limits>

namespace text {

SkiaTextBlob::SkiaTextBlob(const sk_sp<SkTextBlob>& skTextBlob, const gfx::RectF& bounds)
  : TextBlob(bounds)
  , m_skTextBlob(skTextBlob)
{
  ASSERT(skTextBlob);
}

void SkiaTextBlob::visitRuns(const RunVisitor& visitor)
{
  SkTextBlob::Iter iter(*m_skTextBlob);
  SkTextBlob::Iter::ExperimentalRun run;
  TextBlob::RunInfo subInfo;
  std::vector<gfx::PointF> positions;

  while (iter.experimentalNext(&run)) {
    const int n = run.count;
    subInfo.font = base::make_ref<SkiaFont>(run.font);
    subInfo.glyphCount = n;
    subInfo.glyphs = const_cast<glyph_t*>(run.glyphs);
    if (positions.size() < n)
      positions.resize(n);
    for (size_t i = 0; i < n; ++i) {
      positions[i] = gfx::PointF(run.positions[i].x(), run.positions[i].y());
    }
    subInfo.positions = positions.data();

    visitor(subInfo);
  }
}

TextBlobRef SkiaTextBlob::Make(const FontRef& font, const std::string_view& text)
{
  ASSERT(font);
  ASSERT(font->type() == FontType::Native);
  ASSERT(dynamic_cast<SkiaFont*>(font.get()));

  SkFont skFont = static_cast<SkiaFont*>(font.get())->skFont();
  sk_sp<SkTextBlob> textBlob;
  textBlob = SkTextBlob::MakeFromText(text.data(), text.size(), skFont, SkTextEncoding::kUTF8);
  if (textBlob)
    return base::make_ref<SkiaTextBlob>(textBlob);

  return nullptr;
}

} // namespace text
