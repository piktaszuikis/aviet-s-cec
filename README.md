Programa įjungs televizorių ir pasirinks save kaip įvesties šaltinį (TV pradės rodyti avietę). Tada programa pradės klausytis pultelių mygtukų ir bandys juos išversti į klaviatūros mygtukus.

Tai yra wayland versija, skirta naujesnėms, penktos versijos avietėms. Nuo penktos versijos avietės nebenaudoja x11 ir perėjo prie wayland, todėl reikėjo pakeisti klaviatūros mygtukų paspaudimo emuliavimą. Vietoj xdo dabar naudojamas [uinput](https://www.kernel.org/doc/html/v4.12/input/uinput.html).

Kokie pultelio mygtukai į kokius klaviatūros mygtukus išverčiami aprašyta kintamuosiuose **pultelio_veiksmai** ir **pultelio_veiksmai_kai_nuspausa** (tie veiksmai, kurie kartojami kol mygtukas yra nuspaustas). Kintamajame **papildomi_mygtukai** yra surašyti papildomi mygtukai, kurie bus virtualiai spaudžiojami, nes *uinput* reikalauja inicializuojant išvardinti visus programos naudojamus mygtukus.

Daugiau apie projektą: http://piktas-zuikis.netlify.com/2020/05/24/TV-HDMI-RPI-CEC/

# Kompiliavimas
Reikalingi paketai:
```
pacman -S libcec mako
# arba debian
apt-get install libcec libdbus-1-dev mako-notifier
```
Paketas *mako* reikalingas pranešimams parodyti. Veikia ir be jo. Galima naudoti bet kurį kitą "wayland" pranešimų demoną.

Kompiliavimas:
```
git clone https://github.com/piktaszuikis/avietės-cec.git
mkdir avietės-cec/build
cd avietės-cec/build
cmake ../
make
```

Viskas!

Jeigu nori, galima instaliuoti su
```
make install
# peržiūrėk serviso statusą su:
systemctl --user status aviete.service
# logus peržiūrėsi su:
journalctl --user-unit aviete.service -f
```

# Paleidimas
```
./aviete-cec
```
