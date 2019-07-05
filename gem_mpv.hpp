#pragma once

#include "gemframebuffer.h"

#include <mpv/client.h>
#include <mpv/render_gl.h>

class GEM_EXTERN mpv : public gemframebuffer
{
  CPPEXTERN_HEADER(mpv, GemBase);

public:
  mpv(int argc, t_atom*argv);
  ~mpv();

  virtual void startRendering(void);
  virtual void stopRendering(void);
  virtual void render(GemState *state);

  void ext_fb_mess_cb(t_symbol*s, int argc, t_atom*argv);

private:
  mpv_handle* m_mpv{};
  mpv_render_context* m_mpv_gl{};

  bool m_init{};
};
