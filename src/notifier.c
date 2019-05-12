#include "notifier.h"

struct _notifier
{
  ofono_notify_fn cb;
  gpointer user_data;
};

typedef struct _notifier notifier;

void
ofono_notifier_register(GSList **notifiers, ofono_notify_fn cb,
                        gpointer user_data)
{
  notifier *n = g_new(notifier, 1);

  n->cb = cb;
  n->user_data = user_data;

  *notifiers = g_slist_append(*notifiers, n);
}

void
ofono_notifier_notify(GSList *notifiers, const gpointer data)
{
  GSList *l = notifiers;

  for (l = notifiers; l; l = l->next)
  {
    notifier *n = l->data;

    (n->cb)(data, n->user_data);
  }
}

void
ofono_notifier_close(GSList **notifiers, ofono_notify_fn cb, gpointer user_data)
{
  if (cb || user_data)
  {
    GSList *l = *notifiers;

    while(l)
    {
      notifier *n = l->data;

      if (n->cb == cb && n->user_data == user_data)
      {
        l = *notifiers = g_slist_remove(*notifiers, n);
        g_free(n);
      }
      else
        l = l->next;
    }
  }
  else
  {
    g_slist_free_full(*notifiers, g_free);
    *notifiers = NULL;
  }
}
