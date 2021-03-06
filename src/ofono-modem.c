#include <glib.h>

#include <string.h>

#include "ofono-modem.h"
#include "log.h"
#include "dbus-helpers.h"
#include "modem.h"

static GHashTable *modems = NULL;

static void
ofono_modem_notifier_notify(const char *path, gpointer data)
{
  GSList *notifiers = NULL;

  if (modems)
    notifiers = g_hash_table_lookup(modems, path);

  if (notifiers)
    ofono_notifier_notify(notifiers, data);
}

static gboolean
ofono_modem_property_changed(const char *path, DBusMessageIter *iter)
{
  property_changed pc;
  gboolean rv = FALSE;

  OFONO_ENTER

  if (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING)
  {
    dbus_message_iter_get_basic(iter, &pc.property);

    if (!strcmp(pc.property, "Interfaces"))
    {
      pc.val.u64 = 0;
      dbus_message_iter_next(iter);

      if (dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_VARIANT)
      {
        DBusMessageIter variant_iter;

        dbus_message_iter_recurse(iter, &variant_iter);

        if (dbus_message_iter_get_arg_type(&variant_iter) == DBUS_TYPE_ARRAY)
        {
          DBusMessageIter array_iter;

          dbus_message_iter_recurse(&variant_iter, &array_iter);

          while (dbus_message_iter_get_arg_type(&array_iter) ==
                 DBUS_TYPE_STRING)
          {
            const char *iface;

            dbus_message_iter_get_basic(&array_iter, &iface);

            if (!strcmp(iface, "org.ofono.SimManager"))
                pc.val.u64 |= OFONO_MODEM_INTERFACE_SIM_MANAGER;
            else if (!strcmp(iface, "org.ofono.LongTermEvolution"))
                pc.val.u64 |= OFONO_MODEM_INTERFACE_LTE;
            else if (!strcmp(iface, "org.ofono.NetworkRegistration"))
                pc.val.u64 |= OFONO_MODEM_INTERFACE_NETWORK_REGISTRATION;
            else if (!strcmp(iface, "org.ofono.ConnectionManager"))
                pc.val.u64 |= OFONO_MODEM_INTERFACE_CONNECTION_MANAGER;

            dbus_message_iter_next(&array_iter);
          }

          ofono_modem_notifier_notify(path, &pc);
        }
      }
    }
    else
    {
      int type = dbus_helper_read_basic_property(iter, &pc.property, &pc.val);

      if (type != DBUS_TYPE_INVALID)
      {
        if (dbus_helper_is_basic_type(type))
          ofono_modem_notifier_notify(path, &pc);

        rv = TRUE;
      }
    }
  }

  OFONO_EXIT

  return rv;
}

static void
ofono_modem_get_properties_cb(DBusPendingCall *pending, void *user_data)
{
  DBusMessage *reply;
  gchar *path = user_data;

  OFONO_ENTER

  if (pending)
  {
    reply = dbus_pending_call_steal_reply(pending);
    dbus_pending_call_unref(pending);
  }

  if (pending && reply)
  {
    if (dbus_message_get_type(reply) != DBUS_MESSAGE_TYPE_ERROR)
    {
      DBusMessageIter iter;

      dbus_message_iter_init(reply, &iter);

      if (dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY)
      {
        DBusMessageIter array_iter;

        dbus_message_iter_recurse(&iter, &array_iter);

        while (dbus_message_iter_get_arg_type(&array_iter) ==
               DBUS_TYPE_DICT_ENTRY)
        {
          DBusMessageIter dict_iter;

          dbus_message_iter_recurse(&array_iter, &dict_iter);
          ofono_modem_property_changed(path, &dict_iter);

          dbus_message_iter_next(&array_iter);
        }
      }
    }
    else
    {
      OFONO_WARN("GetProperties returned '%s'",
                 dbus_message_get_error_name(reply));
    }

    dbus_message_unref(reply);
  }

  g_free(path);

  OFONO_EXIT
}

static gboolean
ofono_modem_init(const char *path)
{
  DBusMessage *message;
  gboolean rv = FALSE;
  OFONO_ENTER

  message = dbus_message_new_method_call(OFONO_SERVICE, path,
                                         OFONO_MODEM_INTERFACE,
                                         "GetProperties");

  if (message)
  {
    gchar *_path = g_strdup(path);

    if (icd_dbus_send_system_mcall(message, -1, ofono_modem_get_properties_cb,
                                   _path))
    {
      rv = TRUE;
    }
    else
    {
      g_free(_path);
      OFONO_ERR("could not send 'GetProperties' message");
    }

    dbus_message_unref(message);
  }
  else
    OFONO_ERR("could not create 'GetProperties' method call");

  OFONO_EXIT

  return rv;
}

static DBusHandlerResult
ofono_modem_filter(DBusConnection *connection, DBusMessage *message,
                   void *user_data)
{
  OFONO_ENTER

  if (dbus_message_is_signal(message,
                             OFONO_MODEM_INTERFACE,
                             "PropertyChanged"))
  {
    DBusMessageIter iter;

    if (dbus_message_iter_init(message, &iter))
      ofono_modem_property_changed(dbus_message_get_path(message), &iter);
    else
      OFONO_WARN("Invalid arguments for PropertyChanged signal");
  }

  OFONO_EXIT

  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static gboolean
ofono_modem_add_dbus_filter()
{
  return icd_dbus_connect_system_bcast_signal(
        OFONO_MODEM_INTERFACE, ofono_modem_filter, NULL, NULL);
}

static void
ofono_modem_remove_dbus_filter()
{
  icd_dbus_disconnect_system_bcast_signal(
    OFONO_MODEM_INTERFACE, ofono_modem_filter, NULL, NULL);
}

gboolean
ofono_modem_register(const char *path, ofono_notify_fn cb, gpointer user_data)
{
  gboolean rv = TRUE;
  GSList *notifiers;

  if (!modems)
    modems = g_hash_table_new_full(g_str_hash, g_str_equal, g_free, NULL);

  notifiers = g_hash_table_lookup(modems, path);

  if (!notifiers)
  {
    if ((rv = ofono_modem_init(path)))
        rv = ofono_modem_add_dbus_filter();
  }

  if (rv)
  {
    ofono_notifier_register(&notifiers, cb, user_data);
    g_hash_table_insert(modems, g_strdup(path), notifiers);
  }

  return rv;
}

void
ofono_modem_close(const char *path, ofono_notify_fn cb, gpointer user_data)
{
  GSList *notifiers;

  if (!modems)
      return;

  notifiers = g_hash_table_lookup(modems, path);

  if (notifiers)
  {
    ofono_notifier_close(&notifiers, cb, user_data);

    if (!notifiers)
    {
      g_hash_table_remove(modems, path);

      if (!g_hash_table_size(modems))
      {
        ofono_modem_remove_dbus_filter();
        g_hash_table_unref(modems);
        modems = NULL;
      }
    }
    else
      g_hash_table_insert(modems, g_strdup(path), notifiers);
  }
}
