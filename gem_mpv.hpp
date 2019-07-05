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

  void load_mess(const std::string& s);
  void pause_mess();
  void play_mess();

private:
  mpv_handle* m_mpv{};
  mpv_render_context* m_mpv_gl{};

  bool m_init{};
  std::string m_file{};
  bool m_pause{};
};
