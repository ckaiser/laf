// LAF OS Library
// Copyright (c) 2024  Igara Studio S.A.
//
// This file is released under the terms of the MIT license.
// Read LICENSE.txt for more information.

#ifdef HAVE_CONFIG_H
  #include "config.h"
#endif

#include <SDL3/SDL_log.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_render.h>

#include "os/common/system.h"

#include <gfx/matrix_none.h>
#include <gfx/region_pixman.h>

namespace os {

///////
/// YOINKED THIS FROM SDL_gfx TOO LAZY TO INCLUDE IT
//////
namespace {
bool pixel(SDL_Renderer* renderer, Sint16 x, Sint16 y)
{
  return SDL_RenderPoint(renderer, x, y);
}
bool vline(SDL_Renderer* renderer, Sint16 x, Sint16 y1, Sint16 y2)
{
  return SDL_RenderLine(renderer, x, y1, x, y2);
  ;
}
bool hline(SDL_Renderer* renderer, Sint16 x1, Sint16 x2, Sint16 y)
{
  return SDL_RenderLine(renderer, x1, y, x2, y);
  ;
}
int _drawQuadrants(SDL_Renderer* renderer, Sint16 x, Sint16 y, Sint16 dx, Sint16 dy, Sint32 f)
{
  bool result = true;
  Sint16 xpdx, xmdx;
  Sint16 ypdy, ymdy;

  if (dx == 0) {
    if (dy == 0) {
      result &= pixel(renderer, x, y);
    }
    else {
      ypdy = y + dy;
      ymdy = y - dy;
      if (f) {
        result &= vline(renderer, x, ymdy, ypdy);
      }
      else {
        result &= pixel(renderer, x, ypdy);
        result &= pixel(renderer, x, ymdy);
      }
    }
  }
  else {
    xpdx = x + dx;
    xmdx = x - dx;
    ypdy = y + dy;
    ymdy = y - dy;
    if (f) {
      result &= vline(renderer, xpdx, ymdy, ypdy);
      result &= vline(renderer, xmdx, ymdy, ypdy);
    }
    else {
      result &= pixel(renderer, xpdx, ypdy);
      result &= pixel(renderer, xmdx, ypdy);
      result &= pixel(renderer, xpdx, ymdy);
      result &= pixel(renderer, xmdx, ymdy);
    }
  }

  return result;
}
constexpr int DEFAULT_ELLIPSE_OVERSCAN = 4;
bool _ellipseRGBA(SDL_Renderer* renderer,
                  Sint16 x,
                  Sint16 y,
                  Sint16 rx,
                  Sint16 ry,
                  Uint8 r,
                  Uint8 g,
                  Uint8 b,
                  Uint8 a,
                  Sint32 f)
{
  bool result;
  Sint32 rxi, ryi;
  Sint32 rx2, ry2, rx22, ry22;
  Sint32 error;
  Sint32 curX, curY, curXp1, curYm1;
  Sint32 scrX, scrY, oldX, oldY;
  Sint32 deltaX, deltaY;
  Sint32 ellipseOverscan;

  /*
   * Sanity check radii
   */
  if ((rx < 0) || (ry < 0)) {
    return (false);
  }

  /*
   * Set color
   */
  result = true;
  result &= SDL_SetRenderDrawBlendMode(renderer,
                                       (a == 255) ? SDL_BLENDMODE_NONE : SDL_BLENDMODE_BLEND);
  result &= SDL_SetRenderDrawColor(renderer, r, g, b, a);

  /*
   * Special cases for rx=0 and/or ry=0: draw a hline/vline/pixel
   */
  if (rx == 0) {
    if (ry == 0) {
      return (pixel(renderer, x, y));
    }
    else {
      return (vline(renderer, x, y - ry, y + ry));
    }
  }
  else {
    if (ry == 0) {
      return (hline(renderer, x - rx, x + rx, y));
    }
  }

  /*
   * Adjust overscan
   */
  rxi = rx;
  ryi = ry;
  if (rxi >= 512 || ryi >= 512) {
    ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 4;
  }
  else if (rxi >= 256 || ryi >= 256) {
    ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 2;
  }
  else {
    ellipseOverscan = DEFAULT_ELLIPSE_OVERSCAN / 1;
  }

  /*
   * Top/bottom center points.
   */
  oldX = scrX = 0;
  oldY = scrY = ryi;
  result &= _drawQuadrants(renderer, x, y, 0, ry, f);

  /* Midpoint ellipse algorithm with overdraw */
  rxi *= ellipseOverscan;
  ryi *= ellipseOverscan;
  rx2 = rxi * rxi;
  rx22 = rx2 + rx2;
  ry2 = ryi * ryi;
  ry22 = ry2 + ry2;
  curX = 0;
  curY = ryi;
  deltaX = 0;
  deltaY = rx22 * curY;

  /* Points in segment 1 */
  error = ry2 - rx2 * ryi + rx2 / 4;
  while (deltaX <= deltaY) {
    curX++;
    deltaX += ry22;

    error += deltaX + ry2;
    if (error >= 0) {
      curY--;
      deltaY -= rx22;
      error -= deltaY;
    }

    scrX = curX / ellipseOverscan;
    scrY = curY / ellipseOverscan;
    if ((scrX != oldX && scrY == oldY) || (scrX != oldX && scrY != oldY)) {
      result &= _drawQuadrants(renderer, x, y, scrX, scrY, f);
      oldX = scrX;
      oldY = scrY;
    }
  }

  /* Points in segment 2 */
  if (curY > 0) {
    curXp1 = curX + 1;
    curYm1 = curY - 1;
    error = ry2 * curX * curXp1 + ((ry2 + 3) / 4) + rx2 * curYm1 * curYm1 - rx2 * ry2;
    while (curY > 0) {
      curY--;
      deltaY -= rx22;

      error += rx2;
      error -= deltaY;

      if (error <= 0) {
        curX++;
        deltaX += ry22;
        error += deltaX;
      }

      scrX = curX / ellipseOverscan;
      scrY = curY / ellipseOverscan;
      if ((scrX != oldX && scrY == oldY) || (scrX != oldX && scrY != oldY)) {
        oldY--;
        for (; oldY >= scrY; oldY--) {
          result &= _drawQuadrants(renderer, x, y, scrX, oldY, f);
          /* prevent overdraw */
          if (f) {
            oldY = scrY - 1;
          }
        }
        oldX = scrX;
        oldY = scrY;
      }
    }

    /* Remaining points in vertical */
    if (!f) {
      oldY--;
      for (; oldY >= 0; oldY--) {
        result &= _drawQuadrants(renderer, x, y, scrX, oldY, f);
      }
    }
  }

  return (result);
}
} // namespace
///////
/// YOINKED THIS FROM SDL_gfx TOO LAZY TO INCLUDE IT
//////
///
class SDLSurface : public Surface {
protected:
  explicit SDLSurface(SDL_Renderer* renderer)
    : m_surface(nullptr)
    , m_renderer(renderer)
    , m_texture(nullptr)
    , m_w(0)
    , m_h(0) {};

  SDL_Surface* m_surface;
  SDL_Renderer* m_renderer;
  SDL_Texture* m_texture;

  int m_w;
  int m_h;

public:
  explicit SDLSurface(SDL_Surface* surface, SDL_Renderer* renderer)
    : m_surface(surface)
    , m_renderer(renderer)
  {
    SDL_Rect rect;
    SDL_GetSurfaceClipRect(surface, &rect);
    ASSERT(rect.w == w && rect.h == h);
    m_w = rect.w;
    m_h = rect.h;
    m_texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, m_w, m_h);
  };

  ~SDLSurface() override
  {
    SDL_DestroySurface(m_surface);
    SDL_DestroyTexture(m_texture);
  }

  SDL_Texture* getTexture() const { return m_texture; };

  int width() const override { return m_w; };
  int height() const override { return m_h; };
  const ColorSpaceRef& colorSpace() const override { return nullptr; };
  bool isDirectToScreen() const override { return false; };
  void setImmutable() override {};
  int getSaveCount() const override { return 0; };
  gfx::Rect getClipBounds() const override
  {
    SDL_Rect rect;
    SDL_GetSurfaceClipRect(m_surface, &rect);
    return gfx::Rect(rect.x, rect.y, rect.w, rect.h);
  };
  void saveClip() override {};
  void restoreClip() override {};
  bool clipRect(const gfx::Rect& rc) override {};
  void clipPath(const gfx::Path& path) override {};
  void clipRegion(const gfx::Region& region) override {};
  void save() override {};
  void concat(const gfx::Matrix& matrix) override {};
  void setMatrix(const gfx::Matrix& matrix) override {};
  void resetMatrix() override {};
  void restore() override {};
  gfx::Matrix matrix() const override { return gfx::Matrix(); };

  void lock() override
  {
    // SDL_Log("Locking surface %d", m_surface);
    SDL_LockSurface(m_surface);
    SDL_LockTextureToSurface(m_texture, NULL, &m_surface);
  };

  void unlock() override
  {
    // SDL_Log("Unlocking surface %d", m_surface);
    SDL_UnlockTexture(m_texture);
    SDL_UnlockSurface(m_surface);
  };

  void clear() override
  {
    // SDL_Log("Clearing surface %d", m_surface);
    SDL_ClearSurface(m_surface, 0, 0, 0, 0); // TODO??
  };
  uint8_t* getData(int x, int y) const override { return nullptr; }
  void getFormat(SurfaceFormatData* formatData) const override { formatData = nullptr; }
  gfx::Color getPixel(int x, int y) const override
  {
    Uint8 r, g, b, a;
    SDL_ReadSurfacePixel(m_surface, x, y, &r, &g, &b, &a);
    return gfx::rgba(r, g, b, a);
  }

  void putPixel(gfx::Color color, int x, int y) override
  {
    sdlSetRenderColor(color);
    SDL_RenderPoint(m_renderer, x, y);
  }

  void drawLine(float x0, float y0, float x1, float y1, const os::Paint& paint) override
  {
    sdlApplyPaint(paint);

    if (!SDL_RenderLine(m_renderer, x0, y0, x1, y1)) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not render line: %s", SDL_GetError());
    }
  }

  void drawRect(const gfx::RectF& rc, const os::Paint& paint) override
  {
    sdlApplyPaint(paint);

    const SDL_FRect rect = sdlRectFConvert(rc);
    SDL_RenderRect(m_renderer, &rect);
    // TODO: Fill?
    if (paint.style() & PaintBase::Fill)
      SDL_RenderFillRect(m_renderer, &rect);
  };

  void drawCircle(float cx, float cy, float radius, const os::Paint& paint) override
  {
    // TODO: Test me!
    if (!_ellipseRGBA(m_renderer,
                      cx,
                      cy,
                      radius,
                      radius,
                      gfx::getr(paint.color()),
                      gfx::getg(paint.color()),
                      gfx::getb(paint.color()),
                      gfx::geta(paint.color()),
                      paint.style() /* TODO LOL*/)) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not draw circle");
    }
  };

  void drawPath(const gfx::Path& path, const os::Paint& paint) override {
    // SDL_RenderLines()
  };

  void blitTo(Surface* dest, int srcx, int srcy, int dstx, int dsty, int width, int height)
    const override
  {
    const SDL_Rect src{ srcx, srcy, width, height };
    const SDL_Rect dst{ dstx, dsty, width, height };
    SDL_BlitSurface(m_surface, &src, static_cast<SDL_Surface*>(dest->nativeHandle()), &dst);
  };
  void scrollTo(const gfx::Rect& rc, int dx, int dy) override {};

  void drawSurface(const Surface* src, int dstx, int dsty) override
  {
    const SDL_Rect srcrect{ src->bounds().x, src->bounds().y, src->width(), src->height() };
    const SDL_Rect dstrect{ dstx, dsty, width(), height() };

    // Oof ouch owie
    SDL_Surface* srcsurface = static_cast<SDL_Surface*>(const_cast<Surface*>(src)->nativeHandle());
    SDL_BlitSurface(srcsurface, &srcrect, m_surface, &dstrect);
  };

  void drawSurface(const Surface* src,
                   const gfx::Rect& srcRect,
                   const gfx::Rect& dstRect,
                   const Sampling& sampling,
                   const Paint* paint) override
  {
    sdlApplyPaint(*paint);
    const SDL_Rect srcrect{ srcRect.x, srcRect.y, srcRect.w, srcRect.h };
    const SDL_Rect dstrect{ dstRect.x, dstRect.y, dstRect.w, dstRect.h };
    const SDL_ScaleMode mode = sampling.filter == Sampling::Filter::Nearest ? SDL_SCALEMODE_LINEAR :
                                                                              SDL_SCALEMODE_NEAREST;

    SDL_Surface* srcsurface = static_cast<SDL_Surface*>(const_cast<Surface*>(src)->nativeHandle());
    SDL_BlitSurfaceScaled(srcsurface, &srcrect, m_surface, &dstrect, mode);
  };
  void drawRgbaSurface(const Surface* src, int dstx, int dsty) override
  {
    drawSurface(src, dstx, dsty); // TODO: ??
  };
  void drawRgbaSurface(const Surface* src,
                       int srcx,
                       int srcy,
                       int dstx,
                       int dsty,
                       int width,
                       int height) override
  {
    // TODO: Probably not... yeah, not the, uhh, no
    src->blitTo(this, srcx, srcy, dstx, dsty, width, height);
  };
  void drawColoredRgbaSurface(const Surface* src,
                              gfx::Color fg,
                              gfx::Color bg,
                              const gfx::Clip& clip) override
  {
    throw std::runtime_error("idk yet");
  };
  void drawSurfaceNine(os::Surface* surface,
                       const gfx::Rect& src,
                       const gfx::Rect& center,
                       const gfx::Rect& dst,
                       bool drawCenter,
                       const os::Paint* paint) override
  {
    throw std::runtime_error("SurfaceNine not done yet");
    // SDL_BlitSurface9Grid()
  };
  [[nodiscard]] SurfaceRef applyScale(float scaleFactor, const Sampling& sampling) override
  {
    auto* scaledSurface = SDL_ScaleSurface(m_surface,
                                           width() * scaleFactor,
                                           height() * scaleFactor,
                                           sdlFromSampling(sampling));
    auto* surface = new SDLSurface(scaledSurface, m_renderer);
    return SurfaceRef(surface);
  };
  void* nativeHandle() override { return m_surface; };

private:
  // SDL Helper functions
  void sdlTargetTexture() const
  {
    if (!SDL_SetRenderTarget(m_renderer, m_texture)) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not set render target: %s", SDL_GetError());
    }
  }

  void sdlApplyPaint(const os::Paint& paint) const
  {
    sdlTargetTexture();
    sdlSetRenderColor(paint.color());
  }

  void sdlSetRenderColor(const gfx::Color& color) const
  {
    sdlTargetTexture();
    if (!SDL_SetRenderDrawColor(m_renderer,
                                gfx::getr(color),
                                gfx::getg(color),
                                gfx::getb(color),
                                gfx::geta(color))) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not set render draw color: %s", SDL_GetError());
    }
  }

  static SDL_FRect sdlRectFConvert(const gfx::RectF& rc) { return { rc.h, rc.w, rc.x, rc.y }; }

  static SDL_ScaleMode sdlFromSampling(const Sampling& sampling)
  {
    return sampling.filter == Sampling::Filter::Nearest ? SDL_SCALEMODE_NEAREST :
                                                          SDL_SCALEMODE_LINEAR;
  }
};

class SDLWindowSurface : public SDLSurface {
  SDL_Window* m_window;

public:
  SDLWindowSurface(SDL_Window* window, SDL_Renderer* renderer)
    : SDLSurface(renderer)
    , m_window(window)
  {
    SDL_GetWindowSize(m_window, &m_w, &m_h);
    m_surface = SDL_CreateSurface(m_w, m_h, SDL_PIXELFORMAT_RGBA8888);
    if (!m_surface) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Failed to create window surface: %s - %d, %d",
                   SDL_GetError(),
                   m_w,
                   m_h);
    }

    m_texture =
      SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, m_w, m_h);
    if (!m_texture) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR,
                   "Failed to create window texture: %s - %d, %d",
                   SDL_GetError(),
                   m_w,
                   m_h);
    }
  }

  ~SDLWindowSurface() override
  {
    SDL_Log("Destroying Window Surface");
    if (m_surface)
      SDL_DestroySurface(m_surface);
    if (m_texture)
      SDL_DestroyTexture(m_texture);
  }

  bool isDirectToScreen() const override { return true; }; // TODO: ??
};

class SDLWindow : public Window {
public:
  explicit SDLWindow(const WindowSpec& spec)
  {
    bool r = SDL_CreateWindowAndRenderer(
      "REPLACE ME",
      spec.contentRect().w,
      spec.contentRect().h,
      SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_INPUT_FOCUS,
      &m_window,
      &m_renderer);

    // SDL_SetRenderVSync(m_renderer, 1);

    if (!r || m_window == NULL || m_renderer == NULL) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
      throw std::runtime_error("Could not create SDL window");
    }

    SDL_SetWindowPosition(m_window, spec.contentRect().x, spec.contentRect().y);

    m_surface = new SDLWindowSurface(m_window, m_renderer);
  }
  ~SDLWindow() override { SDL_DestroyWindow(m_window); }

  gfx::Rect frame() const override
  {
    gfx::Rect f;
    SDL_GetWindowSize(m_window, &f.w, &f.h);
    SDL_GetWindowPosition(m_window, &f.x, &f.y);
    SDL_Log("Getting frame: %d, %d, %d, %d", f.x, f.y, f.w, f.h);
    return f;
  };

  void setFrame(const gfx::Rect& bounds) override
  {
    SDL_Log("Changing frame");
    SDL_SetWindowSize(m_window, bounds.w, bounds.h);
    SDL_SetWindowPosition(m_window, bounds.x, bounds.y);
  };

  gfx::Rect contentRect() const override { return frame(); };
  gfx::Rect restoredFrame() const override { return {}; };
  int width() const override
  {
    int w;
    SDL_GetWindowSize(m_window, &w, NULL);
    return w;
  };

  int height() const override
  {
    int h;
    SDL_GetWindowSize(m_window, NULL, &h);
    return h;
  };

  int scale() const override
  { // TODO: Display scale
    float x, y;
    SDL_GetRenderScale(m_renderer, &x, &y);
    ASSERT(x == y);
    return static_cast<int>(x);
  };

  void setScale(int scale) override
  { // TODO: DIsplay scale
    SDL_SetRenderScale(m_renderer, scale, scale);
  };

  bool isVisible() const override { return false; }
  void setVisible(bool visible) override {};

  Surface* surface() override
  {
    // TODO: Mark as invalidated when stuff happens?
    delete m_surface;
    m_surface = new SDLWindowSurface(m_window, m_renderer);
    return m_surface;
  };

  void invalidateRegion(const gfx::Region& rgn) override
  {
    SDL_Log("I wanna invalidate a region: %d, %d, %d, %d",
            rgn.bounds().x,
            rgn.bounds().y,
            rgn.bounds().w,
            rgn.bounds().w);
  };

  bool gpuAcceleration() const override
  {
    return true; // hardcoded metal go brrrrrr
  };
  void setGpuAcceleration(bool state) override { ASSERT(state); };
  void swapBuffers() override
  {
    const SDLSurface* sdlSurface = m_surface;
    if (!sdlSurface)
      return;

    SDL_SetRenderTarget(m_renderer, NULL);
    if (!SDL_RenderTexture(m_renderer, sdlSurface->getTexture(), NULL, NULL)) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to render texture: %s", SDL_GetError());
    }

    if (!SDL_RenderPresent(m_renderer)) {
      SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Failed to present: %s", SDL_GetError());
    }
  };
  void activate() override { SDL_RaiseWindow(m_window); };
  void maximize() override { SDL_MaximizeWindow(m_window); };
  void minimize() override { SDL_MinimizeWindow(m_window); };
  bool isMaximized() const override { return false; };   // Impl
  bool isMinimized() const override { return false; };   // Impl
  bool isTransparent() const override { return false; }; // Impl
  bool isFullscreen() const override { return false; };  // Impl
  void setFullscreen(bool state) override { SDL_SetWindowFullscreen(m_window, state); };

  std::string title() const override { return SDL_GetWindowTitle(m_window); }

  void setTitle(const std::string& title) override { SDL_SetWindowTitle(m_window, title.c_str()); };
  void setIcons(const SurfaceList& icons) override {};
  NativeCursor nativeCursor() override { return NativeCursor::Arrow; };
  bool setCursor(NativeCursor cursor) override { return false; };
  bool setCursor(const CursorRef& cursor) override { return false; };
  void setMousePosition(const gfx::Point& position) override {};
  void captureMouse() override { SDL_CaptureMouse(true); };
  void releaseMouse() override { SDL_CaptureMouse(false); };
  void performWindowAction(WindowAction action, const Event* event) override {};
  std::string getLayout() override {};
  void setLayout(const std::string& layout) override {};
  void setInterpretOneFingerGestureAsMouseMovement(bool state) override {};
  os::ScreenRef screen() const override { return nullptr; };
  os::ColorSpaceRef colorSpace() const override { return nullptr; };
  void setColorSpace(const os::ColorSpaceRef& colorSpace) override {};
  NativeHandle nativeHandle() const override { return m_window; };

protected:
  void onQueueEvent(Event& ev) override {};
  void onDragEnter(os::DragEvent& ev) override {};
  void onDrag(os::DragEvent& ev) override {};
  void onDragLeave(os::DragEvent& ev) override {};
  void onDrop(os::DragEvent& ev) override {};
  void onSetDragTarget() override {};

private:
  SDL_Window* m_window;
  SDL_Renderer* m_renderer;
  SDLSurface* m_surface;
};

class SDLSystem final : public CommonSystem {
public:
  SDLSystem()
  {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
      SDL_Log("Failed to initialize SDL: %s", SDL_GetError());
      throw std::runtime_error("Failed to initialize SDL");
    }

    SDL_Log("SDL initialized!");
  };

  ~SDLSystem() override
  {
    SDL_Log("Quitting");
    SDL_Quit();
  }

private:
  WindowRef makeWindow(const WindowSpec& spec) override { return make_ref<SDLWindow>(spec); }
};

SystemRef System::makeSdl()
{
  return make_ref<SDLSystem>();
}

} // namespace os
