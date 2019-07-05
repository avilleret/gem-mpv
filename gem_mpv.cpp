#include "gem_mpv.hpp"
#include "SDL2/SDL.h"

CPPEXTERN_NEW_WITH_GIMME(mpv);

mpv::mpv(int argc, t_atom*argv)
  : gemframebuffer(argc, argv)
{
  m_mpv = mpv_create();
  if (!m_mpv)
  {
    error("mpv: context init failed");
    return;
  }

  // Some minor options can only be set before mpv_initialize().
  if (mpv_initialize(m_mpv) < 0)
      error("mpv: init failed");
}

mpv::~mpv()
{
  if(m_mpv)
    mpv_terminate_destroy(m_mpv);
}

void mpv::render(GemState *state)
{
  gemframebuffer::render(state);

  mpv_opengl_fbo fbo{(int)m_frameBufferIndex, m_width, m_height, m_format};
  int flip(1);

  mpv_render_param params[] = {
      // Specify the default framebuffer (0) as target. This will
      // render onto the entire screen. If you want to show the video
      // in a smaller rectangle or apply fancy transformations, you'll
      // need to render into a separate FBO and draw it manually.
      {MPV_RENDER_PARAM_OPENGL_FBO, &fbo},
      // Flip rendering (needed due to flipped GL coordinate system).
      {MPV_RENDER_PARAM_FLIP_Y,  &flip},
      {}
  };
  // See render_gl.h on what OpenGL environment mpv expects, and
  // other API details.
  if(m_mpv_gl)
    mpv_render_context_render(m_mpv_gl, params);
}

static void *get_proc_address_mpv(void *fn_ctx, const char *name)
{
  // TODO adapt for Windows and MacOS
  return (void *)glXGetProcAddress((const GLubyte*)name);
}

void mpv::startRendering(void)
{
  gemframebuffer::startRendering();

  post("startRendering");
  if(!m_mpv)
    return;

  mpv_opengl_init_params gl_param{.get_proc_address = get_proc_address_mpv};

  mpv_render_param params [] = {
    {MPV_RENDER_PARAM_API_TYPE, (void*)MPV_RENDER_API_TYPE_OPENGL},
    {MPV_RENDER_PARAM_OPENGL_INIT_PARAMS, &gl_param},
    {MPV_RENDER_PARAM_INVALID, 0}
  };

  // This makes mpv use the currently set GL context. It will use the callback
  // (passed via params) to resolve GL builtin functions, as well as extensions.
  if (mpv_render_context_create(&m_mpv_gl, m_mpv, params) < 0)
  {
    error("failed to initialize mpv GL context");
    return;
  }

  load_mess(m_file);

  m_init=true;
}

void mpv::stopRendering(void)
{
  post("stopRendering");
  gemframebuffer::stopRendering();

  if(m_mpv_gl)
    mpv_render_context_free(m_mpv_gl);
}

void mpv::load_mess(const std::string& s)
{
  m_file = s;

  if(m_mpv && !m_file.empty())
  {
    const char *cmd[] = {"loadfile", m_file.c_str(), nullptr};
    mpv_command(m_mpv, cmd);
  }
}

void mpv::pause_mess()
{
  if(m_mpv)
  {
    const char *cmd[] = {"pause", "1", nullptr};
    mpv_command(m_mpv, cmd);
  }
}

void mpv::play_mess()
{
  if(m_mpv)
  {
    const char *cmd[] = {"play", nullptr};
    mpv_command(m_mpv, cmd);
  }
}

void mpv::obj_setupCallback(t_class *classPtr)
{
  gemframebuffer::obj_setupCallback(classPtr);

  CPPEXTERN_MSG1(classPtr, "load",   load_mess, std::string);
  CPPEXTERN_MSG0(classPtr, "play",   play_mess);
  CPPEXTERN_MSG0(classPtr, "pause",  pause_mess);
}
