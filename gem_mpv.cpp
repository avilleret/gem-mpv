#include "gem_mpv.hpp"
#include <string.h>

// greatly inspired by mvp-examples/libmpv/sdl

CPPEXTERN_NEW_WITH_GIMME(mpv);

static void wakeup(void *ctx)
{
  static_cast<mpv*>(ctx)->rise_event_flag();
}

static void *get_proc_address_mpv(void *fn_ctx, const char *name)
{
  // TODO adapt for Windows and MacOS
  return (void *)glXGetProcAddress((const GLubyte*)name);
}

static void node_to_atom(const mpv_node* node, std::vector<t_atom>& res)
{
  switch(node->format)
  {
    case MPV_FORMAT_STRING:
    case MPV_FORMAT_OSD_STRING:
    {
      t_atom a;
      SETSYMBOL(&a, gensym(node->u.string));
      res.push_back(std::move(a));
      break;
    }
    case MPV_FORMAT_FLAG:
    {
      t_atom a;
      SETFLOAT(&a, node->u.flag);
      res.push_back(std::move(a));
      break;
    }
    case MPV_FORMAT_INT64:
    {
      t_atom a;
      SETFLOAT(&a, node->u.int64);
      res.push_back(std::move(a));
      break;
    }
    case MPV_FORMAT_DOUBLE:
    {
      t_atom a;
      SETFLOAT(&a, node->u.double_);
      res.push_back(std::move(a));
      break;
    }
    case MPV_FORMAT_NODE_ARRAY:
    {
      t_atom a;
      for(int i = 0; i< node->u.list->num; i++)
      {
        auto val = node->u.list->values[i];
        node_to_atom(&val, res);
      }
      break;
    }
    case MPV_FORMAT_BYTE_ARRAY:
    case MPV_FORMAT_NODE_MAP:
    {
      error("could not handle this node format : %d", node->format);
      break;
    }
    case MPV_FORMAT_NODE:
    case MPV_FORMAT_NONE:
      break;
  }
}

static void prop_to_atom(mpv_event_property* prop, std::vector<t_atom>& res)
{
  const char* name = prop->name;
  if(name)
  {
    t_atom aname;
    SETSYMBOL(&aname, gensym(name));
    res.push_back(std::move(aname));
  }

  if(!prop->data)
    return;

  switch(prop->format){
    case MPV_FORMAT_FLAG:
    {
      t_atom a;
      bool b = *(bool *)prop->data;
      SETFLOAT(&a, b ? 1. : 0.);
      res.push_back(std::move(a));
      break;
    }
    case MPV_FORMAT_DOUBLE:
    {
      t_atom a;
      double d = *(double *)prop->data;
      SETFLOAT(&a, d);
      res.push_back(std::move(a));
      break;
    }
    case MPV_FORMAT_INT64:
    {
      t_atom a;
      int64_t i = *(int64_t*)prop->data;
      SETFLOAT(&a, i);
      res.push_back(std::move(a));
      break;
    }
    case MPV_FORMAT_STRING:
    case MPV_FORMAT_OSD_STRING:
    {
      t_atom a;
      char* s = (char *)prop->data;
      SETSYMBOL(&a, gensym(s));
      res.push_back(std::move(a));
      break;
    }
    case MPV_FORMAT_NODE:
    {
      mpv_node* node = (mpv_node*)prop->data;
      node_to_atom(node, res);
      break;
    }
    case MPV_FORMAT_NONE:
      break;
    case MPV_FORMAT_NODE_ARRAY:
    case MPV_FORMAT_NODE_MAP:
    case MPV_FORMAT_BYTE_ARRAY:
      std::cout << "format: " << prop->format << " not handled" << std::endl;
  }
}

static void post_mpv_log(mpv* x, mpv_event_log_message* msg)
{
  int level;
  switch(msg->log_level)
  {
    case MPV_LOG_LEVEL_FATAL:
      level=0;
      break;
    case MPV_LOG_LEVEL_ERROR:
      level=1;
      break;
    case MPV_LOG_LEVEL_WARN:
    case MPV_LOG_LEVEL_INFO:
      level=2;
      break;
    case MPV_LOG_LEVEL_V:
    case MPV_LOG_LEVEL_DEBUG:
      level=3;
      break;
    default:
      level=41;
  }
  if(level != -1)
    logpost(x->x_obj, level, "[%s] %s: %s", msg->prefix, msg->level, msg->text);
}

mpv::mpv(int argc, t_atom*argv)
  : gemframebuffer(argc, argv)
{
  m_prop_outlet = outlet_new(this->x_obj, 0);

  m_mpv = mpv_create();
  if (!m_mpv)
  {
    error("mpv: context init failed");
    return;
  }

  // Some minor options can only be set before mpv_initialize().
  if (mpv_initialize(m_mpv) < 0)
    error("mpv: init failed");

  // Actually using the opengl_cb state has to be explicitly requested.
  // Otherwise, mpv will create a separate platform window.
  if (mpv_set_option_string(m_mpv, "vo", "opengl-cb") < 0)
  {
    error("failed to set VO");
    return;
  }

  mpv_request_event(m_mpv, MPV_EVENT_TICK, 1);
  mpv_set_wakeup_callback(m_mpv, wakeup, this);
}

mpv::~mpv()
{
  if(m_mpv)
    mpv_terminate_destroy(m_mpv);
}

void mpv::render(GemState *state)
{

  bool new_frame=false;
  m_event_flag=true; // FIXME it appears that the flag is not always rised by wakeup fn
  while(m_mpv && m_event_flag && !m_modified)
  {
    mpv_event *event = mpv_wait_event(m_mpv, 0);
    switch (event->event_id)
    {
      case MPV_EVENT_NONE:
        m_event_flag=false;
        break;
      case MPV_EVENT_SHUTDOWN:
        break;
      case MPV_EVENT_LOG_MESSAGE:
      {
        struct mpv_event_log_message *msg = (struct mpv_event_log_message *)event->data;
        post_mpv_log(this, msg);
        break;
      }
      case MPV_EVENT_START_FILE:
      {
        m_started=false;
        t_atom a;
        SETSYMBOL(&a, gensym("start"));
        outlet_anything(m_prop_outlet, gensym("event"), 1, &a);
        break;
      }
      case MPV_EVENT_END_FILE:
      {
        m_started=false;
        t_atom a;
        SETSYMBOL(&a, gensym("end"));
        outlet_anything(m_prop_outlet, gensym("event"), 1, &a);
        break;
      }
      case MPV_EVENT_FILE_LOADED:
      {
        t_atom a;
        SETSYMBOL(&a, gensym("file_loaded"));
        outlet_anything(m_prop_outlet, gensym("event"), 1, &a);

        // observed properties doesn't always send changes
        // or at least we may have missed it
        mpv_get_property(m_mpv, "width",  MPV_FORMAT_INT64, &m_media_width);
        mpv_get_property(m_mpv, "height", MPV_FORMAT_INT64, &m_media_height);
        if(m_auto_resize && (m_media_width != m_width || m_media_height != m_height) )
        {
          gemframebuffer::dimMess(m_media_width, m_media_height);
          m_reload = true;
        }
        else
        {
          m_started=true;
          {
            t_atom a[2];

            SETSYMBOL(a, gensym("width"));
            SETFLOAT(a+1, m_media_width);
            outlet_anything(m_prop_outlet, gensym("property"), 2, a);

            SETSYMBOL(a, gensym("height"));
            SETFLOAT(a+1, m_media_height);
            outlet_anything(m_prop_outlet, gensym("property"), 2, a);
          }
          {
            static t_atom argv[2];
            SETSYMBOL(argv, gensym("f"));
            SETSYMBOL(argv+1, gensym("duration"));
            command_mess(gensym("property_typed"), 2, argv);
          }
        }
        break;
      }
      case MPV_EVENT_TICK:
      {
        new_frame=true;
        if(m_started)
        {
          {
            static t_atom argv[2];
            SETSYMBOL(argv, gensym("d"));
            SETSYMBOL(argv+1, gensym("time-pos"));
            command_mess(gensym("property_typed"), 2, argv);
          }
          {
            static t_atom argv[2];
            SETSYMBOL(argv, gensym("d"));
            SETSYMBOL(argv+1, gensym("percent-pos"));
            command_mess(gensym("property_typed"), 2, argv);
          }
        }

        /*
         * This might be interesting but it's a lot of output
        t_atom a;
        SETSYMBOL(&a, gensym("new_frame"));
        outlet_anything(m_prop_outlet, gensym("event"), 1, &a);
        */
        break;
      }
      case MPV_EVENT_PROPERTY_CHANGE:
      {
        mpv_event_property *prop = (mpv_event_property *)event->data;
        handle_prop_event(prop);
        break;
      }
      case MPV_EVENT_COMMAND_REPLY:
      case MPV_EVENT_GET_PROPERTY_REPLY:
      case MPV_EVENT_SET_PROPERTY_REPLY:
      // case MPV_EVENT_TRACKS_CHANGED: // deprecated
      // case MPV_EVENT_TRACK_SWITCHED: // deprecated
      case MPV_EVENT_PAUSE:
      case MPV_EVENT_IDLE:
      case MPV_EVENT_UNPAUSE:
      case MPV_EVENT_QUEUE_OVERFLOW:
      // case MPV_EVENT_HOOK: // not yet available in v0.27.2
        break;
    }
  }

  if(m_size_changed)
  {
    if(m_auto_resize)
    {
      gemframebuffer::dimMess(m_media_width, m_media_height);
    }
    m_size_changed=false;
  }

  if(m_reload && !m_modified)
  {
    m_reload=false;
    // reload file when size changed
    command_mess(gensym("command"), m_loadfile_cmd.size(), m_loadfile_cmd.data());
  }

  // FIXME : when not calling gemframebuffer::render(state),
  // lots of errors are printed in Pd's console like :
  // GL[1284]: stack underflow
  // but we should only draw when new frame are available for performance optimization

  // if(new_frame)
  if(true)
  {
    gemframebuffer::render(state);
    if(m_mpv_gl)
    {
      mpv_opengl_cb_draw(m_mpv_gl, m_frameBufferIndex, m_width, m_height);
    }
  }
}

void mpv::startRendering(void)
{
  gemframebuffer::startRendering();

  post("startRendering");
  if(!m_mpv)
    return;

  // The OpenGL API is somewhat separate from the normal mpv API. This only
  // returns NULL if no OpenGL support is compiled.
  m_mpv_gl = static_cast<mpv_opengl_cb_context*>(mpv_get_sub_api(m_mpv, MPV_SUB_API_OPENGL_CB));
  if (!m_mpv_gl)
  {
    error("failed to create mpv GL API handle");
    return;
  }

  // This makes mpv use the currently set GL context. It will use the callback
  // to resolve GL builtin functions, as well as extensions.
  if (mpv_opengl_cb_init_gl(m_mpv_gl, NULL, get_proc_address_mpv, NULL) < 0)
  {
    error("failed to initialize mpv GL context");
    return;
  }
}

void mpv::stopRendering(void)
{
  post("stopRendering");
  gemframebuffer::stopRendering();

  if(m_mpv_gl)
    mpv_opengl_cb_uninit_gl(m_mpv_gl);
}

void mpv::command_mess(t_symbol *s, int argc, t_atom *argv)
{
  std::string types{};

  if(s == gensym("command_typed") || s == gensym("property_typed"))
  {
    if(argc < 2 || argv->a_type != A_SYMBOL || (argv+1)->a_type != A_SYMBOL)
    {
      error("usage : command_typed|property_typed <types> <command> <arguments...>");
      return;
    }
    types = std::string(argv->a_w.w_symbol->s_name);
    argv++;
    argc--;
  }

  else if(argc == 0 || argv->a_type != A_SYMBOL )
  {
    error("usage: command|property <command> <arguments...>");
    return;
  }

  if(!m_mpv)
  {
    error("mvp not initialized");
    return;
  }


  if(s == gensym("command") || s == gensym("command_typed"))
  {
    if(!types.empty())
      types = " " + types;
    if(argc>1 && argv->a_type == A_SYMBOL)
    {
      const char* s = argv->a_w.w_symbol->s_name;
      if( 0 == strcmp("loadfile", s) )
      {
        m_started=false;

        // save loadfile command call to reload file if needed
        m_loadfile_cmd.clear();
        m_loadfile_cmd.reserve(argc);
        for(int i=0; i<argc; ++i)
          m_loadfile_cmd.push_back(argv[i]);
      }
    }
    node_builder node(argc, argv, types);
    mpv_node res;
    if(mpv_command_node(m_mpv, node.node(), &res) < 0)
    {
      error("Error when executing command");
      return;
    }
    node_autofree f(&res);
  }
  else if (s == gensym("property") || s == gensym("property_typed"))
  {
    auto sname = argv->a_w.w_symbol;
    auto name = argv->a_w.w_symbol->s_name;
    argv++;
    argc--;

    if(argc > 0)
    {
      node_builder node(argc, argv, types);
      auto err = mpv_set_property(m_mpv, name, MPV_FORMAT_NODE, node.node());
      if(err != MPV_ERROR_SUCCESS)
      {
        error("can't set property %s: error code: %d", name, err);
      }
    }
    else
    {
      void* data{};
      mpv_format format = MPV_FORMAT_DOUBLE;
      if(!types.empty())
      {
        switch (types.at(0)) {
          case 'b':
            format = MPV_FORMAT_FLAG;
            data = new bool();
            break;
          case 'i':
            format = MPV_FORMAT_INT64;
            data = new int64_t();
            break;
          case 's':
            format = MPV_FORMAT_STRING;
            break;
          case 'd':
          default:
            format = MPV_FORMAT_DOUBLE;
            data = new double();
            break;
        }
      }
      auto err = mpv_get_property(m_mpv, name, format, data);
      if(err != MPV_ERROR_SUCCESS)
      {
        error("can't get property %s, error code: %d", name, err);
        return;
      }

      switch (format) {
        case MPV_FORMAT_FLAG:
        {
          bool b = *(bool *)data;
          t_atom a[2];
          SETSYMBOL(a, sname);
          SETFLOAT(a+1, b ? 1. : 0.);
          outlet_anything(m_prop_outlet, gensym("property"), 2, a);
          delete (bool*)data;
          break;
        }
        case MPV_FORMAT_DOUBLE:
        {
          double d = *(double *)data;
          t_atom a[2];
          SETSYMBOL(a, sname);
          SETFLOAT(a+1, d);
          outlet_anything(m_prop_outlet, gensym("property"), 2, a);
          delete (double*)data;
          break;
        }
        case MPV_FORMAT_INT64:
        {
          int64_t i = *(int64_t*)data;
          t_atom a[2];
          SETSYMBOL(a, sname);
          SETFLOAT(a+1, i);
          outlet_anything(m_prop_outlet, gensym("property"), 2, a);
          delete (int64_t*)data;
          break;
        }
        case MPV_FORMAT_STRING:
        {
          char* s = (char *)data;
          t_atom a[2];
          SETSYMBOL(a, sname);
          SETSYMBOL(a+1, gensym(s));
          outlet_anything(m_prop_outlet, gensym("property"), 2, a);
          break;
        }
        case MPV_FORMAT_NODE:
        {
          mpv_node* node = (mpv_node*)data;
          switch(node->format)
          {
            case MPV_FORMAT_STRING:
            {
              t_atom a[2];
              SETSYMBOL(a, sname);
              SETSYMBOL(a+1, gensym(node->u.string));
              outlet_anything(m_prop_outlet, gensym("property"), 2, a);
              break;
            }
            case MPV_FORMAT_FLAG:
            case MPV_FORMAT_INT64:
            case MPV_FORMAT_DOUBLE:
            {
              t_atom a[2];
              SETSYMBOL(a, sname);
              switch (node->format) {
                case MPV_FORMAT_FLAG:
                  SETFLOAT(a+1, node->u.flag);
                  break;
                case MPV_FORMAT_INT64:
                  SETFLOAT(a+1, node->u.int64);
                  break;
                case MPV_FORMAT_DOUBLE:
                  SETFLOAT(a+1, node->u.double_);
                  break;
                default:
                  return;
              }
              outlet_anything(m_prop_outlet, gensym("property"), 2, a);
              break;
            }
            default:
            {
              error("could not handle this node format : %d", node->format);
            }
          }
          mpv_free_node_contents(node);
        }
        case MPV_FORMAT_NODE_ARRAY:
        case MPV_FORMAT_NODE_MAP:
        {

        }
      }
    }
  }
}

void mpv::handle_prop_event(mpv_event_property *prop)
{
  if (strcmp(prop->name, "width") == 0) {
    if (prop->format == MPV_FORMAT_INT64) {
      int64_t val = *(int64_t *)prop->data;
      m_media_width = val;
      m_size_changed = true;
    }
  } else if (strcmp(prop->name, "height") == 0) {
    if (prop->format == MPV_FORMAT_INT64) {
      int64_t val = *(int64_t *)prop->data;
      m_media_height = val;
      m_size_changed = true;
    }
  } else {
    std::vector<t_atom> a;
    a.reserve(256);
    prop_to_atom(prop, a);
    outlet_anything(m_prop_outlet, gensym("property"), a.size(), a.data());
  }
}

void mpv :: dimen_mess(int width, int height)
{
  if(width < 0 && height < 0)
  {
    m_auto_resize = true;
    gemframebuffer::dimMess(m_media_width, m_media_height);
  }
  else
  {
    m_auto_resize = false;
    gemframebuffer::dimMess(width, height);
  }
}

void mpv::log_mess(std::string level)
{
  if(!m_mpv)
    return;

  auto err = mpv_request_log_messages(m_mpv, level.c_str());
  if(err != MPV_ERROR_SUCCESS)
  {
    error("can't set log level to '%s', error code: %d", level.c_str(), err);
  }
}

void mpv::obj_setupCallback(t_class *classPtr)
{
  gemframebuffer::obj_setupCallback(classPtr);

  CPPEXTERN_MSG2(classPtr, "dimen",  dimen_mess, int, int); // override gemframebuffer method
  CPPEXTERN_MSG(classPtr, "command",  command_mess);
  CPPEXTERN_MSG(classPtr, "command_typed",  command_mess);
  CPPEXTERN_MSG(classPtr, "property",  command_mess);
  CPPEXTERN_MSG(classPtr, "property_typed",  command_mess);
  CPPEXTERN_MSG1(classPtr, "log_level",  log_mess, std::string);
}
