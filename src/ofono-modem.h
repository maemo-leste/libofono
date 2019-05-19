#include <ofono/dbus.h>
#include "notifier.h"

gboolean ofono_modem_register(const char *path, ofono_notify_fn cb, gpointer user_data);
void ofono_modem_close(const char *path, ofono_notify_fn cb, gpointer user_data);
