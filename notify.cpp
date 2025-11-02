extern "C" {
	#include <dbus-1.0/dbus/dbus.h>
}

#include <iostream>
#include "notify.h"

int zinute_galioja_ms = -1;
unsigned int zinutes_id = 0;

bool prideti_stringa(DBusMessageIter *iter, std::string str)
{
	const void * pointeris = str.c_str();
	return dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &pointeris);
}

void parodyti_pranesima(std::string zinute)
{
	DBusError klaida;
	dbus_error_init(&klaida);

	DBusConnection *jungtis = dbus_bus_get(DBUS_BUS_SESSION, &klaida);
	if (dbus_error_is_set(&klaida))
	{
		std::cerr << "Nepavyko prisijungti prie DBus: " << klaida.message;
		return;
	}

	if (!jungtis)
	{
		std::cerr << "Nesukurtas DBus prisijungimas";
		return;
	}

	DBusMessage *msg = dbus_message_new_method_call("org.freedesktop.Notifications", "/org/freedesktop/Notifications", "org.freedesktop.Notifications", "Notify");
	if (!msg)
	{
		std::cerr << "Nepavyko sukurti DBus žinutės";
		return ;
	}

	DBusMessageIter args, actions, hints;
	dbus_message_iter_init_append(msg, &args);

	prideti_stringa(&args, ""); //app_name
	dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &zinutes_id); //replaces_id
	prideti_stringa(&args, ""); //app_icon
	prideti_stringa(&args, zinute); //summary
	prideti_stringa(&args, ""); //body

	dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_TYPE_STRING_AS_STRING, &actions);
	dbus_message_iter_close_container(&args, &actions);

	dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, DBUS_DICT_ENTRY_BEGIN_CHAR_AS_STRING DBUS_TYPE_STRING_AS_STRING DBUS_TYPE_VARIANT_AS_STRING DBUS_DICT_ENTRY_END_CHAR_AS_STRING, &hints);
	dbus_message_iter_close_container(&args, &hints);

	dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &zinute_galioja_ms);

	DBusMessage *atsakymas = dbus_connection_send_with_reply_and_block(jungtis, msg, DBUS_TIMEOUT_USE_DEFAULT, &klaida);

	if (!atsakymas)
	{
		std::cerr << "Nepavyko išsiųsti DBus žinutės: " << klaida.message;
		return;
	}

	DBusMessageIter iter;
	dbus_message_iter_init(atsakymas, &iter);

	if (DBUS_TYPE_UINT32 == dbus_message_iter_get_arg_type(&iter))
		dbus_message_iter_get_basic(&iter, &zinutes_id); //Visada nautori tą pačią žinutę

	std::cerr << zinutes_id;

	dbus_message_unref(atsakymas);
	dbus_message_unref(msg);
}
