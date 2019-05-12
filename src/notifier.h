#ifndef __ICD_OFONO_NOTIFIER_H__
#define __ICD_OFONO_NOTIFIER_H__

#include <glib.h>

typedef void (*ofono_notify_fn)(const gpointer data, gpointer user_data);

void ofono_notifier_register(GSList **notifiers, ofono_notify_fn cb, gpointer user_data);
void ofono_notifier_notify(GSList *notifiers, const gpointer data);
void ofono_notifier_close(GSList **notifiers, ofono_notify_fn cb, gpointer user_data);

#endif /* __ICD_OFONO_NOTIFIER_H__ */
