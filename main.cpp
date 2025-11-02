#include <linux/uinput.h>
#include <fcntl.h>

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
#include <list>

// Reikalingas aptikti Ctrl+C
#include <signal.h>

#include "notify.h"

int fd = -1;
bool jau_išjungti = false;
bool ar_nuspaustas_alt = false;
bool ar_nuspaustas_shift = false;

std::list<unsigned short> papildomi_mygtukai({
    KEY_LEFTALT,
    KEY_LEFTSHIFT,
    KEY_LEFTCTRL,
    KEY_F5,
});

std::unordered_map<int, unsigned short> pultelio_veiksmai({
	//{pultelio mygtukas, klaviatūros mygtukas},
    {CEC::CEC_USER_CONTROL_CODE_SELECT, KEY_ENTER},
    {CEC::CEC_USER_CONTROL_CODE_EXIT, KEY_BACKSPACE},
    {CEC::CEC_USER_CONTROL_CODE_F2_RED, KEY_ESC},
    {CEC::CEC_USER_CONTROL_CODE_F3_GREEN, KEY_TAB},
    {CEC::CEC_USER_CONTROL_CODE_PLAY, KEY_SPACE},
    {CEC::CEC_USER_CONTROL_CODE_PAUSE, KEY_PLAYPAUSE},
    {CEC::CEC_USER_CONTROL_CODE_REWIND, KEY_PREVIOUS},
    {CEC::CEC_USER_CONTROL_CODE_FAST_FORWARD, KEY_NEXT},
    {CEC::CEC_USER_CONTROL_CODE_ELECTRONIC_PROGRAM_GUIDE, KEY_LEFTMETA},
    {CEC::CEC_USER_CONTROL_CODE_STOP, KEY_F},
    {CEC::CEC_USER_CONTROL_CODE_SOUND_SELECT, KEY_CLOSE},
    {CEC::CEC_OPCODE_RECORD_STATUS, KEY_MENU},
});

//Kai mygtukas nuspasustas ir laikomas nuspaustas
std::unordered_map<int, unsigned short> pultelio_veiksmai_kai_nuspausta({
	//{pultelio mygtukas, klaviatūros mygtukas},

    {CEC::CEC_USER_CONTROL_CODE_UP, KEY_UP},
    {CEC::CEC_USER_CONTROL_CODE_DOWN, KEY_DOWN},
    {CEC::CEC_USER_CONTROL_CODE_LEFT, KEY_LEFT},
    {CEC::CEC_USER_CONTROL_CODE_RIGHT, KEY_RIGHT},
    {CEC::CEC_USER_CONTROL_CODE_BACKWARD, KEY_VOLUMEDOWN},
    {CEC::CEC_USER_CONTROL_CODE_FORWARD, KEY_VOLUMEUP},
});

void nuspausk_mygtuka(unsigned short code){
    if(fd < 0){
        std::perror("Nepavyko paspausti virtualaus mugtuko: dingo klaviatūra.");
        return;
    }
    input_event ie = {
        .type = EV_KEY,
        .code = code,
        .value = 1,
    };

    if(write(fd, &ie, sizeof(ie)) < 0)
    {
        std::perror("Nepavyko išsiųsti mygtuko nuspaudimo.");
    }

    ie.type = EV_SYN;
    ie.code = SYN_REPORT;
    ie.value = 0;
    if(write(fd, &ie, sizeof(ie)) < 0)
    {
        std::perror("Nepavyko išsiųsti mygtuko nuspaudimo SYNC.");
    }
}

void atspausk_mygtuka(unsigned short code){
    if(fd < 0){
        std::perror("Nepavyko paspausti virtualaus mugtuko: dingo klaviatūra.");
        return;
    }
    input_event ie = {
        .type = EV_KEY,
        .code = code,
        .value = 0,
    };

    if(write(fd, &ie, sizeof(ie)) < 0)
    {
        std::perror("Nepavyko išsiųsti mygtuko nuspaudimo.");
    }

    ie.type = EV_SYN;
    ie.code = SYN_REPORT;
    if(write(fd, &ie, sizeof(ie)) < 0)
    {
        std::perror("Nepavyko išsiųsti mygtuko nuspaudimo SYNC.");
    }
}

void spausk_mygtuka(unsigned short code)
{
    std::cerr << "(" << code << ") ";
    nuspausk_mygtuka(code);
    atspausk_mygtuka(code);
}

int sukurti_virtualia_klaviatura()
{
    fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if(fd < 0){
        std::perror("Nepavyko atidaryti '/dev/uinput' virtualios klavietūros sukūrimui.");
        return 1;
    }

    if(ioctl(fd, UI_SET_EVBIT, EV_KEY)){
        std::perror("Nepavyko sukonfigūruoti virtualios klaviatūros: nepavyko nustatyti EV_KEY.");
        close(fd);
        return 1;
    }

    for(auto &it: pultelio_veiksmai)
        ioctl(fd, UI_SET_KEYBIT, it.second);

    for(auto &it: pultelio_veiksmai_kai_nuspausta)
        ioctl(fd, UI_SET_KEYBIT, it.second);

    for(auto &it: papildomi_mygtukai)
        ioctl(fd, UI_SET_KEYBIT, it);

    uinput_setup usetup = {};
    memset(&usetup, 0, sizeof(usetup));

    usetup.id.bustype = BUS_USB;
    usetup.id.vendor = 0x1234; /* sample vendor */
    usetup.id.product = 0x5678; /* sample product */
    strcpy(usetup.name, "Aviete CEC");

    ioctl(fd, UI_DEV_SETUP, &usetup);
    ioctl(fd, UI_DEV_CREATE);

    return 0;
}

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
                //xdo_send_keysequence_window_up(x11, CURRENTWINDOW, "Alt", 0);
				zinute += "be alt";
                atspausk_mygtuka(KEY_LEFTALT);
			}
			else
			{
                //xdo_send_keysequence_window_down(x11, CURRENTWINDOW, "Alt", 0);
				zinute += "ALT+...";
                nuspausk_mygtuka(KEY_LEFTALT);
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
                //xdo_send_keysequence_window_up(x11, CURRENTWINDOW, "Shift", 0);
				zinute += "be shift";
                atspausk_mygtuka(KEY_LEFTSHIFT);
			}
			else
			{
                //xdo_send_keysequence_window_down(x11, CURRENTWINDOW, "Shift", 0);
				zinute += "SHIFT+...";
                nuspausk_mygtuka(KEY_LEFTSHIFT);
			}

			ar_nuspaustas_shift = !ar_nuspaustas_shift;
		}
		// Meniu mygtukas paspaudžia Control+Shift+Alt+F5
		if (msg->keycode == CEC::CEC_USER_CONTROL_CODE_ROOT_MENU)
        {
            nuspausk_mygtuka(KEY_LEFTCTRL);
            nuspausk_mygtuka(KEY_LEFTSHIFT);
            nuspausk_mygtuka(KEY_LEFTALT);
            nuspausk_mygtuka(KEY_F5);

            atspausk_mygtuka(KEY_F5);
            atspausk_mygtuka(KEY_LEFTALT);
            atspausk_mygtuka(KEY_LEFTSHIFT);
            atspausk_mygtuka(KEY_LEFTCTRL);
        }
		else if ( pultelio_veiksmai.find(msg->keycode) != pultelio_veiksmai.end() ) //ar mygtukas sumapintas
		{
            spausk_mygtuka(pultelio_veiksmai[msg->keycode]);
		}
	}
    else if ( pultelio_veiksmai_kai_nuspausta.find(msg->keycode) != pultelio_veiksmai_kai_nuspausta.end() )
	{
        spausk_mygtuka(pultelio_veiksmai_kai_nuspausta[msg->keycode]);
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

    if(sukurti_virtualia_klaviatura())
    {
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
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
		return 1;
	}

	// Bandome automatiškai rasti CEC įrenginius (televizorių)
	std::array<CEC::cec_adapter_descriptor, 10> devices;
    int8_t devices_found = cec_adapter->DetectAdapters(devices.data(), devices.size(), nullptr, true /*quickscan*/);
	if( devices_found <= 0 )
	{
		std::cerr << "Nepavyko automatiškai rasti prie avietės CEC adapterio prijungtų įrenginių.\n";
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
		UnloadLibCec(cec_adapter);
		return 1;
	}

	// Pasirenkame pirmą CEC įrenginį (tikriausiai televizorių)
	if( !cec_adapter->Open(devices[0].strComName) )
	{
		std::cerr << "Nepavyko atidaryti CEC įrenginio su portu: " << devices[0].strComName << std::endl;
        ioctl(fd, UI_DEV_DESTROY);
        close(fd);
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
    ioctl(fd, UI_DEV_DESTROY);
    close(fd);
    cec_adapter->Close();
    UnloadLibCec(cec_adapter);

	return 0;
}
