Programa įjungs televizorių ir pasirinks save kaip įvesties šaltinį (TV pradės rodyti avietę). Tada programa pradės klausytis pultelių mygtukų ir bandys juos išversti į klaviatūros mygtukus.

Tai yra X11 versija, skirta senesnėms, ketvirtos versijos avietėms. Nuo penktos versijos avietės nebenaudoja x11 ir perėjo prie wayland, todėl šitas kodas nebeveiks.

Kokie pultelio mygtukai į kokius klaviatūros mygtukus išverčiami aprašyta kintamuosiuose **pultelio_veiksmai** ir **pultelio_veiksmai_kai_nuspausa** (tie veiksmai, kurie kartojami kol mygtukas yra nuspaustas).

Daugiau apie projektą: http://piktas-zuikis.netlify.com/2020/05/24/TV-HDMI-RPI-CEC/

# Kompiliavimas
Reikalingi pakeitai:
```
pacman -S libcec xdotool
# arba debian
apt-get install libcec libxdo-dev libdbus-1-dev
```

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
systemctl --user status aviete.service
```


# Paleidimas
```
./aviete-cec
```

