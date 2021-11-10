// This is generated file. Do not modify directly.

#include "modules/desktop_capture/linux/pipewire_stubs.h"

#include <stdlib.h>  // For NULL.
#include <dlfcn.h>   // For dlsym, dlopen.

#include <map>
#include <vector>

// The extra include header needed in the generated stub file for defining
// various PipeWire types.

extern "C" {

#include <pipewire/pipewire.h>

}

extern "C" {

// Static pointers that will hold the location of the real function
// implementations after the module has been loaded.
static void (*pw_core_destroy_ptr)(pw_core *core) = NULL;
static pw_type * (*pw_core_get_type_ptr)(pw_core *core) = NULL;
static pw_core * (*pw_core_new_ptr)(pw_loop *main_loop, pw_properties *props) = NULL;
static void (*pw_loop_destroy_ptr)(pw_loop *loop) = NULL;
static pw_loop * (*pw_loop_new_ptr)(pw_properties *properties) = NULL;
static void (*pw_init_ptr)(int *argc, char **argv[]) = NULL;
static pw_properties * (*pw_properties_new_string_ptr)(const char *args) = NULL;
static void (*pw_remote_add_listener_ptr)(pw_remote *remote, spa_hook *listener, const pw_remote_events *events, void *data) = NULL;
static int (*pw_remote_connect_fd_ptr)(pw_remote *remote, int fd) = NULL;
static void (*pw_remote_destroy_ptr)(pw_remote *remote) = NULL;
static pw_remote * (*pw_remote_new_ptr)(pw_core *core, pw_properties *properties, size_t user_data_size) = NULL;
static void (*pw_stream_add_listener_ptr)(pw_stream *stream, spa_hook *listener, const pw_stream_events *events, void *data) = NULL;
static int (*pw_stream_connect_ptr)(pw_stream *stream, enum pw_direction direction, const char *port_path, enum pw_stream_flags flags, const spa_pod **params, uint32_t n_params) = NULL;
static pw_buffer * (*pw_stream_dequeue_buffer_ptr)(pw_stream *stream) = NULL;
static void (*pw_stream_destroy_ptr)(pw_stream *stream) = NULL;
static void (*pw_stream_finish_format_ptr)(pw_stream *stream, int res, const spa_pod **params, uint32_t n_params) = NULL;
static pw_stream * (*pw_stream_new_ptr)(pw_remote *remote, const char *name, pw_properties *props) = NULL;
static int (*pw_stream_queue_buffer_ptr)(pw_stream *stream, pw_buffer *buffer) = NULL;
static int (*pw_stream_set_active_ptr)(pw_stream *stream, bool active) = NULL;
static void (*pw_thread_loop_destroy_ptr)(pw_thread_loop *loop) = NULL;
static pw_thread_loop * (*pw_thread_loop_new_ptr)(pw_loop *loop, const char *name) = NULL;
static int (*pw_thread_loop_start_ptr)(pw_thread_loop *loop) = NULL;
static void (*pw_thread_loop_stop_ptr)(pw_thread_loop *loop) = NULL;

// Stubs that dispatch to the real implementations.
extern void pw_core_destroy(pw_core *core) __attribute__((weak));
void  pw_core_destroy(pw_core *core) {
  pw_core_destroy_ptr(core);
}
extern pw_type * pw_core_get_type(pw_core *core) __attribute__((weak));
pw_type *  pw_core_get_type(pw_core *core) {
  return pw_core_get_type_ptr(core);
}
extern pw_core * pw_core_new(pw_loop *main_loop, pw_properties *props) __attribute__((weak));
pw_core *  pw_core_new(pw_loop *main_loop, pw_properties *props) {
  return pw_core_new_ptr(main_loop, props);
}
extern void pw_loop_destroy(pw_loop *loop) __attribute__((weak));
void  pw_loop_destroy(pw_loop *loop) {
  pw_loop_destroy_ptr(loop);
}
extern pw_loop * pw_loop_new(pw_properties *properties) __attribute__((weak));
pw_loop *  pw_loop_new(pw_properties *properties) {
  return pw_loop_new_ptr(properties);
}
extern void pw_init(int *argc, char **argv[]) __attribute__((weak));
void  pw_init(int *argc, char **argv[]) {
  pw_init_ptr(argc, argv);
}
extern pw_properties * pw_properties_new_string(const char *args) __attribute__((weak));
pw_properties *  pw_properties_new_string(const char *args) {
  return pw_properties_new_string_ptr(args);
}
extern void pw_remote_add_listener(pw_remote *remote, spa_hook *listener, const pw_remote_events *events, void *data) __attribute__((weak));
void  pw_remote_add_listener(pw_remote *remote, spa_hook *listener, const pw_remote_events *events, void *data) {
  pw_remote_add_listener_ptr(remote, listener, events, data);
}
extern int pw_remote_connect_fd(pw_remote *remote, int fd) __attribute__((weak));
int  pw_remote_connect_fd(pw_remote *remote, int fd) {
  return pw_remote_connect_fd_ptr(remote, fd);
}
extern void pw_remote_destroy(pw_remote *remote) __attribute__((weak));
void  pw_remote_destroy(pw_remote *remote) {
  pw_remote_destroy_ptr(remote);
}
extern pw_remote * pw_remote_new(pw_core *core, pw_properties *properties, size_t user_data_size) __attribute__((weak));
pw_remote *  pw_remote_new(pw_core *core, pw_properties *properties, size_t user_data_size) {
  return pw_remote_new_ptr(core, properties, user_data_size);
}
extern void pw_stream_add_listener(pw_stream *stream, spa_hook *listener, const pw_stream_events *events, void *data) __attribute__((weak));
void  pw_stream_add_listener(pw_stream *stream, spa_hook *listener, const pw_stream_events *events, void *data) {
  pw_stream_add_listener_ptr(stream, listener, events, data);
}
extern int pw_stream_connect(pw_stream *stream, enum pw_direction direction, const char *port_path, enum pw_stream_flags flags, const spa_pod **params, uint32_t n_params) __attribute__((weak));
int  pw_stream_connect(pw_stream *stream, enum pw_direction direction, const char *port_path, enum pw_stream_flags flags, const spa_pod **params, uint32_t n_params) {
  return pw_stream_connect_ptr(stream, direction, port_path, flags, params, n_params);
}
extern pw_buffer * pw_stream_dequeue_buffer(pw_stream *stream) __attribute__((weak));
pw_buffer *  pw_stream_dequeue_buffer(pw_stream *stream) {
  return pw_stream_dequeue_buffer_ptr(stream);
}
extern void pw_stream_destroy(pw_stream *stream) __attribute__((weak));
void  pw_stream_destroy(pw_stream *stream) {
  pw_stream_destroy_ptr(stream);
}
extern void pw_stream_finish_format(pw_stream *stream, int res, const spa_pod **params, uint32_t n_params) __attribute__((weak));
void  pw_stream_finish_format(pw_stream *stream, int res, const spa_pod **params, uint32_t n_params) {
  pw_stream_finish_format_ptr(stream, res, params, n_params);
}
extern pw_stream * pw_stream_new(pw_remote *remote, const char *name, pw_properties *props) __attribute__((weak));
pw_stream *  pw_stream_new(pw_remote *remote, const char *name, pw_properties *props) {
  return pw_stream_new_ptr(remote, name, props);
}
extern int pw_stream_queue_buffer(pw_stream *stream, pw_buffer *buffer) __attribute__((weak));
int  pw_stream_queue_buffer(pw_stream *stream, pw_buffer *buffer) {
  return pw_stream_queue_buffer_ptr(stream, buffer);
}
extern int pw_stream_set_active(pw_stream *stream, bool active) __attribute__((weak));
int  pw_stream_set_active(pw_stream *stream, bool active) {
  return pw_stream_set_active_ptr(stream, active);
}
extern void pw_thread_loop_destroy(pw_thread_loop *loop) __attribute__((weak));
void  pw_thread_loop_destroy(pw_thread_loop *loop) {
  pw_thread_loop_destroy_ptr(loop);
}
extern pw_thread_loop * pw_thread_loop_new(pw_loop *loop, const char *name) __attribute__((weak));
pw_thread_loop *  pw_thread_loop_new(pw_loop *loop, const char *name) {
  return pw_thread_loop_new_ptr(loop, name);
}
extern int pw_thread_loop_start(pw_thread_loop *loop) __attribute__((weak));
int  pw_thread_loop_start(pw_thread_loop *loop) {
  return pw_thread_loop_start_ptr(loop);
}
extern void pw_thread_loop_stop(pw_thread_loop *loop) __attribute__((weak));
void  pw_thread_loop_stop(pw_thread_loop *loop) {
  pw_thread_loop_stop_ptr(loop);
}

}  // extern "C"


namespace modules_desktop_capture_linux {

// Returns true if all stubs have been properly initialized.
bool IsPipewireInitialized() {
  if (pw_core_destroy_ptr &&
      pw_core_get_type_ptr &&
      pw_core_new_ptr &&
      pw_loop_destroy_ptr &&
      pw_loop_new_ptr &&
      pw_init_ptr &&
      pw_properties_new_string_ptr &&
      pw_remote_add_listener_ptr &&
      pw_remote_connect_fd_ptr &&
      pw_remote_destroy_ptr &&
      pw_remote_new_ptr &&
      pw_stream_add_listener_ptr &&
      pw_stream_connect_ptr &&
      pw_stream_dequeue_buffer_ptr &&
      pw_stream_destroy_ptr &&
      pw_stream_finish_format_ptr &&
      pw_stream_new_ptr &&
      pw_stream_queue_buffer_ptr &&
      pw_stream_set_active_ptr &&
      pw_thread_loop_destroy_ptr &&
      pw_thread_loop_new_ptr &&
      pw_thread_loop_start_ptr &&
      pw_thread_loop_stop_ptr) {
    return true;
  } else {
    return false;
  }
}

// Initializes the module stubs.
void InitializePipewire(void* module) {
  pw_core_destroy_ptr =
    reinterpret_cast<void (*)(pw_core *core)>(
      dlsym(module, "pw_core_destroy"));
  if (!pw_core_destroy_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_core_destroy, dlerror() says:\n"
      << dlerror();
  }
  pw_core_get_type_ptr =
    reinterpret_cast<pw_type * (*)(pw_core *core)>(
      dlsym(module, "pw_core_get_type"));
  if (!pw_core_get_type_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_core_get_type, dlerror() says:\n"
      << dlerror();
  }
  pw_core_new_ptr =
    reinterpret_cast<pw_core * (*)(pw_loop *main_loop, pw_properties *props)>(
      dlsym(module, "pw_core_new"));
  if (!pw_core_new_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_core_new, dlerror() says:\n"
      << dlerror();
  }
  pw_loop_destroy_ptr =
    reinterpret_cast<void (*)(pw_loop *loop)>(
      dlsym(module, "pw_loop_destroy"));
  if (!pw_loop_destroy_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_loop_destroy, dlerror() says:\n"
      << dlerror();
  }
  pw_loop_new_ptr =
    reinterpret_cast<pw_loop * (*)(pw_properties *properties)>(
      dlsym(module, "pw_loop_new"));
  if (!pw_loop_new_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_loop_new, dlerror() says:\n"
      << dlerror();
  }
  pw_init_ptr =
    reinterpret_cast<void (*)(int *argc, char **argv[])>(
      dlsym(module, "pw_init"));
  if (!pw_init_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_init, dlerror() says:\n"
      << dlerror();
  }
  pw_properties_new_string_ptr =
    reinterpret_cast<pw_properties * (*)(const char *args)>(
      dlsym(module, "pw_properties_new_string"));
  if (!pw_properties_new_string_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_properties_new_string, dlerror() says:\n"
      << dlerror();
  }
  pw_remote_add_listener_ptr =
    reinterpret_cast<void (*)(pw_remote *remote, spa_hook *listener, const pw_remote_events *events, void *data)>(
      dlsym(module, "pw_remote_add_listener"));
  if (!pw_remote_add_listener_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_remote_add_listener, dlerror() says:\n"
      << dlerror();
  }
  pw_remote_connect_fd_ptr =
    reinterpret_cast<int (*)(pw_remote *remote, int fd)>(
      dlsym(module, "pw_remote_connect_fd"));
  if (!pw_remote_connect_fd_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_remote_connect_fd, dlerror() says:\n"
      << dlerror();
  }
  pw_remote_destroy_ptr =
    reinterpret_cast<void (*)(pw_remote *remote)>(
      dlsym(module, "pw_remote_destroy"));
  if (!pw_remote_destroy_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_remote_destroy, dlerror() says:\n"
      << dlerror();
  }
  pw_remote_new_ptr =
    reinterpret_cast<pw_remote * (*)(pw_core *core, pw_properties *properties, size_t user_data_size)>(
      dlsym(module, "pw_remote_new"));
  if (!pw_remote_new_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_remote_new, dlerror() says:\n"
      << dlerror();
  }
  pw_stream_add_listener_ptr =
    reinterpret_cast<void (*)(pw_stream *stream, spa_hook *listener, const pw_stream_events *events, void *data)>(
      dlsym(module, "pw_stream_add_listener"));
  if (!pw_stream_add_listener_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_stream_add_listener, dlerror() says:\n"
      << dlerror();
  }
  pw_stream_connect_ptr =
    reinterpret_cast<int (*)(pw_stream *stream, enum pw_direction direction, const char *port_path, enum pw_stream_flags flags, const spa_pod **params, uint32_t n_params)>(
      dlsym(module, "pw_stream_connect"));
  if (!pw_stream_connect_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_stream_connect, dlerror() says:\n"
      << dlerror();
  }
  pw_stream_dequeue_buffer_ptr =
    reinterpret_cast<pw_buffer * (*)(pw_stream *stream)>(
      dlsym(module, "pw_stream_dequeue_buffer"));
  if (!pw_stream_dequeue_buffer_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_stream_dequeue_buffer, dlerror() says:\n"
      << dlerror();
  }
  pw_stream_destroy_ptr =
    reinterpret_cast<void (*)(pw_stream *stream)>(
      dlsym(module, "pw_stream_destroy"));
  if (!pw_stream_destroy_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_stream_destroy, dlerror() says:\n"
      << dlerror();
  }
  pw_stream_finish_format_ptr =
    reinterpret_cast<void (*)(pw_stream *stream, int res, const spa_pod **params, uint32_t n_params)>(
      dlsym(module, "pw_stream_finish_format"));
  if (!pw_stream_finish_format_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_stream_finish_format, dlerror() says:\n"
      << dlerror();
  }
  pw_stream_new_ptr =
    reinterpret_cast<pw_stream * (*)(pw_remote *remote, const char *name, pw_properties *props)>(
      dlsym(module, "pw_stream_new"));
  if (!pw_stream_new_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_stream_new, dlerror() says:\n"
      << dlerror();
  }
  pw_stream_queue_buffer_ptr =
    reinterpret_cast<int (*)(pw_stream *stream, pw_buffer *buffer)>(
      dlsym(module, "pw_stream_queue_buffer"));
  if (!pw_stream_queue_buffer_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_stream_queue_buffer, dlerror() says:\n"
      << dlerror();
  }
  pw_stream_set_active_ptr =
    reinterpret_cast<int (*)(pw_stream *stream, bool active)>(
      dlsym(module, "pw_stream_set_active"));
  if (!pw_stream_set_active_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_stream_set_active, dlerror() says:\n"
      << dlerror();
  }
  pw_thread_loop_destroy_ptr =
    reinterpret_cast<void (*)(pw_thread_loop *loop)>(
      dlsym(module, "pw_thread_loop_destroy"));
  if (!pw_thread_loop_destroy_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_thread_loop_destroy, dlerror() says:\n"
      << dlerror();
  }
  pw_thread_loop_new_ptr =
    reinterpret_cast<pw_thread_loop * (*)(pw_loop *loop, const char *name)>(
      dlsym(module, "pw_thread_loop_new"));
  if (!pw_thread_loop_new_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_thread_loop_new, dlerror() says:\n"
      << dlerror();
  }
  pw_thread_loop_start_ptr =
    reinterpret_cast<int (*)(pw_thread_loop *loop)>(
      dlsym(module, "pw_thread_loop_start"));
  if (!pw_thread_loop_start_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_thread_loop_start, dlerror() says:\n"
      << dlerror();
  }
  pw_thread_loop_stop_ptr =
    reinterpret_cast<void (*)(pw_thread_loop *loop)>(
      dlsym(module, "pw_thread_loop_stop"));
  if (!pw_thread_loop_stop_ptr) {
    RTC_LOG(LS_VERBOSE) << "Couldn't load pw_thread_loop_stop, dlerror() says:\n"
      << dlerror();
  }
}

// Uninitialize the module stubs.  Reset pointers to NULL.
void UninitializePipewire() {
  pw_core_destroy_ptr = NULL;
  pw_core_get_type_ptr = NULL;
  pw_core_new_ptr = NULL;
  pw_loop_destroy_ptr = NULL;
  pw_loop_new_ptr = NULL;
  pw_init_ptr = NULL;
  pw_properties_new_string_ptr = NULL;
  pw_remote_add_listener_ptr = NULL;
  pw_remote_connect_fd_ptr = NULL;
  pw_remote_destroy_ptr = NULL;
  pw_remote_new_ptr = NULL;
  pw_stream_add_listener_ptr = NULL;
  pw_stream_connect_ptr = NULL;
  pw_stream_dequeue_buffer_ptr = NULL;
  pw_stream_destroy_ptr = NULL;
  pw_stream_finish_format_ptr = NULL;
  pw_stream_new_ptr = NULL;
  pw_stream_queue_buffer_ptr = NULL;
  pw_stream_set_active_ptr = NULL;
  pw_thread_loop_destroy_ptr = NULL;
  pw_thread_loop_new_ptr = NULL;
  pw_thread_loop_start_ptr = NULL;
  pw_thread_loop_stop_ptr = NULL;
}

}  // namespace modules_desktop_capture_linux

namespace modules_desktop_capture_linux {
typedef std::map<StubModules, void*> StubHandleMap;
static void CloseLibraries(StubHandleMap* stub_handles) {
  for (StubHandleMap::const_iterator it = stub_handles->begin();
       it != stub_handles->end();
       ++it) {
    dlclose(it->second);
  }

  stub_handles->clear();
}
bool InitializeStubs(const StubPathMap& path_map) {
  StubHandleMap opened_libraries;
  for (int i = 0; i < kNumStubModules; ++i) {
    StubModules cur_module = static_cast<StubModules>(i);
    // If a module is missing, we fail.
    StubPathMap::const_iterator it = path_map.find(cur_module);
    if (it == path_map.end()) {
      CloseLibraries(&opened_libraries);
      return false;
    }

    // Otherwise, attempt to dlopen the library.
    const std::vector<std::string>& paths = it->second;
    bool module_opened = false;
    for (std::vector<std::string>::const_iterator dso_path = paths.begin();
         !module_opened && dso_path != paths.end();
         ++dso_path) {
      void* handle = dlopen(dso_path->c_str(), RTLD_LAZY);
      if (handle != NULL) {
        module_opened = true;
        opened_libraries[cur_module] = handle;
      } else {
        RTC_LOG(LS_VERBOSE) << "dlopen(" << dso_path->c_str() << ") failed, "
                << "dlerror() says:\n" << dlerror();
      }
    }

    if (!module_opened) {
      CloseLibraries(&opened_libraries);
      return false;
    }
  }

  // Initialize each module if we have not already failed.
  InitializePipewire(opened_libraries[kModulePipewire]);

  // Check that each module is initialized correctly.
  // Close all previously opened libraries on failure.
  if (!IsPipewireInitialized()) {
    UninitializePipewire();
    CloseLibraries(&opened_libraries);
    return false;
  }

  return true;
}

}  // namespace modules_desktop_capture_linux
