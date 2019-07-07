#pragma once

#include "gemframebuffer.h"

#include <mpv/client.h>
#include <mpv/opengl_cb.h>

#include <atomic>
#include <string.h>

// some helper functions
// from qthelper.hpp (see qt_opengl in mpv-examples)

struct node_builder {
    node_builder(int argc, t_atom* argv, std::string types = {}) {
      m_types=types;
      char type{};
      if(!m_types.empty())
        type = m_types.at(0);
      set(&node_, argc, argv, type);
    }
    ~node_builder() {
      free_node(&node_);
    }
    mpv_node *node() { return &node_; }
  private:
    // disable copy by making copy constructor
    // and copy assignment operators private
    node_builder(const node_builder&);
    node_builder& operator=(const node_builder&);

    mpv_node node_;
    std::string m_types;
    unsigned int m_index;

    mpv_node_list *create_list(mpv_node *dst, bool is_map, int num) {
      dst->format = is_map ? MPV_FORMAT_NODE_MAP : MPV_FORMAT_NODE_ARRAY;
      mpv_node_list *list = new mpv_node_list();
      dst->u.list = list;
      if (!list)
        goto err;
      list->values = new mpv_node[num]();
      if (!list->values)
        goto err;
      if (is_map) {
        list->keys = new char*[num]();
        if (!list->keys)
          goto err;
      }
      return list;
    err:
      free_node(dst);
      return NULL;
    }
    /*
    char *dup_qstring(const QString &s) {
        QByteArray b = s.toUtf8();
        char *r = new char[b.size() + 1];
        if (r)
            std::memcpy(r, b.data(), b.size() + 1);
        return r;
    }
    bool test_type(const QVariant &v, QMetaType::Type t) {
        // The Qt docs say: "Although this function is declared as returning
        // "QVariant::Type(obsolete), the return value should be interpreted
        // as QMetaType::Type."
        // So a cast really seems to be needed to avoid warnings (urgh).
        return static_cast<int>(v.type()) == static_cast<int>(t);
    }
    */
    void set(mpv_node *dst, int argc, t_atom* argv, char type = 0) {
      if(argc == 1)
      {
        if (argv->a_type == A_SYMBOL) {
          t_symbol* s = argv->a_w.w_symbol;
          if(!s)
            return;
          dst->format = MPV_FORMAT_STRING;
          dst->u.string = strdup(s->s_name);
          if (!dst->u.string)
            goto fail;
        } else if (argv->a_type == A_FLOAT) {
          float f = argv->a_w.w_float;
          switch(type)
          {
            case 'b':
              dst->format = MPV_FORMAT_FLAG;
              dst->u.flag = f > 0;
              break;
            case 'i':
              dst->format = MPV_FORMAT_INT64;
              dst->u.int64 = f;
              break;
            case 'd':
            default:
              dst->format = MPV_FORMAT_DOUBLE;
              dst->u.double_ = f;
          }
        }
      }
      else
      {
        mpv_node_list *list = create_list(dst, false, argc);
        if (!list)
          goto fail;
        list->num = argc;
        for (int n = 0; n < argc; n++)
        {
          char tag{};
          if(m_types != std::string{})
          {
            if(n > m_types.size())
            {
              error("types size < argc");
            }
            else
              tag = m_types.at(n);
          }
          set(&list->values[n], 1, argv+n, tag);
        }
      }
      return;
    fail:
      error("can't process command");
      dst->format = MPV_FORMAT_NONE;
    }

    void free_node(mpv_node *dst) {
      switch (dst->format) {
        case MPV_FORMAT_STRING:
          delete[] dst->u.string;
          break;
        case MPV_FORMAT_NODE_ARRAY:
        case MPV_FORMAT_NODE_MAP: {
          mpv_node_list *list = dst->u.list;
          if (list) {
            for (int n = 0; n < list->num; n++) {
              if (list->keys)
                delete[] list->keys[n];
              if (list->values)
                free_node(&list->values[n]);
            }
            delete[] list->keys;
            delete[] list->values;
          }
          delete list;
          break;
        }
        default: ;
      }
      dst->format = MPV_FORMAT_NONE;
    }
};
struct node_autofree {
    mpv_node *ptr;
    node_autofree(mpv_node *a_ptr) : ptr(a_ptr) {}
    ~node_autofree() { mpv_free_node_contents(ptr); }
};

class GEM_EXTERN mpv : public gemframebuffer
{
    CPPEXTERN_HEADER(mpv, GemBase);

  public:
    mpv(int argc, t_atom*argv);
    ~mpv();

    virtual void startRendering(void);
    virtual void stopRendering(void);
    virtual void render(GemState *state);

    void command_mess(t_symbol* s, int argc, t_atom* argv);
    void dimen_mess(int width, int height);
    void log_mess(std::string);

    void inline rise_event_flag(){ m_event_flag = true; };

  private:
    void handle_prop_event(mpv_event_property* prop);

    mpv_handle* m_mpv{};

    mpv_opengl_cb_context* m_mpv_gl{};

    bool m_init{};
    t_symbol* m_file{};
    bool m_pause{};
    std::atomic_bool m_event_flag; // flag rised when new event arrived
    bool m_size_changed{};
    bool m_auto_resize{};

    int m_media_width{512};
    int m_media_height{512};

    t_outlet* m_prop_outlet{};
};
