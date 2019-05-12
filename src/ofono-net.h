#include <ofono/dbus.h>
#include "notifier.h"

gboolean ofono_net_register(const char *path, ofono_notify_fn cb, gpointer user_data);
void ofono_net_close(const char *path, ofono_notify_fn cb, gpointer user_data);
