extern "C" {
	#include <xdo.h>
}

#include <libcec/cec.h>

// cecloader.h uses std::cout _without_ including iosfwd or iostream
// Furthermore is uses cout and not std::cout
#include <iostream>
using std::cout;
using std::endl;
#include <libcec/cecloader.h>

#include <unordered_map>
#include <algorithm>
#include <array>
#include <chrono>
#include <thread>

// Reikalingas aptikti Ctrl+C
#include <signal.h>

#include "notify.h"

xdo_t * x11;
bool jau_išjungti = false;
bool ar_nuspaustas_alt = false;
bool ar_nuspaustas_shift = false;

std::unordered_map<int, std::string> pultelio_veiksmai({
	//{pultelio mygtukas, klaviatūros mygtukas},

	{CEC::CEC_USER_CONTROL_CODE_SELECT, "Return"},
	{CEC::CEC_USER_CONTROL_CODE_EXIT, "BackSpace"},
	{CEC::CEC_USER_CONTROL_CODE_ROOT_MENU, "Control+Shift+Alt+F5"},
	{CEC::CEC_USER_CONTROL_CODE_F2_RED, "Escape"},
	{CEC::CEC_USER_CONTROL_CODE_F3_GREEN, "Tab"},
	{CEC::CEC_USER_CONTROL_CODE_F4_YELLOW, "Tab"},
	{CEC::CEC_USER_CONTROL_CODE_PLAY, "space"},
	{CEC::CEC_USER_CONTROL_CODE_PAUSE, "XF86AudioPause"},
	{CEC::CEC_USER_CONTROL_CODE_REWIND, "XF86AudioPrev"},
	{CEC::CEC_USER_CONTROL_CODE_FAST_FORWARD, "XF86AudioNext"},
	{CEC::CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE, "Super_L"},
	{CEC::CEC_USER_CONTROL_CODE_STOP, "f"},
	{CEC::CEC_USER_CONTROL_CODE_SOUND_SELECT, "Alt+F4"},
	{CEC::CEC_OPCODE_RECORD_STATUS, "Menu"},
});

//Kai mygtukas nuspasustas ir laikomas nuspaustas
std::unordered_map<int, std::string> pultelio_veiksmai_kai_nuspausa({
	//{pultelio mygtukas, klaviatūros mygtukas},

	{CEC::CEC_USER_CONTROL_CODE_UP, "Up"},
	{CEC::CEC_USER_CONTROL_CODE_DOWN, "Down"},
	{CEC::CEC_USER_CONTROL_CODE_LEFT, "Left"},
	{CEC::CEC_USER_CONTROL_CODE_RIGHT, "Right"},
	{CEC::CEC_USER_CONTROL_CODE_BACKWARD, "XF86AudioLowerVolume"},
	{CEC::CEC_USER_CONTROL_CODE_FORWARD, "XF86AudioRaiseVolume"},
});

void kai_sisteminis_signalas(int)
{
	jau_išjungti = true;
}

void kai_puletio_mygtukas(void*, const CEC::cec_keypress* msg)
{
    std::string mygtukas;
	std::string zinute;

	// Jei mygtukas nuspaustas, duration bus 0. Jei laikysi myguką nuspaustą, bus
	// daug 'kai_puletio_mygtukas' iškvietimų su duration 0. Kai atleisi - duration
	// bus kiek laiko spaudei myguką.
	// Mes reaguojami tik kai mygtukas atspaustas.
	if ( msg->duration )
	{
		//Mėlynas mygtukas pas mus įpatingas - jis bus Alt mygukas.
		//Vieną kartą paspaudi: Alt nuspaudžiamas, paspaudi dar kartą - atleidžiamas
		if (msg->keycode == CEC::CEC_USER_CONTROL_CODE_F1_BLUE)
		{
			if( ar_nuspaustas_shift )
				zinute = "SHIFT+";

			if( ar_nuspaustas_alt )
			{
				xdo_send_keysequence_window_up(x11, CURRENTWINDOW, "Alt", 0);
				zinute += "be alt";
			}
			else
			{
				xdo_send_keysequence_window_down(x11, CURRENTWINDOW, "Alt", 0);
				zinute += "ALT+...";
			}

			ar_nuspaustas_alt = !ar_nuspaustas_alt;
		}
		//Geltonas mygtukas pas mus irgi įpatingas - jis bus Shift mygukas.
		//Vieną kartą paspaudi: Shift nuspaudžiamas, paspaudi dar kartą - atleidžiamas
		else if (msg->keycode == CEC::CEC_USER_CONTROL_CODE_F4_YELLOW)
		{
			if( ar_nuspaustas_alt )
				zinute = "ALT+";

			if( ar_nuspaustas_shift )
			{
				xdo_send_keysequence_window_up(x11, CURRENTWINDOW, "Shift", 0);
				zinute += "be shift";
			}
			else
			{
				xdo_send_keysequence_window_down(x11, CURRENTWINDOW, "Shift", 0);
				zinute += "SHIFT+...";
			}

			ar_nuspaustas_shift = !ar_nuspaustas_shift;
		}
		else if ( pultelio_veiksmai.find(msg->keycode) != pultelio_veiksmai.end() ) //ar mygtukas sumapintas
		{
			xdo_send_keysequence_window(x11, CURRENTWINDOW, "XF86WakeUp", 0);
			xdo_send_keysequence_window(x11, CURRENTWINDOW, pultelio_veiksmai[msg->keycode].data(), 0);
		}
	}
	else if ( pultelio_veiksmai_kai_nuspausa.find(msg->keycode) != pultelio_veiksmai_kai_nuspausa.end() )
	{
		xdo_send_keysequence_window(x11, CURRENTWINDOW, "XF86WakeUp", 0);
		xdo_send_keysequence_window(x11, CURRENTWINDOW, pultelio_veiksmai_kai_nuspausa[msg->keycode].data(), 0);
	}

	std::cerr << "0x" << std::hex << msg->keycode << std::endl;

	if(!zinute.empty())
		parodyti_pranesima(zinute);
}

void kai_komanda(void* , const CEC::cec_command* msg)
{
	/*
	if(msg->opcode == CEC::CEC_OPCODE_STANDBY)
		jau_išjungti = true; //Televizorius mums liepė eiti miegoti (išsijungti)
	*/
}

int main(int , char* [])
{
	// Pradedame sisteminių signalų stebėjimą, kad patiktumėme Ctrl-C
	if( signal(SIGINT, kai_sisteminis_signalas) == SIG_ERR )
	{
		std::cerr << "Nepavyko pradėti SIGINT signalų stebėjimą.\n";
		return 1;
	}

	x11 = xdo_new(":0.0");
	if(!x11)
	{
		std::cerr << "Nepavyko inicializuoti x11 mygtukų siuntimo.\n";
		return 1;
	}

	// Sukonfigūruoti CEC ir prikabinti callbackus
	CEC::ICECCallbacks        cec_callbacks;
	CEC::libcec_configuration cec_config;
	cec_config.Clear();
	cec_callbacks.Clear();

	const std::string devicename("Avietė");
	devicename.copy(cec_config.strDeviceName, std::min(devicename.size(), static_cast<std::string::size_type>(13)) );

	cec_config.clientVersion       = CEC::LIBCEC_VERSION_CURRENT;
	cec_config.bActivateSource     = 0;
	cec_config.callbacks           = &cec_callbacks;
	cec_config.deviceTypes.Add(CEC::CEC_DEVICE_TYPE_RECORDING_DEVICE);

	cec_callbacks.keyPress         = &kai_puletio_mygtukas;
	cec_callbacks.commandReceived  = &kai_komanda;

	// Inicializuojame cec biblioteką, gauname cec adapterį
	CEC::ICECAdapter *cec_adapter = LibCecInitialise(&cec_config);
	if( !cec_adapter )
	{
		std::cerr << "Nepavyko inicializuoti libcec.so\n";
		return 1;
	}

	// Bandome automatiškai rasti CEC įrenginius (televizorių)
	std::array<CEC::cec_adapter_descriptor, 10> devices;
	int8_t devices_found = cec_adapter->DetectAdapters(devices.data(), devices.size(), nullptr, true /*quickscan*/);
	if( devices_found <= 0 )
	{
		std::cerr << "Nepavyko automatiškai rasti prie avietės CEC adapterio prijungtų įrenginių.\n";
		UnloadLibCec(cec_adapter);
		return 1;
	}

	// Pasirenkame pirmą CEC įrenginį (tikriausiai televizorių)
	if( !cec_adapter->Open(devices[0].strComName) )
	{
		std::cerr << "Nepavyko atidaryti CEC įrenginio su portu: " << devices[0].strComName << std::endl;
		UnloadLibCec(cec_adapter);
		return 1;
	}

	// Įjungti visus įrenginius (t.y. televizorių). Jis gali būti jau įjungtas, bet blogiau nebus
	if(!cec_adapter->PowerOnDevices() )
	{
		std::cerr << "Failed to power on TV. Ignoring...";
		UnloadLibCec(cec_adapter);
	}

	// Perjungti, kad rodytu avietę, o ne kokį nors durną TV kanalą
	if(!cec_adapter->SetActiveSource())
	{
		std::cerr << "Failed to set RPI as active source. Ignoring...";
		UnloadLibCec(cec_adapter);
	}

	// Nieko nedaryti, kol nepaspaustas Ctrl-C
	while( !jau_išjungti )
	{
		// Pamiegoti, kas sekundę patikrinti ar nepaspaustas Ctrl+C
		std::this_thread::sleep_for( std::chrono::seconds(1) );
	}

	// Apsivalymas
	cec_adapter->Close();
	UnloadLibCec(cec_adapter);

	return 0;
}
